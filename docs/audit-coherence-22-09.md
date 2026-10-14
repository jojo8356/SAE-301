# Audit de cohérence du corpus — 22/09/2026 (fin de journée)

| | |
|---|---|
| **Objet** | Vérifier que **tout le corpus** reflète les décisions des 22/09/2026 : **n° 41** (questionnaire rempli par l'équipe), **n° 43** (plus d'encadrant → décisions d'équipe), **n° 44** (tous les défauts = valeurs finales, **sauf le minage 0,3–2 s**) |
| **Périmètre** | 11 documents Markdown · 14 sources PlantUML + 28 exports · `sql/schema.sql` (DDL) — **lecture intégrale**, pas d'échantillonnage |
| **Méthode** | Contrôles croisés automatiques (motifs, comptes, versions, tableaux) + relecture ligne à ligne des zones sensibles ; `pglast` pour le DDL ; `render.py` pour les exports |
| **Conclusion** | **Tout est aligné.** 60+ contrôles automatiques verts, **44 corrections** appliquées (détaillées au §1), **0 incohérence résiduelle** hors historique assumé. **Seul point ouvert : I2** (noms sur les 4 pôles) |

> **Règle de lecture.** Les **journaux de version** et les sections marquées *historique* conservent volontairement les formulations de leur date (anciennes valeurs, anciens comptes, « 60 secondes », « FUSIONNEE »). Seul le **contenu actif** est corrigé — c'est ce que contrôle cet audit.

---

## 1. Ce qui a été corrigé (44 corrections, une par ligne)

### 1.1. Décision n° 44 — minage et valeurs définitives

| Fichier | Ce qui était faux | Corrigé en |
|---|---|---|
| `docs/specification-technique.md` §8.6 | « quelle difficulté puis-je tenir en **≤ 1 s** sur CETTE machine ? » | « dans la **fourchette 0,3–2 s** » |
| `docs/specification-technique.md` §8.7 | bande k=3 `4 096 → 65 535` et seuil à `65 536` | bande **`2 048 → 32 767`**, avertissement < 2 048, justification du seuil **32 768 h/s** (65 536 / 2 s) |
| `docs/specification-technique.md` §4.5 | `rights.c/.h  # légitimation 60 s` | `# légitimation (likes)` — la légitimation n'a plus d'échéance |
| `docs/specification-technique.md` §11.13.3 | titre « le réglage inverse (**à trancher en réunion**) » | « **option B — non retenue** » |
| `cahier-des-charges-sae-s3.md` annexe F ligne 14 | doublon « notification socket » + « **hors blockchain** » contradictoire | ligne réécrite : **option A**, partage inscrit en blockchain, amitié hors chaîne |
| `questionnaire-decisions.md` DEC-15 (×2 : §A et récapitulatif) | « **inscrit en blockchain** … **hors blockchain** » | contradiction supprimée ; date alignée (22/09/2026) |
| `questionnaire-decisions.md` **DEC-25** | « ✅ **Non retenu** dans le périmètre minimal … `share.received` réservé » | « ✅ **RETENU (option A)** » — actions **22/23**, événement **activé**, `shares.block_index` |
| `docs/diff-sujet-2026.md` écart 12 + §5 | idem DEC-25 (deux endroits) | idem — **retenu**, actions 22/23, `shares.block_index` |

### 1.2. Décision n° 43 — plus d'encadrant

| Fichier | Ce qui était faux | Corrigé en |
|---|---|---|
| `cahier-des-charges-sae-s3.md` en-tête | « Encadrant : Johan » (sans date de fin) | « **jusqu'au 22/09/2026** (l'équipe décide seule depuis) » |
| `cahier-des-charges-sae-s3.md` §2 | ligne **R1** « ⚠️ Pas encore obtenu — **Reste ouvert** (§13) » | ✅ **Tranché** : barème du sujet au mot + grille interne (I9/I13) |
| `cahier-des-charges` §2 | « **Action immédiate** : l'encadrant … doit être relu et nos documents resynchronisés » | ✅ **Fait le 22/09/2026** (20 écarts traités, I12 clos) |
| `cahier-des-charges` §16 « Prochaines étapes » | « restent ouvertes les décisions d'équipe … » ; « questions **envoyées** à l'encadrant » | « **tout est tranché**, sauf **I2** » ; « questions **rédigées** » |
| `docs/specification-technique.md` §1 (tableau des vues) | « Pour qui : jury, **encadrant** » (§2 et §14) | « jury, **équipe** *(encadrant jusqu'au 22/09/2026)* » |
| `docs/specification-technique.md` §12.4 | « Qui accède aux journaux ? … et l'**encadrant** » | « l'équipe *(et l'encadrant jusqu'au 22/09/2026)* » |
| `docs/protocole-applicatif.md` §15 | titre « ce qui **reste ouvert** » ; « **Ce qui reste ouvert** après relecture » | « ce qui **a été tranché** » ; « **Rien n'est plus ouvert** » (8 en vol, 60 s, jansson **ratifiés**) |
| `docs/note-format-ligne.md` §2 | usage « Exploitation : **l'encadrant**, le futur mainteneur » | « **l'équipe**, le futur mainteneur » |
| `questions-encadrant.md` §4 | « Ce qui reste ouvert — **en interne seulement** » + liste des paramètres à trancher | « **plus rien, sauf un point d'organisation** » — I2, 5 minutes |
| `questions-encadrant.md` §5 | « les 6 questions telles qu'elles ont été **envoyées** » | « telles qu'elles avaient été **rédigées** » |
| `questionnaire-decisions.md` chapeau + pied | « 6 questions bloquantes **envoyées** » ; « à remplir puis à joindre » | « **rédigées** » ; « **rempli le 22/09/2026 — il fait foi** » |
| `synthese-41-reponses-equipe.md` §4 | « Ce qui **reste** ouvert » + 4 items « à confirmer / à trancher » | « ✅ **tranché le 22/09/2026** (sauf I2) » + réponses en clair (Q2.6 prime, deux sorties, depuis la demande, union) |
| `docs/diff-sujet-2026.md` écart 6 + §5 | « Réponse obtenue …  vous êtes **libres** » (destinataire inexistant) | « **Décision d'équipe** : choix libre — l'équipe retient, déclare et justifie » |
| `docs/diff-sujet-2026.md` écart 8 | couverture « en **demande d'autorisation** avec repli » | **choix d'équipe déclaré** (gcovr / JaCoCo) |

### 1.3. Décision n° 41 — réponses de l'équipe

| Fichier | Ce qui était faux | Corrigé en |
|---|---|---|
| `questionnaire-decisions.md` **I1** | ligne **vide** (nom de la plateforme) | ✅ **AnimeTCG** (18/09/2026) |
| `questionnaire-decisions.md` **I10** | ligne **vide** (arborescence) | ✅ **validée**, sans `bot/` à la racine (DEC-14) ; création = tâche 13 |
| `questionnaire-decisions.md` **I11** | « exports en place (**7 diagrammes**) » | « **14 diagrammes / 28 fichiers** (22/09/2026) » |
| `questionnaire-decisions.md` **DEC-29** | « 🔧 linserv **en référence** » | ✅ « en **recette** ; Podman en **développement** » (aligné sur la spec §15.2) |
| `questionnaire-decisions.md` Q4.2 | « ⚠️ **Formulation à confirmer** » | ✅ **Tranché** : délai **depuis la demande** |
| `questionnaire-decisions.md` §B ligne 9 | fragment d'ancienne ligne encollé (`… |vol (8), battement…`) | ligne nettoyée |
| `questionnaire-decisions.md` §A | en-tête à **4 colonnes**, lignes à **5** (date orpheline) | colonne **« Retenue le »** ajoutée — tableau réparé |
| `questionnaire-decisions.md` §C | « le catalogue de **64** entrées devra être recompté » | « **porté à 69 entrées** ✅ *(fait)* » |
| `questionnaire-decisions.md` fin | items 6 et 7 dans le désordre | réordonnés |
| `questionnaire-decisions.md` récapitulatif | `DEC-15`, `I11` en `18/09/2026` incohérents | dates alignées |

### 1.4. Versions, comptes et numérotation de diagrammes

| Fichier | Ce qui était faux | Corrigé en |
|---|---|---|
| `cahier-des-charges` en-tête + §5.7 | **DOC-09 v0.3** | **v0.7** |
| `cahier-des-charges` annexe sources | note de format **v1.0 / 559 lignes** (×2) ; SQL **v1.4** ; « 46 références » | **v1.3 / 568 lignes** ; SQL **v1.5** ; **2 046 lignes, 49 références** |
| `cahier-des-charges` §5.3 | « Avancement au 19/09 : **12 diagrammes** » et `13-deploiement-demo.puml` | « **14 diagrammes** (28 fichiers) » et **`15-deploiement-demo.puml`** (13 et 14 pris) |
| `cahier-des-charges` §1 exclusions | ligne « bot » **collée à une ancienne version** (5 cellules pour 2) | ligne réécrite en **2 cellules** |
| `cahier-des-charges` journal 0.23 | `\|` parasite cassant le tableau | séparateur remplacé par du texte |
| `checklist-phase1-10-10.md` en-tête | CDC **v0.21**, spec **v0.4**, protocole **v1.1** | **v0.24 / DOC-09 v0.7 / v1.2**, + SQL **v1.5** |
| `checklist` tableau des livrables | DOC-09 **v0.4 / ~2026 lignes / 46+ réf.** ; note **v1.0 / 559 l.** ; protocole « **29 codes** » | **v0.7 / 2 046 l. / 49 réf.** ; **v1.3 / 568 l. / 69 entrées** ; **30 codes** + sign-off |
| `docs/uml/README.md` | SQL **v1.4** ; DOC-09 **v0.4** ; « reste à produire : **14**-deploiement-demo » | SQL **v1.5** ; DOC-09 **v0.7** ; **`15-deploiement-demo`** |
| `docs/specification-technique.md` §4.5, §7.4.1 | `schema.sql … **v1.1**` ; « protocole **v1.1** est figé » | **v1.5** ; **v1.2** |
| `docs/uml/12-modele-relationnel.puml` | `CHECK (state IN (… , 'SANS_OBJET'))` + note CU-17 | contrainte alignée sur le SQL v1.5 (`SANS_OBJET` retiré) — **exports régénérés** |
| `docs/diff-sujet-2026.md` §3 | versions intermédiaires (`v0.20 → v0.21`, `v0.3 → v0.4`, `v1.0 → v1.1`, `v1.1 → v1.3`, `v1.0 → v1.2`) | versions **de fin de journée** : **v0.24**, **v0.7**, **v1.2**, **v1.5**, **v1.3** |
| `synthese-41-reponses-equipe.md` | spec **0.5 (2 038 l.)**, « §15.2 **point ouvert n° 11** », SQL **1.4 (437 l.)**, questionnaire **1 114 l.**, « points ouverts (§B) » | **0.7 (2 046 l.)**, « paramètres **tous tranchés** », **1.5 (444 l.)**, **1 116 l.**, « décisions (§B) » |
| `docs/diff-sujet-2026.md` §2 et §5 | « CU-6 et CU-24 restent des options **non retenues** » ; cellule SQL `SANS_OBJET` | CU-6 **retenu comme outil de test**, **CU-24 retenu** ; cellule SQL corrigée |

---

## 2. Ce qui a été vérifié **sans** changement (et qui doit le rester)

| Famille | Constat |
|---|---|
| **Soixante secondes, `FUSIONNEE`, « 16/32 », « AniCard », « scrutin »** | mentionnés **uniquement** dans les journaux de version ou dans les encadrés *« Historique — mécanisme précédent, remplacé »* → **à conserver tels quels** (traçabilité de l'évolution) |
| **« ≤ 1 s »** | plus aucune occurrence active ; il n'en reste qu'au journal de la version 0.2 de DOC-09 |
| **Comptes du corpus** | 14 tables · 23 actions · 22 événements · 30 codes · 17 cas · 69 entrées de journal · 14 diagrammes · 28 exports — **cohérents partout** (les 12/13/21/29 ont disparu du contenu actif) |
| **Zéro REST / zéro HTTP** | seules occurrences : exclusions du CDC, annexe **E** (comparatif, exception assumée) et citations de réponses d'équipe — **aucune API dans le périmètre** |
| **libuv** | partout marquée **écartée** ; aucune mention active dans un choix technique |
| **Encadrant** | les mentions restantes sont **datées du 18/09/2026** (décisions réellement prises avec lui) ou explicitement marquées « jusqu'au 22/09/2026 » → légitimes |
| **Valeurs définitives** | file **100/60 s**, **8** en vol, heartbeat **60 s**, `JSON_MAX_DEPTH` **32**, port **7001**, `linserv-info-01` **recette** + Podman **dev** — alignées dans CDC, DOC-09 §15.2, protocole, questionnaire, `questions-a-repondre.md` |
| **Dates et barème** | 10/10/2026 (samedi), 12/10 → 18/12/2026, 4 → 15/01/2027, 18–21/01/2027 (jeudi) ; barème 15 / 40 / 10 / 15 / 20 — **aucune trace de « 2 octobre »** ni de « 18–19/01 » |
| **DDL PostgreSQL** | `pglast` : **syntaxe valide** ; **14 tables** ; `ARCHIVEE`, `battle_outcome`, `shares.block_index` + `CHECK share_bloc` |
| **UML** | 14 sources / 28 exports, **exports plus récents que les sources** (régénérés après correction du `12`) |
| **`q4.3-revendication.txt`** | **pièce brute** de la réponse d'équipe (options A→E telles qu'imprimées, dont le « prix » et le « scrutin » **non retenus**) — conservée comme **source de citation**, non comme document de référence : rien à corriger |
| **Tableaux Markdown** | **0 anomalie** de nombre de colonnes sur les 11 documents (3 tableaux étaient cassés : §2 du CDC, §A du questionnaire, journal 0.23) |

---

## 3. Ce qui reste

| # | Point | Nature | Où |
|---|---|---|---|
| 1 | **I2 — noms sur les 4 pôles** (M1 réseau & protocole · M2 chaîne & métier · M3 persistance & données · M4 client & IHM) | organisation (5 min) | CDC §6.1 et §13 ; `questions-a-repondre.md` §1 n° 9 |
| 2 | Tâches de dépôt : `THIRD-PARTY.md` (M3, avant `phase3`), `CREDITS.md` (M4, phase 2), arborescence + README + `.gitlab-ci.yml`, **figement v1.0 + tag `phase1`** | planifiées | `checklist-phase1-10-10.md` tâches 12 à 14 |

*Aucun arbitrage n'est en attente : la réunion de 15 minutes sert à entériner le relevé, nommer les pôles et répartir les tâches 12–14.*
