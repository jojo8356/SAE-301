# Guide d'intégration — Logger C AnimeTCG

| Métadonnée | Valeur |
|---|---|
| **Version** | 1.0 — 14/10/2026 |
| **API** | `server/util/log.h` |
| **Fiche de fonctionnalité** | `docs/fiche-fonctionnalite-logger-c.md` |
| **Format normatif** | `docs/note-format-ligne.md` v1.4, §5–§13 |

> **État d'intégration :** le logger maison est implémenté et testé dans `server/util/`. Le serveur applicatif complet et son point d'entrée ne sont pas encore présents dans ce checkout. Les extraits ci-dessous montrent le contrat réel de l'API C ; ils ne prétendent pas être la preuve d'une exécution de bout en bout dans le serveur.

## 1. Résumé et règles d'or

1. Initialiser une seule instance **avant** de démarrer les threads ou la boucle d'événements.
2. Émettre un identifiant `ANIMETCG_LOG_EVENT_*`, jamais un message libre.
3. Passer les valeurs par `animetcg_log_field_t`, avec les types et clés du catalogue.
4. Régler puis effacer explicitement le contexte `client_id`/`request_id` à chaque requête.
5. Ne jamais journaliser la charge JSON/protocolaire, un secret, un titre ou un texte utilisateur complet.
6. Traiter `ANIMETCG_LOG_DROPPED` comme une perte comptabilisée ; ne pas boucler pour la réessayer.
7. Réserver `flush` et `shutdown` aux chemins hors boucle d'événements.
8. Arrêter les producteurs, puis appeler `animetcg_log_shutdown()` une fois.

## 2. Fichiers, inclusion et édition de liens

Les fichiers à compiler ensemble sont :

```text
server/util/log.c
server/util/log_events.c
```

Les sources applicatives incluent le header public :

```c
#include "util/log.h"
```

Depuis le répertoire racine, ajouter l'include root du serveur et `-pthread` au build C11. Exemple GNU/Linux :

```sh
cc -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic \
   -Iserver server/main.c server/util/log.c server/util/log_events.c \
   -pthread -o server/build/animetcg-server
```

Adapter les autres fichiers et options du programme. Le Makefile de recette du logger est autonome :

```sh
make -C server/util clean test
make -C server/util sanitize
# Cible d'agrégation du serveur :
make -C server test-logger
```

Ne pas compiler `log.c` deux fois si votre build découvre automatiquement tous les `.c`. La définition `ANIMETCG_LOG_TESTING` est réservée au harnais et ne doit pas être activée dans le binaire serveur.

## 3. Démarrage et arrêt

### Initialiser une fois

L'initialisation crée le dossier si nécessaire, ouvre le fichier et démarre le fil écrivain. Elle peut échouer : dans ce cas, le programme doit échouer proprement ou choisir explicitement un mode dégradé. Ne pas prétendre que la journalisation fonctionne si l'ouverture a échoué.

```c
#include "util/log.h"

#include <stdio.h>

static int start_logging(void)
{
    const animetcg_log_config_t config = {
        .directory = "logs",
        .filename = "server.log",
        .minimum_level = ANIMETCG_LOG_LEVEL_INFO
    };
    const animetcg_log_status_t status = animetcg_log_init(&config);

    if (status != ANIMETCG_LOG_OK) {
        /* Secours avant logger : message fixe, sans valeur utilisateur. */
        (void)fputs("server: logger initialization failed\n", stderr);
        return -1;
    }
    return 0;
}
```

`directory` est relatif au répertoire de travail du processus. En déploiement, fixer le `WorkingDirectory` (par exemple le dossier d'installation du serveur) ou passer un chemin absolu contrôlé. Le logger ne dérive pas ce chemin de `argv[0]`.

Niveau configurable au démarrage, sans reconstruire le programme :

```sh
ANIMETCG_LOG_LEVEL=DEBUG ./animetcg-server
```

Valeurs autorisées : `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`, `OFF`. La variable est lue à l'initialisation. Une valeur invalide conserve `minimum_level` et écrit une note fixe sur `stderr`. Ne jamais construire cette variable à partir d'une entrée utilisateur.

### Arrêt ordonné

Le point d'arrêt doit respecter cet ordre :

```c
/* 1. Refuser de nouvelles connexions/requêtes. */
/* 2. Arrêter puis joindre tous les threads producteurs. */
/* 3. Émettre server stopped (si le contexte et la stratégie le permettent). */
(void)animetcg_log_emit(ANIMETCG_LOG_EVENT_SERVER_STOPPED, NULL, 0u);
/* 4. Draine la file, fdatasync, ferme et libère le buffer. */
if (animetcg_log_shutdown() != ANIMETCG_LOG_OK) {
    /* L'arrêt du logger a rencontré une erreur ; traiter selon la politique du serveur. */
}
```

Ne pas appeler `shutdown` depuis un thread qui doit encore produire. L'appel est synchronisé et peut attendre le drainage du ring et la fermeture du fichier.

## 4. Émettre un événement structuré

Les noms d'événements, niveaux, modules et textes sont figés dans `server/util/log_events.c`. Ajouter un nouveau fait signifie faire évoluer le catalogue et la note de format, pas concaténer une phrase dynamique.

### Événement avec entier et texte

```c
static void log_card_created(uint64_t card_id, uint32_t owner_id)
{
    const animetcg_log_field_t fields[] = {
        animetcg_log_u64(ANIMETCG_LOG_KEY_CARD_ID, card_id),
        animetcg_log_u64(ANIMETCG_LOG_KEY_USER_ID, owner_id)
    };

    (void)animetcg_log_emit(ANIMETCG_LOG_EVENT_CARD_CREATED,
                            fields, sizeof(fields) / sizeof(fields[0]));
}
```

Exemple de ligne (le timestamp et les valeurs réelles varieront) :

```text
2026-10-14T12:30:00.123Z | INFO | srv | c=42 | r=7af03812c0de | business | card created | card_id=981 user_id=42
```

Si le niveau courant est au-dessus de `INFO`, l'appel est filtré. Le code de production peut ignorer le retour pour une télémétrie non critique, mais doit examiner les erreurs d'argument/privacy lors de l'ajout d'un nouvel appel.

### Données réseau en tranche d'octets

Pour une tranche qui n'est pas NUL-terminée, ne pas passer un `%s` ou copier des octets dans un buffer temporaire :

```c
static void log_frame_read(uint64_t wire_length)
{
    const animetcg_log_field_t fields[] = {
        animetcg_log_u64(ANIMETCG_LOG_KEY_BYTES, wire_length)
    };

    (void)animetcg_log_emit(ANIMETCG_LOG_EVENT_FRAME_READ,
                            fields, sizeof(fields) / sizeof(fields[0]));
}
```

Si le contrat autorise réellement un extrait textuel, `animetcg_log_text_n(key, ptr, length)` accepte une tranche mémoire ; elle est neutralisée et bornée. N'utiliser ce constructeur **jamais** pour une charge utile complète.

### Décimal à une décimale

Le constructeur `animetcg_log_decimal_1(key, tenths)` reçoit un entier en dixièmes : `125` produit `12.5`, `-3` produit `-0.3`. Utiliser uniquement une clé dont le registre autorise ce type.

## 5. Corrélation par requête et par client

Le contexte est local au fil d'exécution. Le logger copie les valeurs dans chaque record ; il ne conserve pas les pointeurs des appels.

```c
static void handle_request(uint32_t client_id, const char *request_id)
{
    const animetcg_log_status_t context_status =
        animetcg_log_set_context(client_id, request_id);

    if (context_status != ANIMETCG_LOG_OK) {
        /* L'identifiant doit être exactement 12 hexadécimaux minuscules. */
        return;
    }

    (void)animetcg_log_emit(ANIMETCG_LOG_EVENT_REQUEST_RECEIVED, NULL, 0u);

    /* Traiter la requête, puis journaliser ses résultats structurés. */

    animetcg_log_clear_context();
}
```

### Contexte dans un pool de threads

Le TLS ne traverse pas une file de tâches. Copier les identifiants dans le travail, puis installer et retirer le contexte dans le fil qui effectue réellement les logs :

```c
typedef struct {
    uint32_t client_id;
    char request_id[13]; /* 12 caractères + NUL. */
    /* Données de tâche métier ; pas de secret dans le contexte de journal. */
} request_job_t;

static void run_job(const request_job_t *job)
{
    if (animetcg_log_set_context(job->client_id, job->request_id) !=
        ANIMETCG_LOG_OK) {
        return;
    }

    (void)animetcg_log_emit(ANIMETCG_LOG_EVENT_REQUEST_ACCEPTED, NULL, 0u);
    /* Traitement de la tâche. Toutes les sorties doivent suivre les règles privacy. */
    animetcg_log_clear_context();
}
```

Pour gérer les erreurs sans `goto`, place le traitement dans une fonction auxiliaire qui retourne un statut ; la fonction enveloppe appelle ensuite toujours `animetcg_log_clear_context()` avant de retourner. Un contexte oublié peut associer le prochain travail du même thread au mauvais client.

Pour un événement sans requête ni client, poser explicitement `ANIMETCG_LOG_NO_CLIENT_ID` et `NULL`, ou effacer le contexte, avant de loguer.

## 6. Contrôle de niveau et travail coûteux

`animetcg_log_emit()` filtre lui-même l'événement. Si la construction des champs a un coût notable, vérifier d'abord le niveau :

```c
if (animetcg_log_is_enabled(ANIMETCG_LOG_EVENT_QUERY_DONE)) {
    const animetcg_log_field_t fields[] = {
        animetcg_log_u64(ANIMETCG_LOG_KEY_MS, query_duration_ms)
    };
    (void)animetcg_log_emit(ANIMETCG_LOG_EVENT_QUERY_DONE,
                            fields, sizeof(fields) / sizeof(fields[0]));
}
```

`ANIMETCG_LOG_KEY_MS` est une clé déclarée dans `log.h`. Vérifier en plus la matrice événement-clé du catalogue normatif avant d'utiliser une combinaison dans le code applicatif.

L'API ne permet pas de changer le niveau de l'événement à l'appel : le catalogue fait autorité. `animetcg_log_set_level()` est thread-safe et modifie uniquement le seuil global.

## 7. Politique des champs et confidentialité

### Liste de contrôle avant chaque appel

- L'événement est-il exactement celui prévu par la note de format ?
- Les champs figurent-ils dans `animetcg_log_key_t` et dans le tableau de type/usage de la note ?
- Le nombre de champs est-il au plus 8 ?
- Les chaînes sont-elles des identifiants non sensibles, et non des textes utilisateurs ?
- La valeur reste-t-elle sous les limites (64 caractères/valeur, 512 octets/ligne) ?
- L'événement est-il autorisé côté serveur (`client_only == false`) et non déprécié ?

**Portée du contrôle de l'API :** le logger vérifie l'identifiant d'événement, l'appartenance de la clé à l'énumération C, le type générique autorisé, les limites et l'assainissement. Il n'applique pas les listes sémantiques de valeurs (`action`, `status`, `reason`, `op`, etc.) ni la matrice de champs de chaque événement ; le code métier doit les respecter. L'unique règle de champ/événement imposée spécialement par l'API est `ip` sur `client address` en `DEBUG`. Même dans ce cas, l'API ne valide pas la syntaxe IPv4/IPv6 : fournir une adresse déjà validée par la couche réseau. La comparaison de cette matrice et des listes fermées reste une revue/test du producteur, pas une garantie du formatter.

### Adresse IP

Une adresse IP ne peut être émise que comme clé `ip` sur l'événement dédié `ANIMETCG_LOG_EVENT_CLIENT_ADDRESS`, qui est `DEBUG`. Tout autre événement portant `ANIMETCG_LOG_KEY_IP` retourne `ANIMETCG_LOG_ERR_PRIVACY_POLICY`. N'activer cette trace que pour un diagnostic justifié, et désactiver `DEBUG` ensuite.

```c
static void log_peer_address(const char *ip_text)
{
    const animetcg_log_field_t field =
        animetcg_log_text(ANIMETCG_LOG_KEY_IP, ip_text);
    const animetcg_log_status_t status =
        animetcg_log_emit(ANIMETCG_LOG_EVENT_CLIENT_ADDRESS, &field, 1u);
    if (status != ANIMETCG_LOG_OK && status != ANIMETCG_LOG_FILTERED &&
        status != ANIMETCG_LOG_DROPPED) {
        /* Ne pas réémettre l'adresse dans un message d'erreur. */
    }
}
```

## 8. Gérer les résultats, erreurs et statistiques

```c
const animetcg_log_status_t status =
    animetcg_log_emit(ANIMETCG_LOG_EVENT_DB_WRITE_FAILED, fields, field_count);

switch (status) {
case ANIMETCG_LOG_OK:
    break; /* placé dans la file, pas encore forcément écrit sur disque */
case ANIMETCG_LOG_FILTERED:
    break; /* sous le seuil configuré */
case ANIMETCG_LOG_DROPPED:
    break; /* perte déjà comptée ; ne pas réessayer en boucle */
case ANIMETCG_LOG_ERR_PRIVACY_POLICY:
case ANIMETCG_LOG_ERR_ARGUMENT:
    /* Défaut d'appel à corriger ; éviter une récursion dans le logger. */
    break;
case ANIMETCG_LOG_ERR_NOT_RUNNING:
case ANIMETCG_LOG_ERR_STATE:
case ANIMETCG_LOG_ERR_IO:
case ANIMETCG_LOG_ERR_TIMEOUT:
case ANIMETCG_LOG_ERR_BUFFER_TOO_SMALL:
    /* Politique applicative : sortie fixe stderr, arrêter proprement, etc. */
    break;
}
```

Pour la santé du logger, `animetcg_log_get_stats()` fournit les compteurs `accepted`, `written`, `dropped`, `pending_dropped`, `io_errors`, `invalid_calls`, `rotations`, `queue_depth`, `queue_high_water`, `in_flight`. Lire ces compteurs à intervalle raisonnable, hors boucle critique. L'événement interne `log dropped` est une synthèse périodique (au plus toutes les 5 secondes pendant une saturation persistante) ; ne pas créer de second message de perte à chaque requête.

Le résultat `ANIMETCG_LOG_OK` signifie **mis en file**, non durablement écrit. La durabilité est demandée au `shutdown` via `fdatasync`; aucun flush synchronisé n'est exécuté par appel.

## 9. Écriture hors de la boucle d'événements

`animetcg_log_emit()` ne fait pas d'E/S disque et utilise un `pthread_mutex_trylock` : même la contention se traduit par `ANIMETCG_LOG_DROPPED`. Ne jamais contourner cette API par `fprintf` sur le fichier de log.

`animetcg_log_flush(timeout_ms)` attend le ring buffer et l'écriture active. Son utilisation est réservée à une barrière hors boucle (tests, commande administrative dédiée). Ne pas appeler dans le traitement d'un paquet, l'envoi d'une réponse, un callback de socket ou un fil temps réel.

Le thread interne est un fil de fond unique. Il n'est pas un fil par client et ne doit pas être démarré une seconde fois par module.

## 10. Fichiers, permissions, rotation et rétention

Par défaut, passer `directory="logs"`, `filename="server.log"`. À l'initialisation, le logger crée le répertoire s'il manque puis force son mode à `0750`. Il ouvre le fichier actif avec `O_CREAT` et force son mode à `0640`, y compris s'il existait déjà ; le `umask` ne réduit pas ces modes après `chmod`/`fchmod`. Le compte de service doit être propriétaire ou avoir les droits nécessaires. Ne pas rendre `logs/` accessible à tous.

La rotation est intégrée au thread d'écriture : seuil 10 MiB, actif plus `.1` à `.4`, donc cinq fichiers maximum par instance, 50 MiB nominal. Le plus ancien est supprimé. L'ouverture en `O_RDWR|O_APPEND` permet de vérifier la fin du fichier : après un arrêt brutal, le logger retire uniquement le dernier suffixe dépourvu de `LF` jusqu'au record complet précédent. En cas d'écriture partielle, il tente le même rollback ; si celui-ci échoue, il ferme le descripteur et ne rajoute pas de lignes derrière le fragment. Les records complets restent append-only.

La purge de rétention est séparée du logger mais le script d'exploitation `scripts/logs-purge.sh` est livré : par défaut, il cible `./logs`, prend les fichiers réguliers directs dont le nom correspond à `*.log*`, et conserve les 180 derniers jours. Il ne suit ni sous-répertoires ni liens symboliques et refuse la racine du système ou le répertoire de travail courant comme cible.

Toujours examiner la liste avant suppression, puis planifier la commande d'effacement avec le gestionnaire de tâches du déploiement :

```sh
scripts/logs-purge.sh --dry-run
# Après validation de la liste, à intégrer à une tâche systemd/cron :
scripts/logs-purge.sh --dir /var/log/animetcg --days 180
```

Le script ne crée pas de timer : la fréquence, les droits, la supervision et la preuve d'exécution sont à définir sur la machine cible. La centralisation via syslog/journald est également une configuration système séparée : le composant C n'implémente pas d'appender syslog.

## 11. Tests et ajout d'un événement/une clé

### Tests avant merge

```sh
make -C server lint
make -C server/util clean test
make -C server/util sanitize
```

Vérifier aussi la purge sans toucher aux journaux réels, dans un répertoire temporaire :

```sh
set -eu
purge_test_dir=$(mktemp -d)
trap 'rm -rf -- "$purge_test_dir"' EXIT
touch -d '181 days ago' "$purge_test_dir/old.log"
touch "$purge_test_dir/current.log"
scripts/logs-purge.sh --dry-run --dir "$purge_test_dir" --days 180
scripts/logs-purge.sh --dir "$purge_test_dir" --days 180
test ! -e "$purge_test_dir/old.log"
test -f "$purge_test_dir/current.log"
```

Si disponible, lancer également :

```sh
make -C server/util memcheck
```

Le Makefile n'utilise aucun framework de tests tiers. Ne pas considérer une compilation seule comme preuve : le harnais maison vérifie le catalogue, la confidentialité, l'assainissement, la concurrence, la rotation, la saturation, le contexte, les valeurs/format, le nettoyage d'une fin de fichier incomplète, le rollback d'une écriture partielle et les statistiques. Le script de purge se teste séparément sur un répertoire temporaire, sans framework tiers.

### Ajouter un événement

1. Confirmer avec l'équipe qu'il s'agit d'un **nouveau fait**, pas d'une interpolation de donnée dans un message existant.
2. Définir son texte anglais fermé, module et niveau en accord avec `docs/note-format-ligne.md` §8.2.
3. Ajouter son identifiant à la fin de `animetcg_log_event_t` (ou documenter explicitement toute exception de compatibilité) et l'entrée à la même position dans `EVENT_CATALOGUE`.
4. Marquer `client_only`/`deprecated` correctement ; les événements client-only et dépréciés ne sont pas émis par le logger serveur.
5. Étendre tests de catalogue et cas d'émission, puis mettre à jour la documentation fonctionnelle, le guide et le nombre/version de la note.
6. Relancer `clean test`, `sanitize`, `memcheck` si installé et la suite serveur.

Le `_Static_assert` vérifie uniquement l'égalité du nombre d'entrées enum/table ; il ne détecte pas un décalage sémantique entre deux lignes de même compte. Le harnais vérifie le compte (70), quelques politiques et la syntaxe des lignes qu'il produit ; il ne compare pas automatiquement chaque événement, niveau, clé ou valeur de cette note et de la table Markdown. Toute évolution doit donc être relue contre `docs/note-format-ligne.md` et recevoir des assertions explicites dans `test_log.c`.

### Ajouter une clé

1. Valider le nom normalisé, son type, sa limite, son sens et la nécessité d'une nouvelle clé plutôt qu'une clé existante.
2. L'ajouter en fin de l'énumération `animetcg_log_key_t` et de la note (la clé `extra` reste gérée par le formatter comme prévu au contrat).
3. Étendre les métadonnées de type, les valeurs autorisées, les tests et les exemples.
4. Ajouter une règle d'interdiction si la clé expose une donnée personnelle ou un secret ; les données interdites ne deviennent pas autorisées par l'ajout d'une clé.

## 12. Intégration système et diagnostic

- Les chemins sont relatifs au `WorkingDirectory` du service.
- Configurez le gestionnaire `systemd` pour démarrer sous un compte non privilégié et préserver les permissions `0750/0640`.
- Pour centraliser, configurer un agent local (p. ex. rsyslog `imfile`) qui lit le fichier rotatif ; tester les frontières de rotation et l'absence de doublons. Ce n'est pas un appel socket du code C.
- Les lignes sont UTF-8, mono-ligne ; elles peuvent être traitées par un parseur compatible avec le format de la note. Ne pas supposer qu'elles sont du JSON.
- Éviter d'exposer ou d'archiver des journaux DEBUG contenant `ip` au-delà de la durée décidée pour les diagnostics.

## 13. Matrice d'acceptation / statut

| Domaine | Attendu | Statut |
|---|---|---|
| API et catalogue | Événements et clés fermés, champs typés | Implémentés et vérifiés par le test autonome |
| Sécurité | Neutralisation, bornes, règle spéciale `ip`, pas de payload | Implémenté ; l'intégrateur reste responsable de ne pas passer de valeur interdite |
| Performance | 4096 records, `trylock`, thread écrivain | Implémenté ; pas de benchmark de débit propre au serveur publié |
| Rotation | 10 MiB × 5 fichiers au maximum | Implémentée et couverte par le harnais |
| Purge >180 jours | Dry-run, suppression sélective, aucun fichier récent/non-log supprimé | Script `scripts/logs-purge.sh` implémenté et validé sur fichiers temporaires ; timer et recette sur l'hôte de déploiement restent à configurer |
| Sortie syslog/journald | Agent système/forwarding | **À configurer et tester au déploiement** |
| Appels métier réels | `net`, `protocol`, `business`, `chain`, `persistence` | **À intégrer lorsque ces modules seront disponibles** |
| Memcheck | `make -C server/util memcheck` | Non exécuté ici : Valgrind indisponible ; ASan/UBSan a été exécuté avec succès |
