# Note d'ingénierie — Format de ligne de journal AnimeTCG

| | |
|---|---|
| **Référence** | **Note de format de ligne** — rédaction attendue par le **§5.8.2 du cahier des charges** (ENF-56) ; détail normatif repris en **§10 de la spécification technique (DOC-09)** |
| **Version** | **1.3 — 22/09/2026** (1.0 : première rédaction ; 1.1 : **3 événements d'authentification** ajoutés — catalogue à 63 ; 1.2 : événement **`claim closed`** ajouté pour la règle CU-17 ; **1.3 : réponses de l'équipe au questionnaire** — `card merged` → **`card archived`** (Q2.6), `vote cast` devient **définitif** et `vote refused` apparaît (Q2.4), `legitimation resolved` remplacé par **`legitimation liked` + `legitimation taken`** (Q4.1), trois entrées pour le **partage ciblé** (Q5.2), `claim closed` **désactivé** — **catalogue porté à 69 entrées** ; clé `event` alignée sur les **22** événements du protocole) |
| **Auteurs** | M1 (réseau & protocole) et M4 (client) — relecture M2, M3 |
| **Statut** | **Normative**. Le format est figé ici une fois pour les deux programmes ; toute évolution suit le **contrat de compatibilité du §11** |
| **Portée** | Serveur **C** (`zlog`) et client **Java** (SLF4J + Logback) — un seul format, deux producteurs |
| **Ce que cette note ne traite pas** | Où sont écrits les journaux, leur rotation, leur rétention et leur centralisation → **§5.8.3 du CDC** et **§10.8 à §10.11 de DOC-09** |

---

## 1. Objet

Cette note définit **la forme d'une ligne de journal** et tout ce qui la rend exploitable : la **grammaire**, la **nomenclature** (niveaux, noms d'événements, clés), les **règles d'échappement**, le **mapping** vers les référentiels du marché, et le **contrat de compatibilité**.

**Pourquoi une note séparée ?** Parce que le format est le seul artefact du projet écrit par **deux langages différents**. S'il dérive d'un côté (une espace en trop, un niveau en minuscules, une clé renommée), la fusion des journaux — qui est **la** démonstration de REC-17 — cesse de fonctionner, et on ne s'en aperçoit qu'en soutenance. Un format partagé mérite un document de référence unique, opposable en revue.

> **Rappel de méthode.** Les valeurs de cette note ne sont pas inventées : elles sont **choisies dans des standards existants** et **vérifiables** (§3 et §15). Quand nous nous écartons d'un standard, c'est écrit et justifié (§4.3, §12).

---

## 2. Trois usages, trois contraintes

Le format est le résultat d'un compromis entre trois usages réels du projet. Les nommer évite les débats de goût.

| Usage | Qui | Ce qu'il exige | Conséquence sur le format |
|---|---|---|---|
| **Développement** | les quatre | Lisible **à l'œil**, filtrable avec `grep`, sans outil à installer (budget 0 €) | Préfixe **positionnel** (colonnes alignées), pas de JSON à lire |
| **Soutenance** | l'équipe, le jury | **Fusion** de deux programmes en un seul flux, **corrélation** d'une action de bout en bout | Horodatage **triable tel quel**, **une ligne = un événement**, identifiant de demande |
| **Exploitation** | l'équipe, le futur mainteneur | Investigation, comptage, absence de fuite ; reprise possible par un outil tiers | **Vocabulaire fermé**, **clés normalisées**, aucune donnée personnelle |

---

## 3. Ce que disent les standards et les grandes entreprises

### 3.1. RFC 5424 — le format de référence du monde Unix

Le protocole syslog (RFC 5424, 2009) impose la structure :

```abnf
SYSLOG-MSG = HEADER SP STRUCTURED-DATA [SP MSG]
HEADER     = PRI VERSION SP TIMESTAMP SP HOSTNAME SP APP-NAME SP PROCID SP MSGID
```

Trois enseignements directement réutilisables :

| Élément de la RFC | Ce que ça nous apprend |
|---|---|
| **En-tête positionnel** (PRI, VERSION, TIMESTAMP, HOSTNAME, APP-NAME, PROCID, **MSGID**) | Un en-tête **positionnel** de taille fixe, puis des données structurées : c'est **exactement** la structure de notre ligne (préfixe de champs, puis paires `clé=valeur`) |
| **MSGID** : *« the type of message that should be the same for events with the same semantics »* | Les messages ne sont **pas** du texte libre : **un même fait porte toujours le même nom**. C'est la règle qui fonde notre §8 |
| **STRUCTURED-DATA** : `[SD-ID clé="valeur" …]`, échappement de `"`, `\` et `]` | Les valeurs structurées sont **quotées et échappées** — jamais recopiées brutes. C'est la règle qui fonde notre §10 |
| **NILVALUE** = `-` | Un champ absent s'écrit `-`, jamais rien, jamais `null`. Nous adoptons exactement cette convention |
| **TIMESTAMP** = RFC 3339, `T` et `Z` **majuscules**, 1 à 6 décimales | Notre horodatage (UTC, milliseconde, suffixe `Z`) est **conforme** à un standard, pas une convention maison |
| **VERSION** : *« MUST be incremented for any change of the HEADER format »* | Un format **change** ; il faut donc un **contrat de compatibilité** écrit (§11) |
| **Facilities 16 → 23 = `local0` … `local7`** | Pour envoyer nos lignes à `syslog`, on utilise `local0` (**16**) : c'est la plage réservée aux applications locales, et elle donne des `PRIVAL` vérifiables (§6.2) |

**Sévérités syslog (table 2 de la RFC)** — les nombres que nous réutilisons au §7 : `0` Emergency · `1` Alert · `2` Critical · `3` Error · `4` Warning · `5` Notice · `6` Informational · `7` Debug.

### 3.2. logfmt — la convention des grandes plateformes

**logfmt** est la convention de journalisation de **Heroku** (documentée par Brandur Leach en 2013, implémentée par ses bibliothèques et reprise dans de nombreux écosystèmes) : une suite de paires `clé=valeur` séparées par des espaces.

```
key=value otherkey="value with spaces" third="with escaped \"chars\""
```

| Règle logfmt | Usage chez nous |
|---|---|
| Une paire = `clé` `=` `valeur`, séparées par **une seule espace** | Identique (queue de ligne) |
| **Guillemets uniquement si nécessaire** (espace, `=`, guillemet dans la valeur) | Repris au §10 |
| Échappements : `\"`, `\\`, `\n`, `\r`, `\t` | Repris au §10.3, en complément du §10.6 de DOC-09 |
| Conventions de valeur absente (`~null~` dans certaines implémentations) | Nous préférons **`-`** (NILVALUE de la RFC 5424) pour rester homogènes avec les champs positionnels |
| Aucune spécification formelle unique | Raison pour laquelle **nous écrivons la nôtre** : ici, la spécification est cette note |

### 3.3. ECS — le schéma des plates-formes d'observabilité

L'**Elastic Common Schema (ECS)** est le schéma normalisé utilisé par la pile Elastic (et au-delà) pour nommer les champs. Ses exigences :

- champs **obligatoires et ordonnés** pour un événement de journal : `@timestamp`, `log.level`, `ecs.version` ;
- `message` pour le message ; `event.action` pour **le nom de l'événement**, `event.code` pour son code, `event.severity` pour sa **sévérité numérique syslog** (2 = critical, 3 = error, 4 = warning, 6 = information, 7 = debug) ;
- `service.name`, `service.version`, `service.environment` ; `log.logger` pour le composant émetteur ;
- `trace.id` / `transaction.id` pour la corrélation ; `labels.*` pour les paires libres ; `error.type`, `error.message`, `error.stack_trace` pour les erreurs.

**Ce que ça valide** : notre `SERVICE` ↔ `service.name`, notre `MODULE` ↔ `log.logger`, notre `r=` ↔ `trace.id`/`transaction.id`, notre queue `clé=valeur` ↔ `labels.*`. Le schéma industriel **a les mêmes cases que nous** — les nôtres n'ont simplement pas de préfixe à points.

### 3.4. OpenTelemetry — le modèle de données le plus actuel

Le **modèle de journal d'OpenTelemetry** (stable) définit un enregistrement par : `Timestamp`, `ObservedTimestamp`, `TraceId`, `SpanId`, `TraceFlags`, `SeverityText`, `SeverityNumber`, `Body`, `Resource`, `InstrumentationScope`, `Attributes`, **`EventName`**.

Deux règles directement applicables :

1. **`SeverityNumber`** : un **nombre** normalisé, par plages — `1-4` TRACE, `5-8` DEBUG, `9-12` INFO, `13-16` WARN, `17-20` ERROR, `21-24` FATAL. Règle de conversion : *si la source n'a qu'un niveau par plage, prendre la plus petite valeur de la plage*. Notre table du §7 applique cette règle (INFO → 9, WARN → 13, ERROR → 17, FATAL → 21).
2. **`EventName`** (« nom qui identifie la classe d'événement ») : la même idée que le `MSGID` de la RFC 5424, dix-sept ans plus tard. Deux standards indépendants qui disent la même chose : **l'événement a un nom**, le message n'est pas une phrase libre.

### 3.5. Google Cloud et le 12-Factor — la question du transport

- **Google Cloud Logging** (exemple officiel d'une application conteneurisée) produit un objet JSON minimal : `{"severity":"INFO","time":"…","message":"…", "logging.googleapis.com/labels":{…}}`. Autrement dit : **sévérité + horodatage + message + étiquettes** — les mêmes quatre informations que notre ligne, dans un autre encodage.
- **12-Factor, facteur XI** : *« un douze-facteurs n'a jamais à se préoccuper du routage ou du stockage de son flux de sortie ; il n'essaie ni d'écrire ni de gérer des fichiers de journal ; chaque processus écrit son flux d'événements, sans tampon, sur la sortie standard »*.

> **Position du projet, écart assumé et documenté.** Le principe (« le flux d'événements est la sortie, l'environnement s'occupe du stockage ») est **suivi là où la plateforme existe** : dans le conteneur Podman, le serveur écrit sur la **sortie standard** et `podman logs` fait le reste. Sur un **poste de TP sans plateforme** (budget 0 €, aucune infrastructure d'agrégation), l'application gère elle-même un **fichier roté** — c'est un écart au facteur XI, il est **assumé**, borné (10 Mo × 5) et **écrit ici** plutôt que découvert par un correcteur.

### 3.6. Le voisinage : journald, rsyslog, GELF

| Système | Ce qu'il apporte | Ce que nous en retenons |
|---|---|---|
| **systemd-journald** | Champs natifs (`MESSAGE`, `PRIORITY`, `SYSLOG_IDENTIFIER`, `_PID`, `_COMM`) et sélection par étiquette : `journalctl -t animetcg-srv` | Nos **identifiants d'application** `animetcg-srv` / `animetcg-client` sont l'équivalent de `SYSLOG_IDENTIFIER` ; notre niveau alimente `PRIORITY` (même échelle numérique que la RFC 5424) |
| **rsyslog** | Transport vers un collecteur (`@@machine:514`) | Utilisé tel quel au **niveau 1** de centralisation (§5.8.3), **sans code applicatif** |
| **Graylog GELF** | Format d'agrégation JSON (`version`, `host`, `short_message`, `timestamp`, `level`) | **Hors périmètre** (🎁) ; notre §6.3 donne la conversion au cas où |

### 3.7. Ce que dit la comparaison

| Format | Lisible à l'œil | Coût en C (chemin non bloquant) | Fusion par tri | Extensible sans casser les lecteurs | Écosystème | Risque d'injection |
|---|---|---|---|---|---|---|
| **JSON Lines / GELF** | ✗ (sauf outil) | ✗ (échappement + allocations) | ~ (tri possible) | ✓ | ✓✓ | Faible *si* échappé |
| **logfmt seul** | ✓ | ✓ | ✗ (pas de préfixe triable) | ✓ | ~ | Moyen (guillemets) |
| **Positionnel seul** (TSV) | ✓✓ | ✓✓ | ✓✓ | ✗ (réordonner casse tout) | ✗ | Faible |
| **Préfixe positionnel + queue logfmt *(notre choix)*** | ✓✓ | ✓✓ | ✓✓ | ✓ (clés ajoutées en fin) | ~ (mais mapping §6) | Faible (échappement §10) |

---

## 4. La décision

### 4.1. Le format retenu

```
HORODATAGE | NIVEAU | SERVICE | c=CLIENT | r=REQUÊTE | MODULE | message | clés=valeurs
```

**Ce n'est pas une invention locale** : c'est la structure **en-tête positionnel + données structurées + message** de la **RFC 5424** (§3.1), écrite sur une seule ligne séparée par ` | ` au lieu d'un en-tête syslog binaire, et dont la queue suit la convention **logfmt** (§3.2). Les quatre standards examinés désignent **les mêmes cases** (§6) ; nous n'en inventons aucune.

### 4.2. Pourquoi ce format, et pas JSON

| Critère | Constat |
|---|---|
| **Chemin non bloquant (ENF-57)** | Une ligne de journal est construite dans le fil d'écriture : avec un format à champs positionnels, **il n'y a rien à échapper** sauf le séparateur. Un JSON exigerait, pour chaque ligne, un échappement complet et un tampon de sérialisation — du travail d'allocation précisément là où la règle du projet est de **n'en faire aucun** |
| **Sortie de secours** | Si le fichier est tronqué par un disque plein, une ligne positionnelle reste **partiellement lisible** ; une ligne JSON tronquée ne se relit pas |
| **Démonstration** | `tail -f` sur un fichier commun aux deux programmes : le jury **lit** l'entrelacement sans outil (§5.8.3) |
| **Machine** | La queue `clé=valeur` se parse avec une machine à états d'une trentaine de lignes (test du §13), et la conversion vers JSON/ECS est **définie** au §6.3 |
| **Coût d'entrée** | Zéro installation : les deux bibliothèques savent écrire un motif de ligne (`zlog.conf`, `logback.xml`) |

### 4.3. Les deux écarts assumés

1. **Pas de champ VERSION dans la ligne** (la RFC 5424 en a un). À notre échelle, la compatibilité est garantie autrement : **ordre figé** + **ajout de clés uniquement en fin de ligne** + **lecteurs tolérants** (§11). Le jour où une rupture devient nécessaire, elle passera par un **nouvel identifiant de service** (`animetcg-srv2`), ce qui laisse les deux formats cohabiter dans `journalctl`.
2. **Écriture dans un fichier** (facteur XI du 12-Factor). Justifié au §3.5, borné par la rotation, sans effet sur la forme de la ligne.

---

## 5. Grammaire normative

La grammaire ABNF complète est en **§10.3 de DOC-09** (elle y est normative et testée). Elle est rappelée ici pour que cette note soit autoportante :

```abnf
ligne      = horodatage sep niveau sep service sep client sep requete sep module sep message [sep cles] CRLF
sep        = " | "
horodatage = 4DIGIT "-" 2DIGIT "-" 2DIGIT "T" 2DIGIT ":" 2DIGIT ":" 2DIGIT "." 3DIGIT "Z"
niveau     = "TRACE" / "DEBUG" / "INFO" / "WARN" / "ERROR" / "FATAL"
service    = "srv" / "cli"
client     = 1*DIGIT / "-"
requete    = 1*12HEXDIG / "-"
module     = "net" / "protocol" / "business" / "chain" / "persistence" / "util"
           / "session" / "view" / "controller" / "model"
message    = 1*120(texte sans "|" ni CR ni LF)
cles       = paire *(" " paire)
paire      = clé "=" valeur
```

**Les cinq compléments apportés par cette note** (ils ne modifient pas la grammaire, ils la rendent exploitable) :

1. le **message** est choisi dans le **vocabulaire fermé du §8** — il ne contient **jamais** de valeur interpolée ;
2. les **clés** respectent les règles de nommage du **§9** (minuscules, `snake_case`, unité dans le nom) ;
3. les **valeurs** suivent les règles d'échappement et de bornage du **§10** ;
4. le **mapping** vers syslog, ECS, OpenTelemetry, GELF est défini au **§6** (il est donc possible, sans changer la ligne) ;
5. le **contrat de compatibilité** du **§11** interdit tout réordonnancement et impose la tolérance des lecteurs.

---

## 6. Mapping vers les référentiels

### 6.1. Une ligne, quatre vocabulaires

| Notre champ | RFC 5424 (syslog) | ECS | OpenTelemetry | JSON (GELF / Google) |
|---|---|---|---|---|
| `HORODATAGE` | `TIMESTAMP` | `@timestamp` | `Timestamp` | `timestamp` / `time` |
| `NIVEAU` | `SEVERITY` (dans `PRI`) | `log.level` + `event.severity` | `SeverityText` + `SeverityNumber` | `level` / `severity` |
| `SERVICE` | `APP-NAME` | `service.name` | `Resource.service.name` | `host` / `service` |
| `c=` | `STRUCTURED-DATA` (`c`) | `labels.client_id` | `Attributes["client.id"]` | `_client_id` |
| `r=` | `STRUCTURED-DATA` (`r`) | `trace.id` / `transaction.id` | `TraceId` | `_request_id` |
| `MODULE` | `STRUCTURED-DATA` (`module`) | `log.logger` | `InstrumentationScope.name` | `_module` |
| `message` | **`MSGID`** | **`event.action`** | **`EventName`** | `short_message` / `message` |
| `clés=valeurs` | `STRUCTURED-DATA` | `labels.*` + `event.*` | `Attributes` | `_<clé>` |

> **Le point à retenir** : dans les quatre référentiels, le **message est un nom**, pas une phrase. C'est la même idée sous quatre noms différents (MSGID, event.action, EventName, short_message).

### 6.2. Diffusion vers `syslog` (cas des postes séparés)

| Paramètre syslog | Valeur AnimeTCG | Exemple |
|---|---|---|
| `FACILITY` | **`local0` = 16** (plage `local0…local7` réservée aux applications) | — |
| `SEVERITY` | Table du §7 | `INFO` → 6 |
| `PRIVAL` = `facility × 8 + severity` | Calculé | `16 × 8 + 6` = **`<134>`** (INFO) · `<132>` (WARN) · `<131>` (ERROR) · `<130>` (FATAL) · `<135>` (DEBUG) |
| `APP-NAME` | `animetcg-srv` ou `animetcg-client` | — |
| `PROCID` | Numéro de processus | `4821` |
| `MSGID` | **Le message du §8** (vocabulaire fermé) | `block mined` |
| `STRUCTURED-DATA` | `[animetcg@32473 c="7" r="ab12f3c4d5e6" module="chain"]` | Échappement de `"`, `\`, `]` |

> Le **SD-ID `animetcg@32473`** suit la forme `nom@numéro` de la RFC 5424, le nombre appartenant à la plage **réservée à la documentation** (RFC 5612) : personne ne confondra nos données avec celles d'un éditeur réel.

### 6.3. Conversion vers JSON (non normative — pour un agrégateur futur)

```json
{"timestamp":"2026-09-22T10:04:11.980Z","level":"INFO","service":"animetcg-srv",
 "client_id":"7","request_id":"ab12f3c4d5e6","module":"chain",
 "event":"block mined","difficulty":4,"nonce":31847,"hashes":72144,"ms":294}
```

C'est la forme utilisable telle quelle par un collecteur (Graylog/GELF ou Google) : **le mapping est un exercice de renommage, pas une réécriture** — c'est ce que garantit le §6.1.

---

## 7. Niveaux — politique et nombres

### 7.1. Table de correspondance (normative)

| Niveau AnimeTCG | Sens | syslog `SEVERITY` | `PRIVAL` (local0) | ECS `event.severity` | OTel `SeverityNumber` | Logback | `zlog` |
|---|---|---|---|---|---|---|---|
| `TRACE` | Développement uniquement (32 premiers octets d'une trame) | 7 | `<135>` | 7 | **1** | TRACE | `ZLOG_TRACE` |
| `DEBUG` | Détails de développement | 7 | `<135>` | 7 | **5** | DEBUG | `ZLOG_DEBUG` |
| `INFO` | **Les faits** : ce qui s'est passé | 6 | `<134>` | 6 | **9** | INFO | `ZLOG_INFO` |
| `WARN` | Anomalie **récupérée** ou **refus métier** | 4 | `<132>` | 4 | **13** | WARN | `ZLOG_WARN` |
| `ERROR` | Opération **échouée** alors qu'elle devait réussir | 3 | `<131>` | 3 | **17** | ERROR | `ZLOG_ERROR` |
| `FATAL` | Le service ne peut plus être rendu | 2 | `<130>` | 2 | **21** | ERROR *(avec marqueur)* | `ZLOG_FATAL` |

**Trois règles de lecture :**

1. Les nombres **OTel** appliquent la règle du standard : *un seul niveau par plage → la plus petite valeur* (INFO → 9, et non 12).
2. **`TRACE` et `DEBUG` partagent la sévérité syslog 7** : syslog n'a que huit niveaux, l'information « trace » n'existe pas côté système. Le champ `NIVEAU` de notre ligne reste, lui, précis — la perte n'a lieu **qu'au moment de la diffusion**, et elle est alors documentée.
3. **Logback n'a pas de `FATAL`** : on écrit `ERROR` avec un marqueur explicite (le niveau `FATAL` reste celui de notre ligne). Cela évite d'inventer une extension non standard dans une bibliothèque tierce.

### 7.2. Règles d'affectation (rappel, détail en §10.4 de DOC-09)

- Un **refus métier** (100 cartes actives, recommandation en double, vote du propriétaire, second vote) est un **`WARN`** : le système a parfaitement fonctionné.
- **Toute** ligne `ERROR` ou `FATAL` côté serveur porte un `r=` — sans identifiant de demande, une erreur n'est pas corrélable.
- Un état de **saturation** est un `WARN` **avec compteurs** (`in_flight`, `queue_depth`) : c'est une donnée d'exploitation, pas une fatalité.
- Le niveau global se règle **sans recompiler** (ENF-58) : `ANIMETCG_LOG_LEVEL` côté serveur, `logback.xml` côté client. Défaut : **`INFO`**.

---

## 8. Le vocabulaire des événements — le `message`

### 8.1. La règle

Le champ `message` (7ᵉ champ de la ligne) n'est **pas** une phrase libre : c'est **le nom de l'événement**, choisi dans la liste fermée du §8.2.

| Règle | Pourquoi |
|---|---|
| Anglais, minuscules, **2 ou 3 mots**, une seule espace entre les mots | Cohérence ENF-34 ; greppable ; lisible dans une colonne |
| **Aucune valeur interpolée** dans le message | C'est **la** règle qui distingue une ligne de journal d'un `printf` : les données vont dans les clés, jamais dans le texte (anti-patron n° 1 des guides de journalisation structurée) |
| **Un même fait porte toujours le même nom** | Exigence `MSGID` de la RFC 5424 : des événements de même sémantique partagent le même nom |
| **Nom jamais renommé** : il se **déprécie** (garde la même chaîne, cesse d'être émis) | Un renommage casse silencieusement les requêtes et les tableaux de bord des uns et des autres |
| Le nom technique de l'action du protocole (ex. `card.create`) n'est **pas** dans le message : il va dans la clé `action=` | Le message décrit **l'événement**, la clé décrit **la demande** |

### 8.2. Catalogue (liste fermée)

| Module | Événement (`message`) | Niveau | Quand il est écrit |
|---|---|---|---|
| `session` | `server started` | INFO | Démarrage, après le banc de calibrage (`hash_rate`, `difficulty`) |
| `session` | `listen ready` | INFO | Port ouvert (`ip`, `port`, `backlog`) |
| `session` | `server stopped` | INFO | Arrêt propre, ressources libérées (EF-27) |
| `session` | `client connected` | INFO | Socket acceptée, avant toute décision d'admission |
| `session` | `client admitted` | INFO | Une place se libère et est attribuée |
| `session` | `client disconnected` | INFO | Départ propre (`reason=user` \| `reason=timeout` \| `reason=error`) |
| `session` | `client expired` | WARN | Inactivité > 300 s (ENF-25) |
| `session` | `queue enqueue` | INFO | Le client est mis en file (`position`, `depth`) |
| `session` | `queue update` | DEBUG | Position recalculée après un départ |
| `session` | `queue full` | WARN | File pleine → `-32002` puis fermeture |
| `session` | `queue timeout` | WARN | Attente > 60 s → `-32003` puis fermeture |
| `session` | `session welcomed` | INFO | *(client)* Le contexte est reçu et appliqué |
| `session` | `reconnect attempt` | WARN | *(client)* Tentative de reconnexion (`attempt`) |
| `session` | `server busy` | WARN | Rejet de charge (`queue_depth`, `in_flight`) |
| `session` | `resync requested` | WARN | *(client)* Tampon d'événements débordé : resynchronisation demandée |
| `net` | `request sent` | DEBUG | *(client)* Trame émise (`action`, `bytes`) |
| `net` | `request received` | DEBUG | Trame lue (`bytes`) |
| `net` | `response received` | DEBUG | *(client)* Réponse lue (`status`, `bytes`) |
| `net` | `connection lost` | ERROR | *(client)* Socket fermée par le pair ou le réseau |
| `net` | `event broadcast` | DEBUG | Diffusion d'un événement (`event`, `recipients`) |
| `net` | `listen failed` | FATAL | Impossible d'ouvrir la socket d'écoute (`reason`) |
| `protocol` | `frame read` | TRACE | 32 premiers octets d'une trame (développement) |
| `protocol` | `frame rejected` | WARN | Trame refusée avant décodage (`reason`, `bytes`) |
| `protocol` | `request accepted` | INFO | Demande valide, transmise au métier (`action`, `payload_bytes`) |
| `protocol` | `request refused` | WARN | Refus de **forme** ou de **séquence** (`code`, `field`) |
| `protocol` | `protocol error` | ERROR | Enveloppe invalide, action inconnue (`code`) |
| `protocol` | `version refused` | WARN | Version majeure non supportée (`version`) |
| `chain` | `calibration` | INFO | Banc de calibrage terminé (`hash_rate`, `difficulty`, `ms`) |
| `chain` | `block mined` | INFO | Bloc miné (`block_index`, `difficulty`, `nonce`, `hashes`, `ms`) |
| `chain` | `mining deadline exceeded` | ERROR | Délai de garde dépassé (`difficulty`, `hashes`, `ms`) |
| `chain` | `nonce budget exhausted` | ERROR | Budget d'essais épuisé (`difficulty`, `hashes`) |
| `chain` | `chain loaded` | INFO | Chaîne relue depuis la base (`blocks`, `ms`) |
| `chain` | `chain verified` | INFO | Vérification complète (`blocks`, `checked`) |
| `chain` | `verification failed` | ERROR | **Chaîne altérée détectée** (`block_index`, `reason`) |
| `chain` | `difficulty changed` | INFO | Réévaluation appliquée (`difficulty_before`, `difficulty_after`, `hash_rate`) |
| `persistence` | `block committed` | INFO | Bloc + projection écrits dans la même transaction (`block_index`, `ms`) |
| `persistence` | `query done` | DEBUG | Requête terminée (`op`, `ms`) |
| `persistence` | `db slow` | WARN | Requête au-delà du budget (`op`, `ms`) |
| `persistence` | `db write failed` | ERROR | Écriture refusée ou interrompue (`op`) |
| `persistence` | `db unavailable` | ERROR | Connexion à la base perdue (`ms`) |
| `persistence` | `restore done` | INFO | Contexte reconstruit au démarrage (`blocks`, `ms`) |
| `business` | `card created` | INFO | Carte inscrite en chaîne (`card_id`, `user_id`, `block_index`) |
| `business` | `card inactivated` | INFO | Carte rendue inactive (`card_id`, `reason`) |
| `business` | `card archived` | INFO | Carte **archivée** après une défaite (`card_id`, `winner_card_id`) — **plus affichée** ; ses droits restent inchangés (Q2.6). *Remplace `card merged`.* |
| `business` | `value updated` | INFO | Valeur recalculée (`card_id`, `value_before`, `value_after`, `cause`) |
| `business` | `recommendation added` | INFO | Recommandation active (`card_id`, `user_id`, `value_after`) |
| `business` | `recommendation repudiated` | INFO | Recommandation retirée (`card_id`, `user_id`) |
| `business` | `battle opened` | INFO | Battle ouverte (`battle_id`, `card_id`, `other_card_id`) |
| `business` | `vote cast` | INFO | Vote **définitif** enregistré (`battle_id`, `votes`) — Q2.4 |
| `business` | `vote refused` | WARN | Second vote du même votant refusé (`battle_id`, `code=1020`) — le vote ne se modifie pas |
| `business` | `battle closed` | INFO | Clôture et application du résultat (`battle_id`, `outcome` = `gagnee` \| `annulee_quorum` \| `annulee_0_vote`, `winner_card_id`, `votes`) |
| `business` | `legitimation opened` | INFO | Demande de légitimation (`legitimation_id`, `card_id`) |
| `business` | `legitimation liked` | INFO | Like déposé sur une candidature (`legitimation_id`, `user_id`, `likes`) — Q4.1 |
| `business` | `legitimation taken` | INFO | Une candidature devient **porteur légitime** en dépassant les likes (`legitimation_id`, `card_id`, `likes`) |
| `business` | `claim opened` | INFO | Revendication déposée (`claim_id`, `card_id`, `user_id`) |
| `business` | `claim transferred` | INFO | Transfert effectué (`claim_id`, `card_id`, `user_id`) |
| `business` | `claim refused` | WARN | Refus explicite du propriétaire — statu quo, sans recours (DEC-11) |
| `business` | ~~`claim closed`~~ | — | ⚠️ **n'est plus émis** depuis le 22/09/2026 : la règle « sans objet » est retirée (Q2.6 — les droits de la carte perdante restent inchangés). Entrée conservée pour l'historique. *(ancien : re*** (§4.5.4, CU-17) |
| `business` | `transfer scheduled` | INFO | Transfert automatique programmé (`claim_id`, `days`) |
| `business` | `rule refused` | WARN | Refus métier (`code`, `reason`) — voir la règle du §7.2 |
| `util` | `log rotated` | INFO | Fichier de journal fermé et remplacé (`bytes`) |
| `util` | `log dropped` | WARN | File d'écriture saturée, entrée abandonnée (`dropped`) |
| `business` | `card authenticated` | INFO | Carte certifiée par son porteur légitime (`card_id`, `user_id`, `block_index`) — **rétabli le 22/09/2026** (CU-20, DEC-24) |
| `business` | `authentication revoked` | INFO | Authentification retirée par le porteur légitime (`card_id`, `user_id`) |
| `business` | `authentication refused` | WARN | Authentification demandée par un non-légitimé (`code`) |
| `business` | `share sent` | INFO | Carte proposée à un ami (`share_id`, `card_id`, `to_user_id`) — Q5.2 |
| `business` | `share accepted` | INFO | Partage accepté : **le demandeur se déplace** (`share_id`, `card_id`, `block_index`) — action **inscrite en blockchain** depuis le 22/09/2026 |
| `business` | `share refused` | INFO | Partage refusé : personne ne bouge (`share_id`, `card_id`) — **journalisé seuls**, aucun bloc métier |
| `view` | `startup failed` | FATAL | *(client)* Démarrage impossible (`reason`) |

> **Ce tableau est la référence.** Il est recopié en **constante dans les deux langages** (`server/util/log_events.h`, `client/.../LogEvents.java`) et un test vérifie qu'aucune ligne émise ne porte un message hors catalogue (§13.2). Ajouter un événement = **une ligne ici** + **une constante** + **rien à modifier ailleurs**.

---

## 9. Les clés (`clé=valeur`)

### 9.1. Règles de nommage

| # | Règle | Justification |
|---|---|---|
| 1 | **Minuscules**, chiffres et `_` uniquement ; jamais d'accent, jamais de majuscule | Le même champ doit porter le même nom dans les deux langages (règle ECS « un seul nom par information ») |
| 2 | **`snake_case`**, jamais `camelCase` | Convention dominante des schémas de journalisation (`user_id`, jamais `userId`) ; côté Java, cela évite les réflexes de nommage du code |
| 3 | **L'unité est dans le nom** : `ms` (durée), `bytes` (octets), `hashes` (tentatives), `<sujet>_len` (longueur en caractères) | Empêche l'ambiguïté « 300 » (millisecondes ? secondes ?) — le nombre seul n'est jamais interprétable |
| 4 | Les **comptages** portent un nom au pluriel : `cards`, `recipients`, `blocks`, `votes`, `approvals` | Un « count » nu ne dit pas *de quoi* |
| 5 | Les **identifiants** portent le suffixe `_id` : `card_id`, `user_id`, `battle_id`, `claim_id` | On distingue `cards=1` (combien) de `card_id=42` (lesquels) |
| 6 | Les **avant/après** s'écrivent **`<sujet>_before` / `<sujet>_after`** — jamais `old`/`new` seuls | Deux mois plus tard, `new=3` ne veut rien dire ; `difficulty_after=3` si |
| 7 | **8 clés au maximum** par ligne ; au-delà, le reste est résumé par `extra=N` | Garde la ligne courte (§10.4) et oblige à choisir ce qui compte |
| 8 | **Ordre stable** pour une même ligne de code | Deux exécutions identiques produisent deux lignes **identiques** : comparable en test |
| 9 | Une clé **ne change jamais de sens** (pas de réemploi) | Un même nom qui veut dire deux choses rend les requêtes fausses et silencieuses |

### 9.2. Clés autorisées

| Clé | Type | Unité | Exemple | Où |
|---|---|---|---|---|
| `c` | entier ou `-` | — | `c=7` | Positionnel (identifiant serveur du client) |
| `r` | 12 hex ou `-` | — | `r=ab12f3c4d5e6` | Positionnel (identifiant de demande) |
| `action` | nom du protocole | — | `action=card.create` | Requêtes et réponses |
| `status` | liste fermée (protocole) | — | `status=OK` | Réponses |
| `code` | liste fermée (protocole) | — | `code=CARD_LIMIT_REACHED` | Refus et erreurs |
| `field` | liste fermée (champs du sujet) | — | `field=year` | Validation |
| `reason` | liste fermée (maison) | — | `reason=card_limit` | Refus, fermetures, dégradation |
| `event` | liste des **21** événements du protocole | — | `event=card.created` | Diffusions |
| `module`, `service` | listes fermées | — | `module=chain` | Jamais en clé (déjà positionnels) |
| `user_id`, `card_id`, `other_card_id`, `winner_card_id`, `battle_id`, `claim_id`, `legitimation_id`, `authentication_id` | entier | — | `card_id=42` | Métier |
| `block_index` | entier | — | `block_index=118` | Chaîne |
| `hash_head` | 12 hex | — | `hash_head=00045bb88bc0` | Chaîne (les 12 premiers caractères du hash) |
| `difficulty`, `difficulty_before`, `difficulty_after` | entier | — | `difficulty=4` | Chaîne |
| `nonce` | entier | — | `nonce=31847` | Minage |
| `hashes` | entier | tentatives | `hashes=72144` | Minage |
| `hash_rate` | entier | hachages/s | `hash_rate=198432` | Calibrage |
| `ms` | entier | millisecondes | `ms=294` | Partout où une durée existe |
| `bytes`, `payload_bytes` | entier | octets | `bytes=318` | Trames, journaux |
| `len` (sous la forme `<sujet>_len`) | entier | caractères | `justification_len=118` | Textes utilisateur (longueur **seule**) |
| `cards`, `blocks`, `votes`, `approvals`, `rejections`, `recipients`, `checked` | entier | comptes | `recipients=12` | Volumétrie |
| `op` | liste fermée | — | `op=insert` | Persistance (`select`, `insert`, `update`) |
| `pattern` | chaîne | — | `pattern=0000` | Calibrage (`0` répété *k* fois) |
| `position`, `depth`, `in_flight`, `dropped`, `extra`, `attempt`, `days` | entier | — | `position=3` | File d'attente, exploitation |
| `value_before`, `value_after` | décimal | 1 décimale | `value_after=8.0` | Valeur d'une carte |
| `cause` | liste fermée | — | `cause=recommendation` | Changement de valeur |
| `version` | `X.Y` | — | `version=1.0` | Poignée de main |
| `ip`, `port` | chaîne / entier | — | `port=7001` | **`DEBUG` uniquement** (RGPD, §10.7) |

> Toute clé qui n'est pas dans ce tableau **n'existe pas** : le test du §13.3 échoue. Ajouter une clé est un acte volontaire — une ligne dans ce tableau, puis dans la constante de code.

---

## 10. Valeurs, échappement et bornes

### 10.1. Valeur absente

`-` (**NILVALUE** de la RFC 5424). Jamais `null`, `none`, `N/A` ou une chaîne vide — un champ absent doit être **visiblement** absent.

### 10.2. Quand une valeur est-elle quotée ?

Une valeur **reste nue** si elle n'est composée que de `A-Z a-z 0-9 . _ - : / @ +` (cas de tous nos identifiants, noms d'action et codes). Sinon, et **seulement** sinon, elle est entourée de guillemets doubles et échappée :

| Caractère dans la valeur | Écriture | Remarque |
|---|---|---|
| Espace, `=` | valeur entière **quotée** : `message_path="deux mots"` | Convention logfmt |
| `"` | `\"` | Convention logfmt |
| `\` | `\\` | Convention logfmt |
| Retour à la ligne (CR/LF) | `\n`, `\r` **(deux caractères)** | Anti-injection, CWE-117 (§10.6 de DOC-09) |
| Tabulation | `\t` | Lisibilité |
| Séparateur de champ ` \| ` | `\u007C` | Anti-injection (§10.6 de DOC-09) |
| Autre caractère de contrôle (0x00–0x1F, 0x7F) | `\xNN` | Lisibilité sur terminal |
| Octet UTF-8 invalide | `?` | Assainissement définitif |

### 10.3. Bornes (normatives)

| Objet | Borne | Comportement au dépassement |
|---|---|---|
| Message | **120** caractères | Tronqué, suffixe `~` |
| Valeur | **64** caractères | Tronquée, suffixe `~` |
| Clé | **24** caractères | Nom invalide → **test en échec** (on ne tronque pas un nom) |
| Ligne complète | **512** caractères | Tronquée à la fin, suffixe `~` |
| Clés par ligne | **8** | Le reste résumé par `extra=N` |
| Trame journalisée | jamais la charge utile | On journalise identifiants, compteurs, durées (§10.7 de DOC-09) |

### 10.4. Ce qui ne s'écrit jamais dans une valeur

Identique au §10.7 de DOC-09, rappelé ici parce que c'est **la** ligne de défense en soutenance : ni mot de passe, ni charge utile, ni texte saisi par l'utilisateur (synopsis, justification, pseudo en clair), ni valeur recalculable (valeur de carte, propriétaire), ni requête SQL, ni chemin absolu.

---

## 11. Contrat de compatibilité

Six règles, et elles suffisent : c'est l'équivalent de notre champ `VERSION` (§4.3).

| # | Règle | Conséquence |
|---|---|---|
| 1 | **Les 8 champs positionnels ne sont jamais réordonnés, ni renommés, ni supprimés** | Un analyseur écrit aujourd'hui lit les journaux de la phase 4 |
| 2 | **Les clés s'ajoutent uniquement en fin de ligne** | Une ligne reste lisible par un lecteur qui ignore la nouvelle clé |
| 3 | **Un lecteur ignore toute clé inconnue** (lecture tolérante) | C'est ce qui autorise la règle 2 |
| 4 | **Un événement ne se renomme pas** : il se déprécie (§8.1) | Les requêtes et les tableaux restent valides |
| 5 | **Une clé ne change pas de sens** (§9.1, règle 9) | Une clé nouvelle, pas une clé réutilisée |
| 6 | Une **rupture** éventuelle passe par un **nouvel identifiant de service** (`animetcg-srv2`) | Les deux formats cohabitent dans `journalctl` ; aucune coupure |

**Version de la note** : ce document est versionné (en-tête). La configuration des deux bibliothèques porte, en commentaire, la version appliquée (`# note-format-ligne v1.0`) — c'est la trace qui manque le plus souvent dans les projets réels.

---

## 12. Ce qui n'est pas retenu — et ce qu'on en garde

| Format | Pourquoi il n'est pas retenu pour la ligne | Ce qu'on en garde |
|---|---|---|
| **JSON Lines / GELF** | Coût d'échappement et d'allocation dans le chemin d'écriture ; illisible en démonstration | Le **mapping** §6.3 : un agrégateur futur se branche sans changer le code |
| **logfmt seul** | Pas de préfixe triable : la fusion des deux journaux deviendrait un vrai parsing | La syntaxe de la queue (§3.2) |
| **Positionnel seul** (type TSV) | Tout ajout de champ casse les lecteurs | Le préfixe, limité aux champs **stables** |
| **Binaire / protobuf** | Inexploitable à l'œil, aucune bibliothèque embarquée côté C | Rien (mentionné pour mémoire) |
| **Message multiligne** (piles d'appel) | Une exception Java deviendrait 30 lignes : fusion impossible, comptage faux | La pile est **aplatie** sur une ligne, à la suite du message (§10.3 de DOC-09) |
| **CEF / LEEF** (formats SIEM) | Pensés pour des flux de sécurité, pas pour un journal applicatif ; aucun SIEM dans le périmètre | L'idée d'un **vocabulaire fermé** de noms d'événements — déjà prise au §8 |

---

## 13. Vérification

### 13.1. Les tests de la journalisation (REC-17)

Les **10 tests du §10.12 de DOC-09** s'appliquent à cette note : format sur 1000 lignes, une ligne par événement, fusion triée, corrélation de bout en bout, anti-injection, anti-fuite, non-blocage, rotation, niveau réglable, rétention.

### 13.2. Le test du vocabulaire fermé (ajouté par cette note)

Une ligne dont le `message` ou une clé sort du §8 / §9 **fait échouer la suite de tests**. Contrôle :

```bash
# Extrait les messages distincts d'un journal et les compare au catalogue de code
cut -d'|' -f7 logs/server.log | sed 's/^ *//; s/ *$//' | sort -u
# (la liste ci-dessus doit être incluse dans le catalogue du §8.2, sans exception)
```

### 13.3. La commande de conformité de format

```bash
# 1 000 lignes doivent correspondre exactement à la grammaire du §5
grep -vcE '^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}Z \| (TRACE|DEBUG|INFO|WARN|ERROR|FATAL) \| (srv|cli) \| c=([0-9]+|-) \| r=([0-9a-f]{1,12}|-) \| (net|protocol|business|chain|persistence|util|session|view|controller|model) \|' logs/server.log
# sortie attendue : 0
```

### 13.4. Les trois commandes de la soutenance

```bash
# 1) Fusionner les deux programmes en un flux chronologique
sort -t'|' -k1,1 logs/server.log logs/client-*.log

# 2) Suivre une action de bout en bout (client → serveur → base → client)
sort -t'|' -k1,1 logs/server.log logs/client-*.log | grep 'r=ab12f3c4d5e6'

# 3) Vérifier qu'aucune donnée interdite n'a fui
grep -Eic "password|secret|token|synopsis=" logs/*.log     # attendu : 0
```

---

## 14. Exemples commentés

### 14.1. Une action vue des deux côtés (le scénario de REC-17)

```
2026-09-22T10:03:58.101Z | DEBUG | cli | c=7 | r=ab12f3c4d5e6 | net         | request sent         | action=card.create bytes=318
2026-09-22T10:03:58.118Z | INFO  | srv | c=7 | r=ab12f3c4d5e6 | protocol    | request accepted     | action=card.create payload_bytes=294
2026-09-22T10:03:58.412Z | INFO  | srv | c=7 | r=ab12f3c4d5e6 | chain       | block mined          | block_index=118 difficulty=4 nonce=31847 hashes=72144 ms=294
2026-09-22T10:03:58.455Z | INFO  | srv | c=7 | r=ab12f3c4d5e6 | persistence | block committed      | block_index=118 cards=1 ms=43
2026-09-22T10:03:58.457Z | INFO  | srv | -   | r=-            | net         | event broadcast      | event=card.created recipients=12
2026-09-22T10:03:58.481Z | DEBUG | cli | c=7 | r=ab12f3c4d5e6 | net         | response received    | status=OK bytes=214
```

*Lecture :* la 6ᵉ colonne dit **qui** parle (`net`, puis `protocol`, `chain`, `persistence`) ; la 5ᵉ permet de **recoller** les six lignes ; la diffusion (5ᵉ ligne) porte `r=-` parce qu'elle n'appartient à aucune demande — c'est un fait du serveur, adressé à douze clients.

### 14.2. Un refus métier (le système a bien fonctionné : `WARN`)

```
2026-09-22T10:14:22.903Z | WARN  | srv | c=9 | r=77c1e0b4a9f2 | business | rule refused | code=CARD_LIMIT_REACHED card_id=- user_id=9 reason=card_limit
```

### 14.3. Saturation et calibrage (les deux mécanismes phares)

```
2026-09-22T10:04:11.980Z | INFO | srv | - | r=-            | chain   | calibration         | hash_rate=198432 difficulty=4 ms=151
2026-09-22T10:05:02.004Z | INFO | srv | - | r=-            | session | queue enqueue       | position=1 depth=1
2026-09-22T10:05:02.010Z | WARN | srv | - | r=-            | session | queue full          | position=- depth=100 in_flight=100
2026-09-22T10:05:11.117Z | WARN | srv | - | r=-            | session | server busy         | queue_depth=100 in_flight=100
2026-09-22T10:31:07.114Z | INFO | srv | - | r=-            | chain   | difficulty changed  | difficulty_before=4 difficulty_after=3 hash_rate=39120
```

### 14.4. Une alerte de sécurité (la falsification démontrée en phase 4)

```
2026-09-22T10:52:40.332Z | ERROR | srv | - | r=- | chain | verification failed | block_index=54 reason=difficulty_mismatch
```

---

## 15. Sources

**Normes et formats**

- RFC **5424**, *The Syslog Protocol* (en-tête positionnel, `MSGID`, `NILVALUE`, données structurées, sévérités, facilities `local0…local7`) : <https://www.rfc-editor.org/rfc/rfc5424.html>
- RFC **3339**, *Date and Time on the Internet* (horodatage) : <https://www.rfc-editor.org/rfc/rfc3339>
- RFC **5612**, *Enterprise Numbers for Documentation* (base du SD-ID `animetcg@32473`) : <https://www.rfc-editor.org/rfc/rfc5612>
- **logfmt** — convention Heroku, article de référence de Brandur Leach : <https://brandur.org/logfmt> ; spécification de syntaxe (ABNF, échappements, guillemets) dans la page de manuel `Log::Fmt` : <https://manpages.ubuntu.com/manpages/stonking/man3/Log::Fmt.3pm.html> ; implémentation d'origine : <https://github.com/csquared/node-logfmt>
- Spécification informelle de logfmt pour les bibliothèques Rust : <https://github.com/EmbarkStudios/tracing-logfmt>

**Schémas et modèles industriels**

- **Elastic Common Schema (ECS)** — champs obligatoires (`@timestamp`, `log.level`, `ecs.version`), `event.action` / `event.code` / `event.severity`, `service.*`, `trace.id`, `labels.*` : <https://deepwiki.com/elastic/ecs-logging/2-ecs-logging-specification> et <https://doc.wikimedia.org/ecs/>
- **OpenTelemetry**, *Logs Data Model* — `SeverityNumber` (plages 1-4 TRACE à 21-24 FATAL), `EventName`, `Attributes`, `InstrumentationScope` : <https://github.com/open-telemetry/opentelemetry-specification/blob/main/specification/logs/data-model.md>
- **Google Cloud**, journalisation d'une application conteneurisée (forme JSON `severity` / `time` / `message` / étiquettes) : <https://docs.cloud.google.com/run/docs/logging>
- **The Twelve-Factor App**, facteur XI *Logs* — « un douze-facteurs n'essaie ni d'écrire ni de gérer des fichiers de journal » : <https://12factor.net/logs>
- **systemd**, *journal-fields* — champs natifs (`MESSAGE`, `PRIORITY`, `SYSLOG_IDENTIFIER`) et `journalctl -t` : documentation officielle systemd.
- **Graylog**, *GELF — Graylog Extended Log Format* : documentation officielle Graylog.

**Bonnes pratiques de schéma**

- *Structured Logging: Best Practices* — noms de champs constants, unités dans le nom (`http_duration_ms`), champs essentiels de chaque entrée : <https://uptrace.dev/glossary/structured-logging>
- *How to Implement Structured Logging Best Practices* — schéma d'abord, anti-patrons (interpolation de variables dans le message, noms incohérents), `request_id` propagé : <https://oneuptime.com/blog/post/2026-01-25-structured-logging-best-practices/view>
- Journalisation structurée avec les bibliothèques types (Uber `zap`) — champs typés, pas de concaténation dans le message : <https://oneuptime.com/blog/post/2026-02-01-go-logging-zap-zerolog/view>

**Sécurité (repris de DOC-09 §17)**

- OWASP **Logging Cheat Sheet** ; **CWE-117** (injection de journal) ; **CWE-532** (fuite d'information dans un journal) ; OWASP **ASVS V7** (7.1.1 → 7.4.1).
