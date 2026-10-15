# Guide d’intégration — chaîne cryptographique C, sockets TCP et PostgreSQL

| Métadonnée | Valeur |
|---|---|
| **Identifiant** | `GUIDE-CRYPTO-01` |
| **Version** | 1.0 — 14 octobre 2026 |
| **Public visé** | Équipe AnimeTCG — intégration du noyau crypto dans le serveur C |
| **Objectif** | Donner un chemin concret entre une trame TCP, le minage d’un bloc, l’écriture atomique en base et la réponse au client |
| **Code existant** | `server/chain/crypto.h`, `crypto.c`, `sha256.c`, `header.c`, `pow.c`, `verify.c` |
| **Références du dépôt** | `docs/fiche-fonctionnalite-cryptographie.md`, `docs/protocole-applicatif.md`, `docs/specification-technique.md` §§4–8, `sql/schema.sql` |

## Sommaire

1. [Flux complet](#1-le-flux-complet-en-une-vue)
2. [Contrat crypto ↔ SQL](#2-contrat-entre-cryptoh-et-le-schéma-sql)
3. [Environnement et compilation](#3-préparer-lenvironnement-c-et-postgresql)
4. [Trames TCP et sockets](#4-lire-et-écrire-les-trames-tcp-en-c)
5. [Parser la requête et former le payload](#5-parser-la-requête-et-créer-le-payload-métier)
6. [Construire et miner un bloc](#6-construire-et-miner-un-bloc)
7. [Connexion libpq](#7-ouvrir-la-connexion-postgresql-avec-libpq)
8. [Verrou de chaîne et lecture du tip](#8-sérialiser-les-écritures-et-lire-le-tip)
9. [Insertion de `blocks`](#9-insérer-blocks-avec-paramètres-sql)
10. [Transaction et projections](#10-écrire-le-bloc-et-la-projection-métier-dans-la-même-transaction)
11. [Réponses et événements](#11-construire-la-réponse-du-protocole)
12. [Relecture et vérification](#12-relire-et-vérifier-la-chaîne-depuis-postgresql)
13. [Ownership et erreurs](#13-ownership-allocations-et-erreurs)
14. [Sécurité et périmètre](#14-sécurité-et-limites-de-périmètre)
15. [Recette](#15-recette-dintégration-recommandée)
16. [Travaux restants](#16-points-à-compléter-avant-de-dire-intégré)

> **État à connaître avant de commencer.** Le noyau cryptographique et ses tests existent ; ce dépôt ne contient pas encore le serveur C complet, l’adaptateur `libpq`, la couche sockets ni le canonicaliseur JCS. Les exemples ci-dessous sont des modèles d’intégration à adapter au projet. Les sections marquées **À fournir** sont des interfaces manquantes, et non des fonctions déjà livrées. En particulier, ne remplacez pas RFC 8785/JCS par `json_dumps(..., JSON_COMPACT)` : ce n’est pas une garantie de canonicalisation JCS.

---

## 1. Le flux complet en une vue

Pour une action qui modifie l’état métier, le flux serveur attendu est :

```text
socket TCP
  → lire exactement une trame [longueur u32 big-endian][JSON UTF-8]
  → parser et valider l’enveloppe REQ, la version, l’action et les paramètres
  → vérifier les droits et les règles métier côté serveur
  → préparer un événement métier immuable (avec les identifiants attribués par le serveur)
  → sérialiser cet événement en octets RFC 8785/JCS
  → sérialiser les écritures de chaîne (file unique et/ou verrou transactionnel PostgreSQL)
  → prendre le tip, l’index suivant, l’horodatage et le hash précédent
  → construire crypto_block_t et appeler crypto_block_mine()
  → INSERT du bloc + INSERT/UPDATE de la projection métier dans la même transaction
  → COMMIT
  → seulement ensuite, réponse RES au demandeur et diffusion EVT aux clients
```

Une requête refusée par la validation métier ne produit **aucun bloc**. Un échec de minage ou de base provoque un `ROLLBACK` et aucune réponse de succès ni diffusion d’événement.

### Responsabilités des couches

| Couche | Ce qu’elle fait | Ce qu’elle ne doit pas faire |
|---|---|---|
| `net` | Lire/écrire les trames TCP, gérer les sockets et les déconnexions | Interpréter une règle métier ou créer un bloc directement |
| `protocol` | Parser JSON, contrôler `v`, `id`, `type`, `action`, types et tailles | Faire confiance à un champ d’identité fourni par le client |
| `business` | Autoriser l’action, créer un événement métier complet et décider de la projection | Accepter `creator_id`, valeur, difficulté ou hash calculés par le client |
| `chain` | Canonical payload déjà fourni, SHA-256, preuve de travail, vérification | Parser le JSON, accéder à PostgreSQL ou toucher aux sockets |
| `persistence` | Lire le tip, stocker le bloc et écrire la projection sous transaction | Reconstruire `payload_canon` depuis JSONB ou concaténer des valeurs SQL |

---

## 2. Contrat entre `crypto.h` et le schéma SQL

`server/chain/crypto.h` est l’API publique du noyau. Les fonctions utiles à l’intégration sont :

```c
crypto_status_t crypto_payload_sha256(...);
crypto_status_t crypto_header_encode(...);
crypto_status_t crypto_header_sha256(...);
crypto_status_t crypto_block_mine(crypto_block_t *block,
                                  uint64_t *attempts_out);
crypto_status_t crypto_verify_block(const crypto_block_t *block);
crypto_status_t crypto_verify_chain(const crypto_block_t *blocks,
                                    size_t count,
                                    crypto_verify_report_t *report_out);
```

Le hash est représenté en mémoire par **32 octets bruts**. Le format hexadécimal n’est nécessaire qu’à la frontière SQL, protocole ou journal.

### Correspondance du bloc C avec `blocks`

| Champ haché dans l’en-tête C | Stockage dans `sql/schema.sql` | Règle d’adaptation |
|---|---|---|
| `version` | Pas de colonne | Version courante fixée à `1` ; l’ajouter explicitement au schéma lors d’une évolution de format |
| `block_index` (`uint32_t`) | `block_index BIGINT` | Toujours vérifier `0 <= index <= UINT32_MAX` ; le schéma v1.6 applique cette borne |
| `timestamp_ms` (`int64_t`) | `ts TIMESTAMPTZ` | Convertir sans changer la milliseconde ; figer avant le minage |
| `previous_hash[32]` | `previous_hash CHAR(64)` | Convertir binaire ↔ hex ; le genesis est **32 octets nuls en C**, mais `NULL` en SQL |
| `action_id` (`uint32_t`) | `action_type VARCHAR(32)` | Nécessite une table de correspondance stable, documentée et identique dans le serveur et le vérificateur |
| `difficulty` | `difficulty SMALLINT` | Stocker la difficulté appliquée à ce bloc, jamais la difficulté courante du serveur |
| `nonce` (`uint64_t`) | `nonce BIGINT` signé | Le minage normal commence à 0 et reste dans la plage `int64_t`. Refuser un nonce supérieur à `INT64_MAX` avant insertion ; pour persister tout `uint64_t`, faire évoluer le schéma et son adaptateur |
| `payload_hash[32]` | Pas de colonne dédiée | Recalculer depuis `payload_canon` lors de la vérification |
| `payload_canon` | `payload_canon BYTEA` | Écrire et relire les **octets exacts** hachés ; ne jamais les régénérer depuis `payload JSONB` |
| `block.hash[32]` | `hash CHAR(64)` | Convertir binaire ↔ hex minuscule de 64 caractères |

Le schéma stocke également `payload JSONB` pour les recherches SQL. Cette colonne est une représentation de consultation ; elle n’est pas la source utilisée pour recalculer le hash.

### Correspondance d’action à figer

Le champ `action_id` fait partie des 96 octets de l’en-tête, tandis que `blocks.action_type` est un texte. Le schéma actuel ne définit pas encore le mapping numérique. Centralisez-le dans un fichier unique, par exemple `server/chain/action_ids.h`, et ne dépendez jamais de l’ordre implicite d’un `enum` :

```c
/* Valeurs d’exemple : l’équipe doit les figer pour toutes les actions. */
enum {
    ACTION_GENESIS       = 0u,
    ACTION_CARD_CREATE   = 1u, /* 1 est utilisé par le vecteur doré actuel */
    ACTION_RECOMMEND_ADD = 2u
};
```

La conversion inverse est nécessaire pour relire `action_type` depuis PostgreSQL lors de la vérification. Une valeur inconnue doit faire échouer la vérification **fermée** ; ne lui attribuez pas un identifiant par défaut.

---

## 3. Préparer l’environnement C et PostgreSQL

### Dépendances

Le serveur d’intégration utilise les dépendances déjà retenues dans la spécification : OpenSSL (`libcrypto`), libpq (`libpq`) et Jansson pour le parsing JSON. Sur une distribution Debian/Ubuntu, les paquets de développement usuels sont :

```sh
sudo apt install build-essential pkg-config libssl-dev libpq-dev libjansson-dev postgresql-client
```

Initialiser une base de développement avec le schéma du dépôt :

```sh
createdb anicard
psql -d anicard -v ON_ERROR_STOP=1 -f sql/schema.sql
```

Ne mettez pas de mot de passe PostgreSQL en dur dans le code ni dans la chaîne de connexion. Utilisez une configuration libpq (`service=animetcg`), un fichier `.pgpass` avec permissions restrictives ou le gestionnaire de secrets de l’environnement. Exemple sans secret dans le dépôt, dans `~/.pg_service.conf` :

```ini
[animetcg]
host=127.0.0.1
port=5432
dbname=anicard
user=anicard
```

Si un mot de passe est nécessaire, ajoutez-le dans le fichier `.pgpass` de l’utilisateur qui lance le serveur, puis appliquez `chmod 600 ~/.pgpass`. En intégration, remplacez `127.0.0.1` par l’hôte de base configuré pour l’environnement.

### Compiler le noyau avec le serveur

Le Makefile de `server/chain` construit les tests du noyau uniquement ; il ne construit pas encore le serveur socket/DB. Le principe de ligne de compilation pour le futur serveur est :

```sh
cc -std=c11 -D_POSIX_C_SOURCE=200809L \
  -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow \
  -Iserver/chain \
  $(pkg-config --cflags libcrypto libpq jansson) \
  server/main.c server/net/frame.c server/protocol/request.c \
  server/business/card.c server/persistence/blocks.c \
  server/chain/crypto.c server/chain/sha256.c \
  server/chain/header.c server/chain/pow.c server/chain/verify.c \
  -o build/animetcg-server \
  $(pkg-config --libs libcrypto libpq jansson)
```

Remplacez les chemins de fichiers encore absents par ceux de votre implémentation. **N’utilisez pas** `server/chain/*.c` dans cette commande : cela inclurait `test_crypto.c` et son `main()`.

Pour vérifier seulement le noyau déjà livré :

```sh
make -C server/chain test
make -C server/chain sanitize
```

---

## 4. Lire et écrire les trames TCP en C

### Format exact

Chaque message est envoyé sous la forme suivante :

```text
+--------------------------+-----------------------------------------+
| longueur : 4 octets      | JSON UTF-8 : longueur octets            |
| uint32 big-endian        | minimum 2, maximum 65 536               |
+--------------------------+-----------------------------------------+
```

TCP est un flux : un `recv()` ne correspond pas nécessairement à une trame entière. Il peut ne retourner qu’une partie du préfixe, une partie du JSON, ou plusieurs trames au fil des appels. Il faut donc conserver l’état de lecture et boucler jusqu’à avoir la longueur attendue.

### Exemple simple pour sockets bloquantes

Cet exemple lit dans un tampon fourni par l’appelant. Il ne fait aucune allocation et n’ajoute pas de `\0` au JSON : Jansson reçoit la longueur séparément avec `json_loadb()`.

```c
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#define FRAME_MAX_BYTES 65536u

/* 0 = octets lus, 1 = fermeture propre avant le début d'une trame, -1 = erreur. */
static int recv_exact(int fd, uint8_t *dst, size_t wanted,
                      bool clean_eof_allowed)
{
    size_t done = 0u;

    while (done < wanted) {
        const ssize_t received = recv(fd, dst + done, wanted - done, 0);

        if (received > 0) {
            done += (size_t)received;
            continue;
        }
        if (received == 0) {
            if (done == 0u && clean_eof_allowed) {
                return 1;
            }
            errno = ECONNRESET; /* trame tronquée */
            return -1;
        }
        if (errno == EINTR) {
            continue;
        }
        return -1; /* inclut EAGAIN : cet exemple attend une socket bloquante */
    }
    return 0;
}

static uint32_t read_u32_be(const uint8_t bytes[4])
{
    return ((uint32_t)bytes[0] << 24) |
           ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8)  |
           (uint32_t)bytes[3];
}

static void write_u32_be(uint8_t bytes[4], uint32_t value)
{
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

/* Retour : 0 = trame complète, 1 = client déconnecté entre deux trames, -1 = erreur. */
static int recv_frame(int fd, uint8_t body[FRAME_MAX_BYTES], size_t *body_len)
{
    uint8_t prefix[4];
    uint32_t length;
    int rc;

    if (body == NULL || body_len == NULL) {
        errno = EINVAL;
        return -1;
    }
    *body_len = 0u;
    rc = recv_exact(fd, prefix, sizeof(prefix), true);
    if (rc != 0) {
        return rc;
    }

    length = read_u32_be(prefix);
    if (length < 2u || length > FRAME_MAX_BYTES) {
        errno = EMSGSIZE;
        return -1; /* fermer la socket : longueur de trame non conforme */
    }
    rc = recv_exact(fd, body, (size_t)length, false);
    if (rc != 0) {
        return -1; /* le corps annoncé est tronqué */
    }
    *body_len = (size_t)length;
    return 0;
}

static int send_exact(int fd, const uint8_t *src, size_t length)
{
    size_t sent = 0u;

    while (sent < length) {
        const ssize_t n = send(fd, src + sent, length - sent, MSG_NOSIGNAL);

        if (n > 0) {
            sent += (size_t)n;
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n == 0) {
            errno = EPIPE;
        }
        return -1;
    }
    return 0;
}

static int send_frame(int fd, const uint8_t *body, size_t body_len)
{
    uint8_t prefix[4];

    if (body == NULL || body_len < 2u || body_len > FRAME_MAX_BYTES) {
        errno = EMSGSIZE;
        return -1;
    }
    write_u32_be(prefix, (uint32_t)body_len);
    if (send_exact(fd, prefix, sizeof(prefix)) != 0) {
        return -1;
    }
    return send_exact(fd, body, body_len);
}
```

`MSG_NOSIGNAL` est adapté à Linux, cohérent avec l’architecture `epoll` du projet. Sur un système qui ne le fournit pas, configurez une fois `SO_NOSIGPIPE` ou ignorez `SIGPIPE` selon le système.

### Créer le socket d’écoute

Exemple Linux simplifié, non bloquant et prêt à être enregistré dans `epoll`. Définissez `_POSIX_C_SOURCE=200809L` au tout début du fichier, avant les includes ; `SOCK_NONBLOCK`, `SOCK_CLOEXEC` et `accept4()` sont spécifiques à Linux, ce qui correspond au choix `epoll` du projet.

```c
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

static int make_listener(const char *port)
{
    struct addrinfo hints = {0};
    struct addrinfo *addresses = NULL;
    struct addrinfo *it;
    int listener = -1;

    hints.ai_family = AF_UNSPEC;       /* IPv4 ou IPv6 selon la machine */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, port, &hints, &addresses) != 0) {
        return -1;
    }
    for (it = addresses; it != NULL; it = it->ai_next) {
        int reuse = 1;

        listener = socket(it->ai_family,
                          it->ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC,
                          it->ai_protocol);
        if (listener < 0) continue;
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
                       &reuse, sizeof(reuse)) != 0 ||
            bind(listener, it->ai_addr, it->ai_addrlen) != 0 ||
            listen(listener, SOMAXCONN) != 0) {
            (void)close(listener);
            listener = -1;
            continue;
        }
        break;
    }
    freeaddrinfo(addresses);
    return listener; /* fermer une fois avec close(listener) */
}
```

Créez ensuite `epoll_fd = epoll_create1(EPOLL_CLOEXEC)` et ajoutez le listener avec `EPOLLIN`. Sur notification, appelez `accept4(listener, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC)` en boucle jusqu’à `EAGAIN`/`EWOULDBLOCK`, puis ajoutez chaque client à `epoll`. Traitez `EINTR` en recommençant ; toute autre erreur doit être journalisée.

### Important pour `epoll` et sockets non bloquantes

Les fonctions ci-dessus sont pédagogiques pour une socket **bloquante**. Ne les appelez pas dans le fil d’événements `epoll` : un `recv()` incomplet y bloquerait toutes les connexions. Sur socket non bloquante, gardez un état **par connexion** :

```c
typedef struct {
    uint8_t prefix[4];
    size_t prefix_used;
    uint32_t body_length; /* 0 tant que le préfixe n'est pas validé */
    size_t body_used;
    uint8_t body[FRAME_MAX_BYTES];
} frame_rx_t;
```

À chaque événement `EPOLLIN`, bouclez sur `recv()` jusqu’à `EAGAIN` :

1. compléter `prefix` ;
2. dès que les 4 octets sont présents, décoder et refuser immédiatement `length < 2` ou `length > 65536` ;
3. compléter `body` sur les appels suivants ;
4. quand le corps est complet, le copier ou transférer sa propriété à une file bornée, remettre l’état à zéro puis continuer : une même lecture peut contenir la fin d’une trame puis le début de la suivante ;
5. sur `EINTR`, recommencer ; sur `EAGAIN`/`EWOULDBLOCK`, rendre la main à `epoll` ; sur EOF au milieu d’une trame, fermer la connexion comme tronquée.

Ne passez pas `frame_rx_t.body` directement à un worker si vous la réutilisez dès le retour du callback : le worker verrait alors les octets de la trame suivante. Copiez dans un élément de file, ou transférez explicitement un tampon possédé par le worker. La file doit être bornée pour éviter qu’un client lent n’épuise la mémoire.

Même règle pour les réponses : `send_frame()` ci-dessus est bloquant. Dans `epoll`, construire une sortie possédée par la connexion (`préfixe 4 octets + JSON`), conserver un offset d’écriture, appeler `send()` jusqu’à `EAGAIN`, reprendre sur `EPOLLOUT`, puis libérer le tampon quand tous ses octets ont été envoyés. N’activez `EPOLLOUT` que lorsqu’une sortie attend et bornez la taille de la file d’écriture par client.

---

## 5. Parser la requête et créer le payload métier

### Requête filaire

Exemple conforme à l’enveloppe du protocole :

```json
{
  "v": "1.0",
  "id": "ab12c3d4e5f6",
  "type": "REQ",
  "action": "card.create",
  "payload": {
    "title_romaji": "Cowboy Bebop",
    "format": "TV",
    "year": 1998,
    "episodes": 26,
    "genres": ["action", "drame"],
    "themes": ["mecha"]
  }
}
```

Parser l’enveloppe avec la longueur exacte de la trame. Exemple avec Jansson ; `json_object_get()` renvoie des pointeurs **empruntés** par `root`, qui restent valides jusqu’à `json_decref(root)` :

```c
#include <jansson.h>

static json_t *parse_request(const uint8_t *bytes, size_t length,
                             json_error_t *error)
{
    json_t *root;

    if (bytes == NULL || error == NULL || length < 2u ||
        length > FRAME_MAX_BYTES) {
        return NULL;
    }
    root = json_loadb((const char *)bytes, length,
                      JSON_REJECT_DUPLICATES, error);
    if (!json_is_object(root)) {
        json_decref(root);
        return NULL;
    }

    /* À compléter dans le module protocol :
     * - v == "1.0" (après négociation session.hello),
     * - id conforme à 12 caractères hexadécimaux,
     * - type == "REQ",
     * - action est une chaîne connue,
     * - payload est un objet, avec types et bornes propres à l'action.
     */
    if (!json_is_string(json_object_get(root, "v")) ||
        !json_is_string(json_object_get(root, "id")) ||
        !json_is_string(json_object_get(root, "type")) ||
        !json_is_string(json_object_get(root, "action")) ||
        !json_is_object(json_object_get(root, "payload"))) {
        json_decref(root);
        return NULL;
    }
    return root; /* l'appelant devient propriétaire et fera json_decref() */
}
```

Le parsing ne valide pas les règles métier à lui seul. Pour `card.create`, le serveur vérifie les champs et limites de `docs/protocole-applicatif.md`, récupère l’identité du créateur depuis la session serveur, et ignore tout `creator_id`, `owner_id`, valeur ou hash éventuellement envoyé par le client.

> **À harmoniser avant l’intégration.** Les artefacts du dépôt ne sont pas parfaitement alignés : (1) le tableau d’actions du protocole cite `title_carte` et `sources_id` (et indique une limite de 10 cartes actives), alors que le schéma SQL expose `title_romaji` et `source` et que le schéma/spécification technique documente une limite de 100 ; (2) §3.2 du protocole décrit l’ID tantôt comme 12 caractères hexadécimaux sans séparateur, tantôt comme `8HEXDIG-4HEXDIG` avec tiret. Les exemples de ce guide suivent les IDs sans tiret et les colonnes SQL actuelles. Faites confirmer les contrats définitifs, puis créez des mappings explicites entre DTO protocolaire, événement canonique et colonnes SQL ; ne déduisez jamais ces correspondances par convention implicite.

### Payload à hacher : un événement décidé par le serveur

Ne hachez pas l’enveloppe TCP complète. Ne hachez pas non plus aveuglément les paramètres bruts de la requête. Le serveur doit créer un événement immuable et rejouable, après validation, contenant les valeurs finales qui permettent de reconstruire les projections : identifiant de carte réservé par le serveur, créateur/propriétaire issus du contexte serveur, champs validés, etc.

Exemple de forme canonique pour un événement de création (clés ASCII triées, aucun espace) :

```json
{"card_id":42,"creator_id":7,"episodes":26,"format":"TV","genres":["action","drame"],"owner_id":7,"themes":["mecha"],"title_romaji":"Cowboy Bebop","year":1998}
```

L’ordre des éléments de tableaux est conservé par JCS. Si les genres/thèmes sont des ensembles sans ordre métier, triez-les de façon déterministe **avant** canonicalisation. JCS trie les clés d’objets, pas les valeurs des tableaux.

### Canonicalisation JCS — adaptateur à fournir

Le module crypto reçoit des octets ; il ne convertit pas `json_t` en JCS. Définissez une interface unique et vérifiable, par exemple :

```c
typedef struct {
    uint8_t *data; /* mémoire possédée ; data[len] est un '\0' terminal */
    size_t len;    /* longueur JCS en octets UTF-8, sans le '\0' */
} canonical_json_t;

/* À implémenter avec un sérialiseur RFC 8785 testé. */
bool payload_jcs_canonicalize(const json_t *event,
                              canonical_json_t *out);
void canonical_json_free(canonical_json_t *value);
```

Contrat de `payload_jcs_canonicalize()` : vérifier ou garantir une sortie UTF-8 conforme à RFC 8785, propriétés triées selon les règles JCS, représentation déterministe des nombres, absence d’espaces non significatifs ; refuser les données que l’implémentation JCS ne peut pas représenter conformément à la RFC ; retourner 2 à 65 536 octets ; réserver une terminaison `\0` en plus de `len` pour le paramètre texte JSONB ; signaler qui libère le tampon.

> **Pas de raccourci incorrect :** `json_dumps(event, JSON_COMPACT)` peut être utile pour une réponse de protocole, mais ne suffit pas à prouver la conformité RFC 8785, notamment pour le tri des clés et les nombres. JCS ne normalise pas Unicode. Si le protocole impose NFC, appliquez-la une fois au stade métier avant de figer l’événement, puis canonisez.

Pour tester l’interface crypto indépendamment du sérialiseur, le vecteur fixe déjà couvert par le harnais C est :

```c
static const uint8_t golden_payload[] =
    "{\"episodes\":26,\"title_romaji\":\"Cowboy Bebop\",\"year\":1998}";
/* longueur à fournir : sizeof(golden_payload) - 1 */
```

Le test utilise le hash de payload `a79a3d87a822afbd43f4191ae858bf03525779cc2f092362ec83953e3a792a23`, le nonce `1484` et le hash de bloc `000e857b87a46fb7617d396f1a60c941e09318da4ba5ea9d3c37ac64e30daf54` pour l’en-tête du vecteur. Les champs complets sont dans §6.5.6 de `docs/specification-technique.md`.

---

## 6. Construire et miner un bloc

L’horodatage doit être calculé une fois et rester identique jusqu’à l’écriture SQL. Exemple de conversion UTC en millisecondes Unix :

```c
/* À placer avant tout autre include dans le fichier C. */
#define _POSIX_C_SOURCE 200809L
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

static bool utc_now_ms(int64_t *out)
{
    struct timespec now;
    uint64_t seconds;
    uint64_t milliseconds;

    if (out == NULL || clock_gettime(CLOCK_REALTIME, &now) != 0 ||
        now.tv_sec < 0) {
        return false;
    }
    seconds = (uint64_t)now.tv_sec;
    milliseconds = (uint64_t)(now.tv_nsec / 1000000L);
    if (seconds > ((uint64_t)INT64_MAX - milliseconds) / UINT64_C(1000)) {
        return false;
    }
    *out = (int64_t)(seconds * UINT64_C(1000) + milliseconds);
    return true;
}
```

`CLOCK_REALTIME` sert à dater le bloc. Pour mesurer un débit ou une durée de minage, utilisez une horloge monotone (`CLOCK_MONOTONIC`) et non l’horloge civile.

Exemple d’assemblage après que le serveur a déjà obtenu `next_index`, `prev_hash`, `timestamp_ms`, un débit mesuré côté serveur et un payload JCS :

```c
#include "crypto.h"

static crypto_status_t make_block(uint32_t next_index,
                                  const uint8_t previous_hash[32],
                                  int64_t timestamp_ms,
                                  uint32_t action_id,
                                  uint64_t measured_hashes_per_second,
                                  const canonical_json_t *payload,
                                  crypto_block_t *out,
                                  uint64_t *attempts_out)
{
    crypto_block_t candidate = {0};
    crypto_status_t status;
    bool below_target = false;

    if (previous_hash == NULL || payload == NULL || payload->data == NULL ||
        out == NULL || payload->len < CRYPTO_PAYLOAD_MIN_BYTES ||
        payload->len > CRYPTO_PAYLOAD_MAX_BYTES) {
        return CRYPTO_ERR_ARGUMENT;
    }
    status = crypto_difficulty_for_rate(measured_hashes_per_second,
                                        &candidate.header.difficulty,
                                        &below_target);
    if (status != CRYPTO_OK) {
        return status;
    }
    /* Si below_target est vrai, journaliser/alerter selon la politique serveur.
     * Le noyau conserve la difficulté minimale 3 ; il ne peut pas miner à 2. */
    (void)below_target;

    candidate.header.version = CRYPTO_HEADER_VERSION;
    candidate.header.block_index = next_index;
    candidate.header.timestamp_ms = timestamp_ms;
    memcpy(candidate.header.previous_hash, previous_hash, 32u);
    candidate.header.action_id = action_id;
    candidate.payload_canon = payload->data;       /* emprunté, pas copié */
    candidate.payload_canon_len = payload->len;

    status = crypto_block_mine(&candidate, attempts_out);
    if (status == CRYPTO_OK) {
        *out = candidate; /* transfert logique ; payload_canon reste emprunté */
    }
    return status;
}
```

Initialisez `previous_hash` à 32 octets nuls pour le genesis. Pour les autres blocs, décodez le hash hexadécimal du tip SQL en 32 octets bruts. Ne mettez jamais le hash hexadécimal ASCII directement dans `crypto_header_fields_t.previous_hash`.

`crypto_block_mine()` cherche à partir du nonce 0 avec le budget maximal défini pour la difficulté. Il n’offre pas d’annulation par délai mural. Ne faites donc pas le minage dans le callback `epoll` : placez la tâche dans un worker/une file bornée, avec un seul séquenceur de chaîne. Le débit fourni à `crypto_difficulty_for_rate()` doit être mesuré et configuré par le serveur, jamais par le client.

---

## 7. Ouvrir la connexion PostgreSQL avec libpq

Chaque thread/worker qui interroge PostgreSQL doit utiliser une connexion `PGconn` qui n’est pas utilisée simultanément par un autre thread. Pour simplifier l’architecture, un worker de persistance unique peut posséder une seule connexion et traiter une file de commandes.

```c
#include <libpq-fe.h>
#include <stdio.h>

static PGconn *open_database(void)
{
    PGconn *db = PQconnectdb("service=animetcg");

    if (db == NULL || PQstatus(db) != CONNECTION_OK) {
        fprintf(stderr, "connexion PostgreSQL impossible: %s\n",
                db != NULL ? PQerrorMessage(db) : "PQconnectdb a échoué");
        if (db != NULL) {
            PQfinish(db);
        }
        return NULL;
    }
    return db; /* l'appelant doit appeler PQfinish(db) une fois */
}
```

Pour les requêtes sans paramètres, vérifiez le statut puis libérez le résultat :

```c
static bool pg_command_ok(PGconn *db, const char *sql)
{
    PGresult *result = PQexec(db, sql);
    bool ok;

    if (result == NULL) {
        fprintf(stderr, "PostgreSQL: %s\n", PQerrorMessage(db));
        return false;
    }
    ok = PQresultStatus(result) == PGRES_COMMAND_OK;
    if (!ok) {
        fprintf(stderr, "PostgreSQL: %s\n", PQresultErrorMessage(result));
    }
    PQclear(result);
    return ok;
}
```

Pour des requêtes `SELECT`, le statut attendu est `PGRES_TUPLES_OK`, pas `PGRES_COMMAND_OK`. N’insérez jamais une donnée client par concaténation de SQL : utilisez `PQexecParams()` pour chaque valeur.

---

## 8. Sérialiser les écritures et lire le tip

Le champ `block_index` est unique ; deux workers ne doivent jamais lire le même tip puis miner deux blocs avec le même index et le même parent. Il faut un écrivain logique unique. La version simple ci-dessous utilise un verrou advisory transactionnel PostgreSQL ; **tous** les chemins d’écriture de blocs doivent employer la même paire `(301, 1)`.

```c
/* Après BEGIN, avant la lecture du tip. Le SELECT retourne PGRES_TUPLES_OK. */
PGresult *lock = PQexec(db, "SELECT pg_advisory_xact_lock(301, 1)");
if (lock == NULL || PQresultStatus(lock) != PGRES_TUPLES_OK) {
    if (lock != NULL) PQclear(lock);
    goto rollback;
}
PQclear(lock);
```

Verrouiller dans la transaction puis lire le dernier hash :

```sql
SELECT block_index::text, hash
FROM blocks
ORDER BY block_index DESC
LIMIT 1;
```

Règles d’interprétation :

- aucune ligne : `next_index = 0`, `previous_hash = 32 × 0x00` ;
- une ligne : parser l’index sans débordement, refuser `UINT32_MAX`, prendre `next_index = index + 1`, décoder `hash` en 32 octets ;
- ne calculez jamais l’index avec `COUNT(*)` : la source de vérité est le tip et la table est append-only ;
- validez également l’invariant `block_index` contigu au démarrage avec `crypto_verify_chain()`.

Fonctions de conversion sûres utiles à la couche persistance :

```c
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

static bool parse_u64_decimal(const char *text, uint64_t maximum,
                              uint64_t *out)
{
    char *end = NULL;
    const char *p;
    unsigned long long value;

    if (text == NULL || text[0] == '\0' || out == NULL) {
        return false;
    }
    for (p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return false; /* refuse signes/espaces */
    }
    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        (uint64_t)value > maximum) {
        return false;
    }
    *out = (uint64_t)value;
    return true;
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool hex_decode_exact(const char *text, uint8_t *out, size_t bytes)
{
    size_t i;

    if (text == NULL || out == NULL || strlen(text) != bytes * 2u) {
        return false;
    }
    for (i = 0u; i < bytes; ++i) {
        const int high = hex_value(text[2u * i]);
        const int low = hex_value(text[2u * i + 1u]);
        if (high < 0 || low < 0) return false;
        out[i] = (uint8_t)(((unsigned int)high << 4) | (unsigned int)low);
    }
    return true;
}

static void hex_encode(const uint8_t *bytes, size_t length, char *out)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;

    for (i = 0u; i < length; ++i) {
        out[2u * i] = digits[bytes[i] >> 4];
        out[2u * i + 1u] = digits[bytes[i] & 0x0fu];
    }
    out[2u * length] = '\0';
}
```

Ne donnez à `hex_decode_exact()` que des colonnes validées : le code vérifie également la longueur exacte et chaque nibble. Pour une chaîne SQL `CHAR(64)`, les hashes valides font déjà 64 caractères ; toute autre longueur est une erreur d’intégrité.

---

## 9. Insérer `blocks` avec paramètres SQL

La requête ci-dessous écrit simultanément la représentation interrogeable (`JSONB`) et les octets réellement hachés (`BYTEA`). Le cinquième paramètre est envoyé en format **binaire** ; sa longueur est explicite, donc aucun caractère nul ni échappement `bytea` n’est un problème.

```c
#include <inttypes.h>
#include <limits.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>

static bool pg_insert_block(PGconn *db, const crypto_block_t *block,
                            const char *action_type,
                            const canonical_json_t *payload)
{
    char index_text[16];
    char timestamp_text[32];
    char nonce_text[32];
    char difficulty_text[16];
    char previous_hex[CRYPTO_SHA256_HEX_BYTES + 1u];
    char hash_hex[CRYPTO_SHA256_HEX_BYTES + 1u];
    const char *values[9];
    int lengths[9] = {0};
    int formats[9] = {0};
    int written;
    PGresult *result;
    bool ok;

    if (db == NULL || block == NULL || action_type == NULL ||
        action_type[0] == '\0' || strlen(action_type) > 32u || payload == NULL ||
        payload->data == NULL || payload->len < CRYPTO_PAYLOAD_MIN_BYTES ||
        payload->len > CRYPTO_PAYLOAD_MAX_BYTES || payload->len > (size_t)INT_MAX ||
        payload->data[payload->len] != '\0' ||
        block->payload_canon != payload->data ||
        block->payload_canon_len != payload->len ||
        block->header.nonce > (uint64_t)INT64_MAX) {
        return false;
    }

    written = snprintf(index_text, sizeof(index_text), "%" PRIu32,
                       block->header.block_index);
    if (written < 0 || (size_t)written >= sizeof(index_text)) return false;
    written = snprintf(timestamp_text, sizeof(timestamp_text), "%" PRId64,
                       block->header.timestamp_ms);
    if (written < 0 || (size_t)written >= sizeof(timestamp_text)) return false;
    written = snprintf(nonce_text, sizeof(nonce_text), "%" PRIu64,
                       block->header.nonce);
    if (written < 0 || (size_t)written >= sizeof(nonce_text)) return false;
    written = snprintf(difficulty_text, sizeof(difficulty_text), "%" PRIu32,
                       block->header.difficulty);
    if (written < 0 || (size_t)written >= sizeof(difficulty_text)) return false;

    hex_encode(block->hash, CRYPTO_SHA256_BYTES, hash_hex);
    if (block->header.block_index == 0u) {
        if (memcmp(block->header.previous_hash,
                   (const uint8_t[CRYPTO_SHA256_BYTES]){0},
                   CRYPTO_SHA256_BYTES) != 0) {
            return false; /* genesis C = 32 zéros ; SQL = NULL */
        }
        values[5] = NULL;
    } else {
        hex_encode(block->header.previous_hash, CRYPTO_SHA256_BYTES,
                   previous_hex);
        values[5] = previous_hex;
    }

    values[0] = index_text;
    values[1] = timestamp_text;
    values[2] = action_type;
    values[3] = (const char *)payload->data; /* JCS UTF-8, terminé par '\0' */
    values[4] = (const char *)payload->data; /* BYTEA brut, format binaire */
    values[6] = nonce_text;
    values[7] = difficulty_text;
    values[8] = hash_hex;
    formats[4] = 1;                         /* paramètre 5 : binaire */
    lengths[4] = (int)payload->len;          /* taille vérifiée <= 65536 */

    result = PQexecParams(
        db,
        "INSERT INTO blocks "
        "(block_index, ts, action_type, payload, payload_canon, "
        " previous_hash, nonce, difficulty, hash) "
        "VALUES ($1::bigint, to_timestamp($2::double precision / 1000.0), "
        "$3::varchar, $4::jsonb, $5::bytea, $6::char(64), "
        "$7::bigint, $8::smallint, $9::char(64))",
        9, NULL, values, lengths, formats, 0);
    if (result == NULL) {
        fprintf(stderr, "INSERT blocks: %s\n", PQerrorMessage(db));
        return false;
    }
    ok = PQresultStatus(result) == PGRES_COMMAND_OK;
    if (!ok) {
        fprintf(stderr, "INSERT blocks: %s\n", PQresultErrorMessage(result));
    }
    PQclear(result);
    return ok;
}
```

L’appel suppose que `canonical_json_t` respecte le contrat de la section 5, que `action_type` provient d’une table serveur fermée, et que `block` vient d’être vérifié/miné. Pour une relecture plus défensive, appelez `crypto_verify_block(block)` avant la persistance, ou construisez le bloc exclusivement dans le worker de chaîne.

> Les paramètres SQL sont toujours séparés de la requête. Ne remplacez pas `PQexecParams()` par un `sprintf()` qui construit `INSERT ... '%s'` : le JSON, les noms et les raisons peuvent contenir des apostrophes ou des caractères hostiles.

---

## 10. Écrire le bloc et la projection métier dans la même transaction

La table `blocks` est append-only et constitue la source de vérité. Une création de carte doit aussi écrire `cards`, `card_genres` et `card_themes` dans la **même transaction**. Pour que l’historique reconstruise la projection, le payload canonique de l’événement doit contenir les données finales nécessaires, dont `card_id`.

### Attribuer l’identifiant de carte avant de figer l’événement

Comme `card_id` est une séquence `SERIAL`, réserver un identifiant sous le verrou de chaîne avant canonicalisation :

```sql
SELECT nextval(pg_get_serial_sequence('cards', 'card_id'))::text;
```

Les séquences PostgreSQL ne sont pas transactionnelles : un `ROLLBACK` peut laisser un trou dans `card_id`. C’est normal ; **les indices de blocs**, eux, restent attribués à partir du tip sous le verrou.

Le serveur construit ensuite le JSON d’événement avec `card_id`, `creator_id`/`owner_id` du contexte serveur, et les champs validés. Il le canonicalise JCS et mine son bloc.

### Projection de carte — SQL paramétré illustratif

Insérer d’abord le bloc, afin que la clé étrangère `cards.last_block_index` puisse le référencer, puis la carte :

```c
const char *card_values[] = {
    card_id_text, title_romaji, format_text, year_text, episodes_text,
    creator_id_text, owner_id_text, block_index_text
};

PGresult *r = PQexecParams(
    db,
    "INSERT INTO cards "
    "(card_id, title_romaji, format, year, episodes, creator_id, owner_id, "
    " status, last_block_index) "
    "VALUES ($1::integer, $2::varchar, $3::varchar, $4::smallint, "
    "$5::smallint, $6::integer, $7::integer, 'ACTIVE', $8::bigint)",
    8, NULL, card_values, NULL, NULL, 0);
/* Contrôler PGRES_COMMAND_OK puis PQclear(r). */
```

`episodes_text` peut être `NULL` si ce champ est absent et que le validateur/protocole l’autorise ; dans `PQexecParams`, un pointeur `NULL` signifie SQL `NULL`. Pour chaque genre/thème validé, exécuter une insertion paramétrée dans sa table normalisée :

```c
const char *genre_values[] = {card_id_text, genre_name};
PGresult *r = PQexecParams(
    db,
    "INSERT INTO card_genres(card_id, genre) VALUES ($1::integer, $2::varchar) "
    "ON CONFLICT (card_id, genre) DO NOTHING",
    2, NULL, genre_values, NULL, NULL, 0);
/* Même principe pour card_themes(card_id, theme). */
```

L’événement conserve les paramètres validés tels qu’ils ont été décidés ; la projection relationnelle représente ses ensembles par une ligne par valeur. La base peut donc dédupliquer une valeur répétée avec `ON CONFLICT`, sans modifier rétroactivement les octets canonisés. Appliquez le même motif adapté à chaque action (`recommendation`, `battle`, `claim`, `authentication`, `share`, etc.).

### Séquence de transaction côté serveur

```c
bool tx_open = false;
canonical_json_t canonical = {NULL, 0u};
json_t *event = NULL;

if (!pg_command_ok(db, "BEGIN")) goto cleanup;
tx_open = true;
if (!pg_lock_chain_writer(db)) goto rollback;
if (!pg_read_tip(db, &next_index, previous_hash)) goto rollback;
if (!reserve_card_id_if_needed(db, &card_id)) goto rollback;
if (!build_validated_event(request, session, card_id, &event)) goto rollback;
if (!payload_jcs_canonicalize(event, &canonical)) goto rollback;
if (!make_block(next_index, previous_hash, fixed_timestamp_ms, action_id,
                measured_hash_rate, &canonical, &block, &attempts)) goto rollback;
if (!pg_insert_block(db, &block, action_type, &canonical)) goto rollback;
if (!pg_apply_projection(db, event, block.header.block_index)) goto rollback;
if (!pg_command_ok(db, "COMMIT")) goto rollback;
tx_open = false;

/* Après le COMMIT seulement : envoyer RES OK, puis diffuser EVT. */
if (!send_success_response(client_fd, request_id, &block, card_id)) {
    /* La transaction est déjà commitée : ne pas annoncer un rollback fictif. */
    log_client_disconnect_after_commit(request_id, block.header.block_index);
}
goto cleanup;

rollback:
if (tx_open) {
    (void)pg_command_ok(db, "ROLLBACK");
    tx_open = false;
}
cleanup:
canonical_json_free(&canonical);
json_decref(event);
```

Les appels `pg_lock_chain_writer`, `pg_read_tip`, `reserve_card_id_if_needed`, `build_validated_event`, `pg_apply_projection` et `send_success_response` représentent des helpers à écrire dans les couches correspondantes. Le bloc ci-dessus montre l’ordre des opérations et la règle d’ownership ; il n’est pas un fichier compilable autonome.

### Pourquoi répondre après le `COMMIT` ?

- Avant `COMMIT`, la base peut encore annuler le bloc ou une projection.
- Après `COMMIT`, le client reçoit le `block_index` et le hash de l’état réellement durable.
- La diffusion `EVT` part après la transaction ; un événement ne doit pas annoncer une carte que PostgreSQL vient d’annuler.
- Si la socket tombe après le commit mais avant la réponse, le bloc reste valide. Pour éviter un doublon lors d’une relance client, ajoutez une vraie stratégie d’idempotence (par exemple conserver l’ID de requête dans l’événement et/ou une table de déduplication). Le schéma courant ne fournit pas à lui seul cette garantie.

### Verrou pendant le minage

L’exemple simple conserve le verrou advisory et la transaction pendant le minage afin de ne pas construire deux blocs sur le même tip. C’est facile à raisonner, mais cela garde une transaction ouverte pendant le calcul. Pour améliorer l’architecture, sérialisez toutes les actions modifiantes dans un **unique worker de chaîne** ; si le minage est déplacé hors transaction/verrou, relisez et revalidez le tip avant insertion, et abandonnez/recommencez si le tip a changé. N’autorisez jamais deux écrivains à persister le même `next_index`.

---

## 11. Construire la réponse du protocole

Une réponse de succès reprend exactement l’ID de corrélation du client et n’est envoyée qu’après commit :

```json
{
  "v": "1.0",
  "id": "ab12c3d4e5f6",
  "type": "RES",
  "status": "OK",
  "payload": {
    "card_id": 42,
    "block_index": 118,
    "hash": "000e857b87a46fb7617d396f1a60c941e09318da4ba5ea9d3c37ac64e30daf54"
  }
}
```

Construisez les réponses JSON avec Jansson (ou le constructeur de messages du protocole) afin que les chaînes soient correctement échappées. Ne faites pas `sprintf()` d’un pseudo ou d’un titre dans une chaîne JSON. Après sérialisation, utilisez `send_frame()` avec le nombre exact d’octets.

Pour un événement diffusé, le protocole impose `"type":"EVT"`, `"id":null`, un nom d’événement (par exemple `card.created`) et son payload. Les événements ne remplacent pas la réponse corrélée à la requête qui a déclenché l’action.

---

## 12. Relire et vérifier la chaîne depuis PostgreSQL

### Requête de chargement

La vérification doit lire les blocs dans l’ordre et conserver le payload binaire exact. Exemple de requête :

```sql
SELECT block_index::text,
       floor(extract(epoch FROM ts) * 1000)::bigint::text AS timestamp_ms,
       action_type,
       encode(payload_canon, 'hex') AS payload_hex,
       previous_hash,
       nonce::text,
       difficulty::text,
       hash
FROM blocks
ORDER BY block_index ASC;
```

Conversion de chaque ligne en `crypto_block_t` :

1. `version = CRYPTO_HEADER_VERSION` (1) ;
2. parser `block_index` sans dépasser `UINT32_MAX` ;
3. parser `timestamp_ms` en entier signé et vérifier qu’il est non négatif ;
4. convertir `action_type` par la table de correspondance gelée vers `action_id` ;
5. pour le genesis (`previous_hash IS NULL`), remplir les 32 octets avec zéro ; sinon décoder exactement 64 caractères hexadécimaux ;
6. décoder `payload_hex` en mémoire ; vérifier 2..65 536 octets ; affecter ce tampon emprunté à `block.payload_canon` ;
7. parser `nonce` et `difficulty`, décoder `hash` en 32 octets ;
8. garder tous les payloads vivants jusqu’au retour de `crypto_verify_chain()`.

Puis :

```c
crypto_verify_report_t report;
crypto_status_t status = crypto_verify_chain(blocks, block_count, &report);

if (status != CRYPTO_OK) {
    fprintf(stderr, "chaîne invalide à la position %zu, bloc %" PRIu32 ": %s\n",
            report.block_position, report.block_index,
            crypto_status_string(status));
}
```

`crypto_verify_chain()` attend un tableau contigu et renvoie la première erreur. Le loader PostgreSQL doit donc allouer un tableau de blocs et un tampon possédé par payload, puis les libérer après la vérification :

```c
crypto_status_t status = CRYPTO_OK;
crypto_block_t *blocks = NULL;
uint8_t **payloads = NULL;

if (block_count == 0u) {
    status = CRYPTO_ERR_CHAIN_EMPTY;
    goto cleanup;
}
if (block_count > SIZE_MAX / sizeof(*blocks) ||
    block_count > SIZE_MAX / sizeof(*payloads)) {
    status = CRYPTO_ERR_ARGUMENT; /* dans le loader, préférez un statut DB dédié */
    goto cleanup;
}
blocks = calloc(block_count, sizeof(*blocks));
payloads = calloc(block_count, sizeof(*payloads));
if (blocks == NULL || payloads == NULL) {
    /* Remplacer par une erreur mémoire du serveur, puis passer au cleanup. */
    goto cleanup;
}

/* Pour chaque ligne i : payloads[i] = malloc(payload_len);
 * décoder les octets, puis :
 * blocks[i].payload_canon = payloads[i];
 * blocks[i].payload_canon_len = payload_len;
 * remplir blocks[i].header et blocks[i].hash.
 */

status = crypto_verify_chain(blocks, block_count, &report);

cleanup:
if (payloads != NULL) {
    for (size_t i = 0u; i < block_count; ++i) {
        free(payloads[i]); /* free(NULL) est autorisé */
    }
}
free(payloads);
free(blocks);
if (result != NULL) PQclear(result);
```

En vrai code, contrôlez avant les allocations `block_count <= SIZE_MAX / sizeof(*blocks)`, la taille totale des payloads et votre budget mémoire. La table n’a pas de nombre maximal de blocs et l’API actuelle reçoit toute la chaîne en mémoire. Pour une longue chaîne, il faut ajouter un vérificateur incrémental/streaming ou une politique de vérification par segments qui transporte le hash attendu d’un segment au suivant ; ne chargez pas une base illimitée dans une seule allocation.

La vérification cryptographique ne valide pas automatiquement `cards.value_active` ni les autres tables de projection. Un audit complet doit aussi rejouer les événements ou comparer les projections reconstruites aux tables SQL.

---

## 13. Ownership, allocations et erreurs

### Règles de propriété mémoire

| Objet | Créé/libéré par | Durée de vie requise |
|---|---|---|
| Tampon d’une trame (`body`) | Couche `net` | Jusqu’à ce que le parseur ait fini ; copier/transférer avant réutilisation du tampon `epoll` |
| `json_t *root` | `json_loadb()` / appelant (`json_decref`) | Toute la durée où les pointeurs retournés par `json_object_get()` sont utilisés |
| Événement JSON métier | Couche `business` | Jusqu’à la canonicalisation JCS |
| `canonical_json_t.data` | Canonicaliseur / appelant (`canonical_json_free`) | Pendant minage, insertion JSONB/BYTEA et vérification éventuelle |
| `crypto_block_t.payload_canon` | **Emprunté** par le noyau crypto | Les octets doivent rester vivants et immuables jusqu’à la fin de l’appel ; le noyau ne les libère jamais |
| `PGconn *` | `PQconnectdb()` / `PQfinish()` | Une connexion active par worker propriétaire ; pas de partage simultané |
| `PGresult *` | `PQexec*()` / `PQclear()` | Copier les valeurs nécessaires avant `PQclear()` |
| Tampons chargés depuis `BYTEA` | loader DB / `free()` | Jusqu’à la fin de `crypto_verify_chain()` |
| Texte de `json_dumps()` | sérialiseur JSON / allocateur Jansson | Jusqu’à `send_frame()`, puis libérer avec l’allocateur compatible |

Le module `server/chain` n’appelle pas directement `malloc`, `calloc`, `realloc` ou `free`. La couche DB, les files réseau, Jansson et le canonicaliseur peuvent, eux, allouer : chaque chemin de succès **et d’erreur** doit avoir un propriétaire clair.

### Table de décision d’erreurs

| Échec | Action serveur | Effet sur PostgreSQL / socket |
|---|---|---|
| Préfixe invalide, trame > 65 536, JSON malformé ou troncature | Rejeter la trame, journaliser sans recopier de payload hostile | Fermer la socket si erreur de trame ; aucune transaction métier |
| Action/champs invalides ou non autorisés | Réponse `ERR` correspondante du catalogue protocolaire | Aucun bloc, aucune projection |
| JCS indisponible/erreur de canonicalisation | Erreur interne ; ne pas tenter de hacher un JSON arbitraire | `ROLLBACK` si la transaction est déjà ouverte |
| `CRYPTO_ERR_*` | Mapper vers erreur protocole interne/serveur, journaliser `crypto_status_string()` | `ROLLBACK`, aucune diffusion |
| `PQexecParams()`/contrainte/FK en échec | Journaliser l’erreur serveur sans l’exposer brute au client | `ROLLBACK` |
| `send()` échoue après commit | Le bloc reste durable ; journaliser déconnexion | Ne pas simuler un rollback ; appliquer l’idempotence si le client réessaie |

Le code de protocole précis dépend du catalogue existant : les erreurs de paramètres utilisent les codes `-32600`/`-32602`, une saturation/minage impossible peut être traduite par `-32001 SERVER_BUSY`, et une erreur inattendue par l’erreur interne documentée. N’envoyez jamais `PQerrorMessage()` tel quel au client.

---

## 14. Sécurité et limites de périmètre

- Le projet utilise un socket TCP brut ; **TLS et le chiffrement du canal sont explicitement hors périmètre**. N’utilisez pas ce protocole sur un réseau non fiable en prétendant qu’il est chiffré.
- SHA-256 et PoW protègent la cohérence des octets enregistrés ; ils ne prouvent pas qu’une action métier ou les données client sont vraies.
- Le client ne choisit jamais `block_index`, `previous_hash`, `action_id`, `difficulty`, nonce final ou hash.
- La difficulté vient de la politique serveur. `crypto_difficulty_for_rate()` prend un débit déjà mesuré ; le banc de calibration complet reste à intégrer.
- La base de données et la projection ne deviennent cohérentes que si leurs écritures sont atomiques dans la même transaction.
- L’accès SQL est paramétré ; les données journalisées sont limitées et échappées conformément à `docs/note-format-ligne.md`.

---

## 15. Recette d’intégration recommandée

### Tests par étape

1. **Framing socket** : fragmenter le préfixe sur plusieurs `send()`, fragmenter le corps, envoyer deux trames successives, tester EOF propre, trame tronquée, longueur 0, longueur 65 537.
2. **Parsing JSON** : mauvaise version, mauvais type d’enveloppe, ID invalide, clés JSON dupliquées, payload non objet, actions inconnues, champs métier invalides.
3. **JCS** : tester des golden vectors RFC 8785, l’ordre des clés, les nombres, les caractères de contrôle et Unicode ; vérifier que canonicaliser deux fois le même événement donne exactement les mêmes octets.
4. **Noyau crypto** : exécuter `make -C server/chain test` et `make -C server/chain sanitize` ; conserver le golden vector du dépôt.
5. **Persistance** : confirmer que `payload_canon` relu correspond octet pour octet à l’entrée, que `payload JSONB` peut différer en forme textuelle, et que la projection est écrite dans la même transaction.
6. **Concurrence** : lancer deux créations simultanées ; les index doivent rester contigus, un seul writer doit utiliser le tip à la fois, aucun hash ne doit partager le même parent/index par course.
7. **Échec atomique** : provoquer un échec d’insertion de projection après l’insert `blocks` ; vérifier que le `ROLLBACK` retire le bloc et que rien n’est diffusé.
8. **Vérification** : arrêter le serveur, relire toute la chaîne depuis PostgreSQL, reconstruire `crypto_block_t[]`, appeler `crypto_verify_chain()` ; altérer une copie de test, jamais la base de travail.
9. **Mémoire** : compiler sous avertissements stricts, ASan/UBSan, GCC `-fanalyzer`; exécuter Memcheck/Valgrind dans l’environnement où il est installé.

### Vérifier une action réussie de bout en bout

Pour `card.create`, le journal de test doit permettre de corréler, sans enregistrer tout le JSON :

```text
requête reçue (id=ab12c3d4e5f6)
→ validation card.create réussie
→ payload JCS prêt (longueur en octets)
→ bloc miné (block_index, difficulté, nombre d'essais, hash)
→ COMMIT bloc + carte + genres + thèmes
→ réponse RES OK au même id
→ EVT card.created diffusé après le COMMIT
```

Ne journalisez pas d’informations sensibles ni une charge utile complète par défaut. Le hash seul n’est pas un mécanisme de confidentialité.

---

## 16. Points à compléter avant de dire « intégré »

| Point | À livrer dans le dépôt | Pourquoi c’est nécessaire |
|---|---|---|
| `payload_jcs_canonicalize()` | Canonicaliseur RFC 8785/JCS en C, tests de conformité et contrat d’ownership | Le module crypto ne valide pas la canonicalité et la fiche de fonctionnalité suppose des octets JCS |
| Couche framing/`epoll` | `server/net/frame.c/.h` avec lecteur d’état non bloquant, write queue et tests | Le protocole impose une trame TCP 4-octets + JSON et l’architecture serveur retient `epoll` |
| Parser/dispatcher | Jansson, versionnement, validation, mapping des actions | L’enveloppe `REQ` ne doit pas parvenir brute à la couche de chaîne |
| Mapping d’actions | Mapping numérique stable `action_id ↔ action_type` pour toutes les actions, dont genesis | La base conserve le nom tandis que le hash contient l’identifiant numérique |
| Couche persistance | `PGconn`, verrou de writer, lecture du tip, `PQexecParams`, rollback, projections atomiques | Les fonctions crypto ne lisent/écrivent pas PostgreSQL |
| Vérificateur DB | Loader avec conversions sûres + politique mémoire / API streaming si nécessaire | `crypto_verify_chain()` reçoit un tableau en mémoire et le schéma stocke certaines données sous forme texte/hex |
| Idempotence des requêtes | Stockage/déduplication de `id` ou d’une clé d’action | Un client peut se déconnecter après commit sans recevoir la réponse |

---

## 17. Références du projet

- `docs/protocole-applicatif.md` — trames, enveloppes `REQ`/`RES`/`EVT`, actions, erreurs, compatibilité.
- `docs/specification-technique.md` — architecture en couches, contrat d’en-tête, JCS, transaction bloc/projection et vérification.
- `docs/fiche-fonctionnalite-cryptographie.md` — périmètre crypto, règles et limites du noyau.
- `sql/schema.sql` — schéma PostgreSQL actuel, contraintes et tables de projections.
- `server/chain/crypto.h` et `server/chain/test_crypto.c` — API C et exemples testés du noyau.
- RFC 8785 — JSON Canonicalization Scheme : <https://www.rfc-editor.org/rfc/rfc8785>.
