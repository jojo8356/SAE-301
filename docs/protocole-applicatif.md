# Protocole applicatif AnimeTCG — spécification

**Projet AnimeTCG** — SAÉ BUT2 S3 — **phase 1** — version du document : **1.2** (22/09/2026 ; 1.1 : authentification ; 1.0 : 19/09/2026)
**Alignement** : sujet officiel 2026-2027 (§3 services, CU-20 authentification, §5.3 partage ciblé optionnel) — voir §16.1
**Exigences couvertes** : ENF-19 à ENF-27 (§5.3 du cahier des charges) · **Décision** : DEC-16
**Statut** : 🔧 **figé pour la phase 2** — toute évolution passe par une montée de version (§5)

> Ce document répond à **ENF-19** : « le protocole applicatif est défini par le groupe, sur sockets uniquement, et documenté en phase 1 (grammaire des messages, types, sémantique, cas d'erreur) ».

---

## 1. Principe et périmètre

AnimeTCG **n'utilise aucune API REST, aucun HTTP, aucun serveur web** : le **socket TCP est l'unique canal applicatif** (§3.2 du cahier des charges). Cela déplace une responsabilité vers l'équipe : **tout ce que HTTP offrait gratuitement doit être défini ici** — les frontières des messages, la corrélation des réponses, la structure des erreurs, le versionnage, la détection des connexions mortes.

**Ce que le protocole doit garantir** (les 9 exigences, dont la conformité est vérifiée en §14) :

| # | Garantie | Exigence |
|---|---|---|
| 1 | Frontières de messages fiables sur un flux d'octets | ENF-19, ENF-23 |
| 2 | Corrélation requête → réponse | ENF-20 |
| 3 | Messages serveur non sollicités (diffusion) | ENF-21 |
| 4 | Robustesse : malformé, tronqué, surdimensionné, inconnu | ENF-22 |
| 5 | Version de protocole et identifiant de demande | ENF-24 |
| 6 | Détection des clients morts | ENF-25 |
| 7 | Configuration IP + port transmise à la connexion | ENF-26 |
| 8 | Impossible de contourner le protocole | ENF-27 |
| 9 | Comportement à la saturation : file d'attente | ENF-54 |

---

## 2. Ce que font les protocoles industriels — et ce qu'on en retient

Aucun de ces choix n'est inventé : chacun reprend une décision éprouvée ailleurs. Le tableau dit **quelle source** a inspiré **quelle partie** de notre protocole.

| Protocole industriel | Ce qu'il fait | Ce qu'on en retient |
|---|---|---|
| **WebSocket** (RFC 6455) | Découpe le flux TCP en trames : en-tête de **2 à 14 octets** selon la taille (2 octets ≤ 125 o, 4 ≤ 65 535 o, 10 au-delà, +4 de masque client) | La **frontière de message explicite** est indispensable : TCP est un flux d'octets, pas une suite de messages. Notre en-tête est volontairement **plus simple** (4 octets fixes) car nous n'avons ni fragmentation, ni masquage, ni multiplexage |
| **JSON-RPC 2.0** (utilisé par LSP, MCP) | Enveloppe `{jsonrpc, id, method, params}` ; **corrélation par `id`** ; plage d'erreurs réservée **-32768 à -32000** ; codes d'application **hors** de cette plage ; notifications sans `id` | L'**enveloppe**, la **corrélation**, la **grammaire des erreurs** et la **règle des plages de codes**. On n'utilise pas `jsonrpc:"2.0"` : notre champ `v` porte une **version du protocole métier**, pas celle du standard |
| **Redis RESP / HTTP-2 / Protobuf** | **Préfixe de longueur** (longueur connue avant la lecture du corps) | Le choix du **préfixe sur 4 octets** plutôt qu'un délimiteur : pas de problème d'échappement, lecture en une passe, pas d'octet interdit dans la charge utile |
| **RFC 9457** (Problem Details, ex-RFC 7807) | Structure standard d'une erreur : `type`, `title`, `status`, `detail`, `instance` | La forme de notre objet `error` : un code **machine**, un message **humain**, un objet `details` **structuré**, et un drapeau `retryable` |
| **RFC 8259 / RFC 7493** (JSON / I-JSON) | Encodage UTF-8, pas de `NaN`/`Infinity`, pas de doublons de clés | **Contraintes d'encodage** appliquées à la réception (§12) |
| **Principe de robustesse** (RFC 1122, « Postel's Law ») + *Tolerant Reader* | « Conservateur en sortie, tolérant en entrée » ; un lecteur ignore les champs qu'il ne connaît pas | La **politique de compatibilité** (§5) : ajouts seulement, champs inconnus ignorés des deux côtés, jamais de rupture silencieuse |
| **LSP / MCP** (JSON-RPC sur transport) | **Négociation au démarrage** : le client annonce ses versions, le serveur choisit ou refuse | Notre **poignée de main** `session.hello` / `session.welcome` (§6.1) |
| **gRPC / MQTT / LSP** | **Keepalive** explicite (PING / PONG) avec délai maximal | Notre **heartbeat** serveur → client, en complément du timeout d'inactivité (ENF-25), §10 |

**Sources** : voir §15.

---

## 3. Cadrage : comment un message est délimité

### 3.1. Format de trame

```
+---------------------------+--------------------------------+
|  longueur (4 octets)      |  charge utile (JSON, UTF-8)    |
|  entier non signé,        |  exactement « longueur » octets |
|  gros-boutiste (big-endian)|                                |
+---------------------------+--------------------------------+
   0                        4                            4+longueur
```

| Règle | Valeur | Référence |
|---|---|---|
| Longueur | Entier **non signé 32 bits**, **gros-boutiste** | ENF-23 |
| Portée de la longueur | **Charge utile seule**, en-tête exclu | ENF-23 |
| Taille maximale | **65 536 octets (64 Kio)** — au-delà : erreur `-32600`, socket fermée, refus journalisé | ENF-23 |
| Taille minimale | 2 octets (`{}` est un JSON valide mais un message invalide → `-32600`) | ENF-19 |
| Encodage | **UTF-8 sans BOM**, sans séquence d'échappement | RFC 8259 |
| Longueur = 0 | **Erreur de protocole** immédiate, socket fermée | ENF-22 |

> ❓ **Pourquoi pas un délimiteur (`\n`) ?** Un délimiteur exige un **échappement** dans la charge utile (le délimiteur peut apparaître dans une chaîne JSON) et oblige à parcourir chaque octet reçu à la recherche de la fin. Le préfixe de longueur permet de **savoir où s'arrête le message avant de le lire**, donc de lire en une passe dans un tampon de taille connue à l'avance — ce qui est aussi la parade à un client malveillant qui enverrait une longueur absurde (on la **rejette sans allouer**).

### 3.2. Grammaire (ABNF)

```abnf
frame        = length payload
length       = 4OCTET                    ; entier non signé 32 bits, gros-boutiste
payload      = 2*65536OCTET              ; JSON UTF-8, 2 octets minimum

message      = request / response / event
request      = "{" "v" ":" version "," "id" ":" id "," "type" ":" "REQ" ","
               "action" ":" string "," "payload" ":" object "}"
response     = "{" "v" ":" version "," "id" ":" id "," "type" ":" "RES" ","
               "status" ":" ("OK" / "ERR" / "QUEUED") ","
               [ "payload" ":" object ] [ "error" ":" error ] "}"
event        = "{" "v" ":" version "," "id" ":" null "," "type" ":" "EVT" ","
               "event" ":" string "," "payload" ":" object "}"

error        = "{" "code" ":" integer "," "name" ":" string ","
               "message" ":" string
               ["," "details" ":" object ] ["," "retryable" ":" boolean ] "}"

version      = '"' 1*2DIGIT "." 1*2DIGIT '"'   ; MAJEUR.MINEUR, ex. "1.0"
id           = '"' 8HEXDIG "-" 4HEXDIG '"'      ; identifiant de demande du client
```

**Identifiant de demande** : 12 caractères hexadécimaux (`ab12f3cd9012`), **généré par le client** pour chaque demande — jamais réutilisé. C'est lui qui permet la corrélation requête/réponse (ENF-20) **et** la traçabilité dans les journaux (§5.8 du cahier des charges : `r=ab12f3cd9012`).

---

## 4. Enveloppe des messages

### 4.1. Requête (client → serveur)

```json
{
  "v": "1.0",
  "id": "ab12f3cd9012",
  "type": "REQ",
  "action": "card.create",
  "payload": {
    "title_romaji": "Shingeki no Kyojin",
    "format": "TV",
    "studio": "Wit Studio",
    "year": 2013,
    "episodes": 25,
    "genres": ["action", "drame"]
  }
}
```

| Champ | Type | Obligatoire | Rôle |
|---|---|---|---|
| `v` | chaîne `MAJEUR.MINEUR` | ✅ | Version du protocole (ENF-24) |
| `id` | chaîne hex (12) | ✅ | Corrélation et journalisation (ENF-20) |
| `type` | `"REQ"` | ✅ | Discriminant |
| `action` | chaîne pointée | ✅ | Action demandée (§6) |
| `payload` | objet | ✅ (objet vide admis) | Paramètres de l'action |

### 4.2. Réponse (serveur → client)

**Succès** :
```json
{
  "v": "1.0",
  "id": "ab12f3cd9012",
  "type": "RES",
  "status": "OK",
  "payload": { "card_id": 42, "block_index": 118, "hash": "0000a3f1…" }
}
```

**Succès différé — mise en file d'attente** (ENF-54) :
```json
{
  "v": "1.0",
  "id": "ab12f3cd9012",
  "type": "RES",
  "status": "QUEUED",
  "payload": { "position": 7, "queue_length": 7, "max_wait_s": 60 }
}
```

**Échec** :
```json
{
  "v": "1.0",
  "id": "ab12f3cd9012",
  "type": "RES",
  "status": "ERR",
  "error": {
    "code": -32602,
    "name": "INVALID_PARAMS",
    "message": "Le champ episodes doit être un entier supérieur ou égal à -1",
    "details": { "field": "episodes", "received": "vingt-cinq" },
    "retryable": false
  }
}
```

### 4.3. Événement (serveur → client, non sollicité)

```json
{
  "v": "1.0",
  "id": null,
  "type": "EVT",
  "event": "battle.closed",
  "payload": { "battle_id": 12, "winner_card_id": 42, "loser_card_id": 17, "block_index": 121 }
}
```

> **`id` vaut toujours `null` sur un événement.** Le client ne répond **jamais** à un événement (pas d'acquittement applicatif) : si la socket tombe, le contexte est simplement reconstitué à la reconnexion (CU-7). Ce choix suit celui de JSON-RPC 2.0 pour les notifications, qui ne doivent **pas** recevoir de réponse.

---

## 5. Versionnage et compatibilité

### 5.1. Politique

| Règle | Détail |
|---|---|
| Format | `MAJEUR.MINEUR` — `1.0`, `1.1`, `2.0` |
| **Mineur** (1.0 → 1.1) | **Changement compatible** : nouvel `action`, nouvel `event`, nouveau champ **optionnel**, nouvelle valeur d'énumération, nouveau code d'erreur. Le **serveur comme le client restent compatibles** |
| **Majeur** (1.x → 2.0) | **Changement incompatible** : suppression ou renommage d'un champ, changement de type, sémantique modifiée, validation durcie. Le serveur **refuse** une version majeure non reconnue |
| Négociation | Le client annonce `supported: ["1.0"]` dans `session.hello` ; le serveur choisit et répond `session.welcome` avec `v` retenu, ou refuse par `-32005 PROTOCOL_VERSION_UNSUPPORTED` en indiquant les versions qu'il sait parler |
| Évolution en cours de projet | Le protocole est **figé en `1.0` pour la phase 2**. Toute évolution passe par `1.1` et **doit** être documentée dans ce fichier et journalisée dans le cahier des charges |

### 5.2. Lecture tolérante (« Tolerant Reader »)

| Côté | Règle |
|---|---|
| **Récepteur de réponse/événement** (client) | **Ignore les champs inconnus** — un serveur `1.1` ajoutant `card.rating` ne casse pas un client `1.0` |
| **Récepteur de requête** (serveur) | **Ignore les champs inconnus** de `payload`, **mais** valide tous les champs connus (ENF-44). Un champ inconnu **n'est pas** une erreur en lecture ; il ne doit simplement jamais avoir d'effet métier |
| **Émetteur** (les deux) | **Conservateur** : n'envoie que ce qui est défini, jamais de champ expérimental en production (principe de robustesse, RFC 1122) |
| **Champ supplémentaire non reconnu dans un objet requis** | Le serveur répond `-32602 INVALID_PARAMS` **si le champ est mal typé** ; sinon il l'ignore et le journalise en `WARN` (un client fautif doit être visible sans être bloquant) |

---

## 6. Catalogue des actions

**Convention de nommage** : `domaine.verbe`, en minuscules, en anglais (cohérent avec ENF-34). Le catalogue compte **23 actions** : les **10 actions minimales du sujet** (§6 du sujet officiel, §4.10 du cahier des charges, EF-10 — **authentification comprise, rétablie le 22/09/2026**), **11 actions complémentaires** — les lectures nécessaires aux affichages imposés (`card.list`, `card.get`, `recommendation.list`, `battle.list`, `chain.status`), l'inactivité d'une carte (`card.inactivate`), la réponse aux demandes de droit (`legitimation.respond`, `claim.respond`) et la vérification de chaîne (`chain.verify`) — **plus 2 actions de partage ciblé** (`share.send`, `share.respond`), ajoutées le 22/09/2026 avec la réponse Q5.2 (§6.8).

> ✅ **Correction du 22/09/2026 (relecture d'équipe)** : cette section annonçait « 10 actions + 4 techniques = **14 actions** ». Le compte était **faux** — le catalogue en contient **19**. Trois comptes erronés ont été corrigés dans tout le dossier (voir §16) : le nombre d'actions, celui des événements et celui des codes d'erreur. C'est précisément ce que la relecture d'avant-code doit attraper : des nombres annoncés qui ne correspondent pas au contenu.

### 6.1. Session

| Action | Paramètres | Réponse | Notes |
|---|---|---|---|
| `session.hello` | `client_version`, `pseudo`, `supported: ["1.0"]` | `server_version`, `session_id`, `limits` (`max_clients`, `max_payload`) | **Toujours le premier message.** C'est ici que passe IP + port (ENF-26) |
| `session.welcome` *(événement)* | `session_id` | — | Envoyé quand une place se libère si le client était en file |
| `session.ping` | `{}` | `{ "pong": true, "server_time": "…" }` | Keepalive, §10 |
| `session.disconnect` | `{ "reason": "user" }` | `{ "ok": true }` | Libère la socket, conserve les actions déjà inscrites (CU-25) |

**Poignée de main avec file d'attente** (ENF-54) :

```json
→ { "v":"1.0","id":"a1b2c3d4e5f6","type":"REQ","action":"session.hello",
    "payload":{ "client_version":"1.0", "pseudo":"mika", "supported":["1.0"] } }

← { "v":"1.0","id":"a1b2c3d4e5f6","type":"RES","status":"QUEUED",
    "payload":{ "position":7, "queue_length":7, "max_wait_s":60 } }

   … une place se libère …

← { "v":"1.0","id":null,"type":"EVT","event":"session.welcome",
    "payload":{ "session_id":"7f3a91", "context":{ "cards":[…], "recommendations":[…] } } }
```

### 6.2. Cartes

| Action | Paramètres (extraits) | Réponse | Erreurs spécifiques |
|---|---|---|---|
| `card.create` | Les champs du §4.2.1 (obligatoires : `title_carte`, `sources_id`, `format`, `year`, `genres`, `themes` — **11 champs**, Q1.2 ; **doublons autorisés**, Q1.3) | `card_id`, `block_index`, `hash` | `1001 CARD_LIMIT_REACHED` (10 cartes actives) |
| `card.list` | `filter` optionnel (`status`, `owner_id`, `franchise`) | `cards: [...]`, `total` | — |
| `card.get` | `card_id` | `card`, `value`, `owner` | `1002 CARD_NOT_FOUND` |
| `card.inactivate` | `card_id`, `reason` | `block_index` | `1003 CARD_NOT_ACTIVE` |

### 6.3. Recommandations

| Action | Paramètres | Réponse | Erreurs spécifiques |
|---|---|---|---|
| `recommendation.add` | `card_id` | `value_after`, `block_index` | `1004 RECOMMENDATION_DUPLICATE`, `1003 CARD_NOT_ACTIVE` |
| `recommendation.repudiate` | `card_id` | `value_after`, `block_index` | `1005 RECOMMENDATION_NOT_FOUND` |
| `recommendation.list` | `direction` (`given` / `received`) | `recommendations: [...]` | — |

> Le **poids** (1.0 par un tiers, **0.5** par le créateur ou le propriétaire courant) est appliqué **par le serveur**, jamais transmis par le client — application directe d'ENF-45.

### 6.4. Battles

| Action | Paramètres | Réponse | Erreurs spécifiques |
|---|---|---|---|
| `battle.open` | `card_id`, `opponent_card_id` | `battle_id`, `closes_at` | `1006 BATTLE_NOT_PROXIMITY`, `1007 BATTLE_SELF`, `1008 BATTLE_RATE_LIMITED` (3 / 10 min), `1009 CARD_BUSY` |
| `battle.vote` | `battle_id`, `card_id` | `votes_received`, `closes_at` | `1010 VOTE_OWNER_EXCLUDED` (propriétaire ou créateur), `1011 BATTLE_CLOSED` (vote après la clôture), **`1020 VOTE_ALREADY_CAST`** (vote déjà déposé — **le vote est définitif**) |

> ✅ **Règles de vote (réponses de l'équipe Q2.3, Q2.4, Q2.5)** : le vote dure **5 minutes** ; tout utilisateur **connecté pendant** la fenêtre peut voter ; **créateur et propriétaires** des deux cartes sont **exclus** ; le vote est **définitif** — un second vote est **refusé** (`1020`), il ne remplace pas le premier ; un votant qui **se déconnecte** conserve son vote.
> **Trois dénouements** : victoire (égalité tranchée par la **carte la plus ancienne**), **battle annulée** si le quorum n'est pas atteint **ou si aucun vote** n'est déposé — les cartes sont alors **rendues** `ACTIVE`.
| `battle.list` | `state` (`open` / `closed`) | `battles: [...]` | — |

### 6.5. Droits : légitimation et revendication

| Action | Paramètres | Réponse | Erreurs spécifiques |
|---|---|---|---|
| `legitimation.request` | `card_id`, `justification` (≤ 200 car.) | `legitimation_id`, `expires_at` | `1012 CARD_ALREADY_LEGITIMATED` |
| `legitimation.respond` | `legitimation_id` | `likes`, `is_legitimate` | `1012 CARD_ALREADY_LEGITIMATED` *(carte déjà pourvue)* |

> ✅ **Le sens de cette action change (réponse de l'équipe Q4.1)** : `legitimation.respond` devient un **« like »** — elle n'exprime plus un vote pour ou contre dans un scrutin de 60 s, mais **un soutien** au candidat. **Le candidat qui totalise le plus de likes devient le porteur légitime** ; il perd le titre dès qu'un autre le dépasse. Le **justificatif, le nombre de likes, la rareté et le propriétaire** sont visibles par tous.
> ⚠️ `1013 LEGITIMATION_CLOSED` **n'est plus émis** : il n'y a plus de scrutin à clore (le code reste au catalogue, **réservé**).
| `claim.request` | `card_id`, `justification` | `claim_id`, `decision_deadline`, `auto_transfer_at` | `1014 CLAIM_NOT_LEGITIMATE`, `1015 CLAIM_ALREADY_PENDING`, `1016 CLAIM_SELF_OWNED` |
| `claim.respond` | `claim_id`, `decision` (`accept` / `refuse`) | `outcome`, `block_index` | `1017 CLAIM_CLOSED` |

> ~~**Règle CU-17 — disparition de l'objet**~~ ⚠️ **RETIRÉE le 22/09/2026** (réponse de l'équipe, Q2.6) : *« les caractéristiques de la carte perdante restent inchangées »* — une battle perdue **n'annule donc aucun droit** : les revendications en attente **restent en attente**, avec leur échéance. `outcome = "sans_objet"` et l'état `SANS_OBJET` ne sont **plus produits** (état retiré du schéma en v1.4). L'événement `claim.responded` reste émis, avec les seuls résultats `acceptee` ou `refusee`que pour un refus : le client n'a rien de nouveau à coder. La **légitimation** et le **marqueur d'authentification** suivent, eux, la **carte gagnante** (même œuvre, même porteur légitime).

### 6.6. Authentification d'une carte — *rétablie par le sujet officiel 2026-2027*

| Action | Paramètres | Réponse | Erreurs spécifiques |
|---|---|---|---|
| `card.authenticate` | `card_id`, `justification` (optionnelle, ≤ 200 car.) | `card_id`, `block_index`, `authenticated_at` | `1014 CLAIM_NOT_LEGITIMATE` (demandeur non légitimé), `1018 CARD_ALREADY_AUTHENTICATED`, `1003 CARD_NOT_ACTIVE` |
| `card.revoke_authentication` | `card_id` | `card_id`, `block_index` | `1014`, `1019 AUTHENTICATION_NOT_FOUND` |

> **Ce que l'action affirme** : que **les métadonnées de la carte décrivent fidèlement l'œuvre** — jamais une propriété ou un droit. Seul un **porteur légitime actif** peut l'émettre (§4.7.3 du cahier des charges, DEC-24).
> **Une seule authentification active par carte** : une nouvelle demande **remplace** la précédente (l'ancienne reste dans l'historique) ; `1018` n'est renvoyé que si la même carte est authentifiée **deux fois sans révocation**, ce qui est refusé pour éviter les doublons d'affichage.
> **Aucun effet sur la valeur** de la carte ni de l'utilisateur : c'est un **label de confiance**, pas un bonus de score.

> **`decision_deadline`** (120 s) n'est qu'une **fenêtre d'affichage** de démonstration ; **`auto_transfer_at`** porte la vraie règle : `. claim_id` non refusée à cette date → transfert automatique (30 jours, DEC-11). Le client **affiche les deux**, il n'en décide aucune.

### 6.7. Chaîne

| Action | Paramètres | Réponse | Notes |
|---|---|---|---|
| `chain.verify` | `from_block` optionnel | `valid`, `checked`, `first_invalid` | Réservée au profil « administrateur / démonstration » (CU-26) |
| `chain.status` | `{}` | `blocks`, `difficulty`, `hash_rate`, `last_block_hash` | Alimente la barre d'état |

### 6.8. Partage ciblé — *retenu le 22/09/2026* (réponse Q5.2, DEC-15)

| Action | Paramètres | Réponse | Erreurs spécifiques |
|---|---|---|---|
| `share.send` | `to_user_id`, `card_id` | `share_id`, `state` | `1014 CLAIM_NOT_LEGITIMATE` *(le partage est réservé à la liste d'amis)*, `1003 CARD_NOT_ACTIVE`, `1001 CARD_LIMIT_REACHED` *(le destinataire est à 100 cartes actives)* |
| `share.respond` | `share_id`, `decision` (`accept` / `refuse`) | `card_id`, `outcome` | `1015 CLAIM_ALREADY_PENDING` *(réutilisé : la demande est déjà close)* |

> **Règles** : destinataires = **liste d'amis** uniquement ; **acceptation → le demandeur se déplace** (la carte change de propriétaire) ; **refus → personne ne bouge**. Notification **poussée sur la socket** du destinataire.
> ✅ **Le partage est inscrit en blockchain** *(option A, décision d'équipe du 22/09/2026)* : `share.send` et `share.respond` produisent **chacune un bloc**, comme toute action métier qui fait bouger une carte. La **liste d'amis** reste, elle, un réglage d'utilisateur hors chaîne.
> **Anti-spam** : **3 partages**, puis **1 par minute** — limites **personnalisables par l'utilisateur**.

### 6.9. Actions volontairement absentes du périmètre

Les actions **`bot.*`** (client automatique) **n'existent pas** dans ce protocole : le sujet en fait une option et l'équipe a retenu que le **client de test** vit hors périmètre fonctionnel (DEC-14) — et le bot est réservé aux tests (DEC-14). Un client qui les demanderait reçoit **`-32601 METHOD_NOT_FOUND`** — c'est le comportement normal pour une action inconnue, aucune branche de code n'est écrite pour elles.

---

## 7. Catalogue des événements (serveur → client)

| Événement | Déclencheur | Charge utile | Destinataires |
|---|---|---|---|
| `session.welcome` | Admission après attente | `session_id`, `context` | le client admis |
| `session.queue_update` | La position change | `position`, `queue_length` | les clients en attente |
| `client.connected` / `client.disconnected` | Connexion / déconnexion | `user_id`, `pseudo`, `connected_count` | tous les clients admis |
| `card.created` | Création validée | `card`, `block_index` | tous |
| `value.updated` | Toute action modifiant une valeur | `card_id`, `value_before`, `value_after`, `cause` | tous |
| `recommendation.changed` | Reco donnée / répudiée | `card_id`, `user_id`, `state` | l'auteur et le propriétaire |
| `battle.opened` | Battle ouverte | `battle`, (les 2 cartes en `EN_BATTLE`) | tous |
| `battle.vote_cast` | Vote enregistré | `battle_id`, `votes_received`, `closes_at` | tous |
| `battle.closed` | Clôture (quorum, nulle ou délai) | `battle_id`, `winner_card_id`, `outcome` | tous |
| `legitimation.opened` / `legitimation.resolved` | Scrutin ouvert / tranché | `legitimation_id`, `card_id`, `result` | tous |
| `claim.received` | Revendication déposée | `claim_id`, `card_id`, `claimant`, `auto_transfer_at` | **le propriétaire courant** |
| `claim.responded` | Acceptation, refus **ou clôture sans objet** (`outcome`) | `claim_id`, `outcome` | le revendiquant |
| `claim.transferred` | Transfert effectué | `card_id`, `from_user`, `to_user`, `block_index` | tous |
| `block.mined` | Bloc inscrit (inclut la **progression du minage**) | `block_index`, `nonce`, `hashes`, `duration_ms`, `difficulty` | tous |
| `difficulty.changed` | Réévaluation du calibrage (ENF-53) | `old`, `new`, `hash_rate` | tous |
| `server.notice` | Information d'exploitation (base indisponible, etc.) | `level`, `message` | tous |
| `client.resync` | Le tampon sortant d'un client a débordé (256 événements) : l'événement le plus ancien a été remplacé par celui-ci | `reason` | **le seul client concerné** |
| `card.authenticated` | Authentification (ou révocation) d'une carte par son porteur légitime | `card_id`, `by_user_id`, `revoked` (`true` / `false`) | tous |
| `share.received` ✅ **actif** | **Partage ciblé retenu** (réponse Q5.2) : une carte est proposée à un ami | `share_id`, `card_id`, `from_user_id` | le destinataire |
| `share.responded` 🆕 | Le destinataire a **accepté** (la carte se déplace, **bloc miné**) ou **refusé** (personne ne bouge) | `share_id`, `accepted`, `block_index` | l'expéditeur |

**22 événements** au catalogue (dont `client.resync` — ajouté lors de la relecture du 22/09/2026, il était utilisé au §13 sans être déclaré — et `card.authenticated`, ajouté le 22/09/2026 avec le rétablissement de l'authentification ; `share.received` est déclaré mais **réserve** le mécanisme optionnel de partage ciblé). **Deux règles de diffusion** : un événement **ne contient jamais de données qu'un client n'a pas le droit de voir** (le contexte est renvoyé à la connexion, CU-7) ; et **`claim.received` n'est envoyé qu'au propriétaire** — c'est un cas de **diffusion ciblée** (ENF-21), le premier des trois du catalogue avec **`client.resync`** (le seul client concerné) et **`share.received`** (le destinataire, mécanisme réservé).

> **Nommage : un événement de fil, deux noms de journal.** `card.authenticated` couvre **l'authentification et sa révocation** : c'est la clé **`revoked` (`true`/`false`)** qui les distingue sur le fil — un seul événement à coder côté client. Le **journal**, lui, sépare les deux faits (`card authenticated` et `authentication revoked`, voir `docs/note-format-ligne.md`), et connaît un troisième libellé, `authentication refused`, qui n'a **pas** d'événement de fil : un refus est une **réponse d'erreur**, pas une diffusion.

---

## 8. Catalogue des erreurs

### 8.1. Plages de codes

Les plages suivent **la convention JSON-RPC 2.0**, reprise par l'écosystème (LSP, MCP) : les **erreurs de protocole** occupent la plage réservée **-32768 à -32000**, et les **erreurs d'application** doivent être **hors de cette plage**. Les nôtres sont donc des entiers **positifs à partir de 1000**.

| Plage | Nature | Exemples | Effet sur la connexion |
|---|---|---|---|
| `-32768` … `-32600` | Erreurs de **protocole** — le message n'aurait pas dû arriver sous cette forme | `-32700 PARSE_ERROR`, `-32600 INVALID_REQUEST`, `-32601 METHOD_NOT_FOUND`, `-32602 INVALID_PARAMS` | La connexion **survit** (sauf `-32700` et longueur invalide, voir §9) |
| `-32099` … `-32000` | Erreurs **serveur définies** par l'implémentation (plage réservée par le standard) | `-32001 SERVER_BUSY`, `-32002 QUEUE_FULL`, `-32003 QUEUE_TIMEOUT`, `-32004 SESSION_EXPIRED`, `-32005 PROTOCOL_VERSION_UNSUPPORTED`, `-32603 INTERNAL_ERROR` | La connexion **survit** |
| `1000` … `1999` | Erreurs **métier** AnimeTCG | `1001 CARD_LIMIT_REACHED` … `1019 AUTHENTICATION_NOT_FOUND` | La connexion **survit** — ce sont des refus d'action, **jamais** des pannes |

### 8.2. Table complète — **30 codes**

| Code | Nom | Sens | `retryable` |
|---|---|---|---|
| `-32700` | `PARSE_ERROR` | JSON invalide (charge utile reçue mais non parsable) | non |
| `-32600` | `INVALID_REQUEST` | JSON valide mais enveloppe invalide (champ `type` inconnu, `id` manquant, longueur > 64 Kio) | non |
| `-32601` | `METHOD_NOT_FOUND` | `action` inconnue du serveur | non |
| `-32602` | `INVALID_PARAMS` | Paramètres invalides (type, borne, longueur, valeur hors liste fermée) — `details` précise **le champ fautif** | non |
| `-32603` | `INTERNAL_ERROR` | Erreur interne inattendue (jamais de trace technique renvoyée au client) | oui |
| `-32001` | `SERVER_BUSY` | Le serveur ne peut pas traiter cette demande maintenant (charge de minage, base occupée) | **oui** |
| `-32002` | `QUEUE_FULL` | File d'attente pleine (100 places) | oui, plus tard |
| `-32003` | `QUEUE_TIMEOUT` | Attente > 60 s sans admission | oui |
| `-32004` | `SESSION_EXPIRED` | Inactivité > 300 s (ENF-25) : action refusée, socket sur le point d'être fermée | non |
| `-32005` | `PROTOCOL_VERSION_UNSUPPORTED` | `v` majeur non supporté — `details.supported` liste les versions connues | non |
| `1001` | `CARD_LIMIT_REACHED` | **100 cartes actives** maximum par utilisateur (réponse Q1.4) — le client propose alors de **désactiver une carte** | non |
| `1002` | `CARD_NOT_FOUND` | `card_id` inexistant | non |
| `1003` | `CARD_NOT_ACTIVE` | Carte `INACTIVE` ou **`ARCHIVEE`** (§4.4) — l'état `FUSIONNEE` a été remplacé le 22/09/2026 (Q2.6) | non |
| `1004` | `RECOMMENDATION_DUPLICATE` | Une seule recommandation active par utilisateur et par carte | non |
| `1005` | `RECOMMENDATION_NOT_FOUND` | Répudiation d'une recommandation inexistante ou déjà répudiée | non |
| `1006` | `BATTLE_NOT_PROXIMITY` | Aucun critère P1…P5 satisfait | non |
| `1007` | `BATTLE_SELF` | Battle d'une carte avec elle-même | non |
| `1008` | `BATTLE_RATE_LIMITED` | Plus de 3 battles ouvertes par 10 minutes | oui |
| `1009` | `CARD_BUSY` | Carte déjà en battle (`EN_BATTLE`) | non |
| `1010` | `VOTE_OWNER_EXCLUDED` | Le propriétaire courant d'une des deux cartes ne vote pas | non |
| `1011` | `BATTLE_CLOSED` | Vote après clôture | non |
| `1012` | `CARD_ALREADY_LEGITIMATED` | Un porteur légitime actif existe déjà | non |
| `1013` | `LEGITIMATION_CLOSED` | ⚠️ **Réservé — jamais émis** depuis le 22/09/2026 : le scrutin de 60 s est remplacé par la **légitimation au nombre de likes** (Q4.1) | non |
| `1014` | `CLAIM_NOT_LEGITIMATE` | Le demandeur n'est pas porteur légitime (DEC-10) | non |
| `1015` | `CLAIM_ALREADY_PENDING` | Une revendication est déjà en attente sur cette carte | non |
| `1016` | `CLAIM_SELF_OWNED` | Le demandeur est déjà propriétaire courant | non |
| `1017` | `CLAIM_CLOSED` | Revendication déjà refusée, acceptée, transférée ou **close sans objet** | non |
| `1018` | `CARD_ALREADY_AUTHENTICATED` | Une authentification active existe déjà sur cette carte (la révocation précède toujours une nouvelle authentification) | non |
| `1019` | `AUTHENTICATION_NOT_FOUND` | Révocation d'une authentification inexistante ou déjà révoquée | non |
| `1020` 🆕 | `VOTE_ALREADY_CAST` | Un vote **définitif** a déjà été déposé par cet utilisateur sur cette battle (le vote ne se modifie pas — réponse Q2.4) | non |

> **Un refus métier n'est pas une panne.** Les codes `1000+` signalent que **le serveur a bien compris** la demande et l'a **refusée** en application d'une règle du cahier des charges : aucune socket fermée, **aucun bloc créé** (REC-09), une ligne de journal en `INFO` côté serveur et un message clair côté client.

---

## 9. Robustesse — les cas limites, et ce que fait le serveur (ENF-22, REC-13)

**Onze cas** sont traités nommément ci-dessous ; la **douzième ligne** du tableau n'est pas un cas mais la **garantie** qui les résume — c'est elle que l'exigence ENF-22 demande de tenir : « erreur explicite, **jamais** de plantage ».

| Cas reçu | Réaction | Journal |
|---|---|---|
| Prologue « cassé » (premiers octets non identifiables comme longueur plausible) | Fermeture immédiate de la socket | `WARN` + adresse source |
| **Longueur = 0** | Refus, fermeture | `WARN` |
| **Longueur > 64 Kio** | Refus **sans allouer** ni lire le corps, fermeture, réponse `-32600` si la socket est encore saine | `WARN` |
| Charge utile tronquée (le client ferme au milieu) | Le message est abandonné, la socket est libérée — **aucun** traitement partiel | `INFO` |
| JSON invalide | Réponse `-32700`, **socket conservée** | `INFO` |
| Enveloppe invalide (`type` inconnu, `id` absent) | Réponse `-32600`, socket conservée | `INFO` |
| `action` inconnue | Réponse `-32601`, socket conservée | `INFO` |
| Paramètres invalides | Réponse `-32602` avec `details.field` | `INFO` |
| **Débit anormal** (> 100 messages/s soutenus) | Ralentissement puis `-32001` ; en dernier recours fermeture | `WARN` |
| Texte non UTF-8, `NaN`, `Infinity`, clés dupliquées | Réponse `-32700` (I-JSON, RFC 7493) | `INFO` |
| Message valide mais **hors séquence** (`battle.vote` sans `session.hello`) | Réponse `-32600` avec `details.expected: "session.hello"` | `WARN` |
| **Aucun de ces cas ne termine le processus serveur** | C'est l'exigence : « erreur explicite, **jamais** de plantage » | — |

**Stratégie d'implémentation** : le lecteur de trame maintient un **état par connexion** (`AWAIT_LENGTH` → `AWAIT_PAYLOAD`), ne fait **aucune hypothèse** sur le découpage des `recv()` (un message peut arriver en 5 morceaux, ou 3 messages dans un seul `read`), et **plafonne la mémoire par connexion** à 64 Kio + l'en-tête. Toute erreur de protocole sur une connexion est **locale à cette connexion** : elle n'affecte ni les autres clients, ni la boucle d'événements (ENF-18).

---

## 10. Cycle de vie d'une session

| Étape | Détail | Exigence |
|---|---|---|
| 1. Connexion TCP | IP + port saisis côté client, mémorisés localement, envoyés à la connexion | ENF-26 |
| 2. `session.hello` | Premier message **obligatoire**, sous **5 s**, sinon fermeture | ENF-19 |
| 3. Admission **ou** file d'attente | 100 connectés simultanés ; au-delà, `status: "QUEUED"` + position ; admission automatique dans l'ordre d'arrivée | **ENF-54** |
| 4. `session.welcome` + contexte | Reconstitution complète du contexte en parcourant la blockchain (CU-7) | ENF-21 |
| 5. Échanges | Actions et événements ; **au plus 8 demandes en vol** par client (au-delà : `-32001`) | ENF-19 |
| 6. **Keepalive** | Le serveur envoie `session.ping` après **60 s** de silence ; le client répond `{ "pong": true }`. **3 pings sans réponse** → socket fermée | ENF-25 |
| 7. **Timeout d'inactivité** | **300 s** sans aucun message entrant → `-32004 SESSION_EXPIRED`, socket libérée, utilisateur considéré déconnecté | **ENF-25** |
| 8. Déconnexion | `session.disconnect` ou fermeture : socket libérée, **actions déjà inscrites conservées** en blockchain et en base ; le contexte est reconstitué à la prochaine connexion | CU-25 |

**Pourquoi un heartbeat en plus du timeout** : le timeout détecte l'absence de trafic ; il ne détecte pas une **connexion morte silencieuse** (machine éteinte, câble débranché — le client ne pouvant plus envoyer de `RST`). Le PING/PONG du serveur, comme dans LSP et MQTT, révèle ces cas en ≤ 180 s et **libère la place** au lieu de la garder occupée pour une socket fantôme.

---

## 11. Idempotence et anti-rejeu

Toute action métier crée un **bloc définitif**. Une demande renvoyée par un client après un timeout réseau **ne doit pas créer un second bloc**.

| Règle | Détail |
|---|---|
| Registre des demandes | Le serveur garde en mémoire les **10 000 derniers identifiants** de demande traités (fenêtre glissante ≈ 10 min à charge normale) |
| Rejeu détecté | Même `id` **et** même couple (`user_id`, `action`) → la réponse **déjà produite** est renvoyée telle quelle, **aucun second bloc** n'est créé, l'événement n'est pas rediffusé |
| Même `id`, action différente | Traité comme une **nouvelle** demande (`WARN`) — le client a réutilisé un identifiant |
| Unicité côté client | L'identifiant est généré une fois par demande et **jamais réutilisé** pour une autre action |

---

## 12. Sécurité et validation

| Contrôle | Règle | Exigence |
|---|---|---|
| Validation avant métier | **Tout** `payload` est validé (types, bornes, longueurs, listes fermées) **avant** toute règle métier | **ENF-44** |
| Aucune confiance | Identifiants de carte, de propriétaire, poids d'une recommandation, difficulté : **recalculés ou vérifiés côté serveur**, jamais repris du client | **ENF-45** |
| Bornes | Longueurs : `title_*` ≤ 200 car., `synopsis` ≤ 500 car., `justification` ≤ 200 car. · Entiers : `1950 ≤ year ≤ année+1`, `episodes ≥ -1`, `0 ≤ rating ≤ 10` · Tableaux : `genres` ≥ 1 élément, `themes` ≤ 8 | ENF-44 |
| Listes fermées | `format`, `season`, `source`, `status`, `genres`, `themes` : valeur hors liste → `-32602` avec la valeur fautive en `details` | ENF-44 |
| Pas de données personnelles | Aucun champ `password`, `email`, `token` dans le protocole : le pseudo suffit (le sujet impose une légitimation simulée) | §3.2 |
| Pas de trace technique exposée | Le client reçoit un `error.message` en français et un `details` **assaini** ; les traces d'exécution restent dans les journaux du serveur | §5.6 |
| Contournement impossible | Aucune action ne permet d'écrire un fichier, d'exécuter du code ou d'accéder à la base : le seul effet d'un message est un **bloc** et une **projection en base** | **ENF-27** |

---

## 13. File d'attente et contre-pression

| Situation | Comportement |
|---|---|
| 100 clients admis | File **vide** : tout nouvel arrivant est admis |
| Au-delà de 100 | `status: "QUEUED"` + position ; **aucune** action métier acceptée avant `session.welcome` |
| Attente > 60 s | `-32003 QUEUE_TIMEOUT`, socket fermée |
| File pleine (100 en attente) | `-32002 QUEUE_FULL`, socket fermée |
| Place libérée | `session.welcome` envoyé **au premier de la file** (FIFO strict) |
| Coût | **Zéro fil d'exécution** : les sockets en attente ne sont pas lues, la boucle d'événements reste libre (ENF-18) |
| **Événements trop lents à consommer** | Le serveur borne son tampon sortant par client (256 événements) ; au-delà, l'événement le plus **ancien** est remplacé par un événement de resynchronisation (`client.resync`) et le client redemande son contexte (`context.get`). **Le serveur ne bloque jamais** pour un client lent |

---

## 14. Conformité aux exigences

| Exigence | Où elle est satisfaite | Vérification |
|---|---|---|
| ENF-19 — protocole défini et documenté | Cet ensemble de sections | Revue de livrable |
| ENF-20 — corrélation requête/réponse | §3.2 (`id`), §4 | REC-13 |
| ENF-21 — messages non sollicités | §4.3, §7 | Scénario de démonstration |
| ENF-22 — robustesse | **§9, cas par cas** | **REC-13** (messages malformés, tronqués, surdimensionnés, type inconnu) |
| ENF-23 — trame `[4 octets][JSON]`, 64 Kio | §3.1 | REC-13 |
| ENF-24 — version + identifiant | §3.2, §5 | REC-13 |
| ENF-25 — inactivité 300 s | §10 | Test d'intégration |
| ENF-26 — IP + port à la connexion | §6.1 | REC-03 |
| ENF-27 — pas de contournement | §12 | REC-14 (un seul port applicatif ouvert) |
| ENF-54 — file d'attente | §6.1, §13 | **REC-03** |

---

## 15. Ce qui est figé, ce qui a été tranché

**Figé pour la phase 2** (toute évolution = version mineure documentée) :
format de trame · enveloppe · nommage des actions et des événements · **catalogue des 30 codes d'erreur** (nom, numéro et sens : le §12.1 du cahier des charges s'y réfère ligne à ligne) · poignée de main · heartbeat · politique de versionnage.

**Plus rien d'ouvert** ✅ *(décision d'équipe du 22/09/2026 : les valeurs par défaut sont retenues comme valeurs finales ; les deux points ci-dessous sont donc **ratifiés**, ajustables seulement par version mineure)* :
1. ~~**Bibliothèque JSON du serveur C**~~ ✅ **tranché** : **jansson** (licence MIT), **profondeur maximale fixée à 32** côté application et côté parseur — justification, comparatif et conséquences en **§5.3 de `docs/specification-technique.md`**. Repli documenté : cJSON avec `CJSON_NESTING_LIMIT=64`.
2. **Nombre de demandes en vol** : **8** ✅ retenu (ajustable en version mineure sur mesures de latence, phase 2).
3. **Fréquence du heartbeat** : **60 s** ✅ retenu (expiration à 180 s).

---

## 16. Journal des corrections (relecture d'équipe du 22/09/2026)

Cette section est le **procès-verbal de la relecture d'avant-code**. Elle est conservée : un protocole qui se corrige silencieusement n'est pas un protocole figé, c'est un protocole qui dérive.

| # | Ce qui était annoncé | Ce que contient réellement le document | Correction |
|---|---|---|---|
| 1 | « **14 actions** » (§6) | **19 actions** : `session.*` (3), `card.*` (4), `recommendation.*` (3), `battle.*` (3), `legitimation.*`/`claim.*` (4), `chain.*` (2) | Compte corrigé dans le protocole, le cahier des charges, la spécification technique, la checklist et le questionnaire |
| 2 | « **16 événements** » (§7) | **19 événements** : 16 lignes de catalogue dont deux en portent deux chacune (18), **plus `client.resync`** qui était utilisé au §13 sans être déclaré | Événement ajouté au catalogue (ciblé, un seul destinataire) + compte corrigé |
| 3 | « **28 codes d'erreur** » (§8) | **27 codes** : 5 codes de protocole (−327xx), 5 codes serveur (−320xx), 17 codes métier (1001 → 1017) — **portés depuis à 19** (1001 → 1019) | Compte corrigé partout |

> ⚠️ **Ces trois comptes datent de la relecture.** Ils ont été **portés depuis à 23 actions, 22 événements et 30 codes** par la mise à jour d'alignement sur le sujet officiel (**§16.1** entrées 4 et 5, puis **§16.2** entrées 8 à 12) : il s'agissait de compter ce qui existait, puis d'y **ajouter** l'authentification, le partage ciblé et le vote définitif. Les valeurs qui font foi sont celles des §6, §7 et §8.

### 16.1. Mise à jour du 22/09/2026 — alignement sur le sujet officiel 2026-2027

| # | Ce qui change | Pourquoi | Effet |
|---|---|---|---|
| 4 | **`card.authenticate` et `card.revoke_authentication` ajoutées** au §6.6 | Le sujet officiel liste l'**authentification** parmi les services (§3), les cas d'usage (**CU-20**) et les fonctionnalités à **démonter** (§12.1) — alors qu'elle avait été retirée sur consigne orale du 18/09/2026 | **22 actions** (au lieu de 19) ; événement `card.authenticated` ; codes `1018` et `1019` → **30 codes** |
| 5 | **`share.received` déclaré** (réservé) — ⚠️ **superseded par §16.2, entrée 9** : l'équipe a finalement **retenu** le partage ciblé (Q5.2) | Le sujet ouvre un mécanisme **optionnel** de partage ciblé (§5.3) ; l'équipe ne le retient pas (DEC-15) mais le protocole réserve l'événement pour ne pas avoir à le rouvrir | **22 événements** ; aucune action de partage n'est implémentée |
| 6 | **`bot.*` reste absent** | Option du sujet, non retenue (DEC-14) — le client de test n'est pas un utilisateur | Aucun changement de comportement |
| 7 | **`1011` portait deux noms** : `BATTLE_CLOSED` au §8.2 et `VOTE_ALREADY_CAST` au §6.4 | ⚠️ **Invalidée le 22/09/2026** : l'équipe a répondu que le **vote est définitif** (Q2.4) — le refus d'un second vote **existe bien** et porte désormais le code **`1020 VOTE_ALREADY_CAST`** (§16.2, entrée 8). `1011` reste `BATTLE_CLOSED`. *(Cette entrée avait retenu le nom du §8.2 en supposant le vote modifiable ; l'hypothèse est tombée.)* Le nom du §8.2 est le bon : le vote est **modifiable** jusqu'à la clôture (DEC-04, §4.5.3 du CDC ; `battle_votes` a pour clé `(battle_id, user_id)` — **un second vote remplace le premier**, ce n'est pas une erreur). Le seul refus de vote est **après** la clôture | §6.4 aligné sur §8.2 ; la règle est écrite au §6.4 et rappelée au §12.1 du CDC (cas n° 10) |

**Ce que la relecture a validé sans changement** : les **plages de codes** (protocole / serveur / métier) et leur règle d'affectation, la **trame** et son plafond, la **poignée de main** et la place de la file d'attente, le **versionnage** et le refus `-32005`, les **bornes de validation** (§12) et le principe « un refus métier n'est pas une panne ».

**Rien n'est plus ouvert** ✅ *(décision d'équipe du 22/09/2026)* : les trois points du §15 sont **ratifiés aux valeurs par défaut** — **8** demandes en vol, battement de cœur **60 s**, bibliothèque JSON **jansson** (tranchée en §5.3 de la spécification technique).

---

### 16.2. Mise à jour du 22/09/2026 — réponses de l'équipe au questionnaire

| # | Ce qui change | Pourquoi | Effet |
|---|---|---|---|
| 8 | **`1020 VOTE_ALREADY_CAST` ajouté** (30ᵉ code) | Réponse Q2.4 : le vote est **définitif** — un second vote n'est plus un remplacement mais un **refus** | §6.4 et §8.2 mis à jour ; `battle_votes` : un vote par votant, définitif |
| 9 | **`share.send` et `share.respond` ajoutées** (22ᵉ et 23ᵉ actions) ; **`share.received` activé** | Réponse Q5.2 : le **partage ciblé est retenu** (il était déclaré « réservé ») | Nouveau §6.8 ; événement `share.responded` ajouté ; **hors blockchain**, point de conception signalé |
| 10 | **`legitimation.respond` change de sens** : c'est désormais un **like** | Réponse Q4.1 : la légitimation se joue **au nombre de likes**, plus par un scrutin de 60 s | `1013 LEGITIMATION_CLOSED` devient **réservé, jamais émis** ; §6.5 mis à jour |
| 11 | **`1001` passe à 100 cartes actives**, et la battle ouvre sur des **rangs voisins** | Réponses Q1.4 et Q2.2 | §6.2 et §6.4 mis à jour |
| 12 | **Battle : 5 minutes, trois dénouements** (victoire / annulée faute de quorum / **annulée faute de vote**) | Réponses Q2.3, Q2.5 et Q6.3 | §6.4 mis à jour ; les cartes sont **rendues** `ACTIVE` |

---

### 16.3. Sign-off du protocole — **validé par l'équipe le 22/09/2026**

| Élément | Valeur validée |
|---|---|
| Actions | **23** (§6) |
| Événements | **22** (§7) |
| Codes d'erreur | **30** (§8.2) |
| Trame | `[4 octets][JSON UTF-8]`, 64 Kio, 8 demandes en vol, heartbeat 60 s, inactivité 300 s |

> ✅ Le protocole est **figé**. Toute évolution ultérieure passe par une **version mineure** documentée (§15), avec les comptes remis à jour et l'événement correspondant déclaré au §7. **Rien n'est modifié en silence.**

**Standards de protocole**
- **JSON-RPC 2.0** — enveloppe, corrélation par `id`, plage d'erreurs réservée `-32768…-32000`, règle « les codes d'application sont hors de cette plage » : <https://www.jsonrpc.org/specification> · usages industriels (LSP, MCP) et détail des 5 codes standard : <https://jsonic.io/guides/json-rpc>
- **WebSocket, RFC 6455** — en-tête de trame de 2 à 14 octets, longueur sur 7/16/64 bits, absence de frontière de message dans TCP : <https://github.com/mattheworiordan/websocket.org/blob/main/src/content/docs/reference/websocket-vs-tcp.md> · décodage de l'en-tête : <https://adhdecode.com/api-architecture/websocket-apis/websocket-message-framing/>
- **Cadrage par préfixe de longueur vs délimiteur** — 4 octets gros-boutistes, lecture en une passe, pas d'échappement : <https://oneuptime.com/blog/post/2026-03-20-message-framing-python-tcp-sockets/view> et <https://stackoverflow.com/questions/11999792/message-framing-method-advantages-and-disadvantages-in-net>
- **RFC 9457** *(Problem Details for APIs, remplace la RFC 7807)* — structure d'une erreur : `code` machine, message humain, détails structurés : <https://www.rfc-editor.org/rfc/rfc9457>
- **RFC 8259** (JSON) et **RFC 7493** (I-JSON) — UTF-8, pas de `NaN`/`Infinity`, pas de clés dupliquées : <https://www.rfc-editor.org/rfc/rfc8259> · <https://www.rfc-editor.org/rfc/rfc7493>

**Compatibilité et évolution**
- **Principe de robustesse** (RFC 1122) et *Tolerant Reader* — « conservateur en sortie, tolérant en entrée » : <https://stackoverflow.blog/2020/05/13/ensuring-backwards-compatibility-in-distributed-systems/>
- **Changements additifs et ce qui casse un client** — liste des changements compatibles et incompatibles, versionnage par époques : <https://averagedevs.com/blog/api-versioning-backward-compatibility>
- **Lignes directrices Zalando** — évolutivité, extension par défaut, gestion des champs inconnus : <https://deepwiki.com/zalando/restful-api-guidelines/6-api-evolution-and-lifecycle>
- **Compatibilité et lecteurs tolérants** (playbook API) : <https://playbook.hackney.gov.uk/api-playbook/Development%20Lifecycle/Designing%20your%20API/compatibility/>

---

*Protocole AnimeTCG `1.0` — document de phase 1. Les exemples JSON de ce fichier sont la **référence** des tests d'intégration et du client Java.*
