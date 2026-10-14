# Écarts entre le sujet officiel 2026-2027 et nos documents

**Projet AnimeTCG** — SAÉ BUT2 S3 — **22/09/2026**
**Sources comparées** : *SAÉ — BUT 2, semestre 3 — Sujet de projet : plateforme de recommandations fondée sur une blockchain privée* (2026-2027) **vs** `cahier-des-charges-sae-s3.md` v0.20, `docs/specification-technique.md` v0.3, `docs/protocole-applicatif.md` v1.0, `sql/schema.sql` v1.1, `docs/uml/`, `questionnaire-decisions.md`, `questions-encadrant.md`, `checklist-phase1-*.md`.

> **Pourquoi ce document existe.** Le sujet officiel reçu le 22/09/2026 **remplace** la version sur laquelle nos documents avaient été construits, et il contredit sur **trois points** ce que l'encadrant nous avait dit oralement le 18/09. Un écart non tracé est un risque en soutenance : ce document est la **liste de contrôle** de la resynchronisation. Les 20 écarts ci-dessous sont **tous appliqués**.

---

## 1. Les 20 écarts, et ce qu'on en a fait

| # | Sujet officiel 2026-2027 | Ce que disaient nos documents | Décision et application | Impact |
|---|---|---|---|---|
| **1** | Phase 1 livrée le **10/10** (sujet §9) | « J1 = **2 octobre** » partout (CDC, checklist nommée `checklist-phase1-02-10.md`, questionnaire, questions-encadrant, spec) | ✅ **Recadré** : 10/10/2026. Checklist **renommée** `checklist-phase1-10-10.md` ; toutes les occurrences corrigées ; lots L2 → L11 décalés | 🔴 Élevé |
| **2** | Phase 2 **12/10 → 18/12/2026**, phase 3 **4 → 15/01/2027**, phase 4 **18 → 21/01/2027** | Phase 2 « dès le 5 octobre », démo « 18–19/01 » | ✅ **Corrigé** dans le CDC (§7.1), la spec (§9), la checklist (§7), les questions-encadrant | 🟠 Moyen |
| **3** | **Authentification d'une carte** exigée (§3, §5.5, **CU-20**, §12.1 de la démonstration) | **Écartée** le 18/09/2026 sur consigne orale (« on s'en fiche ») : CU-20/EF-20 barrés, DEC-12 retirée, action absente du protocole | ✅ **Rétablie** : CU-20 + EF-20 + **DEC-24** ; nouvelle section **§4.7.3** du CDC (conditions, unicité, révocation, absence d'effet sur la valeur, transfert après battle, réponse au CU-17 laissé ouvert) ; protocole **v1.1** (`card.authenticate`, `card.revoke_authentication`, `card.authenticated`, codes `1018`/`1019`) ; **table `authentications`** (`sql/schema.sql` v1.2) ; **séquence UML `13`** ; étape de démonstration ; **confirmation demandée (R19)** | 🔴 Élevé |
| **4** | Volume : **120 h/étudiant**, 3 projets, SAÉ projet = **60 %** (§1) | « ≈ **20 h/étudiant** », enveloppe ≈ 80 h d'équipe | ✅ **Chiffré** : ≈ **72 h/étudiant** (120 × 0,6) → **≈ 288 h** à 4 étudiants ; §6.2 du CDC réécrit (80 h phase 1, 165 h phase 2, 28 h phase 3, 15 h phase 4) ; risques RQ-04 et I5 mis à jour | 🔴 Élevé |
| **5** | Groupe d'**« environ cinq étudiants »** | 4 étudiants (accepté par l'encadrant) | ⚠️ **Conservé à 4** (accord de l'encadrant), mais **écrit explicitement** dans l'en-tête, la contrainte C10 et l'hypothèse H8 — le jury voit que l'écart est connu et assumé | 🟢 Faible |
| **6** | **Bibliothèques** : liste blanche (OpenSSL, libpq, POSIX, JavaFX, JUnit) + **demande obligatoire** pour les autres (§10) | jansson, zlog, SLF4J/Logback utilisés sans démarche | ✅ **Décision d'équipe du 22/09/2026 : le choix est libre** (l'équipe n'a plus d'encadrant : la demande n'a plus de destinataire) ; ENF-59 est reconvertie en obligation de **déclaration** (nom, version, rôle, licence, repli) : **§5.9.1** du CDC, **§5.3.2** et **§12.5** de la spec, **DEC-27**, I8 clos. *Reste un écart assumé avec la lettre du §10, tracé ici et au journal du CDC* | 🟠 Moyen |
| **7** | **OpenSSL autorisé pour le hachage** (`<openssl/sha.h>`, `-lcrypto`) | `chain/sha256.c` : SHA-256 écrit par l'équipe | ✅ **ADR-13** : OpenSSL devient le moteur, `sha256.c` une **façade de ≈ 30 lignes** ; les **vecteurs dorés restent valables** ; l'argument se déplace vers la **canonicalisation** (§6.5) | 🟠 Moyen |
| **8** | **Aucun framework de tests C** installé (§11) | banc de tests maison + `gcov`/`gcovr` envisagés | ✅ Confirmé conforme (tests C appelant les fonctions + `assert`) ; l'outil de couverture (**gcovr**, côté Java **JaCoCo**) est un **choix d'équipe déclaré**, sans demande extérieure | 🟢 Faible |
| **9** | **`linserv-info-01`**, PostgreSQL du LAN de l'IUT (§8 et §10) | « un poste Debian » générique | ✅ Hypothèse H1, spec §9 et sources mises à jour ; **DEC-29** (`linserv-info-01` en recette, Podman local en développement et pour la démo) | 🟢 Faible |
| **10** | **Images** : licences autorisant l'usage + **crédits** (§10) | Aucune règle | ✅ Nouvelle exigence **ENF-60**, **§5.9.2** du CDC (sources acceptées, `CREDITS.md`, crédit affiché), **DEC-28** ; point sensible pour un domaine sous droit d'auteur | 🟠 Moyen |
| **11** | **SOLID et patrons de conception** en phase 3, justifiés (§11) | Architecture en couches + inversion de dépendance (sans les nommer « SOLID ») | ✅ Nouvelle **ENF-61** + section **§13.0** de la spec : tableau principe → où c'est appliqué → problème résolu, et **ce qu'on refuse** de faire | 🟠 Moyen |
| **12** | **Partage ciblé** : option laissée au groupe, règles à définir si retenu (§5.3, CU-13) | Déjà en bonus (DEC-15), sans règles précises | ✅ **RETENU (option A, arbitrage du 22/09/2026)** : règles complètes au **§4.8**, actions **22/23** (`share.send`/`share.respond`), événement `share.received` **activé**, partage **inscrit en blockchain**ocole, décision **DEC-25** : un choix explicité, pas un oubli | 🟢 Faible |
| **13** | **Battle entre « deux cartes ou plus »** (§5.4, CU-15) | 2 cartes | ✅ **2 cartes conservées** et **justifiées** (DEC-26) : vote non ambigu, quorum atteignable, démo lisible | 🟢 Faible |
| **14** | **CU-17** : *« le traitement des légitimations, revendications et authentifications associées reste à préciser »* (§6) | Non traité | ⚠️ **Règle révisée le 22/09/2026 (réponse Q2.6)** : les droits **restent inchangés** sur la carte perdante — *« les caractéristiques de la carte perdante restent inchangées »* (l'héritage par la gagnante et la clôture `SANS_OBJET` sont retirés) (même œuvre), les revendications en attente sur la perdante sont **clôturées `SANS_OBJET`** et le demandeur est notifié (§4.7.3) | 🟠 Moyen |
| **15** | **Base indisponible au démarrage** ⇒ « aucun contexte de sauvegarde » (CU-2) | Règle inverse (le service continue, la base est réessayée) | ✅ **Règle du sujet appliquée** (démarrage sur genesis) **+ garde-fou** : mode `NO_DB`, refus d'écrire tant que la base n'a pas été relue avec succès — sinon un genesis neuf s'ajouterait **devant** une chaîne existante (§8.4 de la spec) | 🟠 Moyen |
| **16** | **Données hachées** listées : identifiant, timestamp, action, hash précédent, nonce (§9) ; **le pointeur de chaînage n'est pas haché** | En-tête de 96 octets (avec `version` et `difficulty` en plus) sans correspondance écrite | ✅ **§6.5.2** de la spec : tableau de correspondance **donnée par donnée** + justification des deux champs supplémentaires (ENF-52 : la difficulté **doit** être hachée) + rappel que le pointeur technique n'entre pas dans le hash | 🟠 Moyen |
| **17** | **Démonstration** : liste de 19 gestes, dont **authentification**, **nombre de processus/threads**, redémarrage à chaud, altération détectée (§12.1) | Scénario en 17 étapes sans authentification | ✅ Scénario du CDC §9.2 complété (authentification + vérification des fils), checklist §3 et §6 mises à jour | 🟠 Moyen |
| **18** | **Présentation** : plan imposé (§12.2), dont **« un problème technique rencontré »** | DOC-10 mentionné sans plan | ✅ Plan écrit dans le CDC §5.7 (DOC-10) ; sujet du « problème technique » **choisi** : le **hachage de la difficulté variable** (§5.2.1, ENF-52) — vrai problème, vraie analyse, vraie solution | 🟢 Faible |
| **19** | Livrable de phase 1 = **« un document appelé spécification technique »** avec 5 contenus imposés, taggé `phase1` (§9) | Livrable décrit de façon plus vague | ✅ CDC §5.7 : correspondance des 5 contenus (rôles, architecture, données serveur/client/BDD, UML ≤ 5 types, première approche du protocole) ; **R5** réduite à la seule question du **format** | 🟢 Faible |
| **20** | Le sujet **liste 17 choix** laissés au groupe, « explicités et justifiés dans la spécification technique » (§14) | Dispersés dans le CDC | ✅ **Annexe F du CDC** : une page, 17 lignes, chaque choix → notre réponse → la section qui la porte. C'est la table à montrer au jury | 🟠 Moyen |

---

## 2. Ce que le sujet **confirme** (aucun changement, mais c'est bon à savoir)

| Point | Ce que dit le sujet | Ce que nous faisons déjà |
|---|---|---|
| Sockets uniquement | « Toute la communication passe par les sockets. **Aucune autre interface** n'est acceptée, notamment les API REST » (§10) | Canal unique, zéro REST depuis le début ; `REC-14` le prouve |
| Preuve de travail et zéros hexadécimaux | Motif = **zéro hexadécimaux de tête**, difficulté **3 ou 4** (§7 et §9) | Zéros hexadécimaux, {3, 4}, **calibrage automatique** (§5.2.1) — et la difficulté **hachée** (ENF-52), un cran au-delà du minimum |
| 27 cas d'utilisation | CU-1 → CU-27 | Les 27 sont couverts ; CU-6 (bot) **retenu comme outil de test** (DEC-14), **CU-24 (partage ciblé) retenu** le 22/09/2026 |
| Fonctions ≤ 200 lignes, commentaires anglais | §11 | ENF-33/34/35 (+ en-têtes **dans les tests**) |
| PostgreSQL, tables dérivées, **même transaction** | §10 « BDD » recommande explicitement l'état courant et la transaction unique | C'était déjà notre **ADR-03** : la blockchain est la source de vérité, les projections sont écrites dans la même transaction |
| Une carte inactive peut **redevenir active** | CU-21 | État `INACTIVE` + règle de retour en jeu (§4.4) |
| Déconnexion : socket libérée, actions conservées, règle d'absence | CU-25 | §4.9 et DEC-11 |
| Barème 15 / 40 / 10 / 15 / 20 | §13 | Identique |

---

## 3. Où regarder dans les documents

| Document | Version | Ce qui a changé le 22/09/2026 |
|---|---|---|
| `cahier-des-charges-sae-s3.md` | **v0.20 → v0.24** *(fin de journée)* | En-tête (échéance 10/10, volume), §3.2 (authentification rétablie), **§4.7.3** (authentification), §4.10 (EF-20), §5.5 (**ENF-59 à ENF-61**), **§5.9** (bibliothèques, images), §5.7 (UML, DOC-10), §6.1 (rôles du sujet), §6.2 (**enveloppe 288 h**), §7.1–7.2 (dates et lots), §9.2 (démonstration), §11.1 (CU-20), §12 (DEC-24 à DEC-29), §13 (I5, I8, I12 clos), **Annexe F** (les 17 choix), sources, journal 0.21 |
| `docs/specification-technique.md` | **v0.3 → v0.7** *(fin de journée)* | §3.2, §4.2 (`authentication.c`), **§5.3.1/§5.3.2** (OpenSSL, bibliothèques), §6.1 (entité authentification), **§6.5.2** (correspondance avec le sujet), §7.4 (actions 20-21, 29 codes), §8.4 (base indisponible), §9 (tags, linserv), §12.5 (licences), **§13.0** (SOLID), §14 (traçabilité), **ADR-13/ADR-14**, points ouverts 8-10, annexe F, sources |
| `docs/protocole-applicatif.md` | **v1.0 → v1.2** *(fin de journée)* | §6.6 (authentification), §6.8 (actions absentes), §7 (2 événements), §8.2 (**29 codes**), **§16.1** (journal de la mise à jour) |
| `sql/schema.sql` | **v1.1 → v1.5** *(fin de journée)* | Table `authentications` + index unique partiel « une seule active », requête de contexte, nettoyage des tests (**v1.2**) ; puis état `SANS_OBJET` (règle CU-17) et colonne `revoked_block_index` (**v1.3**) — **DDL validé par analyse syntaxique** |
| `docs/uml/` | 12 → **13 sources** (puis **14** le 22/09/2026 : partage ciblé) | `01` (CU-20 rétabli, **UC-7 et UC-25 ajoutés** — deux cas du sujet qui manquaient), `02` (classe `Authentification`), `12` (relations des revendications), **`13-sequence-authentification.puml`** (authentification **et révocation**) ; exports SVG/PNG régénérés (26 fichiers) |
| `docs/note-format-ligne.md` | v1.0 → **v1.3** | 3 événements d'authentification (**catalogue : 63**, puis **69** au 22/09 avec les réponses du questionnaire), clé `authentication_id`, puis **`claim closed`** pour la règle CU-17 (**64 entrées**) et clé `event` alignée sur les **21** événements du protocole |
| `questionnaire-decisions.md` | — | **Q4.4 rouverte et tranchée** (DEC-24) ; **DEC-25 à DEC-29** ; I5 recalculé ; échéance 10/10 |
| `questions-encadrant.md` | — → **v5** | **Plus d'encadrant (22/09/2026)** : les 6 questions bloquantes et les 10 questions d'appoint deviennent des **décisions d'équipe** (R19 close : authentification maintenue ; R2 : `epoll` à la main, **libuv écartée**) ; **R18 close** (le sujet répond : OpenSSL) ; dates corrigées ; ligne « authentification » retournée |
| `checklist-phase1-10-10.md` | *renommée* | Échéance 10/10, enveloppe ≈ 80 h, tâches **15 et 16** ajoutées, ligne « bibliothèques » **close**, définition de « fini » complétée |

---

## 4. Les deux écarts **non** résolus — ⛔ **tranchés en équipe le 22/09/2026** *(plus d'encadrant)*

1. **Authentification** — le sujet écrit l'exige, l'oral l'avait écartée : **le sujet écrit fait autorité**. Décision d'équipe du **22/09/2026** : **authentification maintenue** (DEC-24), développée dans le lot L7. La fonctionnalité est développée, un refus coûterait le lot L7, pas le jalon. *(La question des bibliothèques, elle, est close : choix libre.)*
2. **Volume horaire réel** — le sujet annonce 120 h/étudiant ; le temps réel est **variable et partagé** avec les deux autres SAÉ (R301 et R307). Décision d'équipe du **22/09/2026** : **≈ 72 h/étudiant retenues comme hypothèse de planification non garantie**, planification par lots courts ; la **présence aux créneaux** reste le facteur limitant. Question historique **envoyée le 22/09/2026** (R3).

---

## 5. Contrôle de conformité au 22/09/2026 — vérification à la lettre

Deuxième passe, faite **sujet en main**, écran par écran : chaque écart est confronté à l'endroit exact du document qui doit le porter. La colonne « preuve » dit **où regarder** pour le vérifier sans nous croire.

| # | Écart | Statut | Où c'est écrit (preuve) |
|---|---|---|---|
| 1 | Phase 1 au 10/10 | ✅ | CDC §7.1, en-tête ; `checklist-phase1-10-10.md` (fichier renommé) ; **plus aucune occurrence du 2 octobre** |
| 2 | Fenêtres 12/10→18/12, 4→15/01, 18→21/01 | ✅ | CDC §7.1 (avec les **jours de la semaine vérifiés** : 10/10 = samedi, 21/01 = jeudi), spec §9, checklist §7 |
| 3 | Authentification exigée | ✅ | CDC **§4.7.3** + §4.10 (EF-20) + §11.1 (CU-20) + §12 (DEC-24) ; protocole **§6.6** ; SQL **§8** ; UML **01** (UC-20 actif) et **13** ; CDC §9.2 (étape 10 bis) |
| 4 | 120 h → ≈ 72 h/étudiant (≈ 288 h) | ✅ | CDC §6.2 (tableau par phase), H1/H3/H5/H8, RQ-04, I5 |
| 5 | « environ cinq étudiants » | ⚠️ assumé | CDC en-tête + C10 + H8 : 4 étudiants, accord de l'encadrant, écart écrit noir sur blanc |
| 6 | Liste blanche de bibliothèques | ✅ | CDC **§5.9.1** (statut ✅ + licences) et ENF-59 ; spec **§5.3.2** (déclaration) et §12.5 ; DEC-27 ; I8 clos — **liberté confirmée le 22/09** |
| 7 | OpenSSL pour le hachage | ✅ | Spec **§5.3.1** + **ADR-13** (`sha256.c` = façade de ≈ 30 lignes, vecteurs dorés conservés) |
| 8 | Aucun framework de tests C | ✅ | Spec §11 (banc maison), §13.0 ; checklist t. 9 |
| 9 | `linserv-info-01` | ✅ | CDC H1 et annexe C ; spec §9 et §17 ; DEC-29 (**recette** ; Podman en développement) |
| 10 | Images licenciées + crédits | ✅ | CDC **ENF-60** et **§5.9.2** ; spec §12.5 ; DEC-28 ; `CREDITS.md` planifié (phase 2) |
| 11 | SOLID et patrons justifiés | ✅ | CDC **ENF-61** ; spec **§13.0** (tableau principe → où → pourquoi) |
| 12 | Partage ciblé (option §5.3, CU-13) | ✅ | CDC §4.8 (règles complètes) et annexe F ligne 14 ; protocole : actions **22/23** + `share.received` **activé** ; SQL `shares.block_index` ; DEC-15/**DEC-25 révisée** |
| 13 | Battle « deux cartes ou plus » | ✅ | CDC §4.5.2 et annexe F ligne 5 : **2 cartes**, choix justifié (DEC-26) |
| 14 | CU-17 : droits des cartes fusionnées | ✅ | CDC **§4.7.3** et **§4.5.4** — ⚠️ **règle remplacée le 22/09/2026 (Q2.6)** : droits **inchangés** sur la perdante ; l'état `SANS_OBJET` a été **retiré** (SQL v1.4) et la règle CU-17 du protocole §6.5 est **retirée** |
| 15 | Base indisponible → genesis | ✅ | Spec **§8.4** (genesis + garde-fou `NO_DB`) ; CDC CU-2 |
| 16 | Données hachées du sujet | ✅ | Spec **§6.5.2** (correspondance donnée par donnée + les 2 champs en plus justifiés + **pointeur de chaînage hors hash**) |
| 17 | Démonstration du §12.1 | ✅ | CDC **§9.2** (17 étapes, dont l'authentification en 10 bis et la vérification des fils en 2) |
| 18 | Présentation : plan + problème technique | ✅ | CDC §5.7 (DOC-10) ; problème retenu : **le hachage de la difficulté variable** |
| 19 | Livrable de phase 1 = spécification technique | ✅ | CDC §5.7 (les 5 contenus imposés, correspondance ligne à ligne) ; R5 réduite au format |
| 20 | Les 17 choix du §14 | ✅ | CDC **annexe F** (17 lignes, choix → réponse → section) — **table à montrer au jury** |

### 5.1. Le dossier « authentification » : les quatre artefacts confrontés

Ce que le sujet demande (CU-20), confronté aux quatre endroits qui doivent dire **la même chose**.

| Règle | CDC §4.7.3 | Protocole §6.6 | `sql/schema.sql` | Séquence UML `13` |
|---|---|---|---|---|
| **Qui** : porteur légitime actif, sinon `1014` | ✅ tableau | ✅ `1014` listé | ✅ `legitimation_id NOT NULL` (FK) | ✅ étape 7 |
| **Quoi** : les métadonnées, pas la propriété | ✅ | ✅ encadré | — (sans objet) | ✅ note |
| **Une seule active par carte** | ✅ | ✅ `1018` | ✅ **index unique partiel** `uq_auth_active` | ✅ étape 9 |
| **Carte active exigée** | — *(implicite)* | ✅ `1003` | ✅ FK vers `cards` | ✅ étape 8 |
| **Révocation possible, tracée** | ✅ | ✅ `card.revoke_authentication` | ✅ `REVOQUEE` + `revoked_at` + **`revoked_block_index`** | ✅ **branche « Révocation »** (étapes 21→32) |
| **Révocation ⇒ historique conservé** | ✅ | ✅ (nouveau bloc) | ✅ aucune écriture effacée | ✅ note finale |
| **Aucun effet sur la valeur** | ✅ | ✅ encadré | — (`cards.value_active` non touchée) | ✅ note |
| **Attribut, pas un état** | ✅ | — | ✅ table séparée, `cards.status` inchangé | ✅ |
| **Diffusion `card.authenticated`** | ✅ §9.2 | ✅ événement + clé `revoked` | — | ✅ étape 20 / 32 |
| **Après une battle** | ⚠️ **révisé le 22/09/2026 (Q2.6)** : droits **inchangés** sur la perdante | ⚠️ règle CU-17 **retirée** (§6.5 du protocole) | ✅ `CHECK (state IN ('PROPOSEE','ACCEPTEE','REFUSEE','TRANSFEREE'))` — `SANS_OBJET` **retiré en v1.4** | — (voir diagramme `05`) |

**Trois divergences trouvées et corrigées** par cette confrontation (elles ne se voyaient qu'en superposant les quatre) : ① §4.7.3 disait qu'une authentification **révoquée** ne faisait pas de bloc, alors que la séquence et le protocole en faisaient un — la phrase est corrigée, seule une authentification **refusée** ne fait pas de bloc ; ② la base ne conservait pas le bloc de la révocation (`revoked_block_index` ajouté, SQL v1.3) ; ③ la règle CU-17 (§4.7.3) n'avait aucun état pour la porter en base (`SANS_OBJET`, SQL v1.3) ni de ligne de journal (64ᵉ entrée).

---

## 6. Vérifications automatiques de fin de passe (22/09/2026)

Contrôles rejouables exécutés après la deuxième passe — **tous réussis** — plus l'intégrité des tableaux des 8 documents Markdown (**0 anomalie**) et la validation du DDL PostgreSQL par analyse syntaxique (`pglast`) :

| Famille | Ce qui est contrôlé |
|---|---|
| **Écarts 1 → 20** | Chaque écart du §1 est vérifié par la présence de sa marque **dans le document qui doit la porter**, pas seulement dans ce fichier |
| **Authentification** | Les 4 artefacts (§4.7.3, protocole §6.6, `authentications`, séquence `13`) portent les mêmes règles : `1014`, `1003`, `1018`, `1019`, unicité, révocation tracée, `revoked = true`, absence d'effet sur la valeur |
| **Comptes** | **14 tables** · **23 actions** · **22 événements** · **30 codes** (5 + 5 + 20) · **69 entrées de journal** · **14 diagrammes** · **28 exports** |
| **Plus d'encadrant** (22/09/2026) | Les **6 questions bloquantes** (R1, R2, R3, R5, R6, R19) ont été **tranchées en équipe** le jour même ; **aucune relance n'est à envoyer** — état reflété dans `questions-encadrant.md` **v5**, la checklist (tâche 2 ✅ sans suite) et les « Prochaines étapes » du CDC |
| **Résidus** | Aucune échéance au 2 octobre, aucun « 63 entrées » *(les comptes ont été portés à 69 entrées / 30 codes / 23 actions le 22/09/2026 — les mentions antérieures ont été conservées telles quelles dans les journaux de version)*, aucun « 27 codes » ni « 19 actions » hors des passages **historiques** (journaux de version et §16.1 du protocole, où ils sont datés), aucune promesse de demande d'autorisation |
| **Calendrier** | Les **jours de la semaine** des jalons correspondent au calendrier réel (10/10/2026 = samedi, 21/01/2027 = jeudi) |
| **Intégrité** | Tableaux Markdown : nombre de colonnes constant, y compris dans les lignes réécrites à la main |
| **Cas d'erreur ↔ protocole** (3ᵉ passe, 22/09/2026) | Le **§12.1 du CDC** est la vue « recette » du **catalogue du protocole** (`docs/protocole-applicatif.md` §8.2) : **17 cas**, chacun rattaché à un code existant ; **26 des 30 codes** sont cités par les cas, les **4 autres** sont listés comme hors recette ; les **100 paires « numéro + nom »** de tous les documents ont été comparées au catalogue — **aucune divergence**, et **aucun code cité** n'est absent du catalogue |
| **Erreurs de nommage** (trouvées par cette passe) | `1011` portait **deux noms** (`BATTLE_CLOSED` au catalogue, `VOTE_ALREADY_CAST` au §6.4) : unifié sur `BATTLE_CLOSED`, la règle « le vote est modifiable avant la clôture » étant celle du CDC (DEC-04) et de la clé `(battle_id, user_id)` de `battle_votes` ; `1014` est documenté comme couvrant **deux situations symétriques** (revendication *et* authentification) |
