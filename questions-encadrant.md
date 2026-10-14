# Décisions prises en autonomie — ex-questions à l'encadrant (AnimeTCG)

> ## 📌 Changement de cadre — 22/09/2026
>
> **L'équipe n'aura plus d'encadrant : elle définit tout elle-même.** Les questions qui attendaient une réponse extérieure sont donc **converties en décisions d'équipe**, prises en s'appuyant sur la **seule autorité écrite disponible : le sujet officiel 2026-2027** (et, à défaut, sur le choix le plus **défendable devant un jury**).
>
> **Ce que cela change concrètement** :
> 1. **Aucune réponse extérieure n'est plus attendue** — les 6 questions bloquantes et les 10 questions d'appoint sont **tranchées** (§1 et §2 ci-dessous), avec justification écrite.
> 2. **Le sujet écrit prime sur tout** : là où une consigne orale antérieure contredisait le sujet, c'est le sujet qui s'applique (cas de l'**authentification**, DEC-24).
> 3. **Plus de demande d'autorisation de bibliothèques** (le sujet §10 la prévoyait) : remplacée par une **déclaration écrite** dans le dépôt (`THIRD-PARTY.md` + §5.9.1 du CDC), avec **repli documenté** pour chaque dépendance hors liste.
> 4. **Toute divergence résiduelle est écrite**, pas attendue : elle figure dans `questions-a-repondre.md` (§1, arbitrages internes).
>
> Ce fichier garde la **trace des questions telles qu'elles ont été posées** (§5) et de **toutes les réponses déjà obtenues** (§6), pour qu'on ne redemande jamais deux fois la même chose.

---

## 1. Les 6 questions bloquantes — **tranchées en équipe le 22/09/2026**

| Réf. | Question posée (22/09/2026) | ✅ Décision d'équipe | Justification (autorité écrite) | Effet dans le corpus |
|---|---|---|---|---|
| **R19** | Confirmez-vous l'**authentification d'une carte** (CU-20) que nous avons rétablie ? | **Authentification MAINTENUE** — fonctionnalité du périmètre, développée (lot L7) | Le **sujet écrit** l'exige : §3 (services), **CU-20**, §5.5, et il la liste dans les fonctionnalités à démontrer (§12.1). Une consigne orale ne retire pas une exigence écrite | **DEC-24 confirmée** ; plus aucun écart signalé ; §4.7.3, EF-20, `card.authenticate`/`card.revoke_authentication`, table `authentications`, diagramme `13` |
| **R1** | Répartition des **100 points** ; contenu attendu des 15 pts de conception, des 40 pts codage/tests, des 10 pts qualité ; critères d'un « dépôt GitLab organisé et exploitable » ; support de l'adaptation individuelle | **Le barème du sujet s'applique au mot** : **conception 15** → cahier des charges + spécification technique + UML (tag `phase1`) · **codage et tests 40** → code conforme aux 6 contraintes du sujet + **1 test par fonction** (≥ 30 C, ≥ 15 Java) + 11 cas de robustesse · **qualité 10** → SOLID + patrons justifiés, 0 fonction > 200 lignes, commentaires et en-têtes **en anglais**, doc à jour, **Gantt** · **démonstration 15** → scénario maîtrisé, 4 clients humains, redémarrage à chaud, altération détectée · **adaptation individuelle 20** → soutenance + **historique de commits** + réponses. **« Dépôt organisé et exploitable »** = notre grille interne : tags `phase1`→`phase4`, README à jour, arborescence validée, **CI verte**, MR relues, aucun secret en clair | Aucune grille plus fine n'existe : le sujet est la grille. On documente **nos propres critères** pour montrer la démarche. Politique de branches : **tronc + branche par fonctionnalité + merge request obligatoire** (ENF-39) | §2.2 (O11), §5.10, annexe D (checklist de revue), `docs/specification-technique.md` §12.4 (CI) et §13 (tests) |
| **R2** | **libuv** accepté, ou attendez-vous les **appels système BSD à la main** (`socket`/`bind`/`listen`/`accept`/`poll`/`epoll`) ? | **Sockets BSD à la main, boucle d'événements `epoll` — libuv écarté** | Le sujet impose une communication **par sockets de A à Z** et ne liste que les **interfaces POSIX** (§10) ; **libuv n'y figure pas** et il n'y a plus personne pour l'autoriser. Écrire la boucle soi-même est aussi le **cœur pédagogique** de l'annexe E (`select` O(n) → `epoll` O(1)) | Annexe A.1 réécrite, `docs/specification-technique.md` §4.4/§12.5, diagramme `07`, annexe B (liens : `man 7 epoll`) |
| **R3** | Confirmez-vous **≈ 72 h/étudiant** (120 h × 60 %) ? | **Hypothèse de planification retenue : ≈ 72 h/étudiant (≈ 288 h à 4)**, marquée **non garantie** ; re-chiffrage à chaque fin de lot | Le sujet **écrit** annonce 120 h/étudiant pour trois projets ; la part SAÉ projet (60 %) est notre hypothèse de travail, cohérente avec la réserve **Q8.3** (« volume réel non garanti ») | H8, §6.2, §7.2 : planification par **lots courts** maintenue ; aucun engagement de volume |
| **R5** | Forme du rendu de phase 1 : un seul document ? Quel format ? UML intégrés ou sources suffisantes ? | **Un document unique = la spécification technique** (rédigé en **Markdown**, export **PDF** joint), taggé **`phase1`** dans le dépôt ; **exports SVG/PNG intégrés au document** *et* **sources PlantUML** conservées dans le dépôt | Le sujet écrit désigne le livrable : *« un document appelé spécification technique »*, taggé dans le dépôt, et il exige des **diagrammes UML** (donc visibles dans le rendu). Contenu = **cahier des charges augmenté des fonctionnalités singulières** du projet | §5.7, arborescence `docs/`, checklist de rendu (tâche 14), `docs/uml/README.md` (source + export versionnés ensemble) |
| **R6** | Un **minimum de clients humains** est-il attendu en démonstration ? | **4 clients humains minimum** (un par membre), **plus** le banc de charge qui prouve les **100 connexions** | Le sujet écrit dit « plusieurs clients humains » sans chiffre : nous retenons l'effectif de l'équipe comme minimum garanti, et le dimensionnement à 100 est **mesuré** par le client de test (REC-03) | §9.2 (scénario de démonstration), REC-03, annexe D.5 |

---

## 2. Les questions d'appoint — **tranchées en équipe le 22/09/2026**

| Réf. | Question | ✅ Décision d'équipe | Pourquoi ce choix |
|---|---|---|---|
| **R7** | Redémarrage à chaud et altération : jusqu'où en direct ? | **Les deux joués en direct** devant le jury : restauration < 10 s sur **1 000 blocs**, et **modification frauduleuse en base faite à la main** | C'est ce que la démonstration doit prouver (REC ; §9.2). Une sauvegarde de secours est prête en cas d'incident |
| **R8** | Format d'en-tête des fonctions | **Bloc normalisé maison** en anglais (rôle, paramètres, valeur de retour, effets de bord) — convertible Doxygen si le temps le permet | Le sujet impose « une en-tête pour les fonctions principales » sans format ; un format **unique et vérifiable** vaut mieux qu'un standard non tenu (ENF-35) |
| **R9** | Framework de tests en C et rapport de couverture | **Tests maison + cible `make test`**, **aucun framework externe** ; rapport de couverture (`gcovr` C, JaCoCo Java) **déposé en phase 3** | Le sujet **interdit** explicitement un framework de tests C (§11) ; les assertions maison + banc de charge suffisent et restent lisibles par le jury |
| **R11** | Phase 3 : réécriture de la spec ou révision ciblée ? | **Révision ciblée** (diagrammes de séquence et protocole alignés sur le code, **Gantt ajouté**) + **journal de version** | Trace la démarche (ENF-61 : la qualité se démontre) ; une réécriture consommerait un lot entier |
| **R13** | Durée de la démonstration / présentation, jury, questions chronométrées | **15 min de démonstration + 15–20 min de présentation**, parole **équilibrée entre les 4 membres**, questions non chronométrées *(hypothèse de préparation, à ajuster le jour J)* | Le sujet écrit fixe la structure (démo 15 pts, présentation) sans minutage précis : on prépare un déroulé de **30–35 min** et un format court de 15 min |
| **R14** | Salle, réseau, répétition | **4 à 5 postes sur le réseau local de l'IUT** ; **répétition demandée en janvier**, plus une **répétition interne** avant le 15/01 | Le sujet prévoit des clients sur des postes distincts : la topologie doit être testée **avant** le jour J |
| **R15** | Poste de démonstration et limites système | **Nos machines + conteneur Podman** ; `ulimit -n` relevé (≥ 4 096) et `net.core.somaxconn` documentés dans le **runbook** | Contrainte « budget 0 € » ; les limites système sont écrites pour être contournées **et expliquées** (ENF-16, annexe D.2) |

*Questions fermées depuis :* **R4** (dates), **R10** (bot), **R12** (Podman), **R16** (CI), **R17** (MVC), **R18** (SHA-256 = OpenSSL imposé par le sujet) — voir §6.

---

## 3. Conséquences immédiates des décisions autonomes

| # | Sujet | Décision | Où |
|---|---|---|---|
| 1 | **Bibliothèques hors liste du §10** | **Conservées et déclarées** : `jansson` (JSON C), `zlog` (journal C), `SLF4J + Logback` (journal Java), `gcovr`/`JaCoCo` (couverture). **Repli documenté** pour chacune (`parseur JSON maison`, `rxi/log.c`, `java.util.logging`, `comptage maison`) si une dépendance devient un risque de build | §5.9.1 (ENF-59), `THIRD-PARTY.md`, DOC-09 §5.3.2 et §12.5 |
| 2 | **Boucle d'événements** | **`epoll` écrit à la main** — libuv **écarté**, aucune dépendance réseau | Annexe A.1, DOC-09 §4.4, UML `07` |
| 3 | **Difficulté de preuve de travail** | **Auto-calibrage conservé** (bornes {3, 4} du sujet écrit) ; ✅ **cible = fourchette 0,3 s – 2 s** (arbitrage du 22/09/2026) ; la fourchette 0,3–2 s d'une consigne orale reste **historique** | §5.2.1, DEC-20, DEC-22, §15.2 point 4 |
| 4 | **Politique de branches** | **Tronc stable + branche par fonctionnalité + merge request relue** ; tags `phase1`→`phase4` aux jalons | ENF-39, §5.10 |
| 5 | **Support de présentation (phase 4)** | **Markdown + export PDF**, déposé sous le tag `phase4`, avec le **Gantt** et le scénario de démonstration | §7.2, lot L12 (phase 4) |
| 6 | **Grille « dépôt organisé et exploitable »** | Nos critères écrits (tags, README, arborescence, CI verte, MR relues, aucun secret) | §5.10, annexe D |

---

## 4. Ce qui restait ouvert — **plus rien, sauf un point d'organisation**

| Où | Quoi |
|---|---|
| Arbitrages métier et techniques | ✅ **Tous tranchés le 22/09/2026** — les **10 points** (Q4.1 ↔ Q2.6, Q1.4 ↔ Q2.6, Q4.2, Q2.1, Q5.2, bot de maintenance, Q6.3 cas 2, Q4.3, noms des 4 pôles, **sign-off du protocole**) sont **clos** : le relevé définitif est dans **`questions-a-repondre.md` §1** |
| Paramètres techniques (spec §15.2) | ✅ **Ratifiés aux valeurs par défaut** : file **100/60 s**, 8 en vol, heartbeat 60 s, `JSON_MAX_DEPTH` 32, port 7001, `linserv-info-01` en recette / Podman en développement ; **seule exception : le minage 0,3–2 s** |
| CDC §13 — **I2** | ⚠️ **Seul point restant** : l'**attribution nominative des 4 pôles** (M1 → M4) — 5 minutes en réunion |

---

## 5. Historique — les 6 questions telles qu'elles avaient été rédigées (22/09/2026)

<details>
<summary>Texte de l'envoi (conservé comme trace)</summary>

> **Objet** : SAÉ S3 — groupe 3C — 6 dernières questions avant le rendu du 10 octobre
>
> 1. **Barème** : pourriez-vous détailler les 100 points (contenu attendu des 15 pts de conception, des 40 pts codage et tests, des 10 pts de qualité) ? Comment est apprécié un « dépôt GitLab organisé et exploitable » ? La politique de branches et de commits est-elle imposée ?
> 2. **Sockets** : acceptez-vous **libuv** pour la boucle d'événements, ou souhaitez-vous une implémentation des appels système BSD à la main ?
> 3. **Volume horaire** : confirmez-vous ≈ 72 h/étudiant (120 h × 60 %), et ces heures sont-elles réellement dégagées ?
> 4. **Rendu de phase 1** : un seul document de spécification technique, déposé et taggé `phase1` ? Un format particulier ? Le contenu attendu est-il bien le cahier des charges augmenté des fonctionnalités singulières, avec les diagrammes ?
> 5. **Démonstration** : un minimum de clients simultanés est-il attendu le jour de la soutenance ?
> 6. **Authentification** : le sujet l'exige (CU-20, §12.1) alors qu'elle nous avait été dite hors sujet le 18/09 ; nous l'avons **rétablie** — confirmez-vous ?

</details>

**Statut** : ⛔ **sans objet depuis le 22/09/2026** — ces questions ont été **tranchées en équipe** (§1) : il n'y a plus de destinataire. **Aucune relance n'est à envoyer.**

---

## 6. Réponses déjà obtenues (à ne jamais redemander)

| Sujet | Réponse |
|---|---|
| Domaine des cartes | ✅ Animes — « ce choix est libre et me convient parfaitement » |
| Difficulté de la preuve de travail | ✅ **3 ou 4**, calibré sur la machine ; exploration en fin de projet seulement |
| Seuils de couverture de tests | ✅ Notions vues en R402/S4 (SonarQube) ; mise en place dès le développement, mesure en phase 3 ; outils libres |
| Diagrammes UML | ✅ **5 types max**, PlantUML, granularité croissante |
| Taille de l'équipe | ✅ 4 étudiants acceptés (groupes de 4 à 5) |
| Volume horaire | ✅ 120 h/étudiant pour les trois projets ; le détail par ressource est devenu une **hypothèse d'équipe** (§1, R3) |
| Dates des phases | ✅ Phase 1 **10/10/2026**, phase 2 **12/10 → 18/12/2026**, phase 3 **4 → 15/01/2027**, phase 4 **18 → 21/01/2027** |
| Client automatique (bot) | ✅ **Bot de test et bot de maintenance** ; hors quorum en production, **affiché** avec un marqueur distinct (Q5.1) |
| Podman | ✅ Accepté |
| Intégration continue | ✅ Runners CI disponibles → **ENF-49** |
| Architecture | ✅ **MVC imposé** pour le client (**ENF-50**) |
| Limite de clients connectés | ✅ **100** simultanés puis file d'attente (DEC-13) — tranché en interne |
| Nom de la plateforme | ✅ **AnimeTCG** (Q8.1) — tranché en interne |
| Arborescence du dépôt | ✅ Validée (Q9.2) |
| Volume réel | ✅ Variable et **non garanti** (Q8.3) → lots courts |
| Adaptation à la machine | ✅ Calibrage automatique de la difficulté (§5.2.1) |
| Bibliothèques | ✅ Choix **libre**, sans autorisation à demander → **déclaration** écrite par l'équipe (§5.9.1, DEC-27) |
| Authentification des cartes | ✅ **Exigée par le sujet écrit** → rétablie (DEC-24), confirmée en interne (§1, R19) |
| Choix du sujet, de la blockchain, canal de communication | ⛔ Jamais : décisions d'équipe ou cœur de l'exercice |

---

*Document mis à jour le **22/09/2026** — **v5** : l'équipe n'ayant plus d'encadrant, les **6 questions bloquantes** et les **10 questions d'appoint** deviennent des **décisions d'équipe** (§1 et §2), avec justification par le **sujet écrit** ; **R19 est close** (authentification maintenue) ; **libuv écarté** au profit d'`epoll` à la main ; la **grille du barème** et la **forme du rendu de phase 1** sont fixées. À joindre au dépôt `sae-301` avec le cahier des charges et le questionnaire de décisions.*
