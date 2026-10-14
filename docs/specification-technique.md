# Spécification technique AnimeTCG — DOC-09

| | |
|---|---|
| **Projet** | **AnimeTCG** — plateforme cliente-serveur de cartes d'anime : création, recommandation, battles, chaîne de blocs |
| **Document** | **DOC-09** — spécification technique (exigé par le sujet ; phase 1, enrichi jusqu'en phase 3) |
| **Version** | **0.7** — 22/09/2026 (0.7 : **valeurs définitives + minage 0,3–2 s** ; 0.6 : plus d'encadrant, décisions d'équipe ; 0.5 : réponses du questionnaire ; 0.4 : authentification et bibliothèques ; 0.3 : note de format ; 0.2 : sujets techniques ; 0.1 : 19/09/2026) |
| **Auteurs** | Équipe SAÉ BUT2 S3 — M1 (réseau & protocole), M2 (chaîne & métier), M3 (persistance & données), M4 (client & IHM) |
| **Cahier des charges de référence** | `cahier-des-charges-sae-s3.md` **v0.24** (22/09/2026) |
| **Documents liés** | `docs/note-format-ligne.md` (**v1.3**, forme des journaux) · `docs/protocole-applicatif.md` (**v1.2**) · `docs/uml/` (**14 diagrammes + 28 exports**) · `sql/schema.sql` **v1.5** (**DOC-07**) · `questionnaire-decisions.md` · `questions-a-repondre.md` (décisions finales) · `checklist-phase1-10-10.md` |
| **Statut** | **Document vivant** : complété à chaque phase, figé pour le tag `phase3`. Les sections marquées 🔧 sont rédigées mais pas encore implémentées ; ✅ = conforme au code livré |

> **Comment ce document est construit.** Il suit la structure de vues recommandée par la norme **IEEE 1016-2009** (*Software Design Description*) et le cadre de description d'architecture **ISO/IEC/IEEE 42010** : plutôt qu'un récit, une série de **points de vue** (contexte, composition, logique, information, interface, processus, déploiement), chacun répondant aux préoccupations d'un lecteur précis. C'est la façon de faire des grandes équipes : un lecteur qui cherche « où est la règle des 10 cartes ? » ne lit pas 60 pages, il va à la bonne vue.

## Sommaire

| § | Vue / sujet | Pour qui | Exigences couvertes |
|---|---|---|---|
| 1 | Objet, périmètre, conventions de lecture | tous | — |
| 2 | **Adossement normatif** — quelle norme industrielle cadre quoi | jury, équipe *(encadrant jusqu'au 22/09/2026)* | — |
| 3 | Vue **contextuelle** — acteurs, systèmes externes, frontière de confiance | M1, M4 | ENF-05, ENF-27 |
| 4 | Vue de **composition** — dépôt, modules, **architecture en couches** (§4.4), **squelette de fichiers** (§4.5), **contrôle automatique** (§4.6) | tous | ENF-33, ENF-50 |
| 5 | Vue **logique** — contrats d'interface C et Java | M1 → M4 | ENF-33 → ENF-36, ENF-42 |
| 6 | Vue **information** — entités, cycle de vie, **charge utile canonique et hachage** (§6.5) | M2, M3 | ENF-28 → ENF-32, ENF-45, **ENF-52** |
| 7 | Vue **interface** — protocole, IHM, contrats internes, **dossier de relecture du protocole** (§7.4) | M1, M4 | ENF-19 → ENF-27, ENF-44 |
| 8 | Vue **processus / exécution** — fils, budgets, sérialisation, **calibrage de la difficulté** (§8.6 → §8.9) | M1, M2 | ENF-18, ENF-42, **ENF-51 → ENF-53** |
| 9 | Vue de **déploiement** — postes, Podman, réseau | tous | RQ-01, H1, H3 |
| 10 | **Journalisation** — rédaction complète (ENF-55 → ENF-58) | M1 → M4 | **ENF-40, ENF-55 → ENF-58** |
| 11 | **Comportement à la saturation** — rédaction complète (ENF-54), **dimensionnement par la loi de Little** (§11.13), **invariants et runbook** (§11.14) | M1, M2 | **ENF-18, ENF-54** |
| 12 | **Sécurité, conformité et licences** — ce qui évite les ennuis | tous | ENF-44 → ENF-47 |
| 13 | Qualité, tests, intégration continue | tous | ENF-36, ENF-49 |
| 14 | **Traçabilité** exigence → section → test | jury, équipe | toutes |
| 15 | Décisions d'ingénierie (ADR) et paramètres retenus | jury | — |
| 16 | Annexes — exemples, configurations, checklists | tous | — |
| 17 | Sources | jury | — |

---

## 1. Objet, périmètre et conventions

### 1.1. Objet

Ce document est la **spécification technique** du projet AnimeTCG : il décrit **comment** le système est construit, alors que le cahier des charges décrit **ce qu'il doit faire**. Il sert trois usages :

1. **Conception** — figer les contrats entre les modules avant d'écrire le code (les quatre membres travaillent en parallèle : sans contrats écrits, l'intégration se paie en fin de phase).
2. **Défense technique** — répondre, preuve à l'appui, aux questions « pourquoi ce choix ? », « pourquoi cette valeur ? », « que se passe-t-il si… ? ».
3. **Exploitation** — permettre à un tiers (encadrant, jury, futur mainteneur) de **lancer, surveiller et diagnostiquer** la plateforme sans avoir écrit une ligne de son code.

### 1.2. Périmètre

| Dans le périmètre | Hors du périmètre (et pourquoi) |
|---|---|
| Serveur C **mono-processus multitâche**, sockets, 100 clients simultanés (ENF-54) | ~~Authentification des cartes~~ — **RÉTABLIE le 22/09/2026** : le sujet officiel 2026-2027 l'exige (CU-20, §12.1) — voir §4.2, §5.2 et §14. Restent hors périmètre : comptes à mot de passe, fédération d'identité, vérification d'identité réelle |
| Client **Java + JavaFX** en architecture **MVC** (ENF-50) | Interface web, client mobile, application distante hors réseau local |
| **Protocole applicatif propre** au projet (`docs/protocole-applicatif.md` v1.0) | Tout autre canal d'échange de données fonctionnelles (le socket est l'unique canal, §3.2 du CDC) |
| **Chaîne de blocs** locale : SHA-256, preuve de travail à difficulté calibrée | Consensus distribué, réseau pair-à-pair, monnaie réelle, minage rémunéré |
| **PostgreSQL** local comme sauvegarde du contexte | Réplication, haute disponibilité, sauvegardes chiffrées hors site |
| **Journalisation** des deux composants, format unique, centralisation | Agrégateur industriel (Graylog/GELF, Fluentd) — 🎁 hors périmètre S3, §5.8.3 du CDC |
| Tests automatisés et pipeline d'intégration continue (ENF-36, ENF-49) | Audit de sécurité externe, test d'intrusion, certification |

### 1.3. Conventions de lecture

- **`ENF-xx`** renvoie à une exigence non fonctionnelle du cahier des charges ; **`EF-xx`** à une exigence fonctionnelle ; **`DEC-xx`** à une décision consignée ; **`REC-xx`** à un critère de recette ; **`RQ-xx`** à un risque. La numérotation est **stable** : un élément retiré est barré, jamais renuméroté.
- **✅** : le comportement décrit est implémenté (ou la décision est prise et écrite) ; **🔧** : décidé, en cours d'implémentation ; **⚠️** : point à surveiller ; **🎁** : hors périmètre, mentionné pour la soutenance.
- Les **blocs de code** sont normatifs quand ils portent la mention *« contrat »* (signatures, formats, grammaires) et illustratifs sinon.
- Les **valeurs numériques** sont toutes justifiées : soit par une mesure, soit par une source citée au §17, soit par décision consignée (`DEC-xx`). Une valeur dont la justification ne tient pas doit être signalée en revue.
- Les sections **10 et 11** sont des **spécifications détaillées** : elles ont été rédigées parce que le cahier des charges les avait identifiées comme « décidées mais à rédiger ». Ce sont les deux sujets où une erreur de conception coûte cher (perte de journaux en démonstration, effondrement sous charge).

---

## 2. Adossement normatif — quelle norme cadre quoi

Le projet n'est pas certifié (ce n'est pas l'objet d'une SAÉ), mais **chaque choix structurant s'appuie sur une référence reconnue** plutôt que sur une intuition d'équipe. C'est ce tableau qui répond à la question « et si on vous demande pourquoi ? ».

| Référence | Ce qu'elle apporte | Où elle est appliquée |
|---|---|---|
| **IEEE 1016-2009** — *Software Design Description* | L'organisation d'un document de conception **par points de vue** (`context`, `composition`, `logical`, `information`, `interface`, `process`, `deployment`), chaque vue répondant à des préoccupations identifiées | **Structure même de ce document** (§3 à §9) |
| **ISO/IEC/IEEE 42010** — description d'architecture | Les notions de **partie prenante**, **préoccupation**, **point de vue**, **vue** ; traçabilité des décisions | §3, §14, §15 |
| **ISO/IEC 25010** — modèle de qualité | Les caractéristiques à traiter explicitement : adéquation fonctionnelle, performance, fiabilité, **sécurité**, maintenabilité, portabilité | §12 et §13, tableau de traçabilité §14 |
| **ISO/IEC 9075** (langage SQL) | Définition normative des **contraintes de table** (`PRIMARY KEY`, `UNIQUE`, `FOREIGN KEY`, `CHECK`) | **DOC-07** : `sql/schema.sql`, discussion en §6.2 |
| **ISO/IEC 27001:2022** — annexe A **A.8.15** (journalisation), **A.8.16** (surveillance), **A.8.17** (synchronisation des horloges), **A.5.33** (protection des enregistrements) | Ce qu'une organisation doit produire, protéger, conserver et **relire** ; l'obligation d'horloges synchronisées pour que les journaux soient corrélables | **§10** (journalisation), §6.4 (horodatage) |
| **ISO/IEC 27002:2022** — 8.15 / 8.16 | Contenu attendu d'une politique de journalisation : quels événements, comment les protéger, combien de temps, qui les relit | §10.1, §10.8 |
| **NIST SP 800-92** — *Guide to Computer Security Log Management* | Centralisation, **rotation**, protection contre l'altération, importance de la revue ; les fourchettes de rétention et de rotation | §10.8, §10.9 |
| **OWASP ASVS 4.0 — chapitre V7** (*Error Handling and Logging*) | Critères vérifiables : ne pas journaliser d'identifiants (7.1.1), journaliser les événements de sécurité (7.1.3), **encoder pour empêcher l'injection de journal** (7.3.1), protéger les journaux (7.3.3), **horloges synchronisées et UTC** (7.3.4), message d'erreur générique + identifiant exploitable (7.4.1) | §10.3 → §10.7, §7.4 |
| **OWASP Logging Cheat Sheet** | Les quatre dimensions d'une entrée (« quand, qui, quoi, où »), la liste des **données à ne jamais écrire**, les techniques de masquage, la revue régulière des journaux | §10.3, §10.7 |
| **CWE-117** — *Improper Output Neutralization for Logs* (injection de journal) | Un message contenant un retour à la ligne peut **fabriquer une fausse ligne** de journal, y compris dans syslog | §10.6 (règles d'assainissement, test dédié) |
| **CWE-532** — *Insertion of Sensitive Information into Log File* | Les journaux sont **moins protégés** que la base : ce qu'on y écrit peut fuir par un simple `cat` | §10.7 (liste d'interdits) |
| **CWE-400 / CWE-770** — épuisement de ressources, allocation sans limite | Une file non bornée, un tampon sans plafond, un fil par connexion : trois façons de faire tomber un serveur | **§11** (file bornée), §5.2 (tampon de trame), §10.10 (file de journalisation bornée) |
| **CWE-674** — récursion non contrôlée | Un document JSON profondément imbriqué peut épuiser la pile du processus (des vulnérabilités récentes de parseurs C reposent exactement là-dessus) | §5.2 (limite de profondeur du parseur), §11.7 |
| **CWE-89 / CWE-20** — injection SQL, validation d'entrée insuffisante | Requêtes **paramétrées** uniquement ; toute entrée client validée **côté serveur** | §12.2, ENF-44 → ENF-46 |
| **RFC 5424** (syslog), **RFC 3339 / ISO 8601** (horodatage), **RFC 8259 / RFC 7493** (JSON, JSONI) | Formats d'échange et d'horodatage standards — pas de format maison quand un standard existe | §7.1, **§10.3**, §10.9 |
| **RGPD** art. 5.1.c (minimisation), 5.1.e (limitation de conservation), art. 32 (sécurité du traitement) + **recommandation CNIL** sur la journalisation (délibération 2021-122) | Ce qu'on a le droit d'écrire dans un journal, combien de temps, et à quelles conditions | **§12.4** |
| **SPDX** (identifiants de licence) | Nommer les licences sans ambiguïté et vérifier leur compatibilité | **§12.5** |
| **Google SRE** — *Handling Overload* et *Addressing Cascading Failures* | File **bornée**, file d'attente courte, **rejet précoce plutôt que latence longue**, budgets de réessai, backoff avec jitter, dégradation gracieuse | **§11.5, §11.7** |

> **Ce qu'on ne fait pas.** Aucune de ces références n'est citée « pour faire sérieux » : chacune a modifié une décision concrète. Le test simple, en relecture : *si on retire cette référence, quelle phrase de ce document devient fausse ?* Les références dont aucune phrase ne dépend n'ont pas leur place ici.

---

## 3. Vue contextuelle

### 3.1. Acteurs et systèmes

| Élément | Nature | Interaction | Points d'attention |
|---|---|---|---|
| **Joueur (utilisateur du client)** | Acteur humain | Saisit la connexion, crée des cartes, recommande, lance des battles | Aucun compte à mot de passe : le pseudo **n'est pas** une identité vérifiée (hors périmètre) |
| **Propriétaire courant d'une carte** | Rôle joué par un acteur | Reçoit les revendications, accepte ou refuse | Reçoit le **seul événement ciblé** du protocole (`claim.received`) |
| **Revendiquant** | Rôle joué par un acteur | Dépose une revendication sur une carte | Le **silence de 30 jours** du propriétaire vaut accord (DEC-11, §4.7.2 du CDC) |
| **Jury / encadrant** | Acteur humain | Observe la démonstration, consulte les journaux | La journalisation est **la** preuve de fonctionnement en soutenance (§10) |
| **Serveur AnimeTCG** (C) | Composant du système | Accepte les connexions, applique les règles, écrit en base | **Seul composant qui parle à la base et à la chaîne** (ENF-05, ENF-27) |
| **Clients AnimeTCG** (Java) | Composant du système | Jusqu'à 100 simultanés, au-delà : file d'attente | Un client lent **ne doit jamais** ralentir le serveur (§11.6) |
| **PostgreSQL local** | Système externe (SGBD) | Persistance de la chaîne et des projections | Défaillance = service dégradé, **pas** arrêt du serveur (§8.4) |
| **Système de journaux** (fichiers, `journald`/`rsyslog`) | Système externe (exploitation) | Reçoit les lignes des deux composants | **Ne transporte aucune donnée métier** (§10.9) |
| **Bibliothèques** (zlog, jansson, libpq, SLF4J/Logback, JavaFX — **sans libuv, écartée le 22/09/2026**) | Dépendances | Liées au code | Licences et compatibilité en **§12.5** |

**Aucun service distant n'est appelé** : la plateforme fonctionne hors ligne, sur le réseau local de la salle. C'est une contrainte du sujet (et une bonne nouvelle : pas de dépendance à un tiers le jour de la démonstration).

### 3.2. Frontière de confiance

| Zone | Ce qu'on y suppose | Conséquence de conception |
|---|---|---|
| **Le client** | **Non fiable** : il peut être modifié, rejouer des messages, envoyer n'importe quoi | Toute entrée est validée côté serveur (ENF-44) ; le client ne fournit **jamais** une valeur calculable (ENF-45) ; les actions inconnues sont refusées (`-32601`) |
| **Le canal (socket TCP local)** | **Écoutable et modifiable** par quiconque est sur le poste, mais **pas d'adversaire distant** en phase de démonstration | Pas de chiffrement (décision Q6.1, option A) — **assumé et documenté** : le canal ne circule pas hors du réseau de l'IUT |
| **Le serveur** | Zone de confiance | Unique écrivain de la base (ENF-05) ; sérialise les actions ; recalcule toute vérité métier |
| **PostgreSQL** | Zone de confiance, accès **local uniquement**, compte dédié au projet | Contraintes déclaratives en seconde ligne de défense (§6.2) ; l'altération reste **détectable** (ENF-47) |

> ⚠️ **Ce que cette frontière implique en soutenance.** Si le jury demande « et si je modifie le client pour augmenter ma valeur ? », la réponse tient en une phrase : *la valeur est recalculée par le serveur à partir des recommandations et des victoires stockées ; le client ne peut pas l'influencer, seulement déclencher une action dont la validité est revérifiée* (ENF-45).

---

## 4. Vue de composition

### 4.1. Arborescence du dépôt (validée, Q9.2)

```
sae-301/
├── server/                  # code C du serveur + Makefile
│   ├── net/                 # sockets, acceptation, diffusion, file d'attente
│   ├── protocol/            # trames, JSON, dispatch des actions
│   ├── business/            # règles métier (§4 du cahier des charges)
│   ├── chain/               # chaîne de blocs, SHA-256, preuve de travail
│   ├── persistence/         # libpq, sauvegarde, restauration
│   └── util/                # journalisation, horloge, helpers
├── client/                  # Java + JavaFX + Maven → MVC (ENF-50)
│   └── src/main/java/.../{model, view, controller, net}/
├── tests/                   # tests unitaires, d'intégration, de charge, client de test
├── sql/                     # schéma et scripts (DOC-07)
├── scripts/                 # lancement, journaux, sauvegarde, jeu de démonstration
└── docs/                    # cahier des charges, protocole, spécification technique, UML
```

> **Pas de dossier `bot/`** : le « bot » de test est un **client de test** rangé dans `tests/` (DEC-14). Il parle le protocole comme un vrai client — il sert à la démonstration de la charge (§11.10), pas à simuler des joueurs dans l'application.

### 4.2. Modules du serveur — rôle et frontière

| Module | Responsabilité (une seule) | Ne fait **pas** | Dépend de |
|---|---|---|---|
| `net` | Accepter, lire, écrire, **borner**, fermer ; tenir la **file d'attente d'admission** | Aucune règle métier, aucune lecture de base | `util`, `protocol` (pour délimiter) |
| `protocol` | Délimiter les trames, décoder/encoder le JSON, **valider la forme** (types, bornes, listes fermées) | Aucune règle métier, aucun accès base | `util` |
| `business` | Appliquer les règles (§4 du CDC) et décider de l'issue d'une action | Aucune E/S : ni socket, ni fichier, ni base (testable en pur) | `chain`, `persistence` *(via interfaces)* |
| `chain` | Calculer SHA-256, miner, **vérifier** la chaîne | Aucun accès base (on lui **donne** les données) | `util` |
| `persistence` | Ouvrir la connexion, exécuter les **requêtes paramétrées**, reconstruire le contexte | Aucune décision métier, aucune connaissance du protocole | `util` |
| `util` | Journalisation, horloge UTC/monotone, files bornées, allocation contrôlée | Rien de spécifique au métier | — |

**Règle de dépendance** (vérifiable par relecture du `Makefile` et des `#include`) : `net → protocol → business → chain/persistence → util`. **Aucun chemin inverse.** Un `business` qui inclut un en-tête de `net` est un défaut de conception, pas un détail.

### 4.3. Modules du client

Le détail est dans le diagramme de classes **`docs/uml/08-classes-client.puml`** (DOC-03) : `view` (JavaFX), `controller`, `model`, `net`. La règle est la même qu'au serveur, dans l'autre sens : **la vue ne connaît rien du réseau** ; `ServerConnection` est le seul point de contact avec la socket (ENF-50, REC-14).

---
### 4.4. Vue en couches — la règle de dépendance

Les cinq couches du serveur et les quatre paquets du client sont **ordonnés** : une couche ne connaît que celles situées **sous** elle. C'est la règle unique qui rend le reste vérifiable — « dépendre vers le bas, jamais vers le haut ». Elle vient de l'architecture en couches classique (chaque couche ne parle qu'à sa voisine inférieure), et l'**inversion de dépendance** est appliquée là où elle rapporte réellement : le métier ne connaît ni la socket ni PostgreSQL, seulement des **interfaces qu'il possède**.

```
        ┌─────────────── CLIENT (Java / JavaFX) ───────────────┐
        │  view  →  controller  →  model  →  net (socket)      │
        └──────────────────────────────────────────────────────┘
                              │  protocole applicatif (v1.0)
                              ▼
        ┌─────────────── SERVEUR (C) ──────────────────────────┐
        │  ① net        sockets, admission, file d'attente     │
        │        ↓                                             │
        │  ② protocol   trames, JSON, validation de forme       │
        │        ↓                                             │
        │  ③ business   règles métier, décisions (PUR, sans E/S)│
        │      ↙     ↘                                         │
        │  ④ chain   persistence    ④ (implémentations)         │
        │  (preuve de travail, SHA-256) (SQL paramétré, libpq)  │
        │        ↘     ↙                                        │
        │  ⑤ util       journal, horloges, files bornées        │
        └──────────────────────────────────────────────────────┘
```

| Couche | Responsabilité unique | Peut importer | N'importe **jamais** | Testable sans infrastructure |
|---|---|---|---|---|
| ① `net` | Accepter, lire, écrire, **borner**, fermer ; file d'admission | `protocol`, `util` | `business`, `chain`, `persistence` | Non (mais fake socket possible) |
| ② `protocol` | Délimiter, décoder/encoder, valider **la forme** | `util` | `business` *(il lui transmet un DTO déjà typé)*, `net`, `persistence` | **Oui** |
| ③ `business` | Appliquer les règles et décider de l'issue | `util` + **interfaces** `chain_api.h`, `store_api.h` | `net`, `protocol`, `persistence`, `chain` *(concrètement)* | **Oui** |
| ④ `chain` | SHA-256, minage, vérification de chaîne | `util` | tout le reste | **Oui** |
| ④ `persistence` | SQL paramétré, transactions, reconstruction | `util` | `business`, `net`, `protocol` | Oui (PostgreSQL local) |
| ⑤ `util` | Journal, horloges, files bornées, allocation | *rien* (hors libc) | tout le reste | **Oui** |

**La matrice des dépendances** (ce qui est autorisé ↔ ce qui est un défaut de conception) :

| Depuis ↓ / Vers → | `net` | `protocol` | `business` | `chain` | `persistence` | `util` |
|---|---|---|---|---|---|---|
| **`net`** | — | ✅ | ❌ | ❌ | ❌ | ✅ |
| **`protocol`** | ❌ | — | ❌ | ❌ | ❌ | ✅ |
| **`business`** | ❌ | ❌ | — | ✅ *(via `chain_api.h`)* | ✅ *(via `store_api.h`)* | ✅ |
| **`chain`** | ❌ | ❌ | ❌ | — | ❌ | ✅ |
| **`persistence`** | ❌ | ❌ | ❌ | ❌ | — | ✅ |
| **`util`** | ❌ | ❌ | ❌ | ❌ | ❌ | — |

**L'inversion de dépendance, concrètement** — c'est le point qui rend ENF-36 tenable (un test par fonction, sans monter une base ni un serveur) :

```c
/* business/chain_api.h — l'INTERFACE appartient à business, pas à chain */
typedef struct {
    /* Mines one block. Returns 0 on success. Bounded by design (§8.9). */
    int (*mine)(const uint8_t *header, size_t len, uint16_t difficulty,
                uint64_t *nonce_out, uint8_t hash_out[32]);
    /* Verifies one block against its OWN difficulty (ENF-52). */
    int (*verify)(const uint8_t *header, size_t len, uint16_t difficulty,
                  const uint8_t hash[32]);
} chain_api_t;

/* business/store_api.h — l'INTERFACE appartient à business */
typedef struct {
    int (*append_block)(const block_t *b, const proj_t *p);  /* UNE transaction */
    int (*load_context)(long user_id, ctx_out_t *out);
} store_api_t;

/* Les implémentations concrètes (chain/, persistence/) DÉPENDENT de ces en-têtes.
   Le câblage se fait au démarrage, en un seul endroit : server/main.c */
```

Conséquence directe : dans les tests unitaires, `business` reçoit des **doublures** (`fake_chain`, `fake_store` en mémoire) et les règles métier se testent en quelques microsecondes, sans PostgreSQL ni socket.

> **Pourquoi l'architecture en couches plutôt qu'une architecture hexagonale complète ?** Les quatre architectures classiques (couches, hexagonale, oignon, *clean*) disent la même chose avec plus ou moins de cérémonie ; la littérature est explicite : *« get your layers and namespaces clean, and the jump to one of these is a refactor, not a rewrite »*. Avec **4 développeurs** et un sujet qui impose déjà la structure en deux langages, une architecture hexagonale complète (ports, adaptateurs, injection partout) ajouterait de l'indirection **sans** bénéfice mesurable. On garde donc **les couches strictes** et on n'inverse les dépendances **que là où c'est utile** : `business` vis-à-vis de `chain` et `persistence`. C'est le choix que la littérature recommande pour un projet de cette taille, et il reste réversible.

### 4.5. Squelette de fichiers

Le squelette est **la forme exécutable de la vue en couches** : l'arborescence dit à elle seule où chaque chose a le droit de vivre. Fichiers créés à la tâche 13 de la checklist.

```
sae-301/
├── server/
│   ├── main.c                     # câblage : config, journal, couches, boucle
│   ├── Makefile                   # -Wall -Wextra -Werror, cible test, cible coverage
│   ├── net/
│   │   ├── listener.c/.h          # socket d'écoute, accept(), backlog
│   │   ├── admission.c/.h         # file FIFO bornée (ENF-54) — §11
│   │   ├── conn.c/.h              # connexion : état, tampons, envoi borné
│   │   └── broadcast.c/.h         # diffusion des événements, ciblage claim.received
│   ├── protocol/
│   │   ├── frame.c/.h             # [longueur 4 o BE][JSON], plafond 64 Kio (ENF-23)
│   │   ├── json_io.c/.h           # façade jansson : décodage borné, profondeur 32
│   │   ├── validate.c/.h          # validation de FORME (types, bornes, listes) — §7.4
│   │   ├── actions.c/.h           # table de dispatch des 23 actions
│   │   └── errors.c/.h            # catalogue des 30 codes, message FR + retryable
│   ├── business/
│   │   ├── card.c/.h              # création, inactivité, archivage, limites (≤ 100 actives)
│   │   ├── recommendation.c/.h    # poids 1.0/0.5, répudiation, unicité active
│   │   ├── battle.c/.h            # proximité titre/format, rang voisin, vote 5 min définitif, quorum, clôture
│   │   ├── rights.c/.h            # légitimation (likes), revendication, silence 30 j
│   │   ├── authentication.c/.h    # authentification d'une carte (CU-20, DEC-24)
│   │   ├── value.c/.h             # V = Σ poids + 3 × victoires
│   │   ├── context.c/.h           # constitution du contexte d'un client (CU-7)
│   │   ├── chain_api.h            # interface possédée par business (mine/verify)
│   │   └── store_api.h            # interface possédée par business (append/load)
│   ├── chain/
│   │   ├── sha256.c/.h            # implémentation conforme FIPS 180-4
│   │   ├── header.c/.h            # en-tête canonique 96 octets (ENF-52) — §6.5
│   │   ├── pow.c/.h               # minage, nonce 64 bits, budget d'essais (§8.9)
│   │   ├── calibrate.c/.h         # banc de mesure + décision de difficulté (§8.6-8.8)
│   │   └── verify.c/.h            # vérification par bloc, sa propre difficulté
│   ├── persistence/
│   │   ├── db.c/.h                # libpq : connexion, requêtes paramétrées (ENF-46)
│   │   ├── blocks.c/.h            # append bloc + projection en UNE transaction
│   │   ├── projections.c/.h       # lectures indexées (cards, recos, battles, claims)
│   │   └── restore.c/.h           # reconstruction du contexte (REC-02 ≤ 10 s)
│   └── util/
│       ├── log.c/.h               # façade zlog (repli rxi/log.c) — §10
│       ├── sanitize.c/.h          # anti-injection de journal (CWE-117) — §10.6
│       ├── clock.c/.h             # UTC + monotone (§6.4)
│       └── ring.c/.h              # files bornées réutilisables (admission, journal)
├── client/
│   └── src/main/java/fr/iut/animetcg/
│       ├── Launcher.java
│       ├── model/    SessionModel, Card, Recommendation, Battle, Vote, User
│       ├── view/     MainWindow, CardsTab, BattlesTab, RightsTab, … (DOC-03)
│       ├── controller/ Connection, Card, Recommendation, Battle, Rights
│       └── net/      ServerConnection, ProtocolCodec, RequestCorrelator,
│                     EventDispatcher, ErrorCatalog
├── tests/
│   ├── unit/          → ≥ 30 tests C, ≥ 15 tests Java (ENF-36)
│   ├── integration/   → scénarios serveur + PostgreSQL + client de test
│   ├── load/          → queue_test.sh : 7 phases du §11.10 (REC-03)
│   ├── logs/          → 10 tests de journalisation du §10.12 (REC-17)
│   └── client_de_test/  # le « bot » de charge est ICI, pas à la racine (DEC-14)
├── sql/               # schema.sql (DOC-07, v1.5) + restauration
├── scripts/           # logs.sh, logs-purge.sh, check-layers.sh, backup.sh, restore.sh
├── third_party/       # sources des libs vendues (repli rxi/log.c), THIRD-PARTY.md
└── docs/              # cahier des charges, protocole, spec technique, UML, exports
```

**Les en-têtes de circulation entre couches** — chaque couche n'expose qu'un petit nombre de symboles, et rien d'autre n'est visible :

| En-tête | Ce qu'il expose | Ne doit **jamais** exposer |
|---|---|---|
| `protocol/frame.h` | `frame_read`, `frame_write`, `FRAME_MAX_BYTES` | une structure d'action métier |
| `protocol/actions.h` | `actions_dispatch(msg, out)` — un seul point d'entrée | les détails des règles |
| `business/*.h` | des fonctions pures `*_validate`, `*_apply` | `libpq`, `sys/socket.h` |
| `chain/header.h` | `header_build(...) → 96 octets` | `block_t` de la base |
| `persistence/db.h` | `db_open`, `db_append_block`, `db_load_context` | un type JSON du protocole |
| `util/log.h` | `log_init`, `log_close`, `log_set_context` | zlog directement |

### 4.6. Faire respecter la règle — contrôle automatique et métriques

Une règle d'architecture non contrôlée est une intention, pas une contrainte. Deux mécanismes, tous les deux **automatiques** :

**① Un contrôle statique des inclusions** — `scripts/check-layers.sh`, exécuté en **étape 5 du pipeline CI** (§13.4) :

```bash
#!/usr/bin/env bash
# Fails if a layer includes a header from a layer it is not allowed to know.
set -u
fail=0
check() { # $1 = dossier source, $2 = motifs autorisés
  while IFS= read -r f; do
    while IFS= read -r inc; do
      grep -Eq "$2" <<<"$inc" || { echo "VIOLATION $f : $inc"; fail=1; }
    done < <(grep -E '^#include "' "$f" || true)
  done < <(find "server/$1" -name '*.c' 2>/dev/null)
}
check net         'net/|protocol/|util/'
check protocol    'protocol/|util/'
check business    'business/|util/'
check chain       'chain/|util/'
check persistence 'persistence/|util/'
exit $fail
```

Trois propriétés visées, toutes vérifiables : **aucun cycle** entre couches (le graphe des dépendances est un graphe orienté acyclique par construction), **pas de dépendance montante** (aucun `business` → `net`), et **un fan-out faible** (une couche n'appelle qu'un petit nombre de couches).

**② Des métriques de couplage**, relevées une fois par phase et notées dans le journal du projet : pour chaque module, **Ca** (couplage afférent : combien de modules m'appellent), **Ce** (couplage efférent : combien j'appelle) et l'**instabilité** `I = Ce / (Ca + Ce)`.

| Module | Ca | Ce | I | Lecture |
|---|---|---|---|---|
| `util` | 5 | 0 | **0,00** | Le plus stable : tout le monde en dépend, il ne dépend de rien → **on ne le casse pas** |
| `chain` | 1 | 1 | 0,50 | Stable et testable isolément |
| `protocol` | 1 | 2 | 0,67 | Au-dessus de `net`, sous `business` |
| `business` | 1 | 5 | 0,83 | Instable **volontairement** : c'est là que les règles changent |
| `net` | 0 | 3 | **1,00** | Le plus instable : il s'adapte au monde extérieur |

> **La règle qui se lit dans ce tableau** (principe des dépendances stables) : *dépendre dans le sens de l'instabilité décroissante*. Le code qui **change souvent** (`net`, `business`) dépend du code qui **change rarement** (`util`, `chain`) — jamais l'inverse. Un `util` qui se mettrait à appeler `business` ferait exploser son `I` et propagerait chaque évolution à tout le programme.

**Ce qu'on ne fait pas, et pourquoi** : pas de ports/adaptateurs généralisés (indirection inutile à cette échelle), pas de microservices ni de bus d'événements (le sujet impose **un** serveur), pas de cadre d'injection de dépendances (le câblage tient en une page de `main.c`). Ces choix sont **réversibles** : la règle de dépendance est déjà respectée, donc passer à une architecture hexagonale serait un remaniement, pas une réécriture.

---

## 5. Vue logique — contrats d'interface

### 5.1. Règles de code applicables partout (ENF-33 → ENF-36)

| Règle | Formulation opérationnelle | Contrôle |
|---|---|---|
| **Taille** | Aucune fonction > **200 lignes** hors commentaires et lignes vides (ENF-33) | script `scripts/check-style.sh` + relecture croisée |
| **Commentaires** | **En anglais** (ENF-34), en tête de fonction : rôle, paramètres, retour, effets de bord (ENF-35) | relecture croisée avant fusion |
| **Tests** | **Toute nouvelle fonction arrive avec son test** (ENF-36) — c'est la définition de fini d'un lot | pipeline CI (ENF-49) |
| **Retour d'erreur** | `0` = succès, valeur **négative** = code d'erreur du catalogue (§8 du protocole). Jamais de `-1` muet | revue |
| **Mémoire** | Celui qui alloue libère ; toute allocation a une taille **bornée** ; aucune chaîne de format non constante (`snprintf` seulement) | `-Wall -Wextra -Werror`, ASan en développement |
| **Pas d'état global mutable** | Un contexte explicite (`srv_ctx_t`) passé en paramètre | revue |

### 5.2. Contrats des modules serveur (extraits normatifs)

```c
/* ---------- util/log.h — journalisation (voir §10) ---------- */
void log_init(const char *conf_path);        /* charge zlog.conf ; valide via zlog-chk-conf */
void log_close(void);
void log_set_context(long client_id, const char *request_id); /* MDC : c= et r= */

/* ---------- util/clock.h — horloges (§6.4) ---------- */
int64_t now_utc_ms(void);      /* horodatage des journaux : ISO 8601 UTC, milliseconde */
int64_t now_monotonic_ms(void);/* mesure de durée : insensible aux sauts d'horloge système */

/* ---------- protocol/frame.h — délimitation (protocole §3) ---------- */
#define FRAME_MAX_BYTES (64 * 1024)
#define JSON_MAX_DEPTH   32            /* anti CWE-674 : bien au-delà de nos messages */
/* Reads exactly len bytes. Returns 0 on success, negative error code otherwise. */
int frame_read(int fd, uint8_t **out_buf, size_t *out_len);

/* ---------- protocol/decode.h — décodage JSON (jansson) ---------- */
/* Parses a validated frame into a message. Rejects depth > JSON_MAX_DEPTH. */
int msg_decode(const uint8_t *buf, size_t len, msg_t *out);

/* ---------- business/cards.h — règles métier, sans E/S ---------- */
/* Pure function: validates and computes the new card state. No I/O. */
int card_create_validate(const card_dto_t *in, const srv_ctx_t *ctx, err_t *err);

/* ---------- chain/pow.h — preuve de travail (ENF-52) ---------- */
/* Mines a block with the current difficulty (3 or 4). Bounded by design. */
int chain_mine(chain_t *c, const uint8_t *payload, size_t len, block_t *out);

/* ---------- persistence/db.h — requêtes paramétrées uniquement (ENF-46) ---------- */
int db_open(const char *conninfo);
int db_append_block(const block_t *b, const proj_t *p); /* bloc + projection : UNE transaction */
int db_load_context(long user_id, ctx_out_t *out);      /* REC-02 : ≤ 10 s pour 1000 blocs */
```

**Trois choix à retenir de ces signatures** :

1. **`frame_read` lit exactement ce que l'en-tête annonce** — jamais de lecture « au fil de l'eau » jusqu'à un délimiteur : c'est ce qui rend l'attaque par trame longue impossible (le plafond de 64 Kio est appliqué **avant** toute allocation, protocole §3.1).
2. **`chain_mine` ne touche pas la base**, `business` ne touche ni socket ni base : les règles métier sont **testables sans infrastructure**, ce qui est la condition pour tenir ENF-36 (un test par fonction) sans exploser le temps de CI.
3. **`db_append_block` prend le bloc *et* la projection** : l'atomicité « bloc + projection » est inscrite dans la signature, pas laissée à la discipline de l'appelant (§6.2).

### 5.3. Bibliothèque JSON du serveur — décision et justification

> Le protocole laissait ce point **ouvert** (§15 de `docs/protocole-applicatif.md`). Il est tranché ici.

| Candidat | Licence | Ce qui compte pour nous | Verdict |
|---|---|---|---|
| **jansson** | MIT | API stable (objets/tableaux), **limite de profondeur documentée et configurable** (`JSON_PARSER_MAX_DEPTH`, 2048 par défaut → on la fixe à **32**), conformité RFC 8259, sérialisation `JSON_COMPACT` | ✅ **retenu** |
| **cJSON** | MIT | 2 fichiers, très répandu, mais sa limite de profondeur (1000) a été au cœur de **plusieurs vulnérabilités 2026** (dépassement de pile par document imbriqué, complexité exponentielle de la comparaison d'objets) ; les utilitaires de comparaison et de *patch* sont hors de notre besoin | 🔧 **repli**, uniquement avec `CJSON_NESTING_LIMIT=64` et sans jamais appeler les utilitaires visés par les avis |
| **yyjson** | MIT | Très rapide, mais surface plus récente et surdimensionné : nos trames font quelques centaines d'octets | ❌ écarté (complexité inutile) |

**Ce que la décision implique** :

- `JSON_MAX_DEPTH 32` **côté application**, doublé de `JSON_PARSER_MAX_DEPTH 32` **dans la configuration de la bibliothèque** : ceinture et bretelles contre CWE-674.
- Les nombres sont lus **comme entiers ou décimaux selon le champ**, jamais « au mieux » : pas de conversion silencieuse (RFC 8259 : pas de `NaN`, pas d'`Infinity`, pas de clé dupliquée).
- UTF-8 **valide exigé** (RFC 8259) ; une séquence invalide fait échouer le décodage avec `-32700`, jamais un remplacement silencieux.
- Le **repli cJSON** est documenté comme l'est `rxi/log.c` pour la journalisation : même esprit — un plan B écrit, pas improvisé le jour où l'installation pose problème.

#### 5.3.1. Bibliothèque de hachage — **OpenSSL** (et non une implémentation maison)

Le sujet officiel 2026-2027 (§10) **autorise nommément OpenSSL** pour le hachage : `#include <openssl/sha.h>` et l'option `-lcrypto`.

| Avant (v0.1 → v0.3) | Décision du 22/09/2026 | Pourquoi |
|---|---|---|
| `chain/sha256.c` : implémentation complète écrite par l'équipe (conformité FIPS 180-4 vérifiée par vecteurs dorés) | **`chain/sha256.c` devient une façade de ≈ 30 lignes** autour de `SHA256()` d'OpenSSL, conservant exactement la même signature publique | ① Le sujet **autorise** OpenSSL et **n'autorise que lui** dans cette catégorie : réécrire SHA-256 n'apporte plus aucun point ; ② l'implémentation OpenSSL est **accélérée matériellement** (SHA-NI ≈ 1,8 cycle/octet) — le banc de calibrage du §8.6 mesurera donc le débit réel de la machine ; ③ un code cryptographique de moins à maintenir et à défendre en soutenance |

**Ce qui ne change pas** : les **vecteurs dorés du §6.5.5** restent valables (même algorithme, même sortie), l'**en-tête canonique de 96 octets** ne bouge pas, et la **preuve de travail** est vérifiée par bloc contre sa propre difficulté (ENF-52). La valeur ajoutée de l'équipe se déplace du **calcul** vers la **sérialisation canonique** — c'est là que le jury attend l'argument.

> **En une phrase pour la soutenance** : « OpenSSL calcule le SHA-256 ; ce que nous avons dû concevoir, c'est **ce qu'on lui donne à hacher** — un en-tête canonique de 96 octets et une charge utile canonique RFC 8785 (§6.5). »

#### 5.3.2. Bibliothèques hors liste blanche : déclarées et justifiées (ENF-59)

Le sujet (§10) impose une **liste blanche** de bibliothèques — OpenSSL, libpq, POSIX, JavaFX, JUnit — et prévoit une **demande** pour toute autre. **Cette demande n'a plus de destinataire** : depuis le **22/09/2026**, l'équipe n'a plus d'encadrant et **décide seule** — le **sujet écrit** reste la seule autorité. L'équipe **retient** les trois dépendances hors liste nécessaires et **assume la décision**, en produisant la **déclaration** ci-dessous — nom, usage, statut, licence (§12.5) et **repli**. C'est ce tableau qui sera vérifié en revue, pas une autorisation.

> ⚠️ **Point d'attention pour le jury** : la liste blanche du §10 n'est pas respectée à la lettre pour **jansson**, **zlog/SLF4J-Logback** et **gcovr/JaCoCo**. Le choix est **assumé et documenté** : chacune a un **repli prêt** (colonne de droite) et une **façade** qui isole l'appel (`json_io.c`, `log.c`, cibles du `Makefile`), de sorte qu'une bascule coûte une zone de code, jamais une réécriture.

| Dépendance | Usage | Statut | Repli si le besoin change |
|---|---|---|---|
| **OpenSSL** | SHA-256 | ✅ autorisée nommément | — |
| **libpq** | PostgreSQL | ✅ autorisée nommément | — |
| **POSIX** (`pthread`, sockets, `time.h`) | fils, réseau, horloge | ✅ autorisées | — |
| **JavaFX / JUnit** | IHM, tests Java | ✅ autorisées | — |
| **jansson** | décodage JSON (C) | ✅ **retenue** — décision d'équipe du 22/09/2026, déclarée | analyseur JSON restreint maison (≈ 300 lignes, format borné) — `json_io.c` isole déjà l'accès |
| **zlog** (C), **SLF4J + Logback** (Java) | journalisation | ✅ **retenues** — décision d'équipe du 22/09/2026, déclarées | `rxi/log.c` (2 fichiers, embarqués) et `java.util.logging` — **le format de ligne ne change pas** (`docs/note-format-ligne.md`) |
| **gcov / gcovr / JaCoCo** | couverture (phase 3) | ✅ **retenus** — décision d'équipe du 22/09/2026 ; `gcov` est fourni avec GCC | comptage manuel des cas couverts (le sujet rappelle qu'**aucun framework de tests C n'est installé**) |

**Pourquoi la façade reste utile même sans autorisation à demander** : l'isolation de ces trois dépendances (`json_io.c`, `log.c`, cibles du `Makefile`) n'est plus une précaution réglementaire, c'est une **règle d'architecture** — remplacer un analyseur JSON, un moteur de journalisation ou un outil de couverture doit coûter **un fichier**, jamais une reprise du métier. C'est l'application directe de la règle de dépendance du §4.4 et l'un des exemples cités au §13.0 (inversion de dépendance).

**Rappel de traçabilité** : la décision est consignée au journal du cahier des charges (v0.21, ⑨) et en `docs/diff-sujet-2026.md`, écart n° 6 — c'est notre réponse si le jury s'appuie sur la lettre du §10 du sujet.

---

## 6. Vue information

### 6.1. Entités et cycle de vie

| Entité | Source de vérité | Projection (lecture rapide) | Invariant majeur |
|---|---|---|---|
| **Action** | Table `blocks` (**append-only**) | — | Une action acceptée = un bloc ; une action refusée = **aucun** bloc |
| **Carte** | `blocks` (`CREATION_CARTE`, `TRANSFERT_*`) | `cards` + `card_genres` + `card_themes` | `creator_id` immuable ; `owner_id` change par revendication ou héritage |
| **Utilisateur** | `blocks` | `users` | Pseudo unique (insensible à la casse) |
| **Recommandation** | `blocks` (`RECOMMANDATION`, `REPUDIATION`) | `recommendations` | **Une seule active** par couple (carte, utilisateur) — index unique partiel |
| **Battle / vote** | `blocks` | `battles` / `battle_votes` | Clôture à **300 s** ou quorum ; **un vote définitif** par (battle, votant) — `1020` en cas de second vote (Q2.4) |
| **Légitimation** | `blocks` | `legitimations` / **`legitimation_likes`** | Un seul porteur légitime actif par carte = **le plus de likes** (Q4.1) |
| **Authentification** *(rétablie le 22/09/2026)* | `blocks` (`AUTHENTIFICATION`, `REVOCATION_AUTH`) | `authentications` | **Une seule active par carte** (index unique partiel) ; **aucun effet sur la valeur** ; la révocation est une action validée (son propre bloc est conservé, §4.7.3) — DEC-24 |
| **Revendication** | `blocks` | `claims` | **Une seule en attente** par carte ; refus = statu quo définitif ; **les droits restent inchangés** après une battle perdue (Q2.6 — l'état `SANS_OBJET` a été retiré le 22/09/2026) |
| **Valeur d'une carte** | Recalculée | `cards.value_active` + `cards.wins` / `cards.rarity_rank` | `V = points de reco × multiplicateur de rang` (fer 0× → platine 2×) ; **jamais** fournie par un client (ENF-45) — Q3.1 |
| **Valeur d'un utilisateur** *(Q3.2)* | Recalculée | agrégat sur `cards` | **Total des votes des cartes possédées** |
| **Like de légitimation** 🆕 | `blocks` | `legitimation_likes` | **Un like par (candidature, utilisateur)** ; le plus liké est porteur légitime (Q4.1) |
| **Amitié** 🆕 | *(hors chaîne — **réglage d'utilisateur**, pas une action métier : aucune carte ne bouge)* | `friendships` | Liste d'amis **symétrique** ; prérequis du partage ciblé (Q5.2) |
| **Partage ciblé** 🆕 | **`blocks`** ✅ *(décision du 22/09/2026 : option A — le partage est inscrit en chaîne comme toute action métier)* | `shares` | **Une seule offre en attente** par (carte, destinataire) ; acceptation = **le demandeur se déplace** ; refus = statu quo |

Le détail (colonnes, types, contraintes, index) est en **DOC-07** (`sql/schema.sql` + `docs/uml/12-modele-relationnel.puml`).

### 6.2. Deux principes qui commandent la persistance

| Principe | Formulation | Conséquence |
|---|---|---|
| **La chaîne est la source de vérité** | `blocks` est **append-only** : jamais d'`UPDATE`, jamais de `DELETE` | L'historique des actions est la sauvegarde ; il est **vérifiable** (§6.3) |
| **Les autres tables sont des projections** | Écrites **dans la même transaction** que le bloc correspondant | Un démarrage ne rejoue pas la chaîne : il lit des tables indexées (**REC-02**, ≤ 10 s pour 1000 blocs) ; une projection se **reconstruit** toujours à partir des blocs |

**Contraintes déclaratives en seconde ligne de défense** — ce que le protocole refuse ne peut pas non plus entrer en base : `difficulty BETWEEN 3 AND 4`, `hash ~ '^[0-9a-f]{64}$'`, année entre 1950 et l'année courante + 1, poids de recommandation ∈ {0,5 ; 1,0}, états en listes fermées. Trois règles « une seule ligne active » sont exprimées en **index uniques partiels** (recommandation active, revendication en attente, porteur légitime) : elles ne coûtent **aucune ligne de code** et tiennent même si un contrôle applicatif est un jour oublié.

### 6.3. Vérification de cohérence (ENF-47, REC-16)

La chaîne se vérifie en parcourant `blocks` dans l'ordre : recalcul du hash de chaque bloc (**difficulté incluse**, ENF-52) et comparaison avec le `previous_hash` du suivant. Trois falsifications sont détectées : modification de la charge utile, modification du nonce, et **modification de la difficulté appliquée** — c'est précisément pour cette dernière que la difficulté est hachée avec le bloc. La démonstration est prévue en phase 4 sur une copie de la base.

### 6.4. Horodatage

| Usage | Source | Format | Pourquoi |
|---|---|---|---|
| **Journaux** | Horloge système UTC | ISO 8601, milliseconde, suffixe `Z` | Corrélable entre postes (**ISO 27001 A.8.17**, **ASVS 7.3.4**) ; triable par un simple `sort` |
| **Affichage IHM** | Horloge locale | Heure locale de l'utilisateur | Lisibilité — l'IHM convertit, le journal ne ment pas |
| **Durées mesurées** (minage, requête, attente) | Horloge **monotone** | Millisecondes entières | Un ajustement NTP ne doit pas produire une durée négative ou absurde |
| **Échéances métier** (30 j, 300 s) | Horodatage UTC stocké en base | `TIMESTAMPTZ` | Comparaison en base, cohérente entre serveur et redémarrages |

> Les deux horloges sont nécessaires et ce n'est pas un détail : **`TIMESTAMPTZ` pour dater** (ce que le jury vérifie), **horloge monotone pour mesurer** (ce que le calibrage de difficulté exige, §5.2.1 du CDC).

---
### 6.5. Sérialisation canonique du bloc et difficulté hachée (ENF-52)

> **C'est le point technique le plus délicat du projet.** Le cahier des charges l'annonce ainsi (§5.2.1) : la difficulté **varie** d'un bloc à l'autre, donc elle doit être **inscrite dans le bloc** et **incluse dans les données hachées**. Sans cela, une chaîne minée à deux difficultés différentes serait invérifiable et une difficulté falsifiée passerait inaperçue.

#### 6.5.1. Le problème, en une phrase

Une empreinte cryptographique porte sur des **octets**, pas sur du sens. Si deux programmes ne produisent pas **exactement les mêmes octets** pour le même bloc, le même bloc donne deux hash différents — et la vérification de la chaîne échoue sur une chaîne parfaitement saine. C'est le piège classique : il ne se voit pas à l'écriture, il se voit le jour de la vérification.

Deux sources d'ambiguïté dans notre cas :

| Source | Ce qui se passe si on l'ignore |
|---|---|
| **Ordre des champs** de l'en-tête | Deux implémentations (serveur C et l'outil de vérification) produisent deux hash différents → faux positif d'altération |
| **Représentation de la charge utile** | `JSONB` **réordonne les clés** et **supprime les espaces** : recalculer le hash depuis la base donnerait un résultat différent du hash d'origine. Sur une chaîne de 1 000 blocs, la vérification échouerait **partout** |

#### 6.5.2. Correspondance avec les données exigées par le sujet

Le sujet officiel liste ce qui **doit** entrer dans le calcul du hash : *« l'identifiant, le timestamp, les données de l'action client, le hash précédent et le nonce »*. Notre en-tête contient **tout cela** — et trois champs de plus, assumés et justifiés :

| Donnée exigée par le sujet | Où elle est dans notre en-tête | Remarque |
|---|---|---|
| **Identifiant** du bloc | `block_index` (4 o) + `version` (4 o) | L'identifiant du bloc **est** son index dans la chaîne ; la `version` dit la forme de l'en-tête |
| **Timestamp** | `timestamp_ms` (8 o) | Millisecondes UTC (sujet : `time.h`) |
| **Données de l'action client** | `action_id` (4 o) + `payload_hash` (32 o) | Le type d'action **et** l'empreinte de la charge utile canonique : c'est ainsi qu'une charge utile de taille variable reste hachée dans un en-tête de taille fixe |
| **Hash précédent** | `prev_hash` (32 o) | 32 octets nuls pour le *genesis* |
| **Nonce** | `nonce` (8 o) | 64 bits, là où le sujet n'impose rien (le nonce 32 bits de Bitcoin s'épuise — §6.5.6) |
| *(en plus)* **difficulté appliquée** | `difficulty` (4 o) | **ENF-52** : elle varie d'un bloc à l'autre, donc elle doit être hachée — la version naïve du projet laisserait falsifier un bloc en abaissant sa difficulté |
| *(en plus)* **version d'en-tête** | `version` (4 o) | Permet de faire évoluer la structure sans casser la vérification des blocs anciens |

> **Le pointeur de chaînage n'est pas haché** : en mémoire, chaque bloc porte un pointeur vers le précédent, et la table PostgreSQL relie les blocs par `block_index` — **aucun de ces liens techniques n'entre dans le hash** (le sujet le précise explicitement : « un pointeur de chaînage est utilisé entre les blocs mais il ne fait pas partie des données utilisées pour calculer le hash »). Seul `prev_hash` fait foi, ce qui rend la vérification **indépendante de la façon dont les blocs sont stockés** : on pourrait changer de SGBD sans invalider la chaîne.

#### 6.5.3. L'en-tête canonique — 96 octets, aucune ambiguïté

Le hash d'un bloc porte sur un **en-tête de taille fixe**, sérialisé en **gros-boutiste** (« big-endian »), sans séparateur, sans texte :

| Décalage | Taille | Champ | Type | Rôle |
|---|---|---|---|---|
| 0 | 4 o | `version` | entier 32 bits non signé | Format d'en-tête (**1**) — permet de faire évoluer la structure |
| 4 | 4 o | `block_index` | entier 32 bits non signé | Position dans la chaîne (0 = genesis) |
| 8 | 8 o | `timestamp_ms` | entier 64 bits signé | Horodatage **figé avant le minage** (millisecondes depuis l'époque Unix, UTC) |
| 16 | 32 o | `prev_hash` | 32 octets bruts | Hash du bloc précédent (**32 zéros** pour le genesis) |
| 48 | 4 o | `action_id` | entier 32 bits non signé | Type d'action (table du protocole) |
| 52 | 4 o | **`difficulty`** | entier 32 bits non signé | **La difficulté appliquée à CE bloc (3 ou 4)** |
| 56 | 8 o | `nonce` | entier 64 bits non signé | Compteur de minage |
| 64 | 32 o | `payload_hash` | 32 octets bruts | `SHA-256` de la charge utile **canonique** |
| **Total** | **96 o** | | | **`hash_bloc = SHA-256(en-tête de 96 octets)`** |

**Ce que cette structure garantit :**

1. **La difficulté est hachée** (ENF-52) : la passer de 4 à 3 invalide le bloc. La preuve de travail est ensuite vérifiée **contre la difficulté inscrite dans le bloc**, jamais contre une difficulté « courante » (ENF-16).
2. **L'horodatage est figé** au début du minage : si le serveur mettait à jour l'heure pendant la recherche du nonce, le hash trouvé ne correspondrait plus à l'en-tête final, et le bloc serait invérifiable.
3. **La taille est constante** : 96 octets, quelle que soit la longueur de la charge utile — d'où un temps de hachage stable, un débit de minage prévisible et un calibrage possible (§8.6).
4. **L'en-tête n'est pas du JSON** : aucune question de virgules, d'espaces ou de clés dupliquées. Le JSON ne sert qu'à la **charge utile**, et il est canonisé.

#### 6.5.4. La charge utile canonique — la colonne `payload_canon`

La base stocke la charge utile **deux fois**, et ce n'est pas un doublon :

| Colonne | Rôle | Pourquoi les deux |
|---|---|---|
| `payload JSONB` | **Interroger** (`payload->>'card_id'`), indexer, requêter | `JSONB` est commode côté SQL… |
| **`payload_canon BYTEA`** | Conserver **les octets exactement hachés** | …mais `JSONB` réordonne les clés et supprime les espaces : impossible de recalculer un hash à partir de lui |

**Chaîne de traitement, côté serveur, avant tout hachage :**

```
charge utile reçue (JSON de la trame)
   │
   ├─ ① validation de forme (§7.4)      → types, bornes, listes fermées
   ├─ ② canonicalisation NFC des chaînes de caractères
   ├─ ③ sérialisation canonique JSON     → RFC 8785 « JCS » :
   │        clés triées, aucun espace, nombres sans notation scientifique,
   │        caractères de contrôle échappés
   ├─ ④ payload_canon = ces octets       → envoyés à PostgreSQL ET hachés
   └─ ⑤ payload (JSONB)                  → la même donnée, pour l'interrogation
```

**Le choix de la référence** : plutôt qu'une canonicalisation « maison » (trier les clés à la main — la façon dont on découvre six mois plus tard que deux implémentations diffèrent sur un nombre flottant), on s'appuie sur le **JSON Canonicalization Scheme de la RFC 8785** : ordre des clés par unités de code UTF-16, nombres au format IEEE 754 sans zéros superflus, aucun espace insignifiant, échappement fixé pour les caractères de contrôle. C'est la forme canonique utilisée pour les signatures et les hachages de transactions — exactement notre cas d'usage.

> ⚠️ **Robustesse** : la bibliothèque JSON (`jansson`, §5.3) ne fait pas JCS à elle seule. La canonicalisation est donc une **fonction dédiée** (`chain/header.c` → `payload_canonicalize()`), testée par des vecteurs (§6.5.5), et **appliquée en un seul endroit** : aucune autre couche ne produit d'octets hachables.

#### 6.5.5. Algorithme de vérification (ENF-47, REC-07, REC-16)

```text
vérifier_chaîne(blocs triés par block_index):
    attendu = 32 octets nuls                        # prev_hash du genesis
    pour chaque bloc b dans l'ordre:
        si b.prev_hash != attendu:
            return ALTÉRÉ(b.block_index, "chaîne rompue")          # cas 2 ci-dessous
        en-tête = header_build(b.version, b.block_index, b.timestamp_ms,
                               b.prev_hash, b.action_id, b.difficulty,
                               b.nonce, sha256(b.payload_canon))
        h = sha256(en-tête)
        si h != b.hash:
            return ALTÉRÉ(b.block_index, "hash recalculé différent")# cas 1 ci-dessous
        si NON(h commence par b.difficulty zéros hexadécimaux):
            return ALTÉRÉ(b.block_index, "preuve de travail insuffisante"
                          + " pour la difficulté déclarée")            # cas 3 ci-dessous
        attendu = b.hash
    return CHAÎNE VALIDE
```

**Les trois falsifications détectées, et le test qui les démontre** (REC-16, phase 2) :

| Cas | Ce que fait le test | Détection | Message |
|---|---|---|---|
| **1 — Donnée métier altérée** | `UPDATE cards SET value_active = 999` puis vérification | La projection ne correspond plus au bloc ; et si l'on modifie le `payload_canon`, le hash ne se recalcule pas | `hash recalculé différent` |
| **2 — Difficulté falsifiée (4 → 3)** | `UPDATE blocks SET difficulty = 3 WHERE block_index = 54` | L'en-tête reconstruit diffère → hash différent **du bloc 54**, puis `prev_hash` différent **au bloc 55** | Double détection : locale **et** en cascade |
| **3 — Preuve de travail trafiquée** | `UPDATE blocks SET hash = '0000…'` (fabriqué) | La difficulté annoncée n'est pas satisfaite par le hash stocké | `preuve de travail insuffisante` |

> **Pourquoi le cas 2 est le plus important** : c'est *exactement* l'attaque que la version naïve du projet laisserait passer. Si la difficulté n'était pas hachée, un utilisateur pourrait **abaisser la difficulté d'un bloc** dans la base sans que rien ne le signale — la chaîne resterait « cohérente » du point de vue des hash. C'est le scénario que l'encadrant cherchera, et c'est celui que le test de la phase 2 démontre.

#### 6.5.6. Vecteurs de test dorés (*golden vectors*)

Un vecteur doré est une entrée fixe dont la sortie attendue est écrite **dans le test**. Il attrape les régressions que les tests « fonctionnels » laissent passer (une refonte de `sha256.c`, un changement d'`endianness`, un `+1` déplacé).

**A. Conformité de l'implémentation SHA-256** (d'après les vecteurs NIST de la FIPS 180-4) :

| Entrée | `SHA-256(entrée)` attendu (hexadécimal) |
|---|---|
| `""` (chaîne vide) | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `"abc"` | `ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad` |

**B. Sérialisation de l'en-tête** — test de non-régression sur les 96 octets : les 96 octets de l'en-tête de test ci-dessous doivent être produits **à l'octet près**.

**C. Minage de bout en bout** — bloc de test, difficulté **3** :

| Champ | Valeur |
|---|---|
| `version` | 1 |
| `block_index` | 118 |
| `timestamp_ms` | 1789931523412 |
| `prev_hash` | 32 octets nuls (chaîne de test) |
| `action_id` | 1 (création de carte) |
| `difficulty` | 3 |
| `payload_canon` | `{"title_romaji":"Cowboy Bebop","episodes":26,"year":1998}` |
| `payload_hash` | `a08c199cc73fd5ad0bb3d7eed343355760d7e587779906311da15671f24557f0` |
| **nonce trouvé** | **13 423** (13 424 tentatives) |
| **`hash` attendu** | `00045bb88bc02a0900cc54b43ef408d9fadb6fc586ee6fac7e4efce715014f36` |

L'en-tête correspondant, en hexadécimal (96 octets) :

```
00000001 00000076 000001a0c03b8d54 0000…0000 00000001 00000003
000000000000346f a08c199cc73fd5ad0bb3d7eed343355760d7e587779906311da15671f24557f0
```

Ce vecteur est **reproductible** : il a été calculé avec le format décrit ci-dessus, et il tient en trois lignes de test. Il vérifie d'un coup la sérialisation, l'implémentation du hash, la boucle de minage **et** la règle des zéros hexadécimaux de tête.

#### 6.5.7. Politique de nonce

| Point | Décision | Pourquoi |
|---|---|---|
| **Espace de valeurs** | **64 bits non signés** (0 → 2⁶⁴−1) | Le nonce 32 bits de Bitcoin s'épuise fréquemment et oblige à modifier un « extra-nonce ». Avec 64 bits, l'épuisement est **hors de portée** : à 10⁶ h/s, il faudrait ≈ 584 000 ans |
| **Point de départ** | **0**, incrément de 1 | **Reproductibilité des tests** : le même bloc donne le même nonce (indispensable pour un vecteur doré) |
| **Budget d'essais** | `max_essais = 16^(k+1)` = 16 × l'espérance | Filet de sécurité : la probabilité d'échouer est ≈ e⁻¹⁶ ≈ 10⁻⁷. Au-delà → abandon, journal `ERROR`, réponse `-32001` (`retryable`) |
| **Coût mémoire** | Constant (un en-tête de 96 octets réutilisé) | Un minage ne doit pas allouer par tentative (CWE-400) |
| **Parallélisation** | **Aucune** | Décision du cahier des charges : le calibrage mesure la machine, il ne la sature pas (§8.7) |

#### 6.5.8. Pourquoi SHA-256 simple, et pas le double hachage de Bitcoin ?

Bitcoin calcule `SHA-256(SHA-256(en-tête))` — un **double hachage** (SHA256d) — une protection proposée par Ferguson et Schneier (2003) contre les attaques par **extension de longueur** : si l'on connaît `H(secret ‖ message)`, on peut calculer `H(secret ‖ message ‖ remplissage ‖ extra)` sans connaître le secret.

Cette attaque **ne s'applique pas ici**, pour trois raisons qu'il faut savoir énoncer :

1. **Aucun secret** n'est préfixé à nos données : tout ce qui est haché est public.
2. **La longueur est fixe** (96 octets) et connue : rien à « étendre ».
3. Le hash n'est **jamais utilisé comme code d'authentification de message** — c'est là, et seulement là, que l'extension de longueur est dangereuse. Si un jour nous devions authentifier un message, la réponse ne serait pas « hacher deux fois » mais **HMAC** (RFC 2104), qui est la construction prévue pour cela.

Le hash simple est donc **suffisant et conforme au sujet** (« SHA-256 »), et il est deux fois plus rapide — ce qui compte quand on minera 1 000 blocs. Le double hachage reste mentionné ici comme référence industrielle, pas comme exigence.

#### 6.5.9. Ce qui est haché, ce qui ne l'est pas

| Élément | Haché ? | Pourquoi |
|---|---|---|
| `version`, `block_index`, `timestamp_ms`, `prev_hash`, `action_id`, `difficulty`, `nonce`, `payload_hash` | **Oui** | Ce sont les octets de l'en-tête : tout ce qui définit le bloc |
| `payload_canon` (les octets) | **Indirectement** | Via `payload_hash` : c'est ce qui permet une charge utile de taille variable sans changer la taille de l'en-tête |
| `difficulty` **appliquée** | **Oui** | ENF-52 : elle varie, donc elle doit être protégée |
| `duration_ms`, `hashes`, `difficulty_courante` | **Non** | Ce sont des **mesures d'exploitation**, pas des faits de la chaîne. Les inclure rendrait le hash non reproductible (il dépendrait de la machine) — c'est un piège classique |
| L'`id` de la transaction cliente, l'identifiant de session | **Non** | Ils n'appartiennent pas au bloc |
| La valeur recalculée d'une carte | **Non** — la base la recalcule | Elle est **dérivée** des recommandations et des victoires, qui sont, elles, dans la chaîne (ENF-45) |

---


## 7. Vue interface

### 7.1. Interface de communication — le protocole

Le protocole applicatif est **spécifié séparément** : `docs/protocole-applicatif.md` **v1.0** (16 sections, adossé à JSON-RPC 2.0 pour l'enveloppe, RFC 6455 pour la délimitation, RFC 9457 pour les erreurs, RFC 8259/7493 pour le JSON, RFC 1122 « Postel » pour la tolérance). Cette section-ci ne le répète pas : elle dit **ce que l'implémentation doit en respecter**.

| Point du protocole | Section source | Contrainte d'implémentation |
|---|---|---|
| Trame `[longueur 4 octets big-endian][JSON UTF-8]`, ≤ **64 Kio** | §3.1 | Lecture **exactement** de la longueur annoncée ; longueur nulle ou > 64 Kio → refus **sans allocation**, socket fermée |
| Enveloppe `{v, id, type}` — REQ / RES / EVT | §4 | Le client **conserve les demandes en vol** (identifiant → future) et ré-associe la réponse (`ENF-20`) |
| Poignée de main `session.hello` → `session.welcome` | §6.1, §10 | **5 s** pour l'envoyer ; au-delà, la socket est fermée sans cérémonie |
| **23 actions**, **22 événements** *(19 + 2 authentification, puis +2 partage ciblé le 22/09/2026 — §7.4.5 et §15.3)* | §6, §7 | Aucune action hors catalogue : `-32601`. `card.authenticate` et `bot.*` sont **inexistantes** (périmètre) |
| **30 codes d'erreur** sur 3 plages | §8 | Le client **branche son traitement sur le code**, jamais sur le texte |
| Robustesse : **11 cas limites** traités (+ la garantie « aucun ne termine le processus ») | §9 | Chaque cas a une réaction **et** une ligne de journal (§10.4) |
| **8 demandes en vol** par client | §10 | Au-delà : `-32001 SERVER_BUSY` (`retryable`) |
| Battement de cœur 60 s, expiration à 3 battements, inactivité 300 s | §10 | La socket d'un client fantôme est libérée et **la place de la file avance** |
| File d'attente, contre-pression | §13 | Voir **§11** de ce document |

### 7.2. Interface utilisateur (JavaFX)

| Élément | Contenu | Règle de conception |
|---|---|---|
| **Écran de connexion** | IP, port, pseudo ; dernière configuration re-proposée (ENF-26) | La configuration est **envoyée avec la demande de connexion** |
| **Barre d'état permanente** | État de connexion, **position dans la file**, bloc courant, difficulté, nombre de connectés | C'est ici que se **voit** la saturation (§11.9) |
| **6 onglets** | Mes cartes · Explorer · Mes recommandations · Battles · Droits (légitimation/revendication) · Connectés | Aucune règle métier dans la vue (ENF-50) |
| **Journal d'activité** | Liste des événements reçus (`card.created`, `value.updated`, `block.mined`…) | Pédagogique en soutenance : « l'IHM affiche ce que le serveur a décidé » |
| **Indicateurs d'attente** | Squelette/attente visible dès qu'une demande est en vol | L'interface **n'est jamais figée** (ENF-42) |
| **Cartes inactives / archivées** | Présentation distincte (grisée/barrée) | Lisibilité des états `INACTIVE`, `ARCHIVEE` (cette dernière n'est plus listée hors historique) |

**Ce que l'utilisateur voit et ce qui est journalisé ne sont pas la même chose** (principe **ASVS 7.4.1** — message générique côté utilisateur, détail côté exploitation) :

| Situation | Ce que voit l'utilisateur | Ce qui part au journal |
|---|---|---|
| Refus métier (`1001 CARD_LIMIT_REACHED`) | « Vous avez déjà 100 cartes actives — désactivez-en une pour continuer. » | `WARN … métier refusé \| code=1001 card_id=- user=7` |
| Erreur interne | « Une erreur interne est survenue. Réessayez. » **+ identifiant de demande affiché** | `ERROR … exception inattendue \| r=ab12f3 module=persistence` |
| Serveur saturé | « Le serveur est momentanément chargé, nouvelle tentative automatique. » | `WARN … saturation \| in_flight=8 action=card.create` |
| En attente d'admission | « En attente — position 3 sur 12 » | `INFO … file admission \| position=3 depth=12` |

L'**identifiant de demande affiché avec l'erreur** est le point qui rend le support (et la soutenance) possible : il relie ce que l'utilisateur a vu à la ligne de journal correspondante, sans lui montrer de détail technique.

### 7.3. Contrats internes

| Contrat | Règle |
|---|---|
| `net` → `protocol` | `net` **ne décode jamais** le JSON ; il fournit des octets délimités |
| `protocol` → `business` | `protocol` transmet une structure **typée et déjà validée en forme** ; `business` ne refait pas le travail de forme |
| `business` → `persistence` | Un seul appel par action : `db_append_block(bloc, projection)` — jamais deux écritures séparées |
| `business` → diffusion | `business` décide **qui** doit recevoir un événement ; `net` ne fait que l'envoyer |
| Client : `controller` → `model` → `net` | La vue n'appelle jamais `net` ; le modèle ne contient aucun appel JavaFX |

---
### 7.4. Dossier de relecture du protocole (avant de coder)

> **Tâche 8 bis de la checklist.** Relecture à faire **par l'équipe, en réunion**, avant d'écrire la première ligne de code réseau. Ce dossier est le support de la réunion : il présente les trois listes à valider (actions, codes, bornes) **et** le résultat de la relecture du 22/09/2026.

#### 7.4.1. Ce que « validé » veut dire

Le protocole **v1.2** est **figé pour la phase 2** : après validation, toute évolution se fait en **version mineure documentée** (additive et compatible), une rupture exigeant une version **majeure** et un refus explicite `-32005` des clients incompatibles. Valider, c'est donc accepter que **changer coûtera trois fois plus cher après le 2 octobre**. C'est le but de la réunion : trouver les erreurs **maintenant**.

#### 7.4.2. Les 23 actions, validées une par une

> ✅ **Complété le 22/09/2026** — réponses Q1.4, Q2.3, Q2.4, Q4.1 et Q5.2 : la limite de cartes actives passe à **100**, le vote dure **5 min et n'est plus modifiable**, `legitimation.respond` devient un **like**, et le **partage ciblé** ajoute **2 actions** (`share.send`, `share.respond` — lignes 22 et 23).

| # | Action | Paramètres | Bornes de validation | Codes possibles | Point de vigilance en revue |
|---|---|---|---|---|---|
| 1 | `session.hello` | `client_version`, `pseudo`, `supported[]` | Pseudo 3–32 caractères `[A-Za-z0-9_-]` ; `supported` non vide | `-32602`, `-32005`, `-32002`, `-32003` | C'est ici que passe IP + port (ENF-26) ; seul message accepté dans les 5 s |
| 2 | `session.ping` | — | — | `-32004` | Le battement de cœur **ne réveille pas** une session expirée |
| 3 | `session.disconnect` | `reason` | `reason` ∈ liste fermée | — | Conserve les actions déjà inscrites (CU-25) |
| 4 | `card.create` | les 20 champs du §4.2.1 (11 obligatoires : `card_id`, `title_carte`, `sources_id`, `format`, `year`, `genres`, `themes`, `creator_id`, `owner_id`, `status`, `created_at`) | `year` 1950 → année courante + 1 ; `episodes ≥ -1` ; `rating` 0–10 ; `genres` ≥ 1, listes fermées ; textes bornés | `1001`, `-32602` | Limite **≤ 10 cartes actives** par utilisateur ; le créateur est **propriétaire initial** |
| 5 | `card.list` | `filter` | `status` ∈ liste fermée ; `franchise` ≤ 100 car. | `-32602` | Réponse **paginée** : jamais toute la base sur une connexion |
| 6 | `card.get` | `card_id` | entier > 0 | `1002` | |
| 7 | `card.inactivate` | `card_id`, `reason` | `reason` ≤ 200 car. | `1003` | Une carte `EN_BATTLE` ne s'inactive pas (`1009`) |
| 8 | `recommendation.add` | `card_id` | entier > 0 | `1003`, `1004` | **Poids calculé serveur** (1.0 / 0.5) — jamais transmis par le client |
| 9 | `recommendation.repudiate` | `card_id` | entier > 0 | `1005` | Nouveau bloc distinct : l'historique reste |
| 10 | `recommendation.list` | `direction` | `given` \| `received` | `-32602` | |
| 11 | `battle.open` | `card_id`, `opponent_card_id` | entiers > 0, distincts | `1006`, `1007`, `1008`, `1009` | Proximité **P1…P5** (au moins un critère) ; **≤ 3 battles / 10 min** |
| 12 | `battle.vote` | `battle_id`, `card_id` | entiers > 0 | `1010`, `1011` | Le **propriétaire ne vote pas** ; renvoyer la même action **remplace** le vote avant clôture |
| 13 | `battle.list` | `state` | `open` \| `closed` | `-32602` | |
| 14 | `legitimation.request` | `card_id`, `justification` | justification 1–200 car. | `1012` | Une candidature par carte ; **le plus de likes** devient porteur légitime (Q4.1) |
| 15 | `legitimation.respond` | `legitimation_id` | — *(un « like »)* | `1012` | **Le sens change** (Q4.1) : ce n'est plus un vote dans un scrutin de 60 s mais un **soutien** ; le candidat le plus liké prend le titre |
| 16 | `claim.request` | `card_id`, `justification` | justification 1–200 car. | `1014`, `1015`, `1016` | **Le demandeur doit être porteur légitime** (DEC-10) ; une seule revendication en attente |
| 17 | `claim.respond` | `claim_id`, `decision` | `accept` \| `refuse` | `1017` | Refus = **statu quo définitif, aucun recours** (DEC-11) ; silence 30 j = transfert |
| 18 | `chain.verify` | `from_block` | entier ≥ 0 | `-32602` | Coûteux : **réservé** à l'écran de démonstration, jamais en boucle |
| 19 | `chain.status` | — | — | — | Alimente la barre d'état (blocs, difficulté, débit, dernier hash) |
| **20** | **`card.authenticate`** | `card_id`, `justification` (optionnelle) | justification 1–200 car. ; carte `ACTIVE` | `1014`, `1018`, `1003` | **Rétablie le 22/09/2026** (CU-20, DEC-24) : **porteur légitime actif obligatoire** ; une seule authentification active par carte ; **aucun effet sur la valeur** |
| **21** | **`card.revoke_authentication`** | `card_id` | entier > 0 | `1014`, `1019` | Révocation par le porteur légitime ; l'historique reste en chaîne |
| **22** 🆕 | **`share.send`** | `to_user_id`, `card_id` | entiers > 0 ; **destinataire dans la liste d'amis** | `1014`, `1003`, `1001` | **Partage ciblé retenu** (Q5.2). Anti-spam : **3 partages puis 1/min**, limites réglables par l'utilisateur. **Non inscrit en blockchain** ⚠️ |
| **23** 🆕 | **`share.respond`** | `share_id`, `decision` | `accept` \| `refuse` | `1015` | **Accepter = le demandeur se déplace** ; refuser = personne ne bouge. Notifie l'expéditeur (`share.responded`) |

**Trois vérifications transversales** à faire sur les 21 lignes :

1. **Aucune action ne transporte une valeur recalculable** (propriétaire, poids, valeur, difficulté, hash) — ENF-45.
2. **Chaque action qui écrit** est bien inscrite dans un bloc, et **chaque action refusée** n'en crée aucun (REC-09).
3. **Les actions sans état** (`card.list`, `battle.list`, `chain.status`) ne sont **jamais** journalisées en `INFO` : sinon le journal devient illisible en démonstration (§10.4).

#### 7.4.3. Les 30 codes d'erreur

| Plage | Codes | Signification pour le client | `retryable` | Ce que le client fait |
|---|---|---|---|---|
| **Protocole** `-327xx` | `-32700` `PARSE_ERROR` | La trame ne s'est pas décodée | non | Reposer la question ? **Non** : bug côté client → journal + message d'erreur interne |
| | `-32600` `INVALID_REQUEST` | Enveloppe invalide (type, id, longueur) | non | Idem — c'est un défaut du client |
| | `-32601` `METHOD_NOT_FOUND` | Action inconnue (`card.authenticate`, `bot.*`…) | non | **Ne pas réessayer** : c'est une erreur de conception du client |
| | `-32602` `INVALID_PARAMS` | Un champ est fautif (`details.field`) | non | Corriger la saisie et afficher le champ concerné |
| | `-32603` `INTERNAL_ERROR` | Panne côté serveur | **oui** | Attente croissante + jitter, 3 tentatives |
| **Serveur** `-320xx` | `-32001` `SERVER_BUSY` | Charge momentanée | **oui** | Arrêt de l'envoi, reprise (§11.5) |
| | `-32002` `QUEUE_FULL` | File d'attente pleine | plus tard | Écran de connexion, message clair, pas de boucle serrée |
| | `-32003` `QUEUE_TIMEOUT` | Attente > 60 s | oui | Idem |
| | `-32004` `SESSION_EXPIRED` | Inactivité > 300 s | non | Se reconnecter (nouvelle session) |
| | `-32005` `PROTOCOL_VERSION_UNSUPPORTED` | Version majeure inconnue | non | Mettre à jour le client (`details.supported`) |
| **Métier** `1001` → `1020` | 20 codes | **Le serveur a compris et a refusé** : ce n'est pas une panne | non | Message explicite à l'utilisateur, jamais de nouvelle tentative automatique |

**Les trois règles qui rendent ces codes exploitables** (elles viennent de l'ASVS et des bonnes pratiques de RPC) :

1. **Le client branche sur le code, jamais sur le texte** : le message peut être traduit ou reformulé, le code non.
2. **Le message utilisateur et le message journal sont différents** (§7.2) : « Vous avez déjà 100 cartes actives » côté écran, `code=1001 card_id=- user=7` côté journal.
3. **Aucune trace technique ne sort du serveur** : pas de chemin de fichier, pas de requête SQL, pas de pile d'appel dans la réponse (`-32603` = message générique **+ identifiant de demande** affiché à l'utilisateur, pour retrouver la ligne de journal).

#### 7.4.4. Les bornes de validation (§12 du protocole)

| Catégorie | Règle appliquée | Refusé en pratique | Code renvoyé |
|---|---|---|---|
| **Principe général** | **Liste blanche**, jamais liste noire : on décrit ce qui est accepté et **tout le reste est refusé** | Espace, accent ou emoji dans un identifiant technique | `-32602` |
| **Canonicalisation avant validation** | Normalisation **Unicode NFC** des textes, rejet des caractères de contrôle, puis contrôle de longueur | Deux écritures équivalentes du même pseudo | `-32602` |
| **Longueurs** | Bornes **en octets** (pour borner la trame) **et** en nombre de caractères (pour borner l'affichage) | `synopsis` de 12 000 caractères | `-32602` |
| **Entiers** | Type entier strict, **bornes basses et hautes** ; un flottant n'est pas un entier | `"year": 1998.5`, `"year": "1998"` | `-32602` |
| **Décimales** | Un seul usage (`rating`) : 0–10, une décimale | `"rating": 11` | `-32602` |
| **Listes fermées** | `format`, `season`, `source`, `genres[]`, `themes[]`, `status`, `decision` | `"format": "Film"` | `-32602` |
| **JSON lui-même** | RFC 8259 : pas de `NaN`/`Infinity`, pas de clé dupliquée, UTF-8 **valide** | `{"a":1,"a":2}` | `-32700` |
| **Profondeur** | ≤ **32** niveaux (application **et** parseur) | Objet imbriqué 1 000 fois | `-32700` |
| **Taille** | Trame ≤ **64 Kio**, refus **sans allocation** au-delà | Trame de 200 Kio | `-32600` |
| **Champs inconnus** | **Tolérés** (lecture tolérante, RFC 1122) mais ignorés et journalisés une fois | — | — |
| **Champs obligatoires manquants** | Refus avec `details.field` nommant le premier champ absent | `card.create` sans `studio` | `-32602` |
| **Rejeu** | Registre des **10 000 derniers identifiants** : une demande rejouée reçoit **la même réponse**, sans créer un **second bloc** | Renvoi après expiration de délai réseau | réponse rejouée |

#### 7.4.5. Ce que la relecture du 22/09/2026 a corrigé

| Constat | Correction |
|---|---|
| Le protocole annonçait « 14 actions » alors que le catalogue en contenait **19** (porté depuis à **21** avec l'authentification) | Comptes corrigés dans le protocole (§16), le cahier des charges (ENF-19, ENF-27), cette spécification, la checklist et le questionnaire |
| Il annonçait « 28 codes d'erreur » alors qu'il y en avait **27** (portés depuis à **29**) | Idem |
| Il annonçait « 16 événements » alors qu'il y en avait **18** à ce moment-là (porté depuis à **21** avec `client.resync` et les deux événements d'authentification), et `client.resync` était **utilisé sans être déclaré** | Événement ajouté au catalogue (§7 du protocole) → **19 événements**, dont **2 ciblés** (`claim.received`, `client.resync`) |

Aucune **règle** n'a changé : seule la **comptabilité** du document était fausse. C'est le genre d'écart qu'un jury trouve en trente secondes (« montrez-moi vos 14 actions ») et qui coûte une crédibilité entière — d'où l'intérêt d'une relecture **avant** le code.

#### 7.4.6. Grille de relecture et signatures

| Point à valider en réunion | Qui | Fait le |
|---|---|---|
| Les **23 actions** : nommage, paramètres, bornes, codes associés | tous (M1 anime) | ☐ |
| Les **30 codes** : plages, `retryable`, message utilisateur / message journal | M1 + M4 | ☐ |
| Les **bornes de validation** du §7.4.4 (listes fermées, longueurs, NFC) | M2 + M3 | ☐ |
| La **file d'attente** et ses bornes (100 / 60 s) — §11.2 | M1 | ☐ |
| Les **budgets de temps** (§8.2) et le **calibrage** (§8.6 → §8.9) | M2 | ☐ |
| Le **format de journal** (§10.3) : grammaire ABNF et champs | M4 | ☐ |
| **Décision finale** : protocole v1.0 figé pour la phase 2 | tous | ☐ |

---


## 8. Vue processus / exécution

### 8.1. Fils d'exécution du serveur

| Fil | Rôle | Ne doit **jamais** | Priorité |
|---|---|---|---|
| **1 — boucle d'événements** | Accepter, lire, écrire, fermer ; tenir les admissions ; répondre **vite** | Faire du minage, écrire en base, écrire un journal sur disque (ENF-18) | Haute |
| **2 — séquenceur métier** | Appliquer **une action à la fois** : valider → miner → écrire → diffuser | Attendre un client (E/S bloquante sur un socket) | Normale |
| **3 — échéances** | Battles **300 s**, revendications 30 j, battement de cœur 60 s, inactivité 300 s *(la légitimation n'a plus d'échéance : elle s'arbitre par le nombre de likes)* | Modifier une structure partagée sans le verrou de la chaîne | Normale |
| **4 — écriture des journaux** | Vider la **file bornée** vers `zlog` | Bloquer si la file est pleine (on **abandonne** l'entrée et on compte) | Basse |
| **5 — minage** | Miner **un seul bloc à la fois** (jamais en parallèle, §5.2.1 du CDC) | Prendre d'autres travaux | Basse |

> **Pourquoi un seul séquenceur et pas un fil par client côté métier ?** Parce que la chaîne doit être **déterministe** : deux actions concurrentes qui créent un bloc porteraient des index identiques. Le serveur est le seul écrivain (ENF-05) ; ce n'est pas une limite, c'est la propriété qui rend la vérification de chaîne possible. La concurrence est donc dans l'**E/S** (100 clients servis en parallèle), pas dans l'**application des règles** (une à la fois, en série).

### 8.2. Déroulé d'une action, avec ses budgets de temps

| Étape | Ce qui se passe | Budget | Si dépassé |
|---|---|---|---|
| 1 | Trame lue, taille vérifiée, JSON décodé, enveloppe validée | ≤ **1 ms** | `-32700` / `-32600` |
| 2 | Validation **métier** (droits, limites, états) — calcul pur, sans E/S | ≤ **5 ms** | code métier (par ex. `1001`) |
| 3 | Mise en file du travail (bornée) | immédiat | `-32001 SERVER_BUSY` (`retryable`) |
| 4 | Minage du bloc (difficulté 3 ou 4, calibrée §5.2.1) | **0,3 s – 2 s** (fourchette retenue le 22/09/2026) | dépassement du délai de garde (2 × max) → `-32001`, journal `ERROR` |
| 5 | Transaction « bloc + projection » | ≤ **50 ms** (PostgreSQL local) | service **dégradé** : bloc en mémoire, `WARN`, réessai (§8.4) |
| 6 | Diffusion de l'événement + réponse | ≤ **1 ms** (mise en tampon, jamais d'écriture bloquante) | tampon borné → `client.resync` |
| **Total perçu par le client, hors minage** | | **≤ 500 ms** (latence cible du CDC) | — |

L'**engagement de latence** porte sur le total **hors minage** : annoncer 500 ms en incluant une preuve de travail calibrée dans la fourchette 0,3–2 s serait malhonnête. Le minage est donc **affiché** dans l'IHM (progression, `block.mined duration_ms`) pour que le jury mesure la part réelle du calcul.

### 8.3. Événements et diffusion

- Diffusion **à tous les admis** pour les événements collectifs (cartes, valeurs, battles, blocs, difficulté).
- **Un seul événement ciblé** : `claim.received` → propriétaire courant uniquement. Un événement ne contient **jamais** une donnée que le destinataire n'a pas le droit de voir.
- Chaque client a un **tampon sortant borné à 256 événements** ; au-delà, l'événement le plus ancien est remplacé par `client.resync` et le client redemande son contexte. **Le serveur ne bloque jamais pour un client lent** (protocole §13).

### 8.4. Défaillances et dégradation

| Défaillance | Réaction prévue | Ce que voit l'utilisateur | Trace |
|---|---|---|---|
| **PostgreSQL indisponible au démarrage** | **Règle du sujet (CU-2)** : *« si la base de données est indisponible ou en erreur, le serveur considère qu'aucun contexte de sauvegarde n'existe »* → il démarre sur une **chaîne neuve (genesis)**. **Garde-fou ajouté par l'équipe** : il se met en mode `NO_DB`, journalise `ERROR`, et **refuse d'écrire** dans la base tant qu'elle n'a pas été relue avec succès — sinon un genesis fraîchement miné viendrait s'ajouter **devant** une chaîne existante et la corromprait | Le serveur démarre (comme l'exige le sujet) ; bandeau « base indisponible — session non sauvegardée » | `ERROR db unavailable`, puis `WARN` à chaque tentative |
| **PostgreSQL devient indisponible en cours de route** | Le service **continue** : le bloc est calculé et conservé en mémoire, l'écriture est réessayée ; les lectures repartent du contexte en mémoire | Fonctionnement normal, `server.notice` discret | `WARN` + `ERROR` si persistant |
| **Base lentement** | Délai de garde sur la transaction ; au-delà, même traitement que ci-dessus | Ralentissement annoncé | `WARN` + durée mesurée |
| **Redémarrage du serveur** | Reconstruction du contexte depuis les projections (`REC-02`), puis vérification de chaîne (`ENF-47`) | Reconnexion, contexte retrouvé | `INFO` : blocs rechargés, durée |
| **Un client plante** | Socket détectée fermée (ou expirée par battement de cœur) ; place libérée | Les autres clients voient `client.disconnected` | `INFO` |
| **Un client est lent** | Tampon sortant borné, jamais de blocage | Ce client seul resynchronise | `WARN` |
| **Minage anormalement long** | Délai de garde, action refusée (`retryable`), journal `ERROR` | « Réessayez » | `ERROR` avec `difficulty`, `ms` |
| **File de journalisation pleine** | Entrées **abandonnées** et comptées ; ligne de synthèse au plus une fois par 5 s | Rien (l'application n'est pas concernée) | `WARN` : `dropped=N` |

### 8.5. Observabilité minimale (compteurs affichables)

`connexions_acceptees` · `clients_admis` · `clients_en_attente` · `profondeur_max_file` · `attente_max_ms` · `refus_file_pleine` · `expirations_attente` · `demandes_en_vol_max` · `actions_refusees_charge` · `blocs_mines` · `difficulte_courante` · `duree_minage_p95` · `lignes_journal_abandonnees`.

Ces compteurs alimentent **la barre d'état** (§7.2) et les lignes de synthèse du §11.9. Ils sont la réponse au « comment savez-vous que ça tient ? » du jury.

---
### 8.6. Le banc de calibrage au démarrage (ENF-51)

**Principe** : le serveur ne reçoit pas une difficulté écrite en dur, il **mesure la machine sur laquelle il tourne**. L'encadrant l'a dit le 18/09 : « la tolérance 3 ou 4 est justement pour permettre de s'adapter aux machines ».

| Étape | Ce qui est fait | Détail |
|---|---|---|
| **1. Quand** | **Avant** d'ouvrir le port d'écoute, donc avant d'accepter la moindre demande métier | Aucune action ne peut être traitée avec une difficulté non décidée |
| **2. Quoi** | On hache des **en-têtes factices de 96 octets** (§6.5.2), avec la **même fonction** que le minage réel | Mesurer autre chose (par exemple `sha256sum` sur un gros fichier) donnerait un débit **faux** : le minage réel paie en plus la construction de l'en-tête et la comparaison des zéros |
| **3. Combien de temps** | **3 passes de ≈ 50 ms** (≤ **250 ms** au total) | Assez pour être stable, assez court pour ne pas retarder le démarrage |
| **4. Comment** | Horloge **monotone** (§6.4), comptage des hachages, `débit = hachages / durée` | Une horloge système ajustée par NTP fausserait la mesure |
| **5. Robustesse** | On retient la **médiane** des 3 passes ; si une passe s'écarte de plus de **30 %**, on la considère aberrante et on refait la mesure (au plus une fois) | Une machine qui démarre est une machine chargée : c'est exactement le cas où une moyenne simple trompe |
| **6. Trace** | `INFO calibration \| hash_rate=… difficulty=… pattern=0000 ms=…` | La valeur mesurée est **vérifiable en soutenance** (et comparable au banc natif *vs* Podman, REC-15) |

**Ordres de grandeur, pour situer** (sources au §17) : un cœur moderne avec accélération matérielle SHA atteint ≈ **1,8 cycle par octet** (≈ 800 Mo/s sur des entrées de 64 octets, soit ≈ 12 millions d'appels par seconde en théorie) ; une implémentation C **naïve**, sans optimisation ni SHA-NI explicite, se situe **plusieurs ordres de grandeur en dessous**. C'est précisément pour cela qu'on **mesure** au lieu de supposer : entre le débit théorique d'un cœur et le débit des 96 octets hachés dans la boucle de minage, l'écart est de plusieurs facteurs.

**Ce que le banc n'est pas** : un test de performance du serveur, une mesure de charge, ou un outil de comparaison entre machines. Il répond à une seule question : *« quelle difficulté puis-je tenir dans la fourchette 0,3–2 s sur CETTE machine ? »*

### 8.7. La décision : quelle difficulté retenir (ENF-51)

**Règle** : on retient la **plus grande** valeur `k` de l'ensemble **{3, 4}** telle que le **temps attendu** reste ≤ **2 s** (borne haute de la fourchette **0,3–2 s** retenue le 22/09/2026) :

```
temps_attendu(k) = 16^k / débit_mesuré        # espérance du nombre d'essais : 16^k
k* = max { k ∈ {3, 4} : temps_attendu(k) ≤ 2 s }   # cible haute de la fourchette 0,3–2 s
```

| Débit mesuré (hachages/s) | Difficulté retenue | Espérance d'essais | Temps attendu |
|---|---|---|---|
| ≥ **32 768** | **4** (motif `0000`) | 65 536 | 2,0 s à 32 768 h/s · 0,5 s à 130 k h/s · 0,33 s à 200 k h/s · **0,07 s à 1 M h/s** — sous la borne basse sur les machines rapides, la durée mesurée est affichée |
| **2 048 → 32 767** | **3** (motif `000`) | 4 096 | 2,0 s à 2 048 h/s · 0,5 s à 8 000 h/s · 0,3 s à ≈ 13 650 h/s |
| **< 2 048** | **3**, avec **avertissement** | 4 096 | **> 2 s** : la machine est trop lente — on le **signale** au lieu de mentir (difficulté 2 interdite par le sujet) |
| Banc indisponible ou mesure aberrante | **3** (repli) | 4 096 | — |

**Trois décisions à comprendre, et à savoir défendre :**

1. **Pourquoi le seuil est-il exactement 32 768 ?** Parce que `16^4 = 65 536` et que la cible haute est **2 s** : 65 536 / 2 s = **32 768 h/s**, le débit à partir duquel **un** bloc de difficulté 4 tient dans **deux** secondes. La constante n'est pas choisie au hasard, elle sort de la définition de la difficulté (zéros **hexadécimaux**, donc 16 valeurs par symbole).
2. **Pourquoi ne pas monter à 5 quand la machine est très rapide ?** Parce que les bornes **{3, 4}** sont **celles du sujet** et que l'encadrant les a voulues ainsi. La difficulté 5 demande **16 fois** plus d'essais (1 048 576) : sur un poste de démonstration, cela ferait 1 à 10 secondes par bloc — la démonstration deviendrait une salle d'attente. La valeur 4 est le **plafond du périmètre ferme** ; au-delà, c'est une exploration conditionnelle de fin de projet (annexe D.1 du CDC), pas une nécessité.
3. **Pourquoi ne pas descendre à 2 quand la machine est lente ?** Pour la même raison, mais dans l'autre sens : en dessous de 3, la preuve de travail ne démontre plus rien (4 096 essais = un millième de seconde sur n'importe quelle machine). Mieux vaut **assumer un avertissement** que casser la démonstration de l'algorithme.

### 8.8. La réévaluation continue (ENF-53)

Une machine n'est pas constante : un conteneur bridé, un autre TP qui tourne, une compilation en arrière-plan, et le temps de minage double. La difficulté est donc **réévaluée**, mais avec un **amortissement** — c'est le point de conception à ne pas rater.

| Mécanisme | Valeur | Pourquoi |
|---|---|---|
| **Indicateur** | Moyenne glissante des durées de minage des **20 derniers blocs** | Une mesure isolée est bruitée ; 20 blocs lissent sans être inertes |
| **Bande morte** | On ne bouge que si la moyenne sort de **[0,2 s ; 1 s]** | Une bande morte évite l'oscillation : à 0,9 s on reste en 4, on ne « pompe » pas entre 3 et 4 |
| **Confirmation** | Sortie constatée sur **deux évaluations consécutives** | Un pic isolé (une compilation, un autre processus) ne doit pas changer la difficulté |
| **Délai minimal** | **5 minutes** entre deux changements | Un changement de difficulté est un événement : il se journalise, se diffuse, et se voit en démonstration |
| **Bornes** | **Jamais** en dehors de **{3, 4}** | Le périmètre ferme s'applique aussi à la réévaluation |
| **Diffusion** | Événement `difficulty.changed` (`old`, `new`, `hash_rate`) | Le client l'affiche dans sa barre d'état : la démonstration **voit** le mécanisme travailler |
| **Trace** | `INFO difficulty changed \| difficulty_before=4 difficulty_after=3 hash_rate=41 …` | Preuve horodatée, corrélable avec les temps de minage |
| **Après redémarrage** | La difficulté en vigueur est **celle du dernier bloc** (relue en base), puis réévaluée | Reprendre à une difficulté « par défaut » créerait une discontinuité visible dans la chaîne |

**À quoi cela ressemble** (exemple de journal, machine chargée par un conteneur bridé) :

```
2026-09-22T10:04:11.980Z | INFO | srv | - | r=- | chain | calibration      | hash_rate=198432 difficulty=4 pattern=0000 ms=151
2026-09-22T10:31:07.114Z | INFO | srv | - | r=- | chain | block mined      | difficulty=4 nonce=84213 hashes=120488 ms=612
2026-09-22T10:52:40.332Z | INFO | srv | - | r=- | chain | window analysis  | blocks=20 avg_ms=1043 out_of_band=2
2026-09-22T10:52:40.335Z | INFO | srv | - | r=- | chain | difficulty changed | difficulty_before=4 difficulty_after=3 hash_rate=39120 reason=window_above_1s
2026-09-22T10:52:40.336Z | INFO | srv | - | r=- | chain | block mined      | difficulty=3 nonce=2214 hashes=3391 ms=88
```

> **L'analogue industriel** : Bitcoin fait exactement cela à grande échelle — un ajustement périodique visant un **temps de bloc cible**, avec un **plafond de variation** (facteur 4) pour empêcher les changements brutaux. Notre bande morte + confirmation sur 2 mesures + 5 minutes de délai minimal jouent le même rôle : **empêcher l'oscillation** plutôt que suivre la moindre fluctuation.

### 8.9. La preuve de travail, pas à pas

```text
chain_mine(en-tête_partiel, difficulty, budget_ms):
    t0 = monotone_ms()                       # mesure de DURÉE (jamais l'horloge UTC)
    en-tête.timestamp_ms = horodatage figé   # FIGÉ avant la boucle (§6.5.2)
    max_essais = 16^(difficulty + 1)         # 16 × l'espérance : filet de sécurité
    pour nonce de 0 à max_essais-1:
        en-tête.nonce = nonce                # 64 bits : épuisement hors de portée
        h = sha256(en-tête)                  # 96 octets, allocation nulle
        si h commence par `difficulty` zéros hexadécimaux:
            duration_ms = monotone_ms() - t0
            journal(INFO, "block mined", difficulty, nonce, essais, duration_ms)
            return OK(h, nonce, duration_ms, essais)
        si monotone_ms() - t0 > budget_ms:   # délai de garde
            journal(ERROR, "mining deadline exceeded", difficulty, essais, elapsed)
            return ERR_SERVER_BUSY           # -> -32001, retryable
    journal(ERROR, "nonce budget exhausted", difficulty, essais)
    return ERR_SERVER_BUSY
```

**Choix d'implémentation à ne pas perdre de vue :**

| Point | Décision | Raison |
|---|---|---|
| **Horodatage** | Figé avant la boucle | Sinon le hash trouvé ne correspond plus à l'en-tête final (bloc invérifiable) |
| **Durée** | Horloge **monotone**, jamais incluse dans le hash | Mesurer avec l'horloge système et hacher la durée rendrait le bloc non reproductible |
| **Budget** | `16^(k+1)` essais **et** un délai de garde | Protège contre une machine devenue pathologiquement lente (CWE-400). **Délai de garde = 4 s** (2 × la borne haute de la fourchette 0,3–2 s), relevé à 2 × le maximum mesuré si celui-ci dépasse 2 s |
| **Allocation** | **Zéro** allocation dans la boucle (un en-tête de 96 octets réutilisé) | Une allocation par tentative ferait chuter le débit et créerait une pression mémoire |
| **Parallélisation** | **Aucune** : un seul bloc miné à la fois, un seul fil (§8.1) | Décision du CDC : le calibrage **mesure**, il ne sature pas la machine |
| **Blocage** | La boucle d'événements **n'est pas** celle qui mine | ENF-18 : les 100 clients restent servis pendant le minage |

**Ce que le client affiche pendant le minage** : la barre d'état montre la difficulté courante et le dernier `duration_ms` reçu par `block.mined` ; l'onglet d'activité affiche l'événement. En soutenance, on peut donc montrer **le temps réel du calcul**, et comparer :

| Configuration | Difficulté attendue | Ce que le jury doit voir |
|---|---|---|
| Serveur **natif** sur le poste | 4 (débit élevé) | `hash_rate` élevé, blocs en quelques dixièmes de seconde |
| Serveur en conteneur **bridé** (`--cpus=0.5`) | 3 après réévaluation | Un `difficulty changed old=4 new=3` **visible dans le journal**, et des blocs qui restent ≤ 1 s |

C'est le critère **REC-15** : la même chaîne, deux machines, **deux difficultés** — et une chaîne **toujours vérifiable**, parce que chaque bloc porte la sienne.

---


## 9. Vue de déploiement

| Élément | Détail | Source |
|---|---|---|
| **Postes de démonstration** | 4 postes sur le réseau local : 1 serveur (C + PostgreSQL) + 3 clients JavaFX (le 4ᵉ membre pilote la démonstration et lit les journaux) | §3.1 du CDC |
| **Conteneurisation** | Image **Podman** reproductible : PostgreSQL + `zlog` + serveur compilé. Podman est validé par l'encadrant et sert de **parade** si l'installation de `zlog` échoue sur un poste (H3) | §5.8, H3 |
| **Réseau** | Un **seul** port d'écoute applicatif, déclaré en configuration (valeur par défaut proposée : **7001** — aucun port n'est imposé par le sujet, valeur à figer en réunion) | ENF-26, ENF-27 |
| **Configuration** | `zlog.conf`, `logback.xml`, `server.ini` (port, file d'attente, difficulté), `client.properties` (dernière IP/port) | §10.2, §11.2 |
| **Journaux** | `logs/` (non versionné) en local ; `journald`/`rsyslog` si les postes sont séparés | §10.9 |
| **Sauvegarde de secours** | `pg_dump` daté + `scripts/restore.sh` **testé** avant la soutenance (RQ-01) | §12.1 du CDC |
| **Tags** | `phase1` (**10/10/2026**), `phase2` (18/12/2026), `phase3` (15/01/2027), `phase4` (démo **18–21/01/2027**) | §11 du CDC et sujet officiel §9 à §12 |
| **Prérequis système** | `net.core.somaxconn` ≥ 128, `ulimit -n` ≥ 4096, horloge synchronisée, accès disque ~200 Mo pour les journaux bornés | §11.8, §10.8 |

---

## 10. Spécification détaillée n° 1 — Journalisation (ENF-55 → ENF-58)

> **Statut** : 🔧 **décidé**, rédigé ici pour être implémenté tel quel. Le cahier des charges pose le *quoi* (§5.8) ; cette section est le *comment*, avec les valeurs, les formats et les tests.

### 10.1. Objectifs et cadre

Un journal n'existe pas pour « tracer que ça marche », mais pour **répondre à trois questions** : *que s'est-il passé ?* (investigation), *est-ce normal ?* (surveillance), *peut-on le prouver ?* (audit et soutenance). Trois exigences industrielles en découlent, reprises telles quelles :

1. **Produire, protéger, conserver, relire** (ISO/IEC 27001:2022 **A.8.15** et **A.8.16**) — un journal qu'on n'ouvre jamais ne satisfait aucune des quatre.
2. **Ne pas transformer le journal en fuite** (OWASP ASVS **7.1.1/7.1.2**, **CWE-532**) — les journaux sont **moins protégés** que la base : ils sont le premier endroit qu'un curieux lit.
3. **Rester exploitable sans être bloquant** (ENF-18, ENF-42, **ENF-57**) — un journal qui ralentit l'application sera désactivé par l'équipe, et un journal désactivé ne protège personne.

### 10.2. Bibliothèques retenues

| Côté | Retenu | Licence | Conséquence pratique |
|---|---|---|---|
| **Serveur (C)** | **`zlog`** (`-lzlog`), configuration `zlog.conf`, validation par `zlog-chk-conf` | **LGPL** (2.1) → **liaison dynamique obligatoire** : on lie la bibliothèque partagée, on **ne la modifie pas** et on ne la lie pas statiquement. C'est la façon propre d'utiliser une bibliothèque LGPL dans un projet dont le code n'est pas LGPL | Aucune obligation sur notre code source ; l'image Podman embarque la bibliothèque |
| **Serveur — repli** | `rxi/log.c` (2 fichiers vendus au dépôt), **uniquement** si H3 se réalise | MIT | Mêmes appels via une **façade** `util/log.h` : le jour où l'on bascule, **une seule couche change** (§5.2) |
| **Client (Java)** | **SLF4J** (API appelée) + **Logback** (moteur, `logback.xml`) | SLF4J : type **X11/MIT** · Logback : **EPL-1.0 *ou* LGPL-2.1** (double licence) | Le code n'importe **que** `org.slf4j.Logger` : le moteur reste remplaçable sans toucher au métier (§5.8.1 du CDC) |

**Règle de code** : aucun `printf` ni `System.out.println` de diagnostic dans le code livré (ENF-55). La façade `util/log.h` est **la seule** porte d'entrée côté C — c'est ce qui permet le repli et ce qui rend la vérification `grep` possible en CI.

### 10.3. Format de ligne — **normatif**

Une ligne = un événement. **Jamais de message multiligne** : une exception Java est aplatie sur une seule ligne (la pile est écrite à la suite). Le format est le même des deux côtés, ce qui rend les deux journaux **fusionnables par un simple tri** :

```
HORODATAGE | NIVEAU | SERVICE | c=CLIENT | r=REQUÊTE | MODULE | message | clés=valeurs
```

**Grammaire (ABNF)** — c'est ce que le test de conformité vérifie (§10.12) :

```abnf
ligne      = horodatage sep niveau sep service sep client sep requete sep module sep message [sep cles] CRLF
sep        = " | "
horodatage = 4DIGIT "-" 2DIGIT "-" 2DIGIT "T" 2DIGIT ":" 2DIGIT ":" 2DIGIT "." 3DIGIT "Z"   ; UTC, milliseconde
niveau     = "TRACE" / "DEBUG" / "INFO" / "WARN" / "ERROR" / "FATAL"
service    = "srv" / "cli"
client     = 1*DIGIT / "-"                    ; identifiant serveur ; "-" si aucun
requete    = 1*12HEXDIG / "-"                 ; identifiant de demande (protocole §4.1)
module     = "net" / "protocol" / "business" / "chain" / "persistence" / "util"
           / "session" / "view" / "controller" / "model"
message    = 1*120(texte sans "|" ni CR ni LF)     ; en anglais (ENF-34)
cles       = paire *(" " paire)
paire      = clé "=" valeur                   ; clé = 1*24(ALPHA / DIGIT / "_"), en minuscules
```

| Champ | Contenu | Exemple | Règle |
|---|---|---|---|
| **Horodatage** | ISO 8601 / RFC 3339, **UTC**, milliseconde | `2026-09-18T19:12:03.412Z` | Toujours UTC : corrélable entre postes (**A.8.17**, ASVS 7.3.4) |
| **Niveau** | 6 niveaux identiques des deux côtés | `INFO` | Voir §10.4 |
| **Service** | `srv` ou `cli` | `srv` | Permet de filtrer une seule colonne |
| **Client** | Identifiant serveur ; `-` si aucun | `c=7` | Jamais le pseudo seul : l'identifiant est stable, le pseudo peut changer de casse |
| **Requête** | Identifiant de demande du protocole (ENF-20, ENF-24) | `r=ab12f3c4d5e6` | **La clé de voûte** : elle relie les deux journaux |
| **Module** | Liste fermée (§10.3) | `chain` | Facilite le filtrage par domaine |
| **Message** | Court, en anglais, ≤ **120 caractères** | `block mined` | Pas de phrase longue : les données vont dans les clés |
| **Clés** | Paires `clé=valeur` séparées par une espace | `difficulty=4 nonce=31847 ms=812` | Clés en minuscules, jamais d'accent |

**Règles d'écriture** (normatives) :

1. **Longueurs bornées** : message ≤ 120 caractères, **valeur** ≤ 64 caractères, **ligne complète** ≤ 512 caractères. Un dépassement **tronque** et ajoute le suffixe `~` — on ne laisse jamais une charge utile de 60 Kio entrer dans un journal.
2. **Caractères interdits dans les champs** : `|`, CR, LF, NUL et tout caractère de contrôle. Ils sont **échappés** (§10.6), jamais supprimés silencieusement.
3. **Nombre de clés** : ≤ **8 par ligne**. Au-delà, le reste est résumé (`extra=3`).
4. **Ordre des clés stable** pour une même ligne de code : deux exécutions produisent la même ligne (comparaison de journaux en test possible).
5. **Une action = des lignes corrélées**, jamais un paragraphe : l'IHM affiche, le journal trace.

**Ce que ça donne, des deux côtés, pour une action** — c'est **le** livrable de démonstration de REC-17 :

```
2026-09-18T19:12:03.101Z | DEBUG | cli | c=7 | r=ab12f3c4d5e6 | net      | request sent              | action=card.create bytes=318
2026-09-18T19:12:03.118Z | INFO  | srv | c=7 | r=ab12f3c4d5e6 | protocol | request accepted          | action=card.create payload_bytes=294
2026-09-18T19:12:03.412Z | INFO  | srv | c=7 | r=ab12f3c4d5e6 | chain    | block mined               | difficulty=4 nonce=31847 hashes=72144 ms=294
2026-09-18T19:12:03.455Z | INFO  | srv | c=7 | r=ab12f3c4d5e6 | persistence | block committed        | block_index=118 cards=1 ms=43
2026-09-18T19:12:03.457Z | INFO  | srv | -   | r=-            | net      | event broadcast           | event=card.created recipients=12
2026-09-18T19:12:03.481Z | DEBUG | cli | c=7 | r=ab12f3c4d5e6 | net      | response received         | status=OK bytes=214
```

#### 10.3.1. Compléments normatifs — renvoi à la note de format de ligne

Cette section fixe **la grammaire** ; la **nomenclature** (vocabulaire des messages, nommage des clés, correspondance vers syslog/ECS/OpenTelemetry, contrat de compatibilité) est définie dans **`docs/note-format-ligne.md` (v1.0, 22/09/2026)**, référence unique du format pour les deux langages. Trois de ses règles sont reprises ici parce qu'elles sont **testées** :

| Règle | Portée | Test |
|---|---|---|
| Le **`message`** appartient à un **vocabulaire fermé de 69 entrées** (constantes `server/util/log_events.h` et `LogEvents.java`) — jamais une phrase libre, jamais une valeur interpolée | Les deux programmes | Test du vocabulaire fermé (note §13.2) |
| Les **clés** sont normalisées : minuscules, `snake_case`, **unité dans le nom** (`ms`, `bytes`, `hashes`, `<sujet>_len`), **`<sujet>_before` / `<sujet>_after`** (jamais `old` / `new`), 8 clés au maximum | Les deux programmes | Test 1 (§10.12) + revue |
| **Contrat de compatibilité** : les 8 champs positionnels ne sont **jamais** réordonnés, les clés s'ajoutent **uniquement en fin de ligne**, un lecteur **ignore** toute clé inconnue ; une rupture passerait par un nouvel identifiant de service (`animetcg-srv2`) | Toute évolution ultérieure | Revue de la note §11 |

> **Deux écarts assumés, écrits dans la note** : pas de champ `VERSION` dans la ligne (la compatibilité est garantie par les trois règles ci-dessus) ; écriture dans un fichier roté alors que le facteur XI du *12-Factor* demande de laisser le stockage à l'environnement (nous le faisons dans le conteneur, `podman logs`).

### 10.4. Niveaux et affectation

| Niveau | Ce qu'il contient | Exemples (serveur) | Exemples (client) |
|---|---|---|---|
| `TRACE` | Jamais en production. Réservé aux **32 premiers octets** d'une trame, en développement | `frame head` | `frame head` |
| `DEBUG` | Détails utiles au développement : trames, requêtes SQL, diffusions | `query done`, `event broadcast` | `request sent`, `response received` |
| `INFO` | **Les faits** : ce qui s'est réellement passé, y compris les admissions, minages, transferts | `block mined`, `client admitted`, `claim transferred` | `session welcomed`, `card created` |
| `WARN` | Anomalie **récupérée** : base lente, client lent, file pleine, entrée de journal abandonnée, refus métier | `queue full`, `db slow`, `log dropped` | `server busy`, `resync requested` |
| `ERROR` | Opération **échouée** alors qu'elle devait réussir | `db write failed`, `mining deadline exceeded`, `verification failed` | `connection lost`, `protocol error` |
| `FATAL` | L'application ne peut plus rendre son service | `init failed` (`reason=config_invalid`), `listen failed` | `startup failed` (`reason=…`) |

**Correspondance numérique des niveaux** (détail et sources : note de format de ligne §7.1) — c'est ce qui rend le niveau exploitable par un outil qui n'est pas le nôtre :

| Niveau | syslog `SEVERITY` | `PRIVAL` (facility `local0`) | ECS `event.severity` | OpenTelemetry `SeverityNumber` | Logback | `zlog` |
|---|---|---|---|---|---|---|
| `TRACE` | 7 | `<135>` | 7 | **1** | TRACE | `ZLOG_TRACE` |
| `DEBUG` | 7 | `<135>` | 7 | **5** | DEBUG | `ZLOG_DEBUG` |
| `INFO` | 6 | **`<134>`** | 6 | **9** | INFO | `ZLOG_INFO` |
| `WARN` | 4 | `<132>` | 4 | **13** | WARN | `ZLOG_WARN` |
| `ERROR` | 3 | `<131>` | 3 | **17** | ERROR | `ZLOG_ERROR` |
| `FATAL` | 2 | `<130>` | 2 | **21** | ERROR *(+ marqueur)* | `ZLOG_FATAL` |

> `TRACE` et `DEBUG` partagent la sévérité syslog **7** : syslog n'a que huit niveaux, la nuance « trace » n'existe pas côté système. La perte n'a lieu **qu'à la diffusion** — le champ `NIVEAU` de notre ligne reste précis.

**Règles d'affectation** (pour éviter le débat en revue) :

- Un **refus métier** (limite de 100 cartes, doublon de recommandation…) est un **`WARN`**, pas une `ERROR` : le système a fonctionné correctement.
- **Toute** erreur écrite côté serveur porte `r=` — sans identifiant de demande, une erreur n'est pas corrélable.
- Un **état de saturation** est journalisé en `WARN` avec les compteurs (`in_flight`, `queue_depth`) : c'est une donnée d'exploitation, pas une fatalité.
- Le niveau **global est réglable sans recompiler** (ENF-58) : `ANIMETCG_LOG_LEVEL` côté C, `logback.xml` côté Java. Valeur par défaut : **`INFO`** ; `DEBUG` en développement ; `TRACE` jamais en démonstration.

### 10.5. Corrélation — c'est le `request_id` qui fait tout le travail

| Identifiant | Qui le crée | Durée de vie | Où il vit |
|---|---|---|---|
| **`request_id`** (`r=`) | **Le client**, 12 caractères hexadécimaux aléatoires, à l'émission de chaque demande | Le temps de la demande (aller-retour) | Envoyé dans la trame, repris par le serveur, placé dans le **MDC** des deux côtés |
| **`client_id`** (`c=`) | **Le serveur**, à l'admission | Toute la session | MDC |
| **`session_id`** | Le serveur, à `session.welcome` | Toute la session | Journalisé au cycle de vie ; utile pour distinguer une reconnexion |

**MDC = la même technique dans les deux langages** (`zlog` et Logback l'offrent tous les deux) : `c=` et `r=` sont posés **une fois** par connexion et par demande, puis ajoutés automatiquement à **chaque** ligne. C'est ce qui garantit que le format ne dépend pas de la discipline du développeur au moment d'écrire le message.

**Exemple de lecture croisée** — la commande qui fusionne et filtre :

```bash
# Toutes les lignes d'une action, des deux programmes, en ordre chronologique
sort -t'|' -k1,1 logs/server.log logs/client-*.log | grep 'r=ab12f3c4d5e6'
```

### 10.6. Assainissement — se protéger de l'injection de journal (CWE-117)

**Le risque, en une phrase** : un utilisateur inscrit comme pseudo un texte contenant un retour à la ligne suivi d'une fausse ligne de journal ; cette fausse ligne se retrouve ensuite **lue comme un fait** dans le fichier, voire dans `syslog`. C'est le **CWE-117**, et ASVS **7.3.1** l'exige explicitement.

**Fonction unique côté C** (côté Java, un `PatternLayout` avec `%replace` joue le même rôle) :

```c
/* Escapes control characters and the field separator so that one log call
   always produces exactly one line. Never truncates in the middle of a
   UTF-8 sequence: the tail is dropped instead. */
int log_sanitize(const char *in, char *out, size_t out_max);
```

| Entrée | Sortie | Pourquoi |
|---|---|---|
| CR, LF | `\r`, `\n` **(deux caractères)** | Un retour à la ligne ne peut plus fabriquer une ligne |
| Tabulation | `\t` | Préserve l'alignement lisible |
| `\|` | `\u007C` | Le séparateur de champs ne peut plus être forgé |
| Autres caractères de contrôle (0x00–0x1F, 0x7F) | `\xNN` | Lisibilité et sûreté sur les terminaux |
| UTF-8 invalide | `?` par octet invalide | Assainissement définitif : le journal reste lisible par un outil |
| Valeur > 64 caractères | 64 caractères + `~` | Borne la ligne (§10.3) |

**Test associé (obligatoire, automatisé)** : créer une carte dont le titre contient `\n2026-01-01T00:00:00.000Z | INFO | srv | ...` puis vérifier que le fichier de journal a **augmenté d'exactement une ligne** et qu'aucune ligne ne **commence** par l'horodatage forgé. Ce test est en phase 2, pas « plus tard ».

### 10.7. Ce qui ne va jamais dans un journal (CWE-532, RGPD)

| Interdit | Pourquoi | Ce qu'on écrit à la place |
|---|---|---|
| Mots de passe, jetons, clés | CWE-532, ASVS 7.1.1 | *Sans objet* : aucun mot de passe dans le périmètre |
| **Charge utile complète** d'une trame | Elle contient du texte libre saisi par l'utilisateur (titre, synopsis, justification) | Le **nom de l'action**, le nombre d'octets, l'identifiant de l'objet (`card_id`) |
| Texte libre saisi (synopsis, justification, pseudo en clair dans un message) | Donnée personnelle ou contenu utilisateur : inutile à l'investigation | `card_id=42`, `claim_id=7`, `justification_len=118` |
| Adresse IP **en clair dans les messages** | Donnée personnelle (RGPD) | `c=7` (identifiant) ; l'adresse IP reste **disponible dans une ligne dédiée `DEBUG`** du serveur, avec la même rétention que le reste — utile pour l'investigation, jamais recopiée ailleurs |
| Valeurs recalculables (valeur d'une carte, propriétaire) | Elles changent : un journal faux vaut pire que pas de journal | Les **variations** : `value_before=7.0 value_after=8.0 cause=recommendation` |
| Chemin absolu d'un fichier système, contenu de `/etc/passwd`, variables d'environnement | Fuite d'information gratuite (CWE-532) | Rien |

> **La règle simple à retenir en revue** : *une ligne de journal doit rester vraie et utile si elle est lue par quelqu'un qui n'a pas accès à la base.* Ni secret, ni texte utilisateur, ni donnée recalculable.

### 10.8. Rotation, rétention et protection

| Sujet | Valeur retenue | Justification |
|---|---|---|
| **Rotation (C)** | `zlog`, **10 Mo × 5 fichiers** par journal, rotation **native** | Borne le disque sans outil externe (NIST SP 800-92 recommande une rotation fréquente et un plafond explicite) |
| **Rotation (Java)** | `RollingFileAppender`, **10 Mo × 5**, `totalSizeCap` 50 Mo | Idem, symétrique |
| **Plafond total** | **50 Mo par programme** (serveur, puis chaque client) | Un disque plein en démonstration est un risque évité par construction |
| **Rétention** | **6 mois maximum**, purge par `scripts/logs-purge.sh` (fichiers > 180 j) | Position constante de la **CNIL** sur les journaux d'accès applicatifs ; principe de **limitation de conservation** (RGPD art. 5.1.e). Les 50 Mo bornés font que la purge est un filet de sécurité, pas le mécanisme principal |
| **Emplacement** | `logs/` à côté de l'exécutable, **non versionné** (`.gitignore`) | Ce sont des **données d'exécution** : aucun journal ne part dans le dépôt |
| **Droits** | Dossier `0750`, fichiers `0640`, propriétaire = compte de service | Les journaux ne sont **pas** lisibles par tous (protocole §12 : la zone serveur reste de confiance, mais autant éviter le `cat` de trop) |
| **Intégrité** | Écriture **append-only**, jamais de réécriture en place ; horodatage UTC monotone | Un journal réécrit n'est plus une preuve (ISO 27001 **A.5.33**, protection des enregistrements) ; la protection forte vient du niveau 1 de centralisation (§10.9) |
| **Horloges** | `systemd-timesyncd`/NTP, UTC sur tous les postes | Sans horloges synchronisées, la fusion chronologique est fausse (**A.8.17**) |

### 10.9. Centralisation — trois niveaux, dont deux dans le périmètre

| Niveau | Contexte | Mécanisme | Commande de démonstration |
|---|---|---|---|
| **0** | Développement et tests : tout sur la même machine | Fichiers `logs/`, **format identique** | `sort -t'\|' -k1,1 logs/server.log logs/client-*.log` |
| **1** | Démonstration : postes séparés | C → `syslog` · Java → `SyslogAppender` · **`journald`** centralise | `journalctl -f -t animetcg-srv -t animetcg-client` |
| **2** | 🎁 Hors périmètre S3 | Agrégateur (Graylog/GELF, Fluentd), tableaux de bord, alertes | *À mentionner en soutenance comme évolution* |

**Correspondance avec syslog (RFC 5424)** — la conversion est faite par la configuration, pas par le code :

| Notre champ | Champ RFC 5424 | Valeur |
|---|---|---|
| Horodatage | `TIMESTAMP` | Même valeur ISO 8601 |
| Niveau | `SEVERITY` | `INFO`→6, `WARN`→4, `ERROR`→3, `FATAL`→2 (facility 1 = user) |
| Service | `APP-NAME` | `animetcg-srv` / `animetcg-client` |
| Module | `MSGID` | `chain`, `net`, … |
| `c=`, `r=` | **données structurées** `[animetcg@32473 c="7" r="ab12f3c4d5e6"]` | Transportés, donc filtrables |
| Message + clés | `MSG` | Le reste de la ligne, inchangé |

> **Pourquoi `@32473` ?** RFC 5424 impose qu'un identifiant de données structurées privé porte un numéro d'entreprise IANA. Le **32473** est le numéro **réservé à la documentation** (RFC 5612) : c'est donc le choix correct pour un projet pédagogique, et il faudrait un vrai numéro d'entreprise pour un déploiement hors du cadre scolaire.
>
> ⚠️ **Aucune contradiction avec « un seul canal applicatif : le socket »** (§3.2 du CDC) : le socket transporte les **données fonctionnelles** ; la journalisation passe par des **outils d'exploitation** et ne transporte **aucune** donnée métier.

### 10.10. Ne jamais bloquer (ENF-57)

| Mécanisme | Côté serveur | Côté client |
|---|---|---|
| File d'attente | **Bornée à 4096 entrées**, message ≤ 512 o → ≈ **2 Mo au pire** | `AsyncAppender`, `queueSize=4096`, `discardingThreshold=0` (donc **jamais** de blocage appelant) |
| Écriture | **Fil dédié n° 4** (§8.1) : la boucle d'événements **dépose** et continue | Le fil JavaFX **ne journalise jamais** en écriture directe |
| File pleine | L'entrée est **abandonnée**, un compteur s'incrémente, une ligne de synthèse **au plus une fois par 5 s** (`WARN log dropped \| dropped=N`) | Événement de perte récupérable via un `StatusPrinter` au démarrage |
| Interdits | Écriture disque depuis la boucle d'événements ; `fsync` par ligne ; journalisation d'une charge utile complète | Idem côté client |

Le principe est celui d'**ENF-18** appliqué au journal : *on perd une ligne de trace plutôt que la réponse à 100 clients*. Une perte de journal est **visible** (compteur + ligne de synthèse), donc jamais silencieuse.

### 10.11. Volumétrie — dimensionner plutôt que subir

Base de calcul : **≈ 160 octets par ligne**, **≈ 800 octets par action** répartis entre les deux programmes (3 lignes serveur, 2 lignes client).

| Scénario | Débit d'actions | Volume produit | Temps pour remplir 50 Mo (un programme) |
|---|---|---|---|
| **Démonstration** (4 postes, niveau `INFO`) | ≈ 1 action/s | ≈ 2,9 Mo/h | **≈ 17 h** — très au-delà d'une soutenance |
| **Montée à 100 clients** (niveau `INFO`) | ≈ 20 actions/s | ≈ 58 Mo/h | **≈ 52 min** — la rotation fait son travail, sans saturer le disque |
| **Développement** (niveau `DEBUG`) | ≈ 1 action/s | ≈ 8,6 Mo/h | ≈ 6 h — `DEBUG` reste réservé au poste de développement |

**Règle de configuration** : `INFO` par défaut sur les postes de démonstration, `DEBUG` en développement, `TRACE` **jamais** en démonstration. Le passage d'un niveau à l'autre ne demande **aucune recompilation** (ENF-58).

### 10.12. Tests de la journalisation (REC-17)

| # | Test | Critère de réussite |
|---|---|---|
| 1 | **Format** — 1000 lignes échantillonnées, serveur et client | 100 % conformes à la grammaire §10.3 (regex `^\d{4}-…Z \| (TRACE\|DEBUG\|INFO\|WARN\|ERROR\|FATAL) \| (srv\|cli) \| c=…`) |
| 2 | **Une ligne = un événement** — provoquer une exception Java | La pile apparaît sur **une seule** ligne ; le nombre de lignes augmente de 1 |
| 3 | **Fusion** | `sort -t'\|' -k1,1 logs/server.log logs/client-*.log` produit un ordre chronologique **strictement croissant** |
| 4 | **Corrélation de bout en bout** | Pour 20 identifiants de demande tirés au sort : présents **des deux côtés**, avec la ligne client **avant** la ligne serveur |
| 5 | **Anti-injection (CWE-117)** | Pseudo/titre forgé (§10.6) → **+1 ligne exactement**, aucune ligne commençant par l'horodatage forgé |
| 6 | **Anti-fuite (CWE-532)** | `grep -Eic "password\|secret\|token\|synopsis=" logs/*.log` → **0** |
| 7 | **Non-blocage (ENF-57)** | Avec la file de journalisation saturée (point d'injection de test), une demande d'un client admis répond en **≤ 500 ms** et `dropped=N` apparaît |
| 8 | **Rotation** | Écrire > 10 Mo : au plus **5** fichiers par journal, total **≤ 50 Mo**, le plus ancien supprimé |
| 9 | **Niveau réglable (ENF-58)** | Redémarrer avec `DEBUG` : de nouvelles lignes `DEBUG` apparaissent, **sans recompilation** |
| 10 | **Rétention** | `scripts/logs-purge.sh --dry-run` liste les fichiers > 180 j, et uniquement ceux-là |

### 10.13. Conformité — ce que l'on peut répondre au jury

| Référence | Exigence | Notre réponse |
|---|---|---|
| ISO 27001 **A.8.15** | Produire, protéger, conserver, relire | Deux bibliothèques dédiées, format normatif, droits `0640`, rétention 6 mois, revue en démonstration |
| ISO 27001 **A.8.16** | Surveiller, détecter l'anomalie | Compteurs d'observabilité (§8.5), lignes `WARN` de saturation, revue en soutenance |
| ISO 27001 **A.8.17** | Horloges synchronisées | UTC, milliseconde, NTP, horloge monotone pour les durées |
| ISO 27001 **A.5.33** | Protection des enregistrements | Append-only, droits restreints, copie syslog possible |
| NIST SP 800-92 | Centralisation, rotation, protection, revue | 3 niveaux de centralisation, rotation 10 Mo × 5, droits, revue |
| ASVS **7.1.1 / 7.1.2** | Ne pas journaliser de données sensibles | Liste d'interdits (§10.7) + test 6 |
| ASVS **7.1.3** | Journaliser les événements de sécurité | Poignée de main, refus, dépassements de charge, erreurs de validation : tous en `WARN`/`INFO` |
| ASVS **7.1.4** | Horodatage + éléments d'investigation | Horodatage, `c=`, `r=`, module, action, code d'erreur |
| ASVS **7.3.1** | Encoder pour empêcher l'injection | `log_sanitize` + test 5 |
| ASVS **7.3.3 / 7.3.4** | Protéger les journaux ; UTC | Droits restreints, dossier non versionné, UTC partout |
| ASVS **7.4.1** | Message générique côté utilisateur | Tableau §7.2 (identifiant de demande affiché, détail dans le journal) |

---

## 11. Spécification détaillée n° 2 — Comportement à la saturation (ENF-54)

> **Statut** : 🔧 **décidé** (Q7.2 : **100 clients simultanés**, au-delà : **file d'attente FIFO**), rédigé ici pour être implémenté tel quel.

### 11.1. De quoi parle-t-on exactement

« Saturation » désigne **trois situations différentes**, souvent confondues dans une soutenance. Les distinguer, c'est déjà la moitié de la réponse :

| Situation | Question posée | Réponse du système |
|---|---|---|
| **Trop de connexions** | « 101ᵉ client, que fais-tu ? » | **File d'attente d'admission** : `status: "QUEUED"` + position ; admission automatique dès qu'une place se libère (FIFO strict) |
| **Trop de travail** | « 100 clients agissent en même temps, que fais-tu ? » | **Sérialisation** des actions par le séquenceur métier (§8.1) + **8 demandes en vol** par client ; au-delà : `-32001 SERVER_BUSY` (`retryable`) |
| **Client trop lent** | « Un client ne consomme pas ses événements » | **Tampon sortant borné** (256 événements) → `client.resync` ; **le serveur ne bloque jamais** |

**Ce que le serveur ne fait jamais** : mettre en attente **sans borne**, accepter du travail qu'il ne pourra pas terminer, ni prévenir le client par un silence. Toute limitation est **documentée, chiffrée, observable et testée** — c'est la différence entre une protection et un bug.

### 11.2. Les trois files bornées et leurs paramètres

| File | Capacité | Durée maximale | Comportement à saturation | Où c'est configuré |
|---|---|---|---|---|
| **Admission** (FIFO des connectés non admis) | **100 places** | **60 s** d'attente | File pleine → `-32002 QUEUE_FULL` + fermeture ; attente > 60 s → `-32003 QUEUE_TIMEOUT` + fermeture | `server.ini` : `admission.max=100`, `admission.wait_seconds=60` |
| **Actions en attente** (travail déposé par la boucle d'événements) | **256 actions** | — | `-32001 SERVER_BUSY` (`retryable`) : c'est un **rejet explicite et immédiat**, pas une attente invisible | `server.ini` : `work.queue=256` |
| **Écriture des journaux** | **4096 entrées** | — | Entrée **abandonnée**, compteur `dropped=N`, ligne de synthèse au plus 1 fois / 5 s | §10.10 |
| *(rappel)* Tampon sortant par client | **256 événements** | — | Remplacement du plus ancien par `client.resync` | protocole §13 |

**Justification des deux valeurs ajoutées par l'équipe** (à valider en réunion — elles ne viennent pas du sujet) :

- **100 places d'attente** : la file sert à absorber un **pic d'arrivée** (tous les postes qui se connectent au début de la séance), pas à faire patienter indéfiniment. 100 places = la taille du service admis, soit un doublement temporaire supportable, et une borne mémoire faible (§11.6). Une file plus grande ne créerait pas de débit : elle créerait de l'attente.
- **60 s** : au-delà, mieux vaut un **refus clair** qu'une attente qui ressemble à une panne. La valeur est alignée sur le temps d'inactivité (ENF-25, 300 s) mais bien plus courte : un client en attente occupe une place dans le processus, il doit la libérer vite s'il n'a pas d'espoir.

### 11.3. Machine à états d'une connexion

```
ACCEPTEE ──hello (≤ 5 s)──┬─ place disponible ──► ADMISE ──bye / erreur / timeout ──► FERMEE
                          │                          │
                          └─ 100 admis ──► EN_ATTENTE ┴─ place libérée ──► ADMISE
                                              │
                                              ├─ attente > 60 s ──► FERMEE (-32003)
                                              ├─ file pleine ─────► FERMEE (-32002)
                                              └─ déconnexion ─────► FERMEE (retrait de la file)

Toute erreur de format, de version ou de poignée de main ──────► FERMEE (erreur du §8 du protocole)
```

| État | Ce que le client peut faire | Ce que le serveur fait |
|---|---|---|
| `ACCEPTEE` | Envoyer `session.hello` (≤ 5 s) | Attend la poignée de main ; rien d'autre n'est accepté |
| `EN_ATTENTE` | Recevoir sa position, envoyer `session.bye` | Garde la socket **sans fil dédié**, envoie `session.queue_update` à chaque changement de position, fait **avancer la file** |
| `ADMISE` | Toutes les actions du catalogue | Service normal ; événements diffusés |
| `FERMEE` | Rien | Libère la place d'admission, **admet le premier de la file**, journalise la sortie |

> **Point de conception à retenir pour le jury** : une place **ne se libère jamais d'elle-même** — elle se libère parce qu'une connexion **est fermée proprement** ou **expirée** (battement de cœur, ENF-25) ou **retirée de la file** (délai de 60 s dépassé). La place d'un client « fantôme » est récupérée en **≤ 180 s** (3 battements de cœur manqués), sans intervention.

### 11.4. Comportement observable, cas par cas

| Cas | Échange (extrait) | Ce que voit le client | Journal serveur |
|---|---|---|---|
| **1ᵉʳ au 100ᵉ client** | `session.hello` → `RES status=OK` + `session.welcome` | Connexion immédiate | `INFO client admitted \| c=… admitted=…` |
| **101ᵉ client** | `session.hello` → `RES status="QUEUED"` + `position=1`, `queue_length=1` | « En attente — position 1 » | `INFO queue enqueue \| c=… position=1 depth=1` |
| **Position qui avance** | `EVT session.queue_update` | La barre d'état décrémente, sans action de sa part | — (événement calculé) |
| **File pleine (100 en attente)** | `RES status=ERR code=-32002 QUEUE_FULL` puis fermeture | Message explicite : « serveur complet, réessayez plus tard » | `WARN queue full \| depth=100 refused=…` |
| **Attente > 60 s** | `RES status=ERR code=-32003 QUEUE_TIMEOUT` puis fermeture | Message explicite, retour à l'écran de connexion | `WARN queue timeout \| c=… waited_ms=60004` |
| **Place libérée** | `EVT session.welcome` (avec le contexte, CU-7) | La plateforme s'ouvre | `INFO client admitted \| c=… waited_ms=12480` |
| **Client en attente qui se déconnecte** | Fermeture de socket | — | `INFO queue leave \| c=… position=12` — la file **avance** |
| **Client admis qui plante** | Détection de fermeture, ou expiration en ≤ 180 s | Les autres voient `client.disconnected` | `WARN session expired \| c=… last_seen_ms=…` |
| **Client qui n'envoie pas sa poignée de main en 5 s** | Fermeture | — | `WARN handshake timeout \| peer=…` |
| **Action inconnue hors catalogue** | `RES status=ERR code=-32601` | Message d'erreur | `WARN unknown action \| action=card.authenticate` |
| **Semaine de démonstration, tout va bien** | — | Barre d'état : `admis=4/100, en attente=0, difficulté=4` | `INFO` périodique (1 ligne / 60 s, niveau configurable) |

**Invariant important** : un client **en attente** ne peut déclencher **aucune** action métier — ni création, ni recommandation, ni vote. La seule chose qu'il peut faire est **partir**. C'est ce qui garantit qu'aucun travail n'est accepté qu'on ne pourrait pas honorer.

### 11.5. Politique de rejet et de reprise

**Rejeter tôt et clairement vaut mieux qu'accepter tard.** C'est la recommandation explicite de la littérature d'exploitation (Google SRE, *Handling Overload* : « petite file, rejet précoce ; un refus immédiat est plus utile qu'une réponse trente secondes plus tard »). Nous en tirons trois règles :

1. **Rejet immédiat et explicite** plutôt qu'attente invisible : `-32001 SERVER_BUSY` (action) ou `-32002 QUEUE_FULL` (connexion), les deux avec un message utilisateur clair.
2. **`retryable` renseigné** : le client **sait** qu'il peut réessayer, et ne le devine pas.
3. **Reprise avec attente croissante et aléatoire** (`exponential backoff with jitter`), bornée — pour ne pas transformer un pic en tempête :

| Tentative | Attente avant reprise | Jitter | Nombre maximal |
|---|---|---|---|
| 1 | 1 s | ± 20 % | — |
| 2 | 2 s | ± 20 % | — |
| 3 | 4 s | ± 20 % | **3 tentatives**, puis message définitif |

Le **jitter** évite que 100 clients refusés au même instant reviennent au même instant (effet de foule). Après 3 échecs, le client revient à l'écran de connexion et **affiche l'identifiant de demande** : en soutenance, cette valeur permet de retrouver la ligne de journal du refus.

### 11.6. Coût en ressources — pourquoi « en attente » ne veut pas dire « un fil par client »

| Choix | Mémoire par client en attente | Pour 100 en attente | Verdict |
|---|---|---|---|
| **Un fil d'exécution par connexion** (approche naïve) | 1–2 Mo de pile (8 Mo par défaut sur certaines configurations) | **100 Mo à 800 Mo** pour ne rien faire | ❌ inacceptable, et exactement le mécanisme des effondrements décrits dans l'annexe E du CDC |
| **Notre approche : enregistrement de connexion + socket surveillé** | ≈ 200 o de structure + mémoire noyau de socket (quelques Ko) | **< 1 Mo** | ✅ **retenu** |

Dans l'état `EN_ATTENTE`, la connexion **n'est pas lue** et **n'occupe aucun fil** : la boucle d'événements sait seulement qu'elle existe, quand elle a été acceptée et à quelle position. La conséquence directe : **la file d'attente ne consomme pas la ressource rare** (le temps CPU), elle consomme une ressource abondante (quelques kilo-octets).

### 11.7. Protections liées — la saturation n'est pas le seul épuisement possible

| Menace | Protection | Où |
|---|---|---|
| **Trame géante** (CWE-400) | Plafond **64 Kio** appliqué **avant allocation** ; longueur nulle ou excessive → refus **sans allocation** | protocole §3.1, §9 |
| **JSON profondément imbriqué** (CWE-674) | Profondeur maximale **32** (application **et** parseur, §5.3) ; rejet `-32700` | §5.3 |
| **Flot d'actions d'un client** | **8 demandes en vol** maximum par client (`-32001` au-delà) ; file de travail bornée à 256 | protocole §10, §11.2 |
| **Client qui ne lit pas** | Tampon sortant borné (256) + `client.resync` | §8.3 |
| **Minage sans fin** | Difficulté **calibrée** (3 ou 4) pour rester dans la fourchette **0,3–2 s** ; délai de garde à 2 × la borne haute (4 s) → refus `retryable` | §5.2.1 du CDC |
| **Journal qui remplit le disque** | Rotation bornée 10 Mo × 5 par programme | §10.8 |
| **Connexions zombies** | Battement de cœur 60 s, expiration à 3 battements, libération de la place | ENF-25, protocole §10 |
| **Tempête de reconnexions** | Attente croissante **avec jitter**, 3 tentatives maximum | §11.5 |

### 11.8. Réglages du système d'exploitation (la file du noyau n'est pas la nôtre)

**Deux files distinctes, à ne pas confondre** — c'est une question de jury classique :

| File | Où elle vit | Capacité | Ce qu'elle contient | Si elle déborde |
|---|---|---|---|---|
| **File d'acceptation du noyau** | Noyau | **minimum** entre le *backlog* demandé à `listen()` et `net.core.somaxconn` | Connexions TCP **établies mais pas encore `accept()`ées** par l'application | **Le client peut ne jamais voir sa connexion aboutir** : la connexion est perdue, sans message |
| **File d'admission AnimeTCG** (ENF-54) | Notre processus | **100 places** | Connexions **acceptées** qui attendent une place de service | **Refus explicite** : `-32002`, message clair, socket fermée |

**Conséquence de conception** : on `accept()` **toujours très vite** (la boucle d'événements ne fait que ça) et on met l'attente **dans notre file**, où elle est **bornée, message d'erreur compris**. C'est ce qui transforme une perte silencieuse en **refus explicite** — la différence entre un jury qui comprend et un jury qui constate une panne.

| Réglage | Valeur par défaut observée | Valeur requise pour la démonstration | Pourquoi |
|---|---|---|---|
| `net.core.somaxconn` | 128 (jusqu'à 4096 selon le noyau) | ≥ **128** (4096 si disponible) | Le backlog doit absorber les rafales d'`accept()` |
| Backlog passé à `listen()` | selon code | **128** | Au-delà, inutile : `somaxconn` borne |
| `tcp_max_syn_backlog` | 128–1024 | ≥ **128** | Connexions semi-ouvertes (poignée de main TCP en cours) |
| `RLIMIT_NOFILE` (`ulimit -n`) | souvent **1024** | ≥ **4096** | 100 sockets clients + fichiers de journal + connexions base : la marge est nécessaire |
| Horloge | — | NTP actif | Corrélation des journaux (A.8.17) |
| Vérification | — | `ss -lnt`, `nstat -az \| grep -E 'ListenOverflow\|ListenDrops'` | Un compteur qui monte = la file noyau déborde, donc notre `accept()` est trop lent |

### 11.9. Observabilité et journalisation de la saturation

| Compteur | Signification | Seuil d'alerte (revue humaine) |
|---|---|---|
| `clients_admis` / `clients_en_attente` | État courant | `en_attente > 0` pendant > 60 s |
| `profondeur_max_file` | Pic d'attente observé | > 50 (moitié de la borne) |
| `attente_max_ms` | Pire attente vécue | > 30 000 ms |
| `refus_file_pleine` | Nombre de `-32002` | Toute valeur > 0 mérite un mot en soutenance |
| `expirations_attente` | Nombre de `-32003` | > 0 = la file ne se vide pas assez vite |
| `actions_refusees_charge` | Nombre de `-32001` | > 0 = le pic de travail dépasse la capacité instantanée |
| `demandes_en_vol_max` | Pic de demandes simultanées | doit rester ≤ 8 × clients admis |

**Lignes de journal correspondantes** (format §10.3) — elles sont **le** point de preuve en soutenance :

```
2026-09-18T19:20:01.104Z | WARN | srv | c=101 | r=-            | net     | queue enqueue       | position=1 depth=1 admitted=100
2026-09-18T19:20:31.220Z | INFO | srv | c=101 | r=-            | net     | client admitted     | waited_ms=30116 depth=0
2026-09-18T19:21:02.007Z | WARN | srv | c=207 | r=-            | net     | queue full          | depth=100 refused_total=7
2026-09-18T19:21:44.511Z | WARN | srv | c=206 | r=-            | net     | queue timeout       | waited_ms=60004
2026-09-18T19:22:10.900Z | WARN | srv | -   | r=-            | business| work queue saturated | pending=256 action=card.create
```

### 11.10. Test de charge (REC-03)

Script `tests/load/queue_test.sh` : il lance **N clients de test** (client de test du dossier `tests/`, pas un « bot » dans l'arborescence — DEC-14) qui parlent le protocole comme de vrais clients.

| Phase | Scénario | Assertion attendue |
|---|---|---|
| **A** | 100 clients se connectent | 100 × `status=OK` + `session.welcome` ; `admis=100` |
| **B** | 1 client de plus | `status="QUEUED"`, `position=1` ; aucun accès métier accordé |
| **C** | 99 clients de plus (200 au total) | 100 admis + 100 en attente ; le **201ᵉ** reçoit `-32002 QUEUE_FULL` puis la socket est fermée |
| **D** | Un client en attente attend 61 s sans qu'une place se libère | `-32003 QUEUE_TIMEOUT` puis fermeture |
| **E** | 5 clients admis se déconnectent | les **5 premiers de la file** reçoivent `session.welcome`, **dans l'ordre d'arrivée** ; les positions des autres diminuent (`session.queue_update`) |
| **F** | Mesure de latence sur un client admis | **p95 ≤ 500 ms** hors minage ; aucune réponse perdue |
| **G** | Mesure de ressources pendant tout le test | **nombre de fils constant** (± 1), résident stable (± 10 %) — c'est la preuve du §11.6 |

**Ce que ce test ne couvre pas** (et pourquoi on le dit avant que le jury ne le demande) : 10 000 clients, réplication, répartition de charge entre plusieurs serveurs. Ces paliers sont décrits **en perspective** dans l'annexe E du cahier des charges, avec leurs goulots (descripteurs de fichiers, `max_connections` de PostgreSQL, taille de la chaîne) — ils ne sont **pas** implémentés en S3.

### 11.11. Hors périmètre de cette spécification

| Sujet | Pourquoi | Où c'est traité |
|---|---|---|
| Répartition de charge entre plusieurs serveurs | Perspective de montée en charge | Annexe E du CDC (🎁) |
| Priorités entre clients (un client « important » passe devant) | Complexité non justifiée : 100 clients **égaux** en démonstration ; l'équité FIFO est plus simple à prouver | — |
| Dégradation fonctionnelle (désactiver des fonctionnalités sous charge) | Nos actions sont courtes ; le goulot mesuré est le **minage**, déjà borné par le calibrage | §5.2.1 du CDC |
| Historique « 16 clients / refus explicite » | **Décision remplacée** par Q7.2 (100 clients + file d'attente) ; conservé uniquement dans le journal des versions | CDC §13 |

### 11.12. Conformité ENF-54 — vérification ligne à ligne

| Exigence (ENF-54) | Réalisée par | Preuve |
|---|---|---|
| 100 clients simultanés | Limite d'admission configurable | Test A |
| Au-delà : **mise en file d'attente** | État `EN_ATTENTE` + réponse `status="QUEUED"` | Test B |
| Réponse « **en attente, position N** » | Champs `position`, `queue_length` ; mises à jour par `session.queue_update` | Tests B, E |
| **Sans accès aux actions métier** | Seules `session.bye` et la réception d'événements sont permises en attente | Test B (toute action → `-32001` ou refus de séquence) |
| **Admission automatique**, dans l'**ordre d'arrivée** (FIFO) | File FIFO stricte, aucune priorité | Test E |
| File **bornée à 100** | Capacité configurable | Test C |
| Attente **maximale de 60 s** | Minutage + `-32003` | Test D |
| **Message d'erreur explicite** et **fermeture** au-delà | `-32002` / `-32003` avec message utilisateur | Tests C, D |
| Position, admission et refus **journalisés** (ENF-40) | Lignes `queue enqueue`, `client admitted`, `queue full`, `queue timeout` | §11.9 |
| La mise en attente **ne bloque pas** la boucle d'événements (ENF-18) | Aucun fil consommé, aucune lecture des sockets en attente | Test G |
| **Cas démontré par REC-03** | Test de charge complet | Tests A → G |

---
### 11.13. Pourquoi 100 et 60 s — le dimensionnement par la loi de Little

Les deux bornes du §11.2 ont été **ajoutées par l'équipe** (elles ne viennent pas du sujet). Elles doivent donc être **justifiées par un calcul**, pas par une intuition. Le calcul tient en une ligne et vient de la **loi de Little**, un résultat de la théorie des files d'attente que tout jury d'informatique reconnaît :

```
L = λ × W        L : nombre moyen de clients dans le système
                 λ : débit d'arrivée (clients par seconde)
                 W : temps moyen passé dans le système (secondes)
```

**Ce qui rend cette loi utilisable** : elle est **vraie quelles que soient** la distribution des arrivées et la discipline de service — donc elle s'applique à nos connexions, qui n'arrivent certainement pas selon une loi de Poisson bien propre. Une seule chose compte : que le système soit dans un régime **stable** (il ne s'accumule pas indéfiniment).

#### 11.13.1. Ce que la borne « 100 places / 60 s » dit exactement

| Grandeur | Valeur | Lecture |
|---|---|---|
| `L_max` (file d'admission) | **100** clients | Au-delà : `-32002 QUEUE_FULL` |
| `W_max` (attente tolérée) | **60 s** | Au-delà : `-32003 QUEUE_TIMEOUT` |
| Débit d'arrivée absorbable `λ_max = L_max / W_max` | **1,67 client/s** = **100 clients par minute** | C'est **la capacité de pointe** de la file : tant que moins de 100 nouvelles connexions arrivent par minute **et que personne ne quitte**, la file ne déborde pas |

Autrement dit : **la file encaisse un pic d'arrivée de 100 personnes sur une minute**, ce qui correspond exactement au scénario réel — une salle de TP entière qui se connecte au début de la séance. C'est la justification de la borne haute.

#### 11.13.2. La conséquence, énoncée franchement

Une file d'attente ne crée pas de places : elle **transmet** celles qui se libèrent. Le débit d'admission est donc borné par le **débit de sortie**, lui-même donné par la durée moyenne d'une session `D` (100 places occupées, sorties toutes les `D/100` secondes en moyenne) :

| Durée moyenne de session `D` | Débit de sortie (admissions/s) | Clients admis en 60 s d'attente | Ce que cela signifie pour un pic de 100 clients en attente |
|---|---|---|---|
| 2 min (démonstration courte, on se déconnecte souvent) | 0,83 | ≈ **50** | La moitié du pic est absorbée |
| 5 min (séance de TP réaliste) | 0,33 | ≈ **20** | Le premier cinquième passe ; le reste reçoit `-32003` |
| 20 min (postes laissés ouverts) | 0,08 | ≈ **5** | La file joue le rôle d'un **tampon de courte durée**, pas d'une salle d'attente |

**Ce n'est pas un défaut caché, c'est un arbitrage assumé** : le §11.2 l'écrit noir sur blanc — *« au-delà de 60 s, mieux vaut un refus clair qu'une attente qui ressemble à une panne »*. Un client qui reste 4 minutes devant un écran « en attente » conclut que le système est cassé. Un client qui reçoit « la salle est pleine, réessayez dans une minute » a une **information exploitable**.

#### 11.13.3. Le réglage inverse (option B — **non retenue**)

Si l'équipe préfère que **tout un pic de 100 clients finisse par être servi**, la loi de Little donne la condition :

```
W_max ≥ 100 / (débit de sortie) = D          →   attendre au moins la durée typique d'une session
```

Concrètement : avec des sessions de 5 minutes, il faudrait une attente maximale de **300 s** — soit exactement la valeur d'inactivité déjà retenue par ailleurs (ENF-25). Deux options, donc, et c'est le point ouvert n° 1 du §15.2 :

| Option | `W_max` | Ce que gagne le client | Ce que perd le serveur |
|---|---|---|---|
| **A — telle que décidée** | **60 s** | Refus clair, jamais d'attente zombie | Un pic important n'est absorbé qu'en partie |
| **B — alignée sur l'inactivité** | **300 s** | La quasi-totalité d'un pic finit par entrer | Une place d'attente immobilisée 5 min ; un client peut croire à une panne ; 100 sockets dormantes au lieu de 0 |

**Aucune des deux n'est fausse**, et c'est pour cela que le document les présente au lieu d'en choisir une en douce : la décision appartient à la réunion d'équipe, et le code prend la valeur dans **un seul** fichier de configuration.

#### 11.13.4. La même loi appliquée aux 100 places d'admission

`100 places = λ × D` : avec des sessions de 5 minutes, le serveur admet en régime stable **100 / 300 s = 0,33 client par seconde**, soit **un nouveau client toutes les 3 secondes**. C'est très au-dessus de ce qu'exige une séance de TP (4 à 8 connexions au total) — la limite de 100 n'est donc **pas** contraignante pour l'usage réel : elle sert à **borner** le système (mémoire, sockets, fils), pas à limiter les utilisateurs.

> **En une phrase pour la soutenance** : « 100 places et 60 secondes ne sont pas deux chiffres posés au hasard : leur rapport fixe la capacité de pointe de la file à 100 connexions par minute, soit exactement une salle de TP complète qui se connecte d'un coup. »

### 11.14. Invariants, équité FIFO et runbook de démonstration

#### 11.14.1. Les invariants vérifiables

Ils sont écrits pour être **testés** (assertions dans le test de charge, §11.10) et **cités** en soutenance :

| # | Invariant | Formulation vérifiable | Vérifié par |
|---|---|---|---|
| **I1** | Bornes jamais dépassées | `admis ≤ 100` **et** `en_attente ≤ 100`, à tout instant | Tests A, B, C |
| **I2** | Aucune place perdue | `places_libérées == admissions` en fin de scénario (un départ ⇒ une admission, ou une place vide) | Test E |
| **I3** | Ordre strict | Les positions forment toujours la séquence **1..n** sans trou, et la position d'un client **ne remonte jamais** | Test E |
| **I4** | Équité | Aucun client ne peut être admis avant un client arrivé plus tôt et encore en attente | Test E |
| **I5** | Étanchéité | Un client en attente **ne peut pas** provoquer d'écriture métier (aucun bloc créé pour lui) | Test B |
| **I6** | Traçabilité | Tout refus (`-32002`), tout dépassement (`-32003`) et toute admission ont **une ligne de journal** | Tests C, D, §11.9 |
| **I7** | Non-blocage | Le **nombre de fils reste constant** et la latence des clients admis reste ≤ 500 ms pendant toute la mesure | Tests F, G |

#### 11.14.2. L'équité FIFO, démontrée plutôt qu'affirmée

Une structure de données unique porte toute la preuve — c'est l'argument à donner au jury :

| Élément | Implémentation | Propriété obtenue |
|---|---|---|
| Structure | **Une seule** file (tableau circulaire d'indices de connexion), pas de liste par état | Pas de chemin alternatif : un client ne peut pas « doubler » par un autre chemin |
| Entrée | **Queue** uniquement (`enqueue`), au moment de la réception de `session.hello` | L'ordre d'entrée est l'ordre d'arrivée **observé par le serveur** |
| Sortie | **Tête** uniquement (`dequeue`), appelée quand une place se libère | Le premier arrivé est le premier servi |
| Priorités | **Aucune** : ni par identifiant, ni par ancienneté d'utilisateur, ni par type de carte | Aucun paramètre ne peut inverser deux positions |
| Position annoncée | Indice dans la file, recalculé à chaque départ | Le client voit son rang **réel** |
| Retrait | Départ **ou** expiration : le retrait en milieu de file **ne réordonne personne** (les indices suivants reculent de 1, et le client en reçoit la nouvelle valeur par `session.queue_update`) | L'ordre relatif des présents est préservé |

**Ce que FIFO ne garantit pas** — et il faut le dire avant qu'on ne nous le reproche : être premier dans la file ne garantit pas d'être servi **dans un temps borné**, si personne ne quitte. C'est exactement pour cela que la borne `W_max` (60 s) **et** le code `-32003` existent : ils transforment une attente potentiellement infinie en **fin explicite**. Beaucoup de projets oublient cette borne et laissent des clients « en attente » pour toujours ; c'est le défaut que ce paragraphe évite.

#### 11.14.3. Runbook — démonstration de la saturation en soutenance

| Étape | Ce qu'on fait | Ce qu'on doit voir (client) | Ce qu'on montre (serveur) |
|---|---|---|---|
| 1 | Lancer le serveur | Démarrage, banc de calibrage : `hash_rate=… difficulty=…` | §8.6 |
| 2 | Connecter **100 clients de test** (`tests/load/queue_test.sh --phase A`) | `session.welcome` ×100 | `client admitted` ×100, `admis=100` |
| 3 | Connecter le **101ᵉ** client (fenêtre graphique) | Écran **« En attente — votre position : 1 »** | `queue enqueue position=1` |
| 4 | Connecter 99 clients de plus | Positions 2 → 100 ; le **201ᵉ** reçoit « salle pleine » | `queue full`, fermeture de socket |
| 5 | Déconnecter 5 clients admis | Les **5 premiers** reçoivent `session.welcome` **dans l'ordre** ; positions mises à jour | `client admitted` ×5, `queue_update` |
| 6 | Laisser un client en attente 61 s | « Attente dépassée, réessayez » (`-32003`) | `queue timeout` |
| 7 | Mesurer pendant tout l'exercice | Latence des admis ≤ 500 ms | `client.connected`, latence p95, **nombre de fils** affiché par le moniteur |
| 8 | Conclure | — | « 200 connexions ouvertes, **un seul fil** d'acceptation, 100 admis, 100 en file, et la latence des admis n'a pas bougé. » |

**Le piège à éviter en démonstration** : lancer les 100 clients de test **et** garder 100 fenêtres graphiques ouvertes. Le runbook utilise **un seul** poste graphique (celui du §3, étape 3) et des clients de test sans interface pour la charge : cela rend la démonstration lisible et évite de faire ramer la machine de présentation.

---


## 12. Sécurité, conformité et licences

### 12.1. Modèle de menace (méthode STRIDE)

| Menace | Application à AnimeTCG | Parade en place | Statut |
|---|---|---|---|
| **Spoofing** (usurpation) | Un client se déclare sous le pseudo d'un autre | **Hors périmètre et assumé** : aucun mot de passe, le pseudo n'est **pas** une identité vérifiée (décision de périmètre, §3.2 du CDC). Conséquence : les rôles (propriétaire, porteur légitime) sont **simulés**, ce qui est explicitement demandé par le sujet | ⚠️ assumé |
| **Tampering** (altération) | Modification d'une ligne de la base pour s'attribuer une carte ou une valeur | Chaîne **vérifiable** (hash + `previous_hash`), **difficulté incluse dans le hash** donc non modifiable discrètement ; vérification complète en phase 4 (ENF-47, REC-16) | ✅ |
| **Repudiation** (négation) | Un utilisateur nie avoir recommandé, revendiqué, voté | Chaque action acceptée est inscrite dans **un bloc horodaté** ; l'historique ne se réécrit pas (append-only) | ✅ |
| **Information disclosure** | Fuite par les journaux, le dépôt ou une réponse trop bavarde | Journaux minimisés et assainis (§10.6, §10.7) ; aucune donnée personnelle inutile ; messages d'erreur génériques côté utilisateur (§7.2) ; pas de secret dans le dépôt (§12.6) | ✅ |
| **Denial of service** | Saturer le serveur : 200 connexions, trames géantes, JSON profond, flot d'actions | **§11** : file bornée, rejet explicite, tampon 64 Kio, profondeur 32, 8 demandes en vol, journal borné, minage borné | ✅ |
| **Elevation of privilege** | Contourner les règles depuis le client | Le client **ne parle qu'au serveur** (ENF-27) ; aucune action hors catalogue ; les valeurs sont **recalculées** (ENF-45) ; les contraintes de base sont la seconde ligne de défense (§6.2) | ✅ |

### 12.2. Défense en profondeur — trois lignes, et ce qu'elles couvrent

| Ligne | Où | Ce qu'elle arrête | Référence |
|---|---|---|---|
| **1. Validation du protocole** | `protocol` (forme : types, bornes, listes fermées) + `business` (règles) | Entrées malformées, valeurs hors bornes, actions inconnues, abus de séquence | ENF-44, ENF-45, protocole §12 |
| **2. Contraintes déclaratives** | PostgreSQL (`CHECK`, `UNIQUE` partiels, clés étrangères, `NOT NULL`) | Ce qui aurait échappé au code : une règle « une seule ligne active » tient même si un contrôle applicatif est oublié | ISO/IEC 9075, **DOC-07** |
| **3. Journalisation et vérification** | `util/log` + vérification de chaîne | Détection *a posteriori* : modifier la base **après** coup laisse une trace détectable ; l'investigation est possible | ENF-47, §10 |

**Faiblesses connues visées** (CWE) et traitement :

| CWE | Nom | Traitement |
|---|---|---|
| **CWE-20** | Validation d'entrée insuffisante | ENF-44 : types, bornes, longueurs, listes fermées, côté serveur exclusivement |
| **CWE-89** | Injection SQL | ENF-46 : requêtes **paramétrées** uniquement ; le `Makefile`/la revue interdisent toute concaténation |
| **CWE-117** | Injection de journal | `log_sanitize` (§10.6) + test dédié (§10.12, test 5) |
| **CWE-400 / CWE-770** | Épuisement de ressources | Toutes les files et tampons sont **bornés** : §11.2, §11.7 |
| **CWE-532** | Données sensibles dans un journal | Liste d'interdits (§10.7) + test 6 |
| **CWE-674** | Récursion non contrôlée | Profondeur JSON maximale 32, côté application **et** parseur (§5.3) |

### 12.3. Ce qui n'est **pas** protégé — et pourquoi c'est écrit noir sur blanc

Un document de sécurité qui ne liste pas ses limites n'est pas un document de sécurité. Sont **hors périmètre, consciemment** :

| Limite | Raison | Ce qu'il faudrait en production |
|---|---|---|
| **Pas de chiffrement du canal** | Décision Q6.1 (option A) : usage local, réseau de l'IUT, coût de mise en place élevé pour un bénéfice nul dans ce contexte | Une couche de chiffrement et une authentification mutuelle |
| **Pas d'authentification utilisateur** | Périmètre fixé par le sujet : les rôles sont simulés | Comptes, mots de passe hachés, sessions expirantes |
| **Pas de protection contre un utilisateur local du poste serveur** | Qui a un accès physique au serveur peut lire la base et les journaux | Contrôle d'accès système, chiffrement au repos, sauvegardes hors site |
| **Pas de contrôle d'intégrité du journal** (le journal n'est pas signé) | Complexité disproportionnée en S3 ; l'intégrité forte existe côté **chaîne** (elle, vérifiable) | Copie syslog vers un hôte distant en écriture seule, scellement périodique |
| **Pas de test d'intrusion** | Hors périmètre pédagogique et temporel | Audit externe, tests d'intrusion, analyse statique industrielle |

> **Formulation à réutiliser devant le jury** : *les données fonctionnelles sont protégées par la chaîne et la validation ; le canal et l'identité sont hors périmètre par décision documentée. Ce n'est pas un oubli, c'est un arbitrage écrit, avec sa conséquence : la plateforme est un jeu local, pas un service exposé.*

### 12.4. RGPD et CNIL — ce qui a été vérifié

| Question | Réponse retenue | Source |
|---|---|---|
| Quelles données personnelles sont traitées ? | **Pseudo** (choisi par l'utilisateur) et **adresse IP** (journal `DEBUG` du serveur uniquement) ; **aucune** donnée sensible (santé, origine, opinion), aucun identifiant officiel | Minimisation, RGPD art. 5.1.c |
| Pourquoi les journaliser ? | **Sécurité du traitement** : détection d'abus, investigation d'incident — c'est la finalité légitime de la journalisation | RGPD art. 32 ; recommandation CNIL sur la journalisation |
| Combien de temps ? | **6 mois** maximum (purge automatique), aligné sur la recommandation CNIL pour les journaux d'accès applicatifs | Délibération **2021-122** |
| Contenu minimal d'une ligne ? | **Identifiant de l'auteur, date et heure, nature de l'action** — exactement `c=`, horodatage, action/module | Recommandation CNIL |
| Y a-t-il des traceurs, des cookies, des transferts hors UE ? | **Non** : application de bureau, réseau local, aucun service tiers appelé | — |
| Qui accède aux journaux ? | Les membres de l'équipe (et l'encadrant jusqu'au 22/09/2026), sur les postes du projet | Droits `0640`, dossier non versionné |

> **Précision honnête** : en S3, **aucun utilisateur externe réel** n'est traité — les données sont celles des membres de l'équipe pendant les tests. Le cadre ci-dessus est appliqué quand même, parce qu'il coûte peu et qu'il rend la démonstration défendable : le jour où quelqu'un demande « et le RGPD ? », la réponse est écrite, chiffrée et sourcée.

### 12.5. Licences des composants (inventaire à tenir à jour)

| Composant | Rôle | Licence (SPDX) | Obligation pratique | Vérifié |
|---|---|---|---|---|
| **zlog** | Journalisation C | **LGPL-2.1** | **Liaison dynamique** (`-lzlog`) ; pas de modification de la bibliothèque ; si on la modifie, les modifications se publient. Notre code reste sous notre licence | ✅ (documentation du projet) |
| **rxi/log.c** (repli) | Journalisation C | **MIT** | Conserver la notice de copyright dans le fichier vendu dans le dépôt | ✅ |
| **OpenSSL** (`libcrypto`) | SHA-256 | **Apache-2.0** (3.x) | ✅ **Autorisée nommément par le sujet** (§10) | ✅ |
| **jansson** | JSON (C) | **MIT** | ✅ **Déclarée** (choix libre, 22/09/2026) — ENF-59 ; notice conservée | ✅ |
| **cJSON** (repli) | JSON (C) | **MIT** | Idem | ✅ |
| **SLF4J** | Façade de journalisation Java | **MIT** (type X11) | Aucune | ✅ |
| **Logback** | Moteur de journalisation Java | **EPL-1.0** *ou* **LGPL-2.1** (double licence, on choisit) | Aucune en usage interne | ✅ |
| **JavaFX** | IHM | **GPL-2.0 *avec* exception Classpath** | L'exception autorise l'usage en bibliothèque sans contaminer notre code ; **ne pas modifier** JavaFX | ✅ |
| ~~**libuv**~~ | ~~E/S asynchrones~~ | MIT | — | ❌ **Écartée le 22/09/2026** (décision d'équipe) : la boucle d'événements est écrite à la main avec **`epoll`**, conformément à la contrainte « sockets de A à Z » du sujet |
| **libpq / PostgreSQL** | Base de données | **PostgreSQL License** (type MIT) | Aucune | ✅ |
| **Maven, JUnit 5, JaCoCo, gcovr, Podman, Git, GitLab CE** | Outillage | Licences libres (Apache-2.0 / EPL-2.0 / permissives) | Aucune ; **aucune n'impose de publier notre code** | 🔧 à confirmer au dépôt (fichier `LICENSE` de chaque projet) avant le tag `phase3` |

**Contexte juridique du projet** : le dépôt est un **dépôt GitLab privé d'établissement**, non distribué, à usage pédagogique, **budget 0 €**. Les obligations de **redistribution** (fournir les sources, notices, offres de reliage) ne se déclenchent pas dans ce cadre ; **la seule obligation structurelle est la liaison dynamique de `zlog`** (LGPL), qui est de toute façon la façon normale de l'utiliser. On garde malgré tout l'inventaire à jour, parce que c'est trois minutes et que c'est ce qu'on attend d'une équipe sérieuse.

**Action de la checklist** : au moment du tag `phase3`, ouvrir `THIRD-PARTY.md` (une ligne par dépendance : nom, version, licence, où c'est utilisé) et faire confirmer les licences marquées 🔧.

### 12.6. Secrets, configuration et dépôt

| Règle | Application |
|---|---|
| **Aucun secret dans le dépôt** | Pas de mot de passe de base, pas de clé, pas de jeton. `server.ini` versionné avec des **valeurs de développement**, surchargé localement (`server.local.ini`, ignoré par Git) |
| **`.gitignore`** | `logs/`, `target/`, `*.o`, `build/`, `server.local.ini`, `client.properties`, dumps `*.sql` de test |
| **Configuration versionnée** | `zlog.conf`, `logback.xml` **sont** versionnés : ce sont des choix de conception, pas des secrets |
| **Revue avant fusion** | Relecture croisée obligatoire ; convention de commits `type(scope): description` en anglais (ENF-39) |

---

## 13. Qualité, tests et intégration continue

### 13.0. Principes SOLID et patrons de conception (exigence de phase 3)

Le sujet de phase 3 demande explicitement de **réorganiser le code à partir de l'expérience acquise** en appliquant les **principes SOLID** et, « lorsqu'ils répondent à un besoin identifié », des **patrons de conception** — avec justification. Voici ce que le projet applique **déjà**, et où c'est visible :

| Principe / patron | Où il s'applique | Problème résolu |
|---|---|---|
| **S** — responsabilité unique | Une couche = un rôle (§4.4) ; `protocol` valide la forme, `business` décide, `persistence` écrit | Un changement de règle métier ne touche pas le décodage des trames |
| **O / L** — ouverture/fermeture, substitution | Table de dispatch des actions (`protocol/actions.c`) : ajouter une action = ajouter une entrée | Pas de `switch` géant à modifier dans trois fichiers |
| **I** — ségrégation des interfaces | `chain_api.h` (2 fonctions) et `store_api.h` (2 fonctions) au lieu d'une interface « serveur » fourre-tout | Le métier ne dépend pas des 20 fonctions de la persistance |
| **D** — inversion de dépendance | `business` **possède** ses interfaces ; `chain` et `persistence` les implémentent (§4.4) | Le métier se teste avec des doublures, sans base ni socket |
| **Stratégie / façade** | `util/log.h` (façade sur `zlog`, repli `rxi/log.c`), `protocol/json_io.h` (façade sur `jansson`), `chain/sha256.h` (façade sur OpenSSL) | Changer une bibliothèque non autorisée = changer **un fichier** |
| **MVC** (patron imposé) | Client Java (ENF-50) | La vue ne contient aucune règle métier |
| **Fabrique** | `session` : création des objets de connexion et de leur cycle de vie en un seul endroit | Évite les fuites de descripteurs et l'oubli de libération des places d'admission |

**Ce qui n'est pas fait, et pourquoi** : pas de patron « visiteur » ni de hiérarchies d'héritage exotiques côté C (le langage s'y prête mal et le besoin n'existe pas) ; pas de conteneur d'injection de dépendances (le câblage tient en une page de `main.c`). **Un patron appliqué sans besoin est un coût, pas une qualité** — et le sujet demande précisément de justifier.

### 13.1. Objectifs de qualité (modèle ISO/IEC 25010)

| Caractéristique | Comment elle se voit dans ce projet | Objectif mesurable |
|---|---|---|
| **Adéquation fonctionnelle** | Les 27 cas d'utilisation du sujet, dont les règles de valeur, de battle, de revendication | 100 % des CU de la démonstration passent |
| **Performance** | Réponse aux actions, minage calibré, montée à 100 clients | p95 ≤ **500 ms** hors minage ; minage dans la **fourchette 0,3–2 s** ; 100 clients sans perte |
| **Fiabilité** | Reconnexion, redémarrage, base indisponible | Reconstruction du contexte ≤ **10 s** pour 1000 blocs (REC-02) ; service maintenu sans base |
| **Sécurité** | §12 | 0 échec sur les tests 5 et 6 du §10.12 |
| **Maintenabilité** | Fonctions ≤ 200 lignes, commentaires en anglais, un test par fonction, arborescence claire | 0 fonction > 200 lignes ; couverture ≥ **60 %** global, ≥ **80 %** critique (phase 3) |
| **Portabilité** | Debian de l'IUT, Podman comme filet | Installation en ≤ 15 min sur un poste neuf, procédure écrite |

### 13.2. Niveaux de test et volumétrie

| Niveau | Ce qui est testé | Volumétrie cible (phase 2) | Outil |
|---|---|---|---|
| **Unitaire C** | `chain` (hash, minage, vérification), `protocol` (décodage, bornes, robustesse), `business` (règles pures) | **≥ 30 tests** | CUnit ou harnais maison + `gcov`/`gcovr` |
| **Unitaire Java** | Modèle, corrélateur, codec, contrôleurs (sans IHM) | **≥ 15 tests** | JUnit 5 + JaCoCo |
| **Intégration** | Serveur + PostgreSQL + client de test : une action de bout en bout, la transaction bloc+projection, la restauration | ≥ 5 scénarios | scripts `tests/integration/` |
| **Bout en bout (e2e)** | Client Java réel ↔ serveur : connexion, création, recommandation, battle, revendication | **≥ 5 scénarios** | client de test + captures |
| **Charge** | File d'attente, saturation, latence | 7 phases du §11.10 | `tests/load/queue_test.sh` |
| **Journalisation** | 10 tests du §10.12 | 10 | scripts `tests/logs/` |

**Règle de fond (ENF-36)** : *une nouvelle fonction arrive avec son test*. Ce n'est pas une recommandation, c'est la **définition de fini d'un lot** : une fonction sans test n'est pas livrable, donc n'est pas « finie ».

### 13.3. Couverture et outillage

| Élément | Choix | Statut |
|---|---|---|
| Couverture C | `gcov` (profondeur) + `gcovr` (rapport lisible, seuils par fichier) | **choix d'équipe**, non imposé par l'encadrant |
| Couverture Java | **JaCoCo** via `jacoco-maven-plugin` | **choix d'équipe**, non imposé |
| Seuils | Phase 2 : **aucun seuil exigé** (cadrage de l'encadrant) ; Phase 3 (4 janvier 2027) : **≥ 60 % global**, **≥ 80 %** sur les modules critiques (`chain`, `protocol`, `business`) | 🔧 |
| Analyse statique | `gcc -Wall -Wextra -Werror` ; `-fsanitize=address,undefined` en développement | 🔧 |
| Qualité de fond / dette technique | **SonarQube** relève du module **R402 (S4)**, pas de cette SAÉ | hors périmètre |

### 13.4. Pipeline d'intégration continue (ENF-49)

| Étape | Contenu | Échec = |
|---|---|---|
| 1. **Compilation C** | `make` en mode strict (`-Werror`) | fusion bloquée |
| 2. **Compilation Java** | `mvn -q compile` | fusion bloquée |
| 3. **Tests unitaires C** | `make test` + couverture `gcovr` | fusion bloquée |
| 4. **Tests unitaires Java** | `mvn test` + couverture JaCoCo | fusion bloquée |
| 5. **Contrôles de forme** | `scripts/check-style.sh` : fonctions ≤ 200 lignes, aucun `printf` de diagnostic, format de journal (regex §10.3) | fusion bloquée |
| 6. **Tests d'intégration** | Compose Podman (PostgreSQL + serveur), scénarios `tests/integration/` | fusion bloquée |
| 7. **Artefacts** | Rapports de couverture, journaux du test, binaire serveur et jar client | publiés dans GitLab |

**Le pipeline est visible dans le dépôt** (badge d'état) : c'est la preuve ENF-49 demandée par l'encadrant.

---

## 14. Traçabilité — exigence → section → preuve

| Exigence | Où c'est spécifié | Comment on le prouve |
|---|---|---|
| **ENF-05** (serveur, unique écrivain) | §4.2, §8.1 | Relecture : un seul module (`persistence`) ouvre la base |
| **ENF-18** (boucle d'événements non bloquante) | §8.1, §8.2, §10.10 | Test 7 du §10.12 ; test G du §11.10 (nombre de fils constant) |
| **ENF-19 → ENF-22** (protocole défini, corrélation, événements, robustesse) | §7.1, **§7.4** + `docs/protocole-applicatif.md` | **Relecture d'avant-code : 23 actions et 30 codes validés** (§7.4.6, signature de l'équipe) ; tests unitaires `protocol` (**11 cas** de robustesse, §9 du protocole) |
| **ENF-23 → ENF-27** (trame, version, inactivité, configuration, étanchéité) | §7.1, §5.2 | Tests `frame_read` (64 Kio, longueur nulle, tronqué) |
| **ENF-28 → ENF-32** (persistance) | §6.1 → §6.3 + DOC-07 | Test d'intégration : transaction bloc + projection ; test de restauration ≤ 10 s |
| **ENF-33 → ENF-36** (taille, commentaires, en-têtes, un test par fonction) | §5.1 | Étape 5 du pipeline |
| **ENF-40** (journalisation structurée) | §10 | Tests 1 à 4 du §10.12 |
| **ENF-42** (IHM réactive) | §7.2, §8.1 | Test manuel filmé : action lente, IHM utilisable |
| **ENF-44 → ENF-47** (validation, confiance, SQL paramétré, intégrité) | §12.2, §6.3 | Tests `business` ; démonstration d'altération en phase 4 |
| **ENF-49** (intégration continue) | §13.4 | Badge du pipeline dans le dépôt |
| **ENF-50** (MVC) | §4.3 + DOC-03 | Relecture : `ServerConnection` seul point réseau |
| **ENF-52** (difficulté dans le hash) | **§6.5** | **Vecteurs dorés §6.5.6** (hash de `""`/`"abc"`, en-tête + nonce 13 423) ; test d'altération : `difficulty` 4 → 3 ⇒ hash différent **au bloc modifié** et chaîne rompue **au bloc suivant** |
| **ENF-51 / ENF-53** (banc de calibrage, réévaluation amortie) | **§8.6 → §8.8** | Ligne de journal `calibration` (mesure ≤ 250 ms) ; `difficulty.changed` après 2 mesures hors bande ; démonstration REC-15 (natif contre bridé) |
| **ENF-54** (saturation, file d'attente) | **§11**, dont **§11.13** (dimensionnement) et **§11.14** (invariants, équité FIFO, runbook) | Tests A → G du §11.10 (REC-03) et invariants **I1 → I7** |
| **ENF-55 → ENF-58** (journalisation, format, non-blocage, réglage) | **§10** | Tests 1 → 10 du §10.12 (REC-17) |
| **REC-02** (restauration ≤ 10 s / 1000 blocs) | §6.2, §8.4 | Chronométrage sur un jeu de 1000 blocs |
| **CU-20 / EF-20** (authentification d'une carte) | **§5.2** (contrat), §6.1 (entité + projection `authentications`), **§7.4 ligne 20** | **Test unitaire `authentication`** (légitime / non légitime / déjà authentifiée / révocation) + test de chaîne ; démonstration : étape « authentification » du §9.2 du CDC |
| **ENF-59** (bibliothèques) | **§5.3.1, §5.3.2** | Tableau de **déclaration** (usage, statut, licence, repli) ; **aucune autorisation à demander** (choix libre, 22/09/2026) |
| **ENF-60** (images, licences et crédits) | §12.5 et §5.9.2 du CDC | `CREDITS.md` présent, vérifié en revue avant le tag `phase3` |
| **ENF-61** (SOLID et patrons) | **§13.0** | Revue croisée : chaque patron cité correspond à un endroit du code |
| **ENF-33 / ENF-50** (architecture en couches, MVC) | **§4.4 → §4.6** + `docs/uml/08-classes-client.puml` | Étape 5 du pipeline (`scripts/check-layers.sh`) : aucune inclusion interdite, aucun cycle ; **Ca/Ce/I** relevés une fois par phase |
| **REC-13** (cas d'erreur explicites) | §10.4, §12.2 | Tests de robustesse ; **30 codes** du protocole §8, dont les **17 cas de recette** du §12.1 du CDC (correspondance cas → code vérifiée en revue ; **8 cas prioritaires** désignés par l'équipe) |
| **REC-16** (difficulté non falsifiable) | §6.3 | Test d'altération |
| **REC-17** (journaux fusionnables, action traçable) | §10.3, §10.5, §10.12 | Test 4 : 20 identifiants de demande retrouvés des deux côtés |

---

## 15. Décisions d'ingénierie (ADR condensées) et paramètres retenus

### 15.1. Décisions

| # | Décision | Contexte | Conséquences (+ / −) | Alternative écartée |
|---|---|---|---|---|
| **ADR-01** | Trame à **préfixe de longueur** (4 octets big-endian), plafond 64 Kio | TCP n'a pas de frontières de message | **+** lecture en une passe, pas d'échappement, attaque par longueur impossible ; **−** un octet corrompu décale la lecture (mais borne la casse à une trame) | Délimiteur de fin (fragile), format binaire maison |
| **ADR-02** | **Un séquenceur unique** applique les actions métier | La chaîne doit rester déterministe | **+** aucun conflit d'index de bloc, raisonnement simple ; **−** la charge métier n'est pas parallèle (le parallélisme est dans l'E/S) | Un fil par client côté métier (index de bloc en course) |
| **ADR-03** | **Chaîne = source de vérité**, projections écrites **dans la même transaction** | Deux écritures séparées peuvent diverger | **+** restauration rapide, cohérence garantie ; **−** une transaction par action, plus lente qu'un `INSERT` seul | Tout relire depuis la chaîne à chaque démarrage (lent), ou ne garder que les projections (invérifiable) |
| **ADR-04** | **jansson** comme bibliothèque JSON, profondeur max **32** | Il faut décoder du JSON venu du réseau | **+** limite de profondeur documentée et configurable, licence MIT ; **−** une dépendance de plus à installer | cJSON (vulnérabilités 2026 sur les utilitaires, limite à 1000 par défaut) |
| **ADR-05** | Journalisation **`zlog` + SLF4J/Logback**, format unique, MDC des deux côtés | Deux langages, un seul journal à lire | **+** fusion par tri, corrélation par `request_id` ; **−** une bibliothèque C à installer (repli `rxi/log.c` documenté) | `printf`/`println` (non filtrables), `syslog(3)` seul (pensé pour le système) |
| **ADR-06** | File d'**admission bornée** (100 / 60 s) + rejet explicite | 100 clients simultanés est la cible | **+** refus clair, mémoire bornée, zéro fil consommé ; **−** un client peut être refusé alors que le serveur « va » se libérer | File non bornée (effondrement), refus immédiat sans file (perte d'utilisateurs sur un pic) |
| **ADR-07** | Minage **non parallélisé**, difficulté **3 ou 4** calibrée, **temps visé 0,3–2 s** | Éviter d'écraser le processeur en démonstration | **+** débit prévisible, ≤ 1 s ; **−** un bloc à la fois, latence visible | Paralléliser (fausse le calibrage, chauffe la machine) |
| **ADR-08** | **Horloge UTC** pour dater, **horloge monotone** pour mesurer | Un ajustement NTP fausse les durées | **+** corrélation correcte, durées justes ; **−** deux API à ne pas confondre (test de revue) | `clock()` seule, temps local |
| **ADR-09** | Niveau de journalisation **`INFO` par défaut**, réglable sans recompiler | Trop de lignes = journal inutilisable ; trop peu = pas de preuve | **+** volume maîtrisé ; **−** il faut penser à repasser en `DEBUG` pour un diagnostic fin | Compilation conditionnelle (oblige à reconstruire en démonstration) |
| **ADR-11** | **Charge utile canonique** stockée à part (`payload_canon`, RFC 8785 « JCS ») et **en-tête de bloc de 96 octets** à taille fixe | Le hash porte sur des **octets**, pas sur du sens ; `JSONB` réordonne les clés | **+** la chaîne reste vérifiable des années après, la difficulté variable est protégée (ENF-52) ; **−** une colonne supplémentaire, deux représentations à écrire **dans la même transaction** | Hacher le texte JSON affiché, ou recalculer la charge utile depuis `JSONB` (⇒ vérification impossible) |
| **ADR-12** | **Architecture en couches** contrôlée automatiquement (`net → protocol → business → chain/persistence → util`), inversion de dépendance **limitée** à `business` | Quatre développeurs, deux langages, intégration en fin de phase | **+** règle vérifiable en CI, métier testable avec des doublures ; **−** un script de plus à maintenir, discipline d'inclusion à tenir | Architecture hexagonale complète (indirection inutile à cette échelle), ou absence de règle (dette silencieuse) |
| **ADR-13** | **SHA-256 délégué à OpenSSL** (`-lcrypto`), `chain/sha256.c` réduit à une façade | Le sujet **autorise nommément** OpenSSL pour le hachage, et n'autorise que lui | **+** implémentation accélérée (SHA-NI), un code cryptographique de moins à défendre, débit réel mesuré par le banc ; **−** une dépendance binaire de plus, et la valeur ajoutée se déplace vers la **sérialisation canonique** (§6.5) | Écrire soi-même SHA-256 (fait en v0.1 → v0.3, conformité FIPS vérifiée : devient inutile dès lors que la bibliothèque est autorisée) |
| **ADR-14** | L'**authentification d'une carte** est un **attribut** (`authentications`, une seule active), **pas un état de carte**, et **n'a aucun effet sur la valeur** | Le sujet 2026-2027 exige l'authentification (CU-20) ; les états sont déjà `ACTIVE`/`EN_BATTLE`/`INACTIVE`/`ARCHIVEE` | **+** pas d'explosion combinatoire des états (sinon chaque état exigerait une variante authentifiée), pas de fabrication de valeur par un porteur légitime, unicité garantie par un index partiel ; **−** l'IHM doit lire deux sources (carte + authentification) pour afficher le badge | Un état `AUTHENTIFIEE` (impossible à combiner avec `EN_BATTLE` sans doubler les états), ou un bonus de valeur (facile à abuser) |
| **ADR-10** | **`sql/schema.sql`** est la référence, le diagramme en est l'illustration | Un diagramme dérive vite | **+** le DDL est vérifiable (analyse syntaxique), le diagramme sert à expliquer ; **−** deux artefacts à tenir cohérents (revue) | Diagramme seul (jamais exécutable) |

### 15.2. Paramètres retenus — **tous tranchés le 22/09/2026**

> ✅ **Il n'y a plus de point ouvert.** L'équipe n'ayant plus d'encadrant, **toutes les valeurs par défaut ont été retenues comme valeurs finales** (décision du 22/09/2026), **à une exception près** : la cible de temps de minage, passée de « ≤ 1 s » à la **fourchette 0,3 s – 2 s**.
> ⚠️ Toute modification ultérieure de ces valeurs se fait par **version mineure** de ce document, avec justification (le §15.1 en garde les ADR).

| # | Point | Valeur **retenue** | Qui | Statut |
|---|---|---|---|---|
| 1 | **Bornes de la file d'attente** | **100 places / attente 60 s** (option A : refus rapide) | équipe | ✅ final |
| 2 | **Demandes en vol** par client | **8** | M1 (mesures en phase 2, version mineure) | ✅ final |
| 3 | **Battement de cœur** | **60 s** (expiration à 180 s) | M1 | ✅ final |
| 4 | **Temps de minage** | **0,3 s – 2 s** ⚠️ *seule valeur changée par rapport au défaut antérieur (≤ 1 s)* : `k = 4` dès 32 768 h/s, réévaluation hors [0,3 ; 2], **délai de garde 4 s** ; borne basse inatteignable au-delà de ≈ 218 000 h/s (difficulté plafonnée à 4) → le temps mesuré est affiché | équipe | ✅ final |
| 5 | **Licences et `THIRD-PARTY.md`** | à rédiger (outil, non lié au binaire) | M3 | 🗂️ **tâche**, avant le tag `phase3` |
| 6 | **`JSON_MAX_DEPTH`** | **32** | M1 | ✅ final |
| 7 | **Port d'écoute** | **7001** | M1 | ✅ final |
| ~~8~~ | ~~Bibliothèques hors liste~~ | ✅ **déclaration** d'équipe, un repli par dépendance (§5.3.2) | équipe | ✅ clos |
| ~~9~~ | ~~Confirmation de l'authentification (CU-20)~~ | ✅ **authentification maintenue** — le sujet écrit fait autorité (DEC-24) | équipe | ✅ clos |
| 10 | **Hôte PostgreSQL** | **`linserv-info-01` en recette, PostgreSQL local via Podman en développement** (DEC-29) | M3 | ✅ final |
| 11 | **Partage ciblé et blockchain** | ✅ **option A retenue** : le partage (transfert de carte) est **inscrit en blockchain** comme toute action métier. La **liste d'amis** reste un **réglage d'utilisateur** (non métier, hors chaîne) — elle ne déplace aucune carte | équipe + M2 | ✅ final |

> **Ce qui reste à faire sur ce périmètre n'est plus une question mais une tâche** : rédiger `THIRD-PARTY.md` (point 5) et vérifier les licences (§12.5) avant le tag `phase3`.
---

## 16. Annexes

### Annexe A — Extrait de journal réel attendu (à comparer en soutenance)

```
2026-09-18T19:12:01.002Z | INFO  | srv | -   | r=-            | net        | listening                 | port=7001 backlog=128 somaxconn=4096
2026-09-18T19:12:02.110Z | INFO  | srv | -   | r=-            | persistence| database ready            | blocks=117 users=4
2026-09-18T19:12:02.115Z | INFO  | srv | -   | r=-            | chain      | chain verified            | blocks=117 duration_ms=812
2026-09-18T19:12:04.331Z | INFO  | srv | c=1 | r=-            | net        | client admitted           | depth=0 admitted=1
2026-09-18T19:12:04.402Z | DEBUG | cli | c=1 | r=7f3a91c2d4e5 | net        | request sent              | action=card.create bytes=318
2026-09-18T19:12:04.418Z | INFO  | srv | c=1 | r=7f3a91c2d4e5 | protocol   | request accepted          | action=card.create payload_bytes=294
2026-09-18T19:12:04.712Z | INFO  | srv | c=1 | r=7f3a91c2d4e5 | chain      | block mined               | difficulty=4 nonce=31847 hashes=72144 ms=294
2026-09-18T19:12:04.755Z | INFO  | srv | c=1 | r=7f3a91c2d4e5 | persistence| block committed           | block_index=118 cards=1 ms=43
2026-09-18T19:12:04.757Z | INFO  | srv | -   | r=-            | net        | event broadcast           | event=card.created recipients=1
2026-09-18T19:12:04.781Z | DEBUG | cli | c=1 | r=7f3a91c2d4e5 | net        | response received         | status=OK bytes=214
2026-09-18T19:24:10.005Z | WARN  | srv | c=101 | r=-          | net        | queue enqueue             | position=1 depth=1 admitted=100
2026-09-18T19:24:40.117Z | INFO  | srv | c=101 | r=-          | net        | client admitted           | waited_ms=30112 depth=0
2026-09-18T19:25:00.900Z | ERROR | srv | -   | r=-            | chain      | verification failed       | block_index=54 reason=hash_mismatch
```

### Annexe B — Extraits de configuration

**`zlog.conf`** (extrait) :

```ini
[global]
default = main
[rules]
*.*        >stdout; main
*.WARN     "logs/server.log", 10MB * 5 ~ "logs/server-#2r.log"; main
```

**`logback.xml`** (extrait) :

```xml
<appender name="FILE" class="ch.qos.logback.core.rolling.RollingFileAppender">
  <file>logs/client.log</file>
  <rollingPolicy class="ch.qos.logback.core.rolling.FixedWindowRollingPolicy">
    <fileNamePattern>logs/client-%i.log</fileNamePattern><maxIndex>5</maxIndex>
  </rollingPolicy>
  <triggeringPolicy class="ch.qos.logback.core.rolling.SizeBasedTriggeringPolicy">
    <maxFileSize>10MB</maxFileSize>
  </triggeringPolicy>
  <encoder><pattern>%d{yyyy-MM-dd'T'HH:mm:ss.SSS'Z', UTC} | %-5level | cli | c=%X{client} | r=%X{request} | %logger{16} | %replace(%msg){'\n','\\n'} | %X{keys}%n</pattern></encoder>
</appender>
<appender name="ASYNC" class="ch.qos.logback.classic.AsyncAppender">
  <queueSize>4096</queueSize><discardingThreshold>0</discardingThreshold>
  <appender-ref ref="FILE"/>
</appender>
```

**`server.ini`** (extrait, valeurs par défaut) :

```ini
port = 7001
admission.max = 100
admission.wait_seconds = 60
work.queue = 256
inflight.per_client = 8
heartbeat.seconds = 60
idle.timeout.seconds = 300
pow.difficulty.min = 3
pow.difficulty.max = 4
pow.target_ms = 250
```

### Annexe C — Pseudo-code de la file d'admission (FIFO bornée)

```text
accepter(socket):
    si file.pleine():  refuser(socket, QUEUE_FULL); journaliser(WARN); return
    conn = enregistrer(socket, état=ACCEPTEE, arrivée=monotone())
    attendre hello ≤ 5 s, sinon fermer(conn, HANDSHAKE_TIMEOUT)

hello(conn, pseudo):
    si admis < admission.max:
        admettre(conn)                      # session.welcome + contexte
    sinon:
        position = file.enfiler(conn)       # FIFO, bornée à admission.max
        répondre(conn, QUEUED, position)
        diffuser_aux_attente(queue_update)  # chaque position à jour

libérer_place():
    fermer(conn)                            # départ, erreur ou expiration
    si file.non_vide():
        suivant = file.défiler()            # premier arrivé
        admettre(suivant)                   # session.welcome
        diffuser_aux_attente(queue_update)

balayer_chaque_seconde():                   # fil « échéances »
    pour conn dans file:
        si monotone() - conn.arrivée > 60 000 ms: refuser(conn, QUEUE_TIMEOUT)
    pour conn dans admis:
        si monotone() - conn.dernier_trafic > 180 000 ms: fermer(conn, EXPIRED)
```

### Annexe D — Checklist de revue avant fusion

- [ ] La fonction fait **≤ 200 lignes** et porte un en-tête en **anglais**.
- [ ] **Son test existe** et passe en local comme en CI.
- [ ] Aucun `printf` / `System.out.println` de diagnostic ajouté (ENF-55).
- [ ] Toute nouvelle ligne de journal **respecte la grammaire** §10.3 (poids : attention aux `|` et aux retours à la ligne).
- [ ] Aucune donnée personnelle, aucune charge utile complète, aucun secret dans les journaux (§10.7).
- [ ] Toute nouvelle ressource est **bornée** (file, tampon, allocation) — sinon la question est posée en revue.
- [ ] Si une exigence est touchée : la ligne correspondante du **§14** et le **cahier des charges** sont à jour.

### Annexe E — Budgets de performance (rappel)

| Mesure | Budget | Où c'est vérifié |
|---|---|---|
| Décodage d'une trame | ≤ 1 ms | tests `protocol` |
| Validation métier | ≤ 5 ms | tests `business` |
| Minage (difficulté courante) | **0,3 s – 2 s** (temps mesuré affiché si la machine est plus rapide) | banc de calibrage, `block mined ms=` |
| Écriture bloc + projection | ≤ 50 ms | journal `block committed ms=` |
| Réponse totale hors minage | ≤ 500 ms (p95) | test F du §11.10 |
| Restauration du contexte | ≤ 10 s / 1000 blocs | REC-02 |
| Reconstruction de chaîne complète | ≤ 5 s / 1000 blocs | ENF-47 (démonstration phase 4) |
| Mémoire, 100 clients admis + 100 en attente | < 20 Mo côté serveur | test G |

### Annexe F — Journal des révisions du document

| Version | Date | Auteur | Contenu |
|---|---|---|---|
| **0.7** | **22/09/2026** | Équipe SAÉ | **Valeurs par défaut retenues comme définitives + minage en fourchette** — ① **toutes les valeurs par défaut deviennent finales** : file 100 places/60 s (option A), 8 demandes en vol, heartbeat 60 s, `JSON_MAX_DEPTH` 32, port 7001, `linserv-info-01` en recette / Podman en développement, partage ciblé **inscrit en blockchain (option A)**, **reprise de session** pour un pseudo déjà connecté, bot de maintenance = charge + vérification de chaîne, bibliothèques hors liste **retenues et déclarées** ; ② **seule exception : le minage passe de ≤ 1 s à la fourchette 0,3 s – 2 s** (§5.2.1, §8.5, §8.8, §12.5, annexe D) — calibration `k=4` dès 32 768 h/s, réévaluation hors [0,3 ; 2], **délai de garde 4 s** ; ③ **§15.2 est clos** (tous les points tranchés) |
| **0.6** | **22/09/2026** | Équipe SAÉ | **L'équipe n'a plus d'encadrant : les questions externes deviennent des décisions d'équipe** — ① **§15.2** : point 4 (minage) et **point 9 (authentification, R19)** **clos** par décision d'équipe ; ② **libuv écartée** : boucle d'événements écrite à la main avec **`epoll`** (§4.4, §12.5) ; ③ **§5.3.2** : les bibliothèques hors liste du sujet sont **retenues, déclarées et justifiées**, chacune avec un **repli** (plus de demande d'autorisation, plus de destinataire) ; ④ **§12.4/§13** : grille interne du « dépôt organisé et exploitable » (barème du sujet appliqué au mot). Voir `questions-encadrant.md` v5 |
| **0.5** | **22/09/2026** | Équipe SAÉ | **Réponses de l'équipe au questionnaire rempli (n° 41)** — ① **§6.1** : entités `legitimation_likes`, `friendships`, `shares` ajoutées, valeur d'une carte corrigée (`points de reco × multiplicateur de rang`), valeur d'utilisateur (Q3.2), revendication sans `SANS_OBJET` ; ② **§7.4.2** : les actions passent de 21 à **23** (deux actions de partage ciblé, limite de cartes à **100**, vote de 5 min **définitif** → `1020`, `legitimation.respond` devient un « like ») ; **§7.4.3** : **30 codes** ; ③ **§6.4** et **§8.4** : plus de scrutin de légitimation de 60 s, battle de **300 s** ; ④ **§15.2** : nouveau **point ouvert n° 11** — le partage ciblé est hors blockchain alors que la chaîne est la source de vérité. |
| **0.4** | **22/09/2026** | Équipe SAÉ | *(complétée en fin de journée : **§5.3.2** — les bibliothèques hors liste blanche passent de « demande d'autorisation » à **déclaration** ; choix confirmé libre par l'encadrant, point ouvert n° 8 clos, §12.5 et §14 mis à jour ; puis **§6.1** — invariants de l'entité authentification précisés après la vérification croisée des quatre artefacts (CU-17 : révocation = action validée avec son propre bloc), journal → **64 entrées**, vocabulaire à **21** événements de protocole)* **Alignement sur le sujet officiel 2026-2027** : ① **authentification rétablie** (CU-20, DEC-24) — §3.2, §5.2, §6.1, §7.4 lignes 20-21, ADR-14, traçabilité, projetée en base (`authentications`) et couverte par une séquence UML ; ② **OpenSSL pour le hachage** — nouveau **§5.3.1** (façade `sha256.c`), ADR-13, licences, et **§5.3.2** « bibliothèques à faire autoriser » (ENF-59, R19) avec les replis ; ③ **§6.5.2** : correspondance **donnée par donnée** avec la liste du sujet (identifiant, timestamp, action, hash précédent, nonce) et rappel que le **pointeur de chaînage n'est pas haché** ; ④ **§8.4** : règle du sujet pour la base indisponible au démarrage (genesis) **avec garde-fou** ; ⑤ **§13.0** : SOLID et patrons de conception (exigence de phase 3) ; ⑥ protocole aligné (**23 actions, 22 événements, 30 codes**) ; ⑦ **§9** : hôte PostgreSQL du LAN et dates de tags (10/10/2026, démo 18–21/01/2027) ; ⑧ **3 points ouverts** ajoutés (autorisations, confirmation de l'authentification, hôte de base) |
| **0.3** | **22/09/2026** | Équipe SAÉ | **Note de format de ligne intégrée** : `docs/note-format-ligne.md` v1.0 devient la référence de **nomenclature** du format (§10.3.1 : vocabulaire fermé de 60 événements, clés normalisées, contrat de compatibilité) ; **table numérique des niveaux** ajoutée au §10.4 (syslog `SEVERITY`/`PRIVAL`, ECS `event.severity`, OpenTelemetry `SeverityNumber`, Logback, `zlog`) ; forme canonique **`<sujet>_before` / `<sujet>_after`** à la place de `old`/`new` (§8.8) et **`cause=recommendation`** (§10.7) ; **8 références** ajoutées au §17. |
| **0.2** | **22/09/2026** | Équipe SAÉ | **Enrichissement par recherche normative** (4 sujets) : ① **architecture en couches** — règle de dépendance, inversion de dépendance limitée à `business`, matrice autorisé/interdit, **squelette de fichiers** (§4.5), **contrôle automatique** `scripts/check-layers.sh` + métriques Ca/Ce/I (§4.4 → §4.6) ; ② **calibrage de la difficulté et difficulté hachée** — banc ≤ 250 ms (§8.6), règle `16^k / débit ≤ 1 s` au seuil 65 536 h/s (§8.7), réévaluation amortie (§8.8), boucle de minage et budget d'essais (§8.9), **en-tête canonique de 96 octets + charge utile `payload_canon` (RFC 8785)** avec **vecteurs dorés** (§6.5) ; ③ **dossier de relecture du protocole** — 19 actions, 27 codes, bornes de validation par type, grille et signatures (§7.4) ; ④ **saturation** — dimensionnement par la **loi de Little**, invariants, équité FIFO, runbook (§11.13 → §11.14). **Deux ADR ajoutées** (ADR-11, ADR-12) et **11 références de plus** au §17. |
| **0.1** | **19/09/2026** | Équipe SAÉ | Première version : structure par points de vue (IEEE 1016), adossement normatif (§2), vues contextuelle à déploiement (§3 → §9), **rédaction complète de la journalisation (ENF-55 → ENF-58, §10)** et **de la saturation (ENF-54, §11)**, sécurité/RGPD/licences (§12), qualité et CI (§13), traçabilité (§14), décisions et points ouverts (§15), annexes et sources |

---

## 17. Sources

**Conception et documentation**

- IEEE Std **1016-2009**, *Software Design Description* — vues et points de vue de conception : <https://cengproject.cankaya.edu.tr/wp-content/uploads/sites/10/2017/12/SDD-ieee-1016-2009.pdf>
- Modèle de gabarit de SDD aligné IEEE 1016-2009 et ISO/IEC/IEEE 42010 : <https://github.com/jam01/SDD-Template>
- ISO/IEC/IEEE **42010**, description d'architecture (parties prenantes, préoccupations, points de vue) : <https://www.documind.chat/blog/sample-software-design-documentation>
- **ISO/IEC 9075** (langage SQL), contraintes de table — déjà cité en annexe C du cahier des charges ; DDL en `sql/schema.sql`.

**Journalisation, exploitation et conformité**

- OWASP **ASVS 4.0, chapitre V7** *Error Handling and Logging* (7.1.1 → 7.4.3) : <https://github.com/OWASP/ASVS/blob/master/4.0/en/0x15-V7-Error-Logging.md>
- OWASP **Logging Cheat Sheet** — données à exclure, contenu minimal, assainissement : <https://cheatsheetseries.owasp.org/cheatsheets/Logging_Cheat_Sheet.html>
- **CWE-532**, *Insertion of Sensitive Information into Log File* : <https://cwe.mitre.org/data/definitions/532.html>
- **CWE-117**, *Improper Output Neutralization for Logs* (injection de journal) : recherché via OWASP ASVS 7.3.1
- **NIST SP 800-92**, *Guide to Computer Security Log Management* — centralisation, rotation, rétention, revue : <https://csrc.nist.rip/library/alt-SP800-92.pdf>
- **ISO/IEC 27001:2022**, annexe A **8.15** (journalisation) et **8.16** (surveillance) : <https://www.isms.online/iso-27001/annex-a-2022/8-16-monitoring-activities-2022/>
- **ISO/IEC 27001:2022**, exigences d'audit logging (8.15, 8.16, **8.17** synchronisation des horloges, **5.33** protection des enregistrements) : <https://www.bemopro.com/compliance-requirements/iso-27001-audit-logging>
- **ISO/IEC 27002:2022**, contrôle 8.15 — politique de journalisation : <https://sprinto.com/iso-27001/logging-and-monitoring-policy/>
- **CNIL**, recommandation sur la journalisation (délibération **2021-122**), durée de référence de **6 mois** pour les journaux d'accès applicatifs : <https://www.legiscope.com/blog/journalisation-logs-recommandation-cnil.html>
- Rétention comparée (NIST, PCI-DSS, HIPAA) et principe des paliers chaud/froid : <https://www.decryptiondigest.com/blog/security-log-retention-compliance-detection>

**Format de ligne (note dédiée `docs/note-format-ligne.md`)**

- RFC **5424**, *The Syslog Protocol* — en-tête positionnel, `MSGID`, `NILVALUE`, sévérités et facilities `local0…local7` : <https://www.rfc-editor.org/rfc/rfc5424.html>
- RFC **3339** — horodatage (T/Z majuscules, fraction de seconde) : <https://www.rfc-editor.org/rfc/rfc3339>
- **logfmt** (convention Heroku) — article de référence de B. Leach : <https://brandur.org/logfmt> ; spécification de syntaxe (ABNF, échappements, guillemets) : <https://manpages.ubuntu.com/manpages/stonking/man3/Log::Fmt.3pm.html> ; implémentation d'origine : <https://github.com/csquared/node-logfmt>
- **ECS Logging Specification** — champs obligatoires et ordonnés (`@timestamp`, `log.level`, `ecs.version`), `event.action`, `event.severity`, `service.*`, `trace.id` : <https://deepwiki.com/elastic/ecs-logging/2-ecs-logging-specification>
- **OpenTelemetry**, *Logs Data Model* — `SeverityNumber` par plages (1-4 TRACE → 21-24 FATAL), `EventName`, `Attributes` : <https://github.com/open-telemetry/opentelemetry-specification/blob/main/specification/logs/data-model.md>
- **The Twelve-Factor App**, facteur XI *Logs* — « n'écrit ni ne gère de fichiers de journal » : <https://12factor.net/logs>
- **Google Cloud**, journalisation d'une application conteneurisée (forme JSON `severity` / `time` / `message`) : <https://docs.cloud.google.com/run/docs/logging>
- Journalisation structurée — noms de champs constants, **unité dans le nom du champ**, anti-patrons (interpolation dans le message) : <https://uptrace.dev/glossary/structured-logging> et <https://oneuptime.com/blog/post/2026-01-25-structured-logging-best-practices/view>

**Saturation, files d'attente et rejet**

- Google SRE, *Handling Overload* et *Addressing Cascading Failures* — files courtes, rejet précoce, backoff avec jitter, budgets de réessai : <https://danluu.com/google-sre-book/> et <https://sre.google/sre-book/service-best-practices/>
- Files bornées, admission, politique de rejet et contre-pression : <https://leanpub.com/backpressure-in-microservices>
- Distinction **file du noyau** / file applicative, `somaxconn`, mesures `ListenDrops`, rejet explicite plutôt que latence longue : <https://oneuptime.com/blog/post/2026-07-28-504-gateway-timeouts-under-load-pools-queues-workers/view>
- `net.core.somaxconn` : rôle, valeurs recommandées, mémoire par connexion en attente : <https://dohost.us/index.php/2026/01/02/increasing-the-listen-backlog-adjusting-somaxconn-to-prevent-dropped-connections/>

**Bibliothèques, licences et sécurité des parseurs**

- **jansson**, conformité à la RFC 8259 et limite de profondeur (`JSON_PARSER_MAX_DEPTH`, 2048 par défaut) : <https://jansson.readthedocs.io/en/latest/conformance.html>
- **cJSON** : limite de profondeur (`CJSON_NESTING_LIMIT`) : <https://github.com/JohnFPatterson/cJSON>
- Vulnérabilité **CWE-674** par document imbriqué dans cJSON, et recommandation de plafonner la profondeur à 64 : <https://github.com/signalwire/freeswitch/security/advisories/GHSA-2v74-pcgh-75wg>
- Complexité exponentielle de `cJSON_Compare` (déni de service) : <https://www.vulncheck.com/advisories/cjson-cjson-compare-exponential-complexity-denial-of-service>
- **SLF4J** : licence de type X11 et rôle de façade : <https://www.slf4j.org/faq.html>
- **Logback** : double licence EPL / LGPL : <https://ekkescorner.wordpress.com/2010/01/04/logging-in-osgi-applications-logback-now-epl-license/>
- Exception **Classpath** de la GPL (usage de JavaFX en bibliothèque) : <https://vaultinum.com/blog/the-gpl-and-its-unique-gpl-classpath-exception-what-does-it-mean>
- **zlog** : licence LGPL et liaison dynamique : <https://technolinchpin.wordpress.com/2016/01/13/choosing-data-loggers/>
- **SPDX** : identifiants de licence normalisés, utilisés dans l'inventaire §12.5 : <https://spdx.github.io/license-list-data/>

**Alignement avec le sujet officiel 2026-2027**

- **Sujet officiel** : *SAÉ — BUT 2, semestre 3 — plateforme de recommandations fondée sur une blockchain privée* (organisation, cas d'usage CU-1 à CU-27, phases 1 à 4, bibliothèques autorisées, calendrier).
- Exemple de blockchain fourni avec le sujet (**`bc_example.py`**, Python) et supports cités par le sujet : <https://www.geeksforgeeks.org/how-does-theblockchain-work/> · <https://www.geeksforgeeks.org/create-simple-blockchain-using-python/>
- **OpenSSL** — `SHA256()` et `libcrypto` (bibliothèque autorisée par le sujet pour le hachage) : <https://docs.openssl.org/master/man3/SHA256_Init/>
- **PostgreSQL du LAN de l'IUT** : instance `linserv-info-01` (sujet §8 et §10).

**Canonicalisation, hachage et preuve de travail**

- RFC **8785**, *JSON Canonicalization Scheme (JCS)* — ordre des clés, nombres IEEE 754, échappements ; « le hachage porte sur des octets » : <https://www.rfc-editor.org/rfc/rfc8785>
- FIPS **180-4**, *Secure Hash Standard (SHS)* — définition de SHA-256 : <https://csrc.nist.gov/pubs/fips/180-4/upd1/final>
- Vecteurs de test officiels du SHA-256 (message vide, `"abc"`) : NIST / NSRL, *Cryptographic test data* — recherché via « FIPS 180-4 SHA-256 test vectors empty string ».
- Débit comparé du SHA-256 **avec et sans accélération matérielle** (SHA-NI ≈ 1,8 cycle par octet contre ≈ 7,7 en logiciel ; mesure `openssl speed -hmac sha256`) — recherché via « openssl speed sha256 cycles per byte ».
- **Double hachage (SHA256d) et attaque par extension de longueur** : N. Ferguson & B. Schneier, *Practical Cryptography* (2003) ; discussion d'usage : <https://en.bitcoin.it/wiki/Block_hashing_algorithm> — justifie le **hachage simple** dans notre cas (aucun secret, longueur fixe, aucun usage MAC).
- **Ajustement périodique de la difficulté** (temps de bloc cible, plafond de variation) : <https://learnmeabitcoin.com/technical/mining/target/>
- **Épuisement du nonce de 32 bits** et recours à un *extra-nonce* (Bitcoin Stack Exchange) — justifie notre **nonce de 64 bits** : recherché via « bitcoin nonce 32 bits exhausted extranonce ».

**Architecture en couches, couplage et validation**

- *Layered Architecture: Still a Solid Approach* (NDepend Blog) — pas de dépendance montante, pas de cycle : <https://blog.ndepend.com/layered-architecture-still-a-solid-approach/>
- Comparaison **couches / hexagonale / clean** (dépendances vers l'intérieur, coût d'indirection) et **métriques de couplage Ca / Ce / instabilité** — recherché via « layered vs hexagonal vs clean architecture coupling afferent efferent instability ».
- OWASP **ASVS 4.0, V5.1.3 et V5.1.4** — canonicalisation **avant** validation, bornes de longueur, validation serveur autoritaire : <https://github.com/OWASP/ASVS/blob/master/4.0/en/0x13-V5-Validation-Sanitization-Encoding.md>
- **Loi de Little** (`L = λW`), valable quelles que soient la distribution des arrivées et la discipline de service : J. D. C. Little, *A Proof for the Queuing Formula: L = λW* (1961) ; vulgarisation : <https://en.wikipedia.org/wiki/Little%27s_law>

