# Jalon J1 — **samedi 10 octobre 2026** — checklist de rendu de phase 1

> ⚠️ **Échéance corrigée le 22/09/2026** : le sujet officiel 2026-2027 (§9) fixe la livraison de la phase 1 au **10/10** (et non au 2/10). Les fenêtres suivantes sont **12/10 → 18/12/2026**, **4 → 15/01/2027** et **18 → 21/01/2027**.

**Projet AnimeTCG** (nom définitif, Q8.1) — **SAÉ BUT2 S3** — état au **22/09/2026**, cahier des charges **v0.24**, spec technique **DOC-09 v0.7**, protocole **v1.2**, schéma SQL **v1.5**. Document renommé : il s'appelait `checklist-phase1-02-10.md`.

> ⏱️ Il reste **18 jours** (du 22/09 au 10/10), pour une enveloppe de phase 1 désormais **chiffrée par le sujet** : 120 h/étudiant sur les trois projets × 60 % = **≈ 72 h/étudiant** pour la SAÉ projet, dont **≈ 80 h d'équipe** sur la phase 1 (4 étudiants). ⚠️ Le volume réel reste **variable et non garanti** (Q8.3) : une partie de ces heures est consommée par les deux **autres** SAÉ (R301 et R307), qui partagent les mêmes créneaux.
> Sources : `cahier-des-charges-sae-s3.md`, `docs/diff-sujet-2026.md`, `docs/specification-technique.md`, `docs/protocole-applicatif.md`.

---

## 1. Ce qui est rendu exactement

| Élément | Contenu | Source |
|---|---|---|
| **Le livrable** | **Spécification technique + diagrammes UML**, déposés dans le dépôt GitLab et **taggés `phase1`** | §7.1 (J1) |
| **Le contenu attendu** | Le **cahier des charges augmenté des fonctionnalités singulières du projet** (proximité entre animes, formule de valeur, légitimation, revendication, cycle de vie d'une carte) + tous les diagrammes jugés pertinents, **5 types maximum**, granularité croissante | §5.7, cadrage de l'encadrant |
| **Le périmètre technique à figer** | Architecture, **protocole applicatif**, **modèle relationnel**, cas d'erreur | §7.2 lot L2 |
| ✅ **Ce que le sujet précise (22/09/2026)** | Le livrable de phase 1 est **« un document appelé spécification technique »**, déposé dans le dépôt GitLab et **labellisé `phase1`**, contenant rôles, architecture, organisation des données, **5 types d'UML max** et première approche du protocole. ✅ **Format tranché en équipe le 22/09/2026 (R5)** : document unique en **Markdown** + export **PDF**, **exports UML intégrés** *et* sources PlantUML dans le dépôt | §9 (sujet) |

> ⛔ **Plus d'encadrant depuis le 22/09/2026.** L'e-mail des 6 questions bloquantes (R1 barème, R2 libuv, **R3** horaire, **R5** forme du rendu, **R6** clients en démo, **R19** auth) **n'aura pas de réponse** : ces questions ont été **tranchées en équipe** le jour même (`questions-encadrant.md` v5) — barème du sujet appliqué au mot, **sockets BSD + `epoll`** (libuv écartée), ≈ 72 h en hypothèse, rendu = spécification technique (Markdown + PDF), 4 clients humains minimum, **authentification maintenue**. **Aucune relance n'est à envoyer.** *I12 est clos depuis le 22/09.*

---

## 2. Ce qui a changé depuis la première version de cette checklist

> 🆕 **19/09/2026 — trois livrables de plus sont sortis** : le **protocole applicatif** (`docs/protocole-applicatif.md` v1.0), le **modèle relationnel** (`sql/schema.sql` + diagramme `12`) et la **spécification technique DOC-09** (`docs/specification-technique.md` v0.1, 17 sections — dont la **rédaction complète de la journalisation** et de la **saturation**, qui étaient les deux sujets « décidés mais à rédiger »). Les tâches 6, 7, 10, 11 sont faites ; il reste leur **relecture d'équipe** (8 bis, 8 ter).
>
> 🆕 **22/09/2026 — DOC-09 enrichie en v0.2** (`docs/specification-technique.md`, **1943 lignes** à l'époque — **2 046 en v0.7**) : les **quatre sujets techniques** de la checklist sont désormais traités en détail — **architecture en couches** (§4.4 → §4.6, avec contrôle automatique `scripts/check-layers.sh`), **calibrage de la difficulté et difficulté hachée** (§6.5, §8.6 → §8.9), **dossier de relecture du protocole** (§7.4) et **saturation** (§11.13 → §11.14). Les tâches **5, 8 et 9 sont faites** ; au passage, **trois comptes faux** du dossier ont été corrigés (19 actions, 19 événements, 27 codes) et `sql/schema.sql` passe en **v1.1** (charge utile canonique `payload_canon`).

Sept décisions ont été tranchées le **18/09/2026** (les 13 tâches initiales sont donc allégées) :

| Décision | Réponse | Effet sur la phase 1 |
|---|---|---|
| **Q8.1 — Nom** | **AnimeTCG** | ✅ Plus rien à décider : CDC, 4 documents, **14 diagrammes UML** et **28 exports** renommés et régénérés |
| **Q9.2 — Arborescence** | « **ça convient** » | ✅ À créer telle quelle, plus rien à valider |
| **Q9.1 — Outil UML** | **Plusieurs outils** : PlantUML (sources) + éditeurs graphiques | ✅ Socle en place (`docs/uml/` + `render.py`) |
| **Q7.2 — Clients** | **100** simultanés, puis **file d'attente** (ENF-54) | ✅ **Rédigé** dans la spec technique : §11 (comportement à la saturation) + §11.13 (dimensionnement par la loi de Little) |
| **Q7.3 — Tests** | **Un test par nouvelle fonctionnalité** + cibles ≥ 30 C / ≥ 15 Java ; 80 % reporté au **4 janvier** | ✅ Inscrite dans la spec technique (§13) |
| **Q8.3 — Volume horaire** | **Variable et non garanti** | 🔧 Planification par lots courts (déjà appliquée au §7.2) |
| **Q8.2 — Pôles** | Référents « **à définir** », pas de rotation systématique | ❌ **Reste à faire** : attribuer les noms |
| **Authentification des cartes** (CU-20) | **Écartée** (« on s'en fiche ») | ✅ Retirée du périmètre, des UML, de la démo — ne pas la remplir |

**Conséquence** : le nombre de questions ouvertes du questionnaire est passé de **22 à 17**.

---

## 3. État d'avancement au 22/09/2026

| Élément attendu | Réf. | État |
|---|---|---|
| Cahier des charges interne | §5.7, O9 | 🔧 **v0.19** — contenance complète, à **figer en v1.0** |
| Diagramme de cas d'utilisation | DOC-01 | ✅ produit + exporté (UC20 grisé : écarté) |
| Diagramme de classes — serveur | DOC-02 | ✅ produit + exporté |
| Diagramme de classes — **client (MVC)** | DOC-03 | ✅ produit + exporté (`08`) |
| Diagrammes de séquence | DOC-04 | ✅ **6 produits** — connexion+contexte, battle, création de carte, reco+répudiation, revendication, **authentification + révocation** (`13`, ajouté le 22/09) |
| Diagramme d'états d'une carte | DOC-05 | ✅ produit + exporté |
| Composants / déploiement | DOC-06 | ✅ produit + exporté (+ `07` montée en charge, hors décompte) |
| **Modèle relationnel** PostgreSQL | DOC-07 | ✅ produit (`12-modele-relationnel.puml` + **`sql/schema.sql`**) |
| **Spécification technique** | DOC-09 | ✅ **DOC-09 v0.7 du 22/09** (`docs/specification-technique.md`, 17 sections / **2 046 lignes**, **49 références**, 14 ADR), **alignée sur le sujet officiel 2026-2027** — **reste la relecture d'équipe** (8 ter, 1 h) |
| **Note de format de ligne** | §5.8.2, ENF-56 | ✅ **v1.3 du 22/09** (`docs/note-format-ligne.md`, 568 lignes, **69 entrées** de vocabulaire) — vocabulaire fermé, clés, niveaux, mapping syslog/ECS/OpenTelemetry |
| Protocole applicatif figé | ENF-19 → ENF-27 | ✅ **`docs/protocole-applicatif.md` v1.2** — trame, ABNF, **23 actions**, **22 événements**, **30 codes d'erreur**, 17 sources — **sign-off du 22/09** |
| Journalisation : bibliothèques + **format de ligne** | §5.8, ENF-55 → ENF-58 | ✅ **rédigée** (§5.8 du CDC + §10 de la spec) et **format défini** par la note `docs/note-format-ligne.md` v1.0 — reste l'**installation** (tâche 12) |
| Comportement à la saturation (100 clients, file d'attente) | ENF-54 | ✅ **rédigé** (§11 de la spec : files bornées, invariants, runbook) |
| Diagramme de Gantt | DOC-08 | ⏳ phase 3 (§7.2, L11) — **hors jalon** |
| **Authentification d'une carte** | CU-20 / EF-20 / DEC-24 | 🔧 **Rétablie le 22/09** (sujet officiel) : §4.7.3 du CDC, action `card.authenticate`, table `authentications`, séquence `13`, étape de démo — **reste à confirmer (R19)** |
| **Bibliothèques hors liste blanche** | ENF-59, §5.9.1 | ✅ **Clos le 22/09** : l'encadrant confirme que **le choix est libre** → **aucune demande à envoyer** ; les dépendances (jansson, zlog, SLF4J/Logback, gcovr/JaCoCo) sont **déclarées** (version, rôle, licence, repli) au §5.9.1 |
| **Crédits des images** | ENF-60 | ❌ **à créer** : `CREDITS.md` + visuels sous licence libérale (phase 2, avant le tag `phase3`) |

**Déjà en place** : **14 diagrammes** sources PlantUML + **28 exports SVG/PNG** dans `docs/uml/exports/`, régénérables par `python3 docs/uml/render.py` (vérifiés, aucune erreur de syntaxe) — **les 5 types UML sont complets**.

---

## 4. À faire d'ici le 10 octobre

Estimations **proposées**, à ajuster. Pilotes M1–M4 : voir §6.1 du cahier des charges.

| # | Tâche | Pilote | Estim. | Quand |
|---|---|---|---|---|
| ~~1~~ | ~~I12 — relire le sujet officiel mis à jour et resynchroniser nos documents~~ — **✅ FAIT le 22/09/2026** : sujet 2026-2027 intégré, **20 écarts** traités (`docs/diff-sujet-2026.md`), CDC **v0.21**, spec **v0.4**, protocole **v1.1** | tous | ~~2 h~~ | ✅ |
| ~~2~~ | ~~Envoyer l'e-mail des questions bloquantes~~ — **✅ SANS SUITE (22/09/2026)** : les **6 questions** (R1, R2, R3, R5, R6, **R19**) étaient parties dans un seul message. ⛔ **Aucune relance** : l'équipe n'a plus d'encadrant → les 6 questions ont été **tranchées en équipe le 22/09/2026** (`questions-encadrant.md` v5) | M1 | ~~0 h 30~~ | ✅ |
| 3 | **Attribuer nominativement les 4 pôles** (Q8.2 : référents « à définir ») et compléter le §6.1 | M1 | 0 h 30 | semaine du 21/09 |
| 4 | **Réunion de validation — 15 min** *(ramenée de 30 min : tout est tranché par défaut, cf. `questions-a-repondre.md` §4)* — il reste à **entériner le relevé**, **nommer les 4 pôles** et **répartir les tâches 12-13-14**. Ancien ordre du jour **prêt** (`questionnaire-decisions.md`, section « Réponses de l'équipe », §B) : ~~13 propositions~~ ✅ **les 13 questions ont reçu une réponse d'équipe le 22/09/2026** et sont **appliquées au corpus** — il reste **2 contradictions à trancher** (Q4.1 ↔ Q2.6 ; Q1.4 ↔ Q2.6), **3 précisions de libellé** (Q4.2 « 1 mois », Q2.1 « et » ou « ou », bot de maintenance), **un point de conception** (partage hors blockchain), **7 paramètres techniques** (`spec` §15.2), **I2** (attribution nominative des pôles) et le **sign-off du protocole** (§7.4.6) | tous | 1 h | **semaine du 28/09** |
| ~~5~~ | ~~Spec — squelette et architecture en couches~~ — **✅ FAIT le 22/09** : §4.4 → §4.6 de DOC-09 (règle de dépendance, inversion limitée à `business`, arborescence, `check-layers.sh`, métriques Ca/Ce/I) | M2 | ~~2 h~~ | ✅ |
| ~~6~~ | ~~Spec — protocole applicatif~~ — **✅ FAIT le 19/09** (`docs/protocole-applicatif.md` v1.0) | M1 | ~~4 h~~ | ✅ |
| ~~7~~ | ~~Modèle relationnel : tables, clés, contraintes + DDL~~ — **✅ FAIT le 19/09** (`sql/schema.sql`, diagramme `12`) | M3 | ~~3 h~~ | ✅ |
| ~~8~~ | ~~Spec — calibrage de la difficulté et difficulté dans le hash (ENF-52)~~ — **✅ FAIT le 22/09** : §6.5 (en-tête 96 o, `payload_canon` RFC 8785, vecteurs dorés) et §8.6 → §8.9 (banc, seuil 65 536 h/s, amortissement) | M2 | ~~1 h 30~~ | ✅ |
| 8 ter | **Spec — relecture de la spécification technique** : valider les budgets de temps (§8.2), les bornes de files (§11.2) et les 10 tests de journalisation (§10.12) | tous | 2 h | semaine du 28/09 |
| ~~8 bis~~ | ~~Spec — relecture du protocole~~ — **🔧 DOSSIER PRÊT le 22/09** : §7.4 de la spec technique (19 actions validées une par une — **portées depuis à 21** avec l'authentification —, 27 codes — **portés à 29** —, bornes de validation par type) ; **3 comptes erronés corrigés** au passage. **Reste la réunion de validation** (1 h 30, signatures) | tous | 0 h (**1 h 30 restantes**) | ✅ dossier / 🔧 réunion |
| ~~9~~ | ~~Spec — comportement à la saturation~~ — **✅ FAIT le 22/09** : §11 (décidé le 19/09) complété par **§11.13** (loi de Little, options A/B sur les 60 s) et **§11.14** (invariants I1 → I7, équité FIFO, runbook) | M1 | ~~1 h~~ | ✅ |
| ~~10~~ | ~~UML — classes client MVC~~ — **✅ FAIT le 19/09** (`08-classes-client.puml`) | M4 | ~~2 h~~ | ✅ |
| ~~11~~ | ~~UML — 3 séquences restantes~~ — **✅ FAIT le 19/09** (`09`, `10`, `11`) | M2 / M4 | ~~3 h~~ | ✅ |
| 12 | **Journalisation** : installer **`zlog`** (ou l'embarquer dans l'image Podman) et déclarer **SLF4J + Logback** au `pom.xml`. **La note de format de ligne est FAITE le 22/09** (`docs/note-format-ligne.md` v1.2, **64 événements**) | M1 | **1 h** (sur 1 h 30) | semaine du 28/09 |
| 13 | **Dépôt** : créer l'arborescence validée, README, `.gitlab-ci.yml` (ENF-49), `third_party/`, `.gitignore` (`logs/`) | M1 | 1 h | semaine du 28/09 |
| 14 | **Figer le cahier des charges en v1.0**, relecture croisée, puis **poser et pousser le tag `phase1`** | M1 | 1 h 30 | **8–9 octobre** |
| ~~15~~ 🆕 | **Alignement sujet 2026-2027** — ✅ **FAIT le 22/09/2026** (vérification à la lettre, §5 de `docs/diff-sujet-2026.md`) : les **20 écarts** sont appliqués, le **diagramme de cas d'utilisation** est mis à jour (CU-20 actif, **UC-7 et UC-25 ajoutés**). ⚠️ **Reste une seule chose : envoyer R19** (confirmation de l'authentification) — c'est la tâche 2 | M2 | ~~1 h~~ | ✅ |
| ~~16~~ 🆕 | **Authentification — dossier de phase 1** — ✅ **FAIT le 22/09/2026** : les **quatre artefacts** (§4.7.3 ↔ `card.authenticate`/`card.revoke_authentication` ↔ table `authentications` ↔ séquence `13`) ont été confrontés **règle par règle** ; corrections appliquées : contradiction refus/révocation levée, `revoked_block_index` ajouté, état `SANS_OBJET` créé (règle CU-17), séquence complétée par la **branche de révocation**, vocabulaire de journal à **64** entrées. Tableau de contrôle : **§5 de `docs/diff-sujet-2026.md`** | M1 + M4 | ~~1 h~~ | ✅ |
| ~~17~~ | ~~Demande d'autorisation des bibliothèques~~ — **✅ SANS OBJET depuis le 22/09/2026** : l'encadrant a confirmé la liberté de choix. **Remplacée par** la vérification du **tableau de déclaration** (§5.9.1 du CDC ↔ §5.3.2 de la spec ↔ §12.5 des licences) | M1 | 0 h 15 | semaine du 28/09 |
| | **Total restant** | | **≈ 13 h 00** | (sur ≈ **80 h d'enveloppe de phase 1** — le sujet officiel chiffre 120 h/étudiant × 60 % = ≈ 72 h/étudiant pour la SAÉ projet, soit ≈ 80 h pour la phase 1 à 4 étudiants. **≈ 16 h consommées** : 10 h 30 le 19/09, puis ≈ 5 h 30 le 22/09 pour DOC-09 v0.3 et v0.4 **— tâches 5, 8, 9, 15 et 16 —**, la préparation du dossier de relecture du protocole et la **note de format de ligne**) |

> ⚠️ Le total est **au ras de l'enveloppe**, et le volume horaire n'est **pas garanti** (Q8.3). Si le temps manque, l'ordre de coupe est : tâche 12 (journalisation → note d'une page au lieu d'une installation), puis la **réunion de relecture 8 bis** (la relecture se fait alors à deux, avec le dossier §7.4 déjà prêt), puis la **relecture interne 8 ter** limitée aux budgets de temps. **Jamais** le protocole (fait) ni la tâche 14 (tag).

---

## 5. Les 4 risques qui peuvent faire rater le jalon

| Risque | Pourquoi c'est dangereux | Parade |
|---|---|---|
| ~~**R5 sans réponse** (forme du rendu)~~ ✅ **Levé le 22/09/2026** | Le format du rendu est **fixé en interne** (Markdown + PDF, UML intégrés, tag `phase1`) — plus aucune inconnue bloquante | Décision tracée dans `questions-encadrant.md` §1 ; à défaut, livrer **Markdown + exports SVG/PNG dans le dépôt** (la réponse n'est pas bloquante pour le contenu) |
| **I12 — CDC officiel mis à jour** | S'il ajoute ou déplace des exigences, tout ce qui est figé avant est à refaire | Relire l'officiel **avant** de rédiger la spec technique |
| **Protocole figé trop tard** (RQ-13) | Reprise **en C et en Java** en même temps en phase 2 = perte sèche | Le figer **avant** le 10 octobre (tâches 6 et 8 bis) |
| ~~**Contradictions non arbitrées**~~ ✅ **Levées le 22/09/2026** | Q4.1 ↔ Q2.6 (droits de la perdante) et Q5.2 (partage hors chaîne) sont **tranchés** : les droits restent inchangés, et le partage est **inscrit en blockchain** | Aucune action — décisions tracées dans `questions-a-repondre.md` §1 et propagées (CDC, protocole, SQL, UML) |
| **Enveloppe horaire non garantie** (Q8.3) | Un planning bâti sur l'enveloppe théorique (**≈ 72 h/étudiant**) peut s'effondrer si la présence réelle aux créneaux n'est pas au rendez-vous | Lots courts, ordre de coupe défini ci-dessus, fil conducteur d'abord |

---

## 6. Définition de « fini » — à vérifier le 9 octobre

- [ ] Le **cahier des charges v1.0** est dans le dépôt, journal des versions à jour
- [ ] La **spécification technique** est complète et renvoie vers les diagrammes (liens relatifs valides) **et vers `docs/protocole-applicatif.md`**
- [ ] Les **5 types UML** sont présents : cas d'utilisation, classes (serveur **et** client), séquence, états, composants/déploiement
- [ ] Chaque diagramme a sa **source `.puml` et son export SVG/PNG** dans le **même commit**
- [x] Le **modèle relationnel** et son **DDL** sont présents (`sql/schema.sql` + diagramme `12`)
- [x] Le **protocole applicatif** est documenté (`docs/protocole-applicatif.md` v1.0) — **dossier de relecture prêt** (§7.4 de la spec) ; **reste la réunion de validation** (tâche 8 bis)
- [x] Le **comportement à la saturation** est spécifié : 100 clients, file d'attente FIFO et ses bornes (ENF-54) — §11 y compris le **dimensionnement chiffré** (§11.13)
- [x] La **journalisation** est **cadrée** : bibliothèques et **format de ligne unique** documentés (§5.8 du CDC, §10 de la spec, **note `docs/note-format-ligne.md` v1.0**) — reste l'installation (tâche 12)
- [x] La règle « **un test par nouvelle fonctionnalité** » est écrite dans la spec technique (§13, Q7.3)
- [ ] Les **4 pôles ont un référent nommé** (§6.1, Q8.2)
- [x] L'**authentification d'une carte** (CU-20, rétablie par le sujet 2026-2027) est spécifiée : §4.7.3, action `card.authenticate`, table `authentications`, séquence `13`, étape de démonstration
- [x] Le **sujet officiel 2026-2027** est intégré : `docs/diff-sujet-2026.md` à jour, **20 écarts** traités, aucune mention d'une échéance au 2 octobre
- [x] Les **bibliothèques hors liste blanche** sont **déclarées** (§5.9.1 du CDC ↔ §5.3.2 de la spec : version, rôle, licence, repli) — *plus aucune autorisation à demander depuis le 22/09*
- [ ] Le fichier **`CREDITS.md`** existe (images sous licence libérale, crédits visibles) — *créé en phase 2, vérifié avant `phase3`*
- [ ] L'**arborescence du dépôt** est conforme au MVC (ENF-50) et le **README** explique comment lancer
- [ ] Le **tag `phase1`** est posé **et poussé** (`git push --tags`) — vérifier avec `git tag -l`
- [ ] Un **clone neuf** du dépôt contient bien tout (aucun fichier oublié en local)

---

## 7. Rappel du calendrier global

| Jalon | Date | Livrable |
|---|---|---|
| **J1 — Conception et spécification** | **samedi 10 octobre 2026** | **Spécification technique + UML, tag `phase1`** |
| J2 — Codage et tests | **du lundi 12 octobre au vendredi 18 décembre 2026** | Code serveur, clients, tests, tag `phase2` |
| J3 — Qualité | **du lundi 4 au vendredi 15 janvier 2027** | Code, tests, spec mise à jour, Gantt, tag `phase3` |
| J4 — Démonstration et soutenance | **du lundi 18 au jeudi 21 janvier 2027** | Support de présentation, tag `phase4` |

**Barème (100 points)** : conception **15** · codage et tests **40** · qualité **10** · démonstration **15** · soutenance **20**.
→ Le 10 octobre, c'est la part **« conception » (15 points)** qui se joue : la qualité de la spécification technique et des UML, **pas** la quantité de code. *(Le sujet rappelle qu'une **adaptation individuelle** de la note est possible : la répartition de la parole et du travail est donc aussi notée.)*

---

## 8. Deux échéances qui viennent juste après

| Date | Quoi | Où c'est écrit |
|---|---|---|
| **4 janvier 2027** | Décider si l'on monte la couverture globale de **60 % à 80 %** | Q7.3, annexe D.4 |
| **Phase 2 (dès le 12 octobre)** | **Un test par nouvelle fonctionnalité** — c'est la définition de fini de chaque lot | ENF-36, Q7.3 |
