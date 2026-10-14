# Diagrammes UML — AnimeTCG (SAÉ BUT2 S3)

Sources **UML** du projet, au format **PlantUML** (texte, versionnable, diff-friendly).
Le format Mermaid n'est **pas** utilisé dans ce projet.

| Fichier | Diagramme | Référence cahier des charges | Phase | État |
|---|---|---|---|---|
| `01-cas-utilisation.puml` | Cas d'utilisation — **CU-20 (authentification) actif** | DOC-01, §5.7 | 1 | ✅ produit *(mis à jour le 22/09/2026)* |
| `02-classes-serveur.puml` | Classes (serveur C) | DOC-02, §5.7 | 1 | ✅ produit |
| `03-etats-carte.puml` | États d'une carte | DOC-05, §4.4 | 1 | ✅ produit |
| `04-sequence-connexion-contexte.puml` | Séquence — connexion + reconstitution du contexte | DOC-04, CU-5/CU-7 | 1 | ✅ produit |
| `05-sequence-battle.puml` | Séquence — battle complète | DOC-04, CU-15 → CU-17 | 1 | ✅ produit |
| `06-composants-deploiement.puml` | Composants + déploiement | DOC-06, §5.1 | 1 | ✅ produit |
| `07-montee-en-charge.puml` | Déploiement — 3 paliers de montée en charge (100 / 10 000 / 1 M clients) | Annexe E (Q7.2) | 3 (document de perspective) | ✅ produit |
| `08-classes-client.puml` | Classes — client Java **MVC** (vue / contrôleur / modèle / réseau) | DOC-03, ENF-50 | 1 | ✅ produit |
| `09-sequence-creation-carte.puml` | Séquence — création d'une carte (validation, minage, base, diffusion) | DOC-04, CU-11 | 1 | ✅ produit |
| `10-sequence-recommandation-repudiation.puml` | Séquence — recommandation puis répudiation (valeur recalculée) | DOC-04, CU-12/CU-14 | 1 | ✅ produit |
| `11-sequence-revendication.puml` | Séquence — revendication : acceptation, **refus sans recours**, **transfert par silence (30 j)** | DOC-04, CU-19 | 1 | ✅ produit |
| `12-modele-relationnel.puml` | **Modèle relationnel PostgreSQL** (**14 tables** — la chaîne + 13 projections, dont `authentications` —, contraintes, index) | **DOC-07**, §5.4 | 1 | ✅ produit |
| `13-sequence-authentification.puml` | **Séquence — authentification *et révocation* d'une carte** (CU-20, DEC-24) : porteur légitime → contrôle → minage → transaction → diffusion | **DOC-04**, §4.7.3 | 1 | ✅ produit |
| `14-sequence-partage.puml` 🆕 | **Séquence — partage ciblé entre amis** (DEC-15, réponse Q5.2) : contrôle de la liste d'amis et de l'anti-spam → notification socket → acceptation = *le demandeur se déplace* / refus = statu quo — **aucun bloc miné** ⚠️ | **DOC-04**, §4.8 | 1 | ✅ produit (22/09/2026) |

## Exports produits

Les **14 diagrammes** sont exportés en **SVG** (recommandé : vectoriel, net à l'impression et en projection) et en **PNG** dans `docs/uml/exports/`, en **source et export versionnés ensemble** : la source `.puml` est modifiable, l'export est ce qui s'affiche dans la spécification technique.

```bash
python3 docs/uml/render.py     # régénère exports/ en SVG + PNG à partir des .puml
```

Le script encode la source au format d'URL PlantUML (deflate + base64) et interroge le serveur public — aucun outil à installer. Il **vérifie** chaque rendu (taille anormale ou `Syntax Error` → signalé en fin d'exécution) : relancer le script après chaque modification d'un diagramme, avant de committer.

> ⚠️ **Règle de versionnage** : on commit **toujours** le `.puml` et son export **dans le même commit**. Un export seul ne se relit pas, une source seule ne s'affiche pas dans la spec technique.

## Reste à produire (phase 3 uniquement)

- `15-deploiement-demo.puml` — disposition des postes le jour de la soutenance (phase 3) — **numéro 15**, puisque `13` (authentification) et `14` (partage) sont pris
- `gantt` — diagramme de Gantt du projet (phase 3, non UML mais exigé par le sujet) → DOC-08

> ✅ **État au 22/09/2026** : **14 diagrammes produits et exportés** (28 fichiers PNG + SVG), dont la **séquence d'authentification** (`13`, rétablie par le sujet officiel 2026-2027). Les **5 types UML retenus sont complets**. Il ne reste que deux artefacts, tous deux **hors jalon de phase 1** et prévus en phase 3.

## Livrables liés (hors diagrammes)

| Fichier | Contenu | Référence |
|---|---|---|
| `sql/schema.sql` | **DDL PostgreSQL exécutable** (v1.5) : **14 tables** (dont `legitimation_likes`, `friendships`, `shares`), contraintes `CHECK`, index partiels, principes source de vérité / projections | DOC-07, §5.4 |
| `docs/protocole-applicatif.md` | **Spécification du protocole v1.2** : trame, grammaire ABNF, **23 actions**, **22 événements**, **30 codes d'erreur**, versionnage, robustesse, sources industrielles | ENF-19 → ENF-27 |
| `docs/note-format-ligne.md` | **Note de format de ligne v1.3** (22/09/2026) : forme d'une ligne de journal, rattachée à la **RFC 5424** (structure, `MSGID`, `NILVALUE`, facilities), à **logfmt** (queue `clé=valeur`), à **ECS** et à **OpenTelemetry** ; vocabulaire fermé de **64 événements**, clés normalisées, table des niveaux, mapping syslog/JSON, contrat de compatibilité | §5.8.2, ENF-56 |
| `docs/specification-technique.md` | **Spécification technique DOC-09 v0.7** (22/09/2026) : 17 sections en points de vue (IEEE 1016), **architecture en couches** (§4.4 → §4.6), **journalisation** (§10, ENF-55→58), **saturation** (§11 + loi de Little §11.13), **calibrage de la difficulté et hachage canonique** (§6.5, §8.6 → §8.9), **relecture du protocole** (§7.4), sécurité/RGPD/licences, traçabilité, **12 ADR**, 38 références | **DOC-09**, ENF-51 → ENF-58 |

## Comment les produire / les rendre

### Option 1 — PlantUML en ligne (le plus rapide)
1. Ouvrir <https://www.plantuml.com/plantuml/uml/>.
2. Coller le contenu d'un fichier `.puml`.
3. Exporter en **PNG** ou **SVG** et déposer l'export dans le dépôt (ex. `docs/uml/exports/`).
   *Versionner à la fois la source `.puml` et l'export : la source est modifiable, l'export est ce qui s'affiche dans la spec technique.*

### Option 2 — PlantUML en local
```bash
# Debian / Ubuntu
sudo apt install plantuml graphviz
plantuml -tpng docs/uml/*.puml      # génère les PNG à côté des sources
plantuml -tsvg docs/uml/*.puml      # variante SVG (recommandée pour la soutenance)
```

### Option 3 — Via VS Code
Extension **PlantUML** (jebbs) + Java installé : aperçu direct avec `Alt+D`, export en un clic.

### Option 4 — Intégration GitLab
Si l'instance GitLab de l'IUT a l'intégration PlantUML activée, les blocs `@startuml … @enduml` placés dans un fichier `.md` sont rendus automatiquement. Sinon, ce sont les exports PNG/SVG qui font foi. **À vérifier en phase 1** (question I11 du questionnaire de décisions).

### Option 5 — Outil graphique

> ℹ️ **Décision Q9.1 (18/09/2026)** : **plusieurs outils sont utilisés** — PlantUML pour les **sources** (référence versionnée) et, en complément, les éditeurs graphiques ci-dessous pour des rendus soignés (soutenance). **La source `.puml` reste toujours commitée** avec son export.
StarUML, draw.io ou Modelio conviennent pour redessiner un diagramme si un rendu plus soigné est attendu pour la soutenance. Dans ce cas, **conserver la source PlantUML** comme référence versionnée.

## Convention de nommage

`NN-type-sujet.puml` — numéro d'ordre, type de diagramme, sujet traité.
Les exports portent le même nom avec l'extension `.png` ou `.svg`.
