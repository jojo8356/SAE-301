# Questionnaire de décisions — AnimeTCG (SAÉ BUT2 S3)

> ✅ **Mise à jour du 22/09/2026** — sujet officiel 2026-2027 intégré (20 écarts, voir `docs/diff-sujet-2026.md`) ; les **6 questions bloquantes rédigées** le 22/09 à l'intention de l'encadrant sont **tranchées en équipe** le même jour (il n'y a plus d'encadrant : `questions-encadrant.md` v5) (R1, R2, R3, R5, R6, R19) ; **DEC-24 à DEC-29** ajoutées.
>
> ✅ **Mise à jour du 18/09/2026 (2 soumissions)** — Réponses de l'encadrant intégrées : **DEC-01** (domaine anime), **DEC-17** (4 étudiants), **DEC-20** (difficulté PoW), **DEC-14** (client automatique **écarté**, outil de test uniquement) sont tranchées, ainsi que les dates des phases, Podman, l'intégration continue et le **MVC**. Voir `questions-encadrant.md` et le bloc « Décisions validées par l'encadrant » en tête du cahier des charges. Les questions concernées ci-dessous sont marquées ✅.

**À remplir par l'équipe (4 membres) avant le 10 octobre 2026** (samedi). *(Échéance corrigée le 22/09/2026 : le sujet officiel 2026-2027 fixe la phase 1 au 10/10.)*
Chaque question reprend une **ouverture volontaire** du sujet. Une réponse par question, en cochant une option ou en écrivant dans « Réponse ».

> **Mode d'emploi** : la colonne « Proposition par défaut » contient une valeur déjà calculée et déjà intégrée au cahier des charges (`cahier-des-charges-sae-s3.md`). Si l'équipe **ne répond pas**, c'est cette valeur qui s'applique. Répondre ne sert donc qu'à **modifier** un point.
>
> **Impacts** : 🔴 élevé = reprise de code côté serveur **et** client · 🟠 moyen = reprise d'un module · 🟢 faible = paramètre.
>
> **Décideur** : 🧑‍🏫 décision qui aurait relevé de l'encadrant — **tranchée en équipe depuis le 22/09/2026** (l'équipe n'a plus d'encadrant) · 🧑‍💻 équipe · ⚙️ technique (l'équipe tranche, valide).

---

## Thème 1 — Cartes (nature, champs, identifiant, limites)

### Q1.1 — Nature exacte des objets recommandables 🧑‍🏫 🔴
**Question** : quelles œuvres peuvent être représentées par une carte ?

| Option | Contenu | Conséquence |
|---|---|---|
| **A** ✅ proposée | **Animes uniquement** : séries TV, films, OVA, ONA, spéciaux | Périmètre net, critères de proximité simples |
| B | Animes **et** mangas / light novels | Plus riche, mais critères de proximité à étendre (volume, chapitres) |
| C | Toute œuvre animée (y compris animation occidentale) | Critère « franchise/studio » moins discriminant |
| D | Autre : … | — |

**Proposition par défaut** : **A**.
✅ **Validé par l'encadrant le 18/09/2026** : « ce choix est libre et me convient parfaitement ».

✅ **Réponse de l'équipe (relevé du questionnaire rempli)** : **anime uniquement**. Une carte porte : **titre, personnages, lieux, objets, thèmes et liens personnels** (amour, rivalité, maître/apprenti, famille).
> ⚠️ **Impact** : ces composants élargissent le modèle de données au-delà des métadonnées d'œuvre (personnages, lieux, objets, relations à stocker par carte) — à répercuter en Q1.2, dans le schéma PostgreSQL et dans les classes serveur.

### Q1.2 — Champs obligatoires d'une carte 🧑‍💻 🟠
**Question** : parmi les 20 champs du §4.2.1 du cahier des charges, lesquels sont **obligatoires** à la création ?

Champs aujourd'hui obligatoires : `card_id`, `title_romaji`, `format`, `studio`, `year`, `episodes`, `genres` (≥ 1), `creator_id`, `owner_id`, `status`, `created_at` (11 champs, dont 6 saisis par l'utilisateur).

| Option | Contenu |
|---|---|
| **A** ✅ proposée | Les 6 champs saisis obligatoires : titre romanisé, format, studio, année, nombre d'épisodes, au moins 1 genre |
| B | Version minimale : titre + format + année seulement |
| C | Version stricte : A + `source` + `franchise` obligatoires |
| D | Autre : … |

**Réponse de l'équipe (relevé du questionnaire rempli)** : 11 champs obligatoires —
`card_id`, `title_carte`, `sources_id`, `format` (film / série), `year`, `genres` (shojo, seinen…), `themes` (l'histoire), `creator_id`, `owner_id`, `status`, `created_at`.

> ℹ️ **Lecture** : `title_carte` = titre de la carte (« sans casse » pour la proximité, Q2.1) ; `sources_id` = **œuvre d'origine** de l'adaptation ; `genres` = **style/audience** (shojo, seinen…) et `themes` = **sujet de l'histoire**. Les autres champs (studio, épisodes, saison, durée, synopsis, note, franchise…) restent **facultatifs**.

### Q1.3 — Format et unicité de l'identifiant 🧑‍💻 🟠
**Question** : quelle forme doit avoir le `card_id` généré par le serveur ?

| Option | Exemple | Remarque |
|---|---|---|
| **A** ✅ proposée | Numérique croissant : `1`, `2`, `3`… | Simple, lisible en démonstration |
| B | Préfixé et paddé : `C-000042` | Lisible, tri naturel |
| C | UUID v4 / hash court (8 premiers caractères du SHA-256) | Impersonnel, non devinable — mais illisible au tableau |
| D | Autre : … | — |

**Question liée** : faut-il **empêcher les doublons** (deux cartes identiques : même titre + même année + même studio) ?

| Option | Contenu |
|---|---|
| **A** proposée | Refus avec message explicite « une carte identique existe déjà (C-xxxx) » |
| B | Autoriser les doublons, la battle sert précisément à les fusionner |

**Réponse de l'équipe (relevé du questionnaire rempli)** : **option A** — `card_id` **numérique croissant** (`1`, `2`, `3`…), généré par le serveur, jamais saisi.

**Question liée — doublons** : ⚠️ **oui, on garde les doublons**, et c'est délibéré. Deux raisons données par l'équipe : ① les **battles** ont besoin de plusieurs cartes de la même œuvre (gestion des résultats et des combattants) ; ② la **liberté de création de collections** pour les nouveaux joueurs.
> **Conséquence** : **aucune contrainte d'unicité** sur (titre, année, studio) — contrairement à la proposition « A » initiale. La seule unicité qui subsiste porte sur le pseudo (`uq_users_pseudo`) et sur les objets métier « actifs » (une recommandation, une légitimation, une authentification, une revendication en attente).

### Q1.4 — Nombre maximum de cartes actives par utilisateur 🧑‍💻 🟢
**Question** : faut-il limiter, et à combien ?

| Option | Valeur | Justification |
|---|---|---|
| A | Illimité | Plus simple, mais risque de spam et de battles illisibles |
| B | 5 | Très strict, beaucoup de refus en démonstration |
| **C** ✅ proposée | **10** | Compromis lisibilité / liberté |
| D | Dynamique (ex. 5 + 1 par battle gagnée) | Plus original à présenter, plus complexe à tester |
| E | Autre : … | — |

**Réponse de l'équipe (relevé du questionnaire rempli)** — *« dans un premier temps »* :
- Une carte est considérée **active** si elle est **mise au public**, **en deck** et **stockée uniquement côté serveur**.
- Nombre maximum de cartes actives : **100** *(et non 10)*.
- Désactivation **par l'utilisateur** et/ou **après une défaite en combat**, la carte étant alors **archivée**.
- La **carte gagnante obtient les votes de la carte perdante** (addition inscrite en blockchain).

> ⚠️ **Deux impacts majeurs** : ① la limite passe de 10 à **100 cartes actives** (code `1001 CARD_LIMIT_REACHED`, tests, IHM) ; ② un **second sort** apparaît à côté de « désactivée » : la carte **archivée** après défaite, qui **n'est plus affichée** (voir Q2.6 et l'état `ARCHIVEE` à ajouter au cycle de vie).

**Question liée** : que se passe-t-il quand la limite est atteinte ?
- **A** ✅ proposée : la création est **refusée** avec message explicite (aucune donnée supprimée).
- B : la création est refusée et l'interface propose de rendre une carte inactive.

**Réponse de l'équipe** : l'interface **propose de désactiver une des 100 cartes déjà actives** *(c'est l'option B, retenue)*.

> ℹ️ La création est donc **refusée** (`1001`) **et** le client propose immédiatement la liste des cartes actives pour en désactiver une — au lieu d'un simple refus sec.

---

## Thème 2 — Proximité & battle

### Q2.1 — Critères de proximité entre deux cartes 🧑‍🏫 🔴
**Question** : deux cartes sont « suffisamment proches » si… (retenir les critères à implémenter)

| Critère | Description | Retenu ? |
|---|---|---|
| P1 ✅ | Même `franchise` déclarée (ex. *Fate/stay night* vs *Fate/Zero*) | ☐ |
| P2 ✅ | Titres normalisés quasi identiques (distance d'édition ≤ 15 %) — détecte remake / film dérivé | ☐ |
| P3 ✅ | Même studio **et** ≤ 10 ans d'écart | ☐ |
| P4 ✅ | ≥ 2 genres/thèmes communs **et** ≤ 5 ans d'écart | ☐ |
| P5 ✅ | Même `source` d'adaptation + même studio ou même franchise | ☐ |
| P6 | Même `format` **obligatoire** (un film ne se bat qu'avec un film) | ☐ |
| P7 | Vérification **manuelle** : le demandeur saisit une justification textuelle | ☐ |

**Règle de combinaison** :
- **A** ✅ proposée : au moins **un** critère satisfait (union) → permissif
- B : tous les critères de type obligatoires + au moins un optionnel → plus strict
- C : score pondéré avec seuil (ex. score ≥ 3 points) → plus original, plus complexe

**Proposition par défaut** : P1 à P5 retenus, **union**.
**Réponse de l'équipe (relevé du questionnaire rempli)** : **deux critères seulement**, et ils remplacent P1 à P5 :
1. **titre de la carte, sans tenir compte de la casse** ;
2. **format** (film avec film, série avec série…).

**Règle** : **l'un des deux suffit** (union), comme dans la proposition par défaut.
> ℹ️ Les critères P1 à P5 (franchise, studio, distance d'édition, genres communs, source) sont **abandonnés** : ils demandaient des données que la carte n'a pas rendues obligatoires (Q1.2). Le critère `format` devient de surcroît **structurant** pour les battles (Q2.2).

### Q2.2 — Conditions d'acceptation d'une battle 🧑‍💻 🟠

| Condition | Valeur proposée | Retenue ? |
|---|---|---|
| Les deux cartes sont `ACTIVE` | ✅ proposé | ☐ |
| Proximité vérifiée (Q2.1) | ✅ proposé | ☐ |
| Le demandeur est **propriétaire courant** d'au moins une des deux cartes | ✅ proposé | ☐ |
| Aucune des deux cartes n'est déjà `EN_BATTLE` | ✅ proposé | ☐ |
| Anti-abus : ≤ **3 battles par utilisateur et par 10 min** | ✅ proposé | ☐ |
| **Interdiction** de se battre contre sa propre carte | À trancher | ☐ |

**Question ouverte** : une battle peut-elle concerner **plus de deux cartes** ?
- **A** ✅ proposée : non, **exactement 2** (le sujet dit « deux cartes ou plus » mais parle ensuite d'une gagnante et d'une perdante).
- B : oui, jusqu'à 3 ou 4 cartes, avec une règle de classement (1 gagnante, n perdantes fusionnées).
- C : non pour la phase 2, oui en bonus si le temps le permet.

**Réponse de l'équipe (relevé du questionnaire rempli)** : trois conditions, plus une condition de **niveau** :
- les deux cartes sont **actives** ;
- **même format** ;
- **classement par valeur de carte** : *« fer peut affronter bronze, bronze peut affronter fer et argent… »* → seuls des **rangs voisins** peuvent s'affronter.

Conditions de la proposition confirmées par ailleurs : proximité (Q2.1), demandeur **propriétaire courant** d'une des deux cartes, aucune carte déjà `EN_BATTLE`, anti-abus 3 battles / 10 min, et **exactement 2 cartes** (option A).
> ⚠️ **Impact** : les rangs **fer / bronze / argent / or / platine** de Q3.1 deviennent une **règle de matchmaking** — la battle n'est pas ouverte à toute carte proche, mais à une carte proche **de rang voisin**.

### Q2.3 — Durée d'un vote 🧑‍💻 🟢
**Question** : combien de temps dure le vote d'une battle ?

| Option | Durée | Remarque |
|---|---|---|
| A | 30 s | Nerveux, mais peu de temps pour expliquer au jury |
| B | 60 s | Correct |
| **C** ✅ proposée | **90 s** | Compatible avec une démo de 10–15 min et un vrai temps de réflexion |
| D | 120 s ou plus | Allonge la démonstration |
| E | Déclenchement manuel de la clôture par le lanceur | Plus souple, moins prévisible |

**Réponse de l'équipe (relevé du questionnaire rempli)** : **5 minutes** (300 s).

> ⚠️ **Écart avec la proposition (90 s)** : la battle devient **beaucoup plus longue**. Conséquences à répercuter : le scénario de démonstration (§9.2) et le budget de temps de la soutenance ; le **quorum** et la **clôture** (il faut pouvoir clore **avant** 5 min si le quorum est atteint — sinon la démo dure 5 minutes sur une seule battle).

### Q2.4 — Droit de vote et poids des votes 🧑‍🏫 🟠

| Sous-question | Options | Proposition |
|---|---|---|
| Qui peut voter ? | (A) tout utilisateur connecté au moment de l'ouverture ✅ / (B) seulement ceux connectés à l'ouverture **et** encore connectés à la clôture / (C) seulement les utilisateurs ayant déjà ≥ 1 carte active | **A** |
| Les propriétaires des cartes votent-ils ? | (A) non, exclus ✅ / (B) oui, mais poids 0,5 / (C) oui, poids 1 | **A** |
| Poids d'un vote de bot | ✅ **Sans objet** : le client automatique est réservé aux tests et ne participe à aucun vote (DEC-14) | — |
| Poids d'un vote du créateur d'une carte | (A) 0,5 ✅ / (B) 1 / (C) exclu | **A** |
| Le vote peut-il être modifié ? | (A) oui jusqu'à la clôture, un seul vote actif ✅ / (B) non, définitif | **A** |

**Réponse de l'équipe (relevé du questionnaire rempli)** :
| Sous-question | Réponse |
|---|---|
| Qui peut voter ? | **Tout utilisateur connecté pendant la durée du vote** (pas seulement à l'ouverture) |
| Les propriétaires des cartes votent-ils ? | **Non — créateur et propriétaire ne votent pas** |
| Poids d'un vote de bot | **Les bots (test) votent comme un utilisateur** ; leurs votes ne comptent pas dans le **quorum en production** (Q5.1) |
| Poids du vote du créateur | **Exclu** (aucune pondération 0,5) |
| Le vote peut-il être modifié ? | **NON — pas de retour en arrière** : le vote est **définitif** dès qu'il est déposé |

> ⚠️ **Impact protocole** : un **second vote** devient une **erreur** (et non un remplacement). Le protocole doit porter un code dédié — `1020 VOTE_ALREADY_CAST` — et la table `battle_votes` cesse de documenter « le modifier remplace la ligne » : un vote par votant, **définitif**.

### Q2.5 — Égalité et absence de réponse 🧑‍💻 🟠

**Question A — égalité parfaite entre les deux cartes** :
| Option | Règle |
|---|---|
| **A** ✅ proposée | Départage en cascade : (1) valeur active la plus élevée, (2) carte la plus ancienne |
| B | Battle déclarée nulle, aucune fusion |
| C | Re-vote immédiat d'une durée réduite (30 s) |
| D | Le propriétaire demandeur gagne (avantage au demandeur) |

**Question B — quorum non atteint (moins de 3 votants)** :
| Option | Règle |
|---|---|
| **A** ✅ proposée | Battle **nulle** : cartes rendues `ACTIVE`, événement inscrit en blockchain |
| B | La carte du demandeur gagne par forfait |
| C | La battle reste ouverte jusqu'à atteindre le quorum |
| D | La carte ayant la valeur active la plus élevée gagne automatiquement |

**Question C — un votant se déconnecte pendant le vote** : son vote est-il conservé ?
| Option | Règle |
|---|---|
| **A** ✅ proposée | Oui : le vote est valide au moment où il a été exprimé ; la règle d'absence ne s'applique pas |
| B | Non : le vote est annulé à la déconnexion |

**Réponse de l'équipe (relevé du questionnaire rempli)** : **A = la carte la plus ancienne gagne** / **B = battle annulée** / **C = le vote est conservé**.

| Cas | Règle retenue |
|---|---|
| **A** — égalité parfaite | **La carte la plus ancienne** (`created_at`) l'emporte — le départage par la valeur disparaît |
| **B** — quorum non atteint | **Battle annulée** : aucune fusion, cartes **rendues** (elles redeviennent `ACTIVE`) |
| **C** — votant déconnecté | **Vote conservé** (la déconnexion n'annule pas un vote déposé) |

> ℹ️ **B rejoint le cas d'erreur n° 15 du §12.1** (« cartes rendues faute de votes ») : c'est le **troisième** résultat possible d'une battle, ni victoire ni nulle.

### Q2.6 — Sort exact de la carte perdante 🧑‍🏫 🟠
**Question** : quel devenir pour la perdante ?

| Option | Effet | Remarque |
|---|---|---|
| **A** ✅ proposée | `FUSIONNEE` : ses recommandations actives partent à la gagnante, elle devient non recommandable, historique conservé ; réactivation possible **une seule fois** sur demande d'un légitime | Vocabulaire explicite, facile à démontrer |
| B | `ARCHIVEE` : même effet, mais **définitive** (aucun retour) | Plus simple à coder |
| C | `INACTIVE` : même effet, mais réactivable librement | Plus souple |
| D | Suppression logique avec transfert de la propriété à la gagnante | Plus complexe (deux cartes, un propriétaire) |

**Sous-question** : que deviennent les **légitimations et revendications** de la perdante ?
- **A** ✅ proposée : héritées par la gagnante **si** même franchise / même œuvre, sinon conservées sur la perdante
- B : toujours héritées par la gagnante
- C : jamais héritées, elles restent sur la perdante

**Réponse de l'équipe (relevé du questionnaire rempli)** :
- **transfert des points de recommandation** vers la carte gagnante ;
- la perdante est **archivée** (elle **ne peut plus être affichée**) ;
- la victoire est **enregistrée dans un compteur** pour la **rareté** (Q3.1).

**Sous-question — droits associés (légitimations, revendications, authentifications)** : *« les caractéristiques de la carte perdante restent inchangées car perte de toute valeur après défaite »* → **les droits restent sur la perdante, inchangés**.

> ✅ **Contradiction tranchée le 22/09/2026** : **Q2.6 prime** — une victoire en battle **ne retire pas** la légitimation ; seul un dépassement de likes change de porteur. *(Ancienne note : Q4.1 répond que la légitimation peut être retirée « par dépassement de like ou victoire de battle »*, alors que Q2.6 dit les droits **inchangés**. Les deux réponses viennent de la même séance : à arbitrer (impact : héritage ou non du titre de porteur légitime après une battle).
> ⚠️ **Autre écart** : la proposition initiale parlait de `FUSIONNEE` avec réactivation possible ; la réponse de l'équipe retient l'**archivage définitif** (option B), la carte n'étant plus affichée — **mais** Q1.4 prévoit une désactivation « par l'utilisateur » : la réactivation reste donc possible pour une carte **désactivée**, pas pour une carte **archivée par défaite**.

---

## Thème 3 — Valeurs

### Q3.1 — Valeur d'une carte 🧑‍🏫 🟠
**Question** : quelle formule ?

**Formule proposée** :
```
V(carte) = Σ poids(recommandations actives)
             poids = 1.0 autre utilisateur, 0.5 créateur/propriétaire
         + 3 × victoires en battle
```

| Option | Formule | Remarque |
|---|---|---|
| A | Nombre brut de recommandations actives | Le plus simple, mais manipulable (auto-like répété) |
| **B** ✅ proposée | Formule ci-dessus | Pondère les comportements non « communautaires », valorise la battle |
| C | B + bonus d'ancienneté de la carte | Plus original, moins lisible |
| D | B + coefficient dépendant du nombre de votants en battle | Valorise les battles disputées |
| E | Autre : … | — |

**Sous-questions** :
- Faut-il afficher la **valeur détaillée** (part des auto-recommandations distinguée du reste) ? **A** ✅ proposée : oui, deux compteurs distincts — **B** : non, un seul chiffre.
- Une recommandation **répudiée puis redonnée** recompte-t-elle ? **A** ✅ proposée : oui (nouvelle transaction) — **B** : non, une seule fois à vie.

**Réponse de l'équipe (relevé du questionnaire rempli)** — **formule remplacée** : la valeur d'une carte est le produit des points de recommandation par un **multiplicateur de rareté** lié aux victoires en battle.

| Rang | Multiplicateur | Victoires |
|---|---|---|
| **fer** | **0×** | 0 à 10 |
| **bronze** | **1,2×** | 11 à 30 |
| **argent** | **1,5×** | 31 à 60 |
| **or** | **1,7×** | 61 à 100 |
| **platine** | **2×** | **plus de 100** |

- **Valeur détaillée affichée** : **oui**, deux compteurs distincts (points de recommandation d'un côté, rang et victoires de l'autre).
- Recommandation **répudiée** : *« il ne perd jamais la carte, donc pas de récupération »* → une recommandation répudiée **cesse de compter** et **n'est pas récupérée**.

> ⚠️ **Impact** : la formule `Σ recos pondérées + 3 × victoires` **disparaît** au profit de `points de recommandation × multiplicateur(rang)`. La **pondération 1,0 / 0,5** (créateur/propriétaire) tombe, puisque les propriétaires **ne votent pas** (Q2.4) — cohérent, mais à répercuter partout (§4.6 du CDC, spec, tests, IHM).

### Q3.2 — Valeur d'un utilisateur 🧑‍💻 🟢
**Question** : quelle formule ?

| Option | Formule | Remarque |
|---|---|---|
| A | Nombre de recommandations reçues sur ses cartes | Simple mais redondant avec la valeur des cartes |
| **B** ✅ proposée | `(cartes ACTIVE possédées) + Σ V(cartes possédées)` | Reflète l'activité et la légitimité |
| C | B + nombre de battles gagnées **comme votant** (participation) | Valorise la participation |
| D | Autre : … | — |

**Réponse de l'équipe (relevé du questionnaire rempli)** : **total des votes des cartes possédées** — c'est-à-dire la **somme des points de recommandation** des cartes que l'utilisateur possède (et non le nombre de cartes + la somme des valeurs de la proposition).

> ℹ️ Conséquence : la valeur d'un utilisateur ne dépend **que** de ce que la communauté a apporté à ses cartes — pas de son activité de création ni de sa participation aux battles.

---

## Thème 4 — Droits (légitimation, revendication, transaction)

### Q4.1 — Processus de légitimation 🧑‍🏫 🟠
**Question** : comment un utilisateur devient-il « porteur légitime » ? (le sujet impose un processus **simulé ou interne**, sans vérification d'identité réelle)

| Option | Mécanisme | Remarque |
|---|---|---|
| **A** ✅ proposée | **Scrutin interne** : demande publiée 60 s, justificatif obligatoire (200 car. max), acceptée si ≥ 2 approbations hors demandeur ou si aucun refus à l'expiration | Démontrable en direct, mécanique communautaire crédible |
| B | **Code de légitimation** simulé : le serveur génère un code, l'utilisateur le saisit | Rapide à coder, mais peu intéressant à démontrer |
| C | **Auto-déclaration** immédiate, validée par un simple clic | Trop trivial, aucune traçabilité intéressante |
| D | **Question secrète** sur l'œuvre (posée par qui ?) | Créatif mais coûteux à concevoir |
| E | Autre : … | — |

**Sous-questions** :
- Nombre de porteurs légitimes simultanés par carte : **A** ✅ proposée : 1 seul (une nouvelle légitimation révoque la précédente) — **B** : plusieurs possibles, collectif de fans — **C** : 1, non révocable.
- La légitimation peut-elle être **retirée** ? **A** ✅ proposée : oui, par un vote de révocation — **B** : non, définitive.
- Le justificatif est-il visible par tous ? **A** ✅ proposée : oui, dans la fiche de la carte — **B** : conservé mais non affiché.

**Réponse de l'équipe (relevé du questionnaire rempli)** : la légitimation se joue **au nombre de likes** — *« le nombre de likes détermine le plus légitime à recevoir le titre de porteur légitime »*.

| Sous-question | Réponse |
|---|---|
| Mécanisme | **Les « likes » (approbations) désignent le porteur légitime** : le plus soutenu l'emporte, à tout moment |
| Porteurs simultanés | **1 seul** par carte |
| Le titre peut-il être retiré ? | **Oui** — par **dépassement de likes** ou par **victoire de battle** ⚠️ (voir la contradiction relevée en Q2.6) |
| Justificatif visible ? | **Oui, par tous** — affiché avec la **rareté**, le **nombre de likes** et le **propriétaire** |

> ⚠️ **Impact** : le « scrutin de 60 s avec ≥ 2 approbations » **disparaît**. Le mécanisme devient **continu et compétitif** (le titre change de main dès qu'un candidat dépasse le titulaire), ce qui simplifie le protocole — `legitimation.respond` (approve/reject) devient un **like** — et supprime la notion de **délai** (`1013 LEGITIMATION_CLOSED`).

### Q4.2 — Conditions de revendication 🧑‍🏫 🔴
**Question** : dans quelles conditions un utilisateur légitimé peut-il revendiquer une carte (devenir propriétaire courant) ?

| Condition | Proposition | Retenue ? |
|---|---|---|
| Le demandeur est **légitimé** sur la carte | ✅ | ☐ |
| La carte est `ACTIVE` | ✅ | ☐ |
| Le demandeur n'en est pas déjà le propriétaire | ✅ | ☐ |
| Le **propriétaire courant donne son accord** dans un délai de **120 s** | ✅ | ☐ |
| Interdiction de revendiquer une carte créée par soi-même | À trancher | ☐ |

**Question ouverte** — faut-il un **coût** ou une **condition supplémentaire** (justification textuelle, valeur minimale de la carte, ancienneté…) ?
- **A** ✅ proposée : justification textuelle supplémentaire obligatoire
- B : aucun coût, la légitimation suffit
- C : la carte doit avoir une valeur active ≥ 5
- D : autre : …

**Réponse de l'équipe (relevé du questionnaire rempli)** : **posséder la carte depuis 1 mois** — et **aucun coût, aucune condition supplémentaire** (réponse explicite : *« non »*).

> ✅ **Tranché le 22/09/2026** : « posséder la carte depuis 1 mois » peut se lire de deux façons — ① le **demandeur** doit posséder une carte depuis un mois, ou ② la **demande de revendication** doit avoir un mois d'ancienneté. La Q4.3 tranche pour ② (« si ta une demande de propriété de plus d'un mois tu deviens propriétaire ») : c'est la lecture appliquée (délai **après** la demande), cohérente avec DEC-11.

### Q4.3 — Modalités de la transaction de revendication 🧑‍🏫 🟠
**Question** : que contient la transaction, et que se passe-t-il si l'autre partie ne répond pas ?

| Élément de la transaction | Proposition |
|---|---|
| Offre explicite (justification + engagement symbolique) | ✅ |
| Conservation des recommandations acquises par la carte | ✅ |
| Conservation du statut des cartes issues de battles | ✅ |
| Horodatage + identifiant de transaction inscrit en blockchain | ✅ |
| Transaction **symbolique** en points de valeur (comme un « prix ») | ❌ **Non retenue** (arbitrage du 22/09/2026) : **aucun prix** — l'engagement porte sur la conservation des recommandations et du statut |

**Règle d'absence (propriétaire déconnecté ou silencieux)** :
| Option | Règle |
|---|---|
| A | Après 120 s : acceptation par défaut **si** le propriétaire ne s'est pas connecté depuis 7 jours, sinon refus par défaut |
| B | Toujours refus par défaut (attente indéfinie) |
| C | Toujours acceptation par défaut au bout du délai de 120 s |
| D | La transaction reste ouverte jusqu'à connexion du propriétaire |
| E | Arbitrage par scrutin des utilisateurs connectés |
| **F** ✅ **retenue** | **Le silence vaut accord au bout d'un mois** : une demande **non refusée pendant 30 jours** rend le demandeur légitimé **propriétaire courant** de la carte |

**Question liée** : faut-il une notion de **litige** (contestation du résultat) ?
- A : oui, recours du revendiquant légitimé, arbitré par scrutin interne
- **B** ✅ **retenue** : non — la revendication est un **accord entre les deux parties**. Un **refus explicite = statu quo**, la carte reste au propriétaire, **aucun recours**, aucun vote, aucun arbitrage. Seule trace : le refus est **journalisé** (non inscrit comme action validée)

**Réponse** ✅ **tranchée le 18/09/2026 (décision d'équipe)** :

| Point | Décision |
|---|---|
| Contenu de la transaction | Offre explicite : **justification + engagement symbolique** (conservation des recommandations acquises et du statut des cartes issues de battles), **horodatage** et identifiant de transaction inscrits en blockchain |
| **Silence du propriétaire** | **30 jours (1 mois) sans refus → le demandeur devient propriétaire de la carte** (règle de droit commun : un propriétaire absent ne bloque pas une carte indéfiniment) |
| **Refus** | **Statu quo définitif** : la carte reste au propriétaire, la demande est close, **aucun recours** |
| Litige | **Aucune notion de litige** — ni juge, ni vote, ni arbitrage : c'est un accord bilatéral |
| Fenêtre de 120 s | Devient la **fenêtre de décision affichée en démonstration** : sans réponse, la demande **reste en attente** (elle ne bascule plus en refus) |
| Démonstrabilité | 120 s : joué en direct · **30 jours : non démontrable en direct** → documenté + **test automatisé à horloge simulée** |

**Reporté au cahier des charges** : **§4.7.2** (tableau de la transaction), **DEC-10** et **DEC-11** (§12), **annexe D.3** (« silence valant accord : 30 jours » à la place de « inactivité 7 jours »), journal **v0.15**.

**Réponse en texte simple (à recopier telle quelle)**

```
Q4.3 - Modalites de la transaction de revendication

Reponse : la revendication est un accord entre les deux parties, et le silence
du proprietaire pendant un mois vaut accord.

Contenu de la transaction : une offre explicite adressee au proprietaire
courant, avec une justification, un engagement symbolique (la conservation des
recommandations acquises par la carte et du statut des cartes issues de
battles), un horodatage, et un identifiant de transaction inscrit dans la
blockchain.

Condition prealable : le demandeur doit etre legitime sur la carte, la carte
doit etre ACTIVE, et le demandeur ne doit pas en etre deja le proprietaire.

Deux issues possibles :
- Le proprietaire accepte : le transfert de propriete est inscrit dans la
  blockchain, l'ancien proprietaire reste dans l'historique.
- Le proprietaire refuse : statu quo, la carte reste au proprietaire, la
  demande est close. Le refus est journalise mais n'est pas inscrit comme
  action validee.

S'il ne repond pas : le silence vaut accord au bout d'un mois. Une demande de
revendication qui n'a pas ete refusee pendant 30 jours rend automatiquement le
demandeur legitime proprietaire courant de la carte. C'est une regle de droit
commun sur le silence prolonge : elle evite qu'une carte reste bloquee parce
que son proprietaire ne se connecte plus. En demonstration, on affiche une
fenetre de decision de 120 secondes, mais sans reponse la demande ne bascule
pas en refus : elle reste en attente et suit la regle du mois.

Litige : aucun. Il n'y a ni juge, ni vote, ni arbitrage. C'est un accord
bilaterale entre les deux utilisateurs : le proprietaire a le dernier mot et un
refus ne change rien.

Demonstration : la fenetre de 120 secondes et le refus se jouent en direct. La
regle des 30 jours n'est pas demontrable en direct, elle est documentee et
couverte par un test automatise avec horloge simulee.
```

### Q4.4 — Conditions et effets de l'authentification 🔧 ✅ **RÉTABLIE et tranchée en interne (DEC-24)**

> ⚠️ **Revirement documenté (22/09/2026).** Cette question avait été **retirée** le 18/09/2026 sur consigne orale de l'encadrant (« on s'en fiche »). Le **sujet officiel 2026-2027** la remet au programme : elle figure au **§3** (services du serveur), au **§5.5** (concept), au **§6** sous la forme du cas d'usage **CU-20 « Authentifier une carte »**, et au **§12.1** parmi les fonctionnalités **à démonter**. Le sujet écrit prime : la fonctionnalité est **réintégrée** et la question **rouverte puis tranchée**.

**Réponse (équipe, 22/09/2026) — DEC-24** : l'authentification est un **label de confiance émis par un porteur légitime**, et non un droit de propriété.

| Point | Décision | Justification |
|---|---|---|
| **Qui** | Un **porteur légitime actif** (légitimation acceptée). Les autres reçoivent `1014 CLAIM_NOT_LEGITIMATE` | La condition préalable est identique à celle de la revendication : un seul concept à défendre |
| **Ce qui est certifié** | Les **métadonnées de la carte** (titre, studio, année, format, épisodes), pas la propriété | Le sujet parle d'authentifier « une carte », pas un droit |
| **Vérification d'identité** | **Aucune** — processus interne et simulé, comme la légitimation | Cadre du sujet §5.5 |
| **Unicité** | **Une seule authentification active** par carte (index unique partiel) ; une nouvelle remplace l'ancienne, qui reste dans l'historique | Même logique que « un seul porteur légitime actif » |
| **Révocation** | Possible par le porteur légitime (nouvelle action, nouveau bloc) ; **automatique** si la légitimation est retirée | Une certification sans porteur légitime n'aurait plus de fondement |
| **Effet sur la valeur** | **Aucun** (ni +1, ni priorité, ni bonus) | Sinon un porteur légitime pourrait fabriquer de la valeur sans aucun soutien — exactement ce que le sujet interdit au §6.24 pour le bot |
| **Nature technique** | **Attribut**, pas un **état** de carte | Évite de dédoubler les quatre états (`ACTIVE`, `EN_BATTLE`, `INACTIVE`, `ARCHIVEE`) |
| **Après une battle** ✅ **corrigé le 22/09/2026 (Q2.6)** | **Rien ne change** : *« les caractéristiques de la carte perdante restent inchangées »* — le marqueur, la légitimation et les **revendications en attente restent sur la perdante**. La règle précédente (*héritage par la gagnante* + clôture `SANS_OBJET`) est **retirée** ; l'état `SANS_OBJET` disparaît du schéma (v1.4) | Répond à la question laissée ouverte par le sujet ⚠️ à concilier avec Q4.1 (retrait de la légitimation par victoire de battle) au CU-17 |
| **Cas d'erreur** | `1018 CARD_ALREADY_AUTHENTICATED`, `1019 AUTHENTICATION_NOT_FOUND`, `1014` (non légitimé) | Alignés sur la nomenclature du protocole |

**Confirmé en équipe le 22/09/2026** : le **sujet écrit fait autorité** → l'authentification est **maintenue** (l'écart avec la consigne orale du 18/09/2026 est **clos** — plus d'encadrant à poser pour l'authentification). Le travail est **engagé** (lot L7) : la fonctionnalité n'est pas bloquante pour la phase 1, mais le retrait éventuel toucherait §4.7.3, deux actions de protocole, une table, une séquence UML et une étape de démonstration — d'où l'intérêt d'une réponse **avant le 10 octobre**.

**Conséquences appliquées** : §3.2 (périmètre rétabli), **EF-20** et **CU-20** rétablis, **DEC-24** créée, **§4.7.3** (règles complètes), **§5.9.1** (protocole : `card.authenticate` et `card.revoke_authentication`), **`sql/schema.sql` v1.2** (table `authentications`), diagramme `13-sequence-authentification.puml`, événement `card.authenticated`, codes `1018`/`1019`, étape dédiée dans le scénario de démonstration.

---

## Thème 5 — Extensions (client de test, partage ciblé)

### Q5.1 — Client automatique (bot) ✅ **tranchée par l'encadrant**

✅ **Décision (18/09/2026)** : le bot est **réservé aux tests**. Il n'est **pas** retenu comme fonctionnalité du projet : aucune participation à la démonstration, aucun vote, aucune contribution à la valeur d'une carte, aucun affichage parmi les utilisateurs connectés.

**Ce qui reste autorisé** : un **client de test automatisé**, développé et exécuté **hors démonstration**, pour éprouver la tenue du serveur multitâche (montée à 16 connexions), les cas d'erreur et les tests d'intégration.

**Conséquences appliquées au cahier des charges** : §3.2 (hors périmètre), §3.3 (outil de test), §4.1 (n'est plus un acteur métier), §4.5.3 (plus de pondération 0,5), §4.6 (formule de valeur simplifiée), §9.2 (démonstration sans bot), §11.1 (CU-6 et CU-24 recadrés), §12 (DEC-14).

✅ **Réponse de l'équipe (relevé du questionnaire rempli)** — elle **précise** la décision de l'encadrant :
- **deux bots** : un **bot de test** *et* un **bot de maintenance** ;
- **1 bot maximum** en démonstration ;
- ses votes **ne comptent pas dans le quorum** (en production ; ils comptent en test) ;
- il **apparaît dans la liste des utilisateurs connectés** — ⚠️ **avec un marqueur distinct**, comme l'exige le sujet (CU-24 : un client automatique doit être distinguable des humains).

> ⚠️ **Ce que cela change dans DEC-14** : le bot n'est plus « invisible et hors démonstration ». Il reste **hors valeur** et **hors quorum en production**, mais il est **affiché et identifiable**. Le « bot de maintenance » est, lui, un usage à préciser (supervision ? tâches d'entretien ?) — question à poser en réunion.

### Q5.2 — Partage ciblé 🧑‍🏫 🟠
**Question** : ajoute-t-on le mécanisme de partage ciblé (hors périmètre minimal) ?

| Option | Contenu |
|---|---|
| **A** ✅ proposée | **Non** en périmètre ferme ; décision reportée à la semaine du 30 novembre si le fil conducteur de démonstration est complet |
| B | Oui, dès la phase 2 |
| C | Non, définitivement |

**Si oui**, préciser :
- destinataires possibles : **liste d'amis**
- acceptation / refus : **acceptation → le demandeur se déplace** ; **refus → personne ne bouge**
- notification poussée sur la socket : **oui**
- inscription en blockchain : **non**
- limites anti-spam : **oui, personnalisables par l'utilisateur** — **3 messages**, puis **1 par minute**

**Réponse de l'équipe (relevé du questionnaire rempli)** : **OUI — le partage ciblé est retenu** (c'était une option du sujet, elle est levée).

> ⚠️ **Trois impacts lourds** : ① il faut une **liste d'amis** (nouvelle notion, nouveaux écrans, nouvelles actions) ; ② le protocole **active** l'événement jusqu'ici réservé `share.received` et ajoute l'action d'envoi ; ③ **l'opération n'est pas inscrite en blockchain** — or la chaîne est notre **source de vérité** : il faut décider ce qu'il advient des cartes transférées hors chaîne (état hors chaîne, ou inscription différée) **avant** d'implémenter.

---

## Thème 6 — Technique & produit

### Q6.1 — Protocole applicatif (100 % sockets) ✅ **spécifié** — `docs/protocole-applicatif.md` v1.0
**Contexte** : décision déjà prise — **uniquement des sockets**, un seul canal de communication.

**Question A — format des messages** :
| Option | Format | Avantage |
|---|---|---|
| **A** ✅ proposée | Frame `[longueur sur 4 octets big-endian][charge utile JSON UTF-8]` | Lisible au débogage, robuste sur TCP, facile à implémenter en C et en Java |
| B | Ligne par ligne, JSON terminé par `\n` | Très simple, mais fragile si le JSON contient un saut de ligne |
| C | Format binaire TLV (type, longueur, valeur) | Compact et performant, mais plus long à développer et à déboguer |

**Question B — taille maximale d'un message** : **A** ✅ proposée : **64 Kio** — B : 16 Kio — C : 1 Mio.

**Question C — corrélation** : identifiant de demande (`request_id`) repris dans la réponse ? **Oui** ✅.

**Question D — messages du serveur non sollicités (événements)** : diffusion des connexions, ouvertures/clôtures de battle, résultat, progression du minage ?
- **A** ✅ proposée : oui, avec un **abonnement** explicite du client à certains flux
- B : oui, diffusion systématique à tous les connectés
- C : non, uniquement des réponses à des demandes (le client interroge le serveur périodiquement)

**Question E — versionnage du protocole** : champ `protocol_version` dans chaque message ? **Oui** ✅ (permet de refuser un client incompatible).

**Question F — chiffrement** : **A** ✅ proposée : aucun (usage local, hors périmètre) — B : un simple masquage — C : TLS (très coûteux à mettre en œuvre ici).

**Réponse** ✅ **spécifiée le 19/09/2026** — le protocole est **figé en version 1.0** dans **`docs/protocole-applicatif.md`**. Les six questions ci-dessus sont tranchées : **A** (trame `[4 octets big-endian][JSON UTF-8]`), **A** (64 Kio), **oui** (corrélation par identifiant de demande), **A** (événements **diffusés** — pas d'abonnement explicite en phase 1 : c'est le serveur qui décide à qui il envoie, dont le seul cas ciblé `claim.received` au propriétaire), **oui** (champ `v`, avec **négociation à la poignée de main** et refus par `-32005` si la version majeure n'est pas connue), **A** (aucun chiffrement — usage local).

**Ce qui a été ajouté par rapport à ces options** — parce qu'aucun des six points ne suffisait à rendre le protocole implémentable :

| Ajout | Pourquoi | Source retenue |
|---|---|---|
| **Grammaire ABNF** de l'enveloppe | Une trame ne fait pas un message : il fallait la forme exacte des requêtes, réponses et événements | JSON-RPC 2.0 |
| **30 codes d'erreur** répartis en 3 plages (`-32768…-32000` protocole, `-32699…-32600` serveur, **`1000+` métier**) | ENF-22 et REC-13 exigent « une erreur explicite » : un code générique ne dit pas au client quoi faire | JSON-RPC 2.0, RFC 9457 |
| **Politique de versionnage** (mineur = compatible, majeur = rupture) et **lecture tolérante** | Sans règle écrite, la première évolution casse le client | Postel (RFC 1122), *Tolerant Reader*, lignes directrices Zalando |
| **11 cas de robustesse** (§9 du protocole) | Malformé, tronqué, surdimensionné, type inconnu : chacun a une réaction distincte — la 12ᵉ ligne du tableau est la **garantie** (« aucun de ces cas ne termine le processus serveur ») | ENF-22, REC-13 |
| **Idempotence / anti-rejeu** (10 000 derniers identifiants) | Un renvoi après timeout réseau ne doit pas créer un **second bloc** définitif | Pratique des RPC |
| **Contre-pression et file d'attente** (§13) | Intègre ENF-54 et évite qu'un client lent bloque le serveur | ENF-18 |

**Reporté au cahier des charges** : §5.3 (ENF-19 et ENF-22 marquées satisfaites), §3.1, journal **v0.17**.

### Q6.2 — Organisation de l'interface graphique (JavaFX) 🧑‍💻 🟠
**Question** : comment structurer l'IHM ?

| Option | Structure |
|---|---|
| **A** ✅ proposée | Fenêtre principale à **onglets** : `Mes cartes` / `Explorer` / `Mes recommandations` / `Battles` / `Connectés` / `Profil`, avec une **barre d'état** permanente (connexion, bloc courant, difficulté, nombre de connectés) |
| B | Tableau de bord unique avec panneaux redimensionnables |
| C | Plusieurs fenêtres indépendantes (création, battle, profil…) |
| D | Autre : … |

**Sous-questions** :
- Couleur ou visuel distinct pour les **cartes inactives / fusionnées** ? **Oui** ✅.
- Les cartes d'anime affichent-elles une **image** (couverture) ? **A** : URL saisie, affichage si disponible — **B** ✅ proposée : **pas d'image en phase 2** (aucune dépendance réseau externe), ajout possible en bonus.
- Faut-il un écran de connexion dédié (IP + port + identifiant) ? **Oui** ✅.
- Faut-il un **journal d'activité** visible dans l'IHM (utile en démonstration) ? **A** ✅ proposée : oui, simple liste des événements reçus.

**Réponse de l'équipe (relevé du questionnaire rempli)** :
- **Onglets retenus** : `Mes cartes` / `Explorer` / `Mes recommandations` / `Battles` / **`Amis`** / `Profil`, avec une **barre d'état** permanente (connexion, bloc courant, difficulté, nombre de connectés).
- Couleur ou visuel distinct pour les cartes **inactives / fusionnées / archivées** : **oui**.
- **Image de couverture définie par l'utilisateur** : **oui**.
- **Écran de connexion dédié** (IP + port + identifiant) : **oui**.
- **Journal d'activité** visible dans l'IHM (liste des événements reçus) : **oui**.

> ⚠️ **Trois écarts avec la proposition** : ① l'onglet **`Amis`** remplace `Connectés` (il porte le **partage ciblé**) ; ② l'**image de couverture fournie par l'utilisateur** doit être encadrée par la règle de licences et de crédits (ENF-60) — une image déposée par un utilisateur n'est pas couverte par nos crédits ; ③ le **journal d'activité** dans l'IHM est un bon outil de démonstration, à distinguer du **journal serveur** (fichier, format RFC 5424/logfmt).

### Q6.3 — Cas d'erreur pris en charge ✅ **traitée avec le protocole**
**Question** : parmi les cas d'erreur listés au §12.1 du cahier des charges, lesquels sont **prioritaires** ?
- **A** ✅ proposée : **tous les cas listés**, aucun ajout. *(Ils étaient **14** au 19/09/2026, date de la réponse ; le rétablissement de l'authentification en a ajouté **deux** le 22/09 — voir la réponse ci-dessous.)*
- B : les cas listés + les cas de concurrence (deux clients agissent en même temps sur la même carte).
- C : sélection à faire ci-dessous : ✅ **c'est cette option qui a été retenue** — voir la réponse ci-dessous.

**Question liée — format des refus** : **A** ✅ **retenue** : **code d'erreur + message en français**, journalisé côté serveur.

**Réponse de l'équipe (relevé du questionnaire rempli)** : **8 cas prioritaires**, **plus un cas nouveau** ajouté par l'équipe —
**1** (connexion refusée, max de connectés), **2** (identifiant invalide, inconnu **ou déjà connecté**), **4** (message au-delà de 64 Kio), **6** (action sur carte inactive ou fusionnée), **9** (battle refusée : non proches, non actives, déjà en battle), **10** (vote hors fenêtre, vote de propriétaire, **double vote**), **13** (revendication par un non-légitimé, ou sans réponse du propriétaire dans le délai), **14** (exploitation : base indisponible, altération, déconnexion brutale, arrêt pendant un minage) — **et 15** : *« dire que les cartes sont rendues à cause d'un 0 vote »*.

> ℹ️ **Lecture** : il s'agit de la **liste des cas prioritaires** à couvrir en priorité, **pas** d'une limitation — les autres cas du §12.1 restent couverts par le protocole (29 codes) et testés comme cas de robustesse. Le **cas 15** est un **résultat de battle** (0 vote → cartes rendues), il rejoint la réponse Q2.5-B.
> ✅ **Cas 2 tranché le 22/09/2026** : un pseudo déjà connecté **reprend la session existante** — aucun refus, aucun code ajouté. *(Ancienne note : le cas 2 mentionne « déjà connecté » : le protocole actuel ne refuse pas une seconde session du même pseudo (elle **reprend le compte**) — à trancher : refus explicite, ou reprise de session., et ils sont désormais **chacun rattaché à un code d'erreur précis** du catalogue du protocole (`docs/protocole-applicatif.md`, §8.2 — **29 codes**). La liste compte **16 cas** depuis le 22/09 : les **14** d'origine, plus **`1018 CARD_ALREADY_AUTHENTICATED`** et **`1019 AUTHENTICATION_NOT_FOUND`**, nés du rétablissement de l'authentification (DEC-24). Le §12.1 du cahier des charges est la **vue « recette »** de ce catalogue : chaque ligne y renvoie au code exact, et **25 des 29 codes** y sont cités (les 4 autres sont listés comme hors recette). Le format des refus est celui de l**'option A** — objet `error` à trois niveaux :

| Niveau | Contenu | Pour qui |
|---|---|---|
| **`code`** + **`name`** | Entier et nom machine (`-32602 INVALID_PARAMS`, `1001 CARD_LIMIT_REACHED`) | Le **client**, qui branche son traitement dessus |
| **`message`** | Phrase en français, prête à afficher | L'**utilisateur** |
| **`details`** | Objet structuré (`{"field": "episodes", "received": "vingt-cinq"}`) | Le **développeur** — le champ fautif sans avoir à lire les journaux |

Un drapeau **`retryable`** indique en plus si la demande peut être retentée (utile pour `-32001 SERVER_BUSY` face à `-32602 INVALID_PARAMS`, qui ne le sera jamais).

**Le cas de concurrence (option B) est couvert sans code supplémentaire** : les règles métier sont vérifiées **au moment de l'application**, pas à la réception. Deux clients qui tentent la même action en même temps : le premier crée le bloc, le second reçoit un code métier explicite (`1004 RECOMMENDATION_DUPLICATE`, `1015 CLAIM_ALREADY_PENDING`, `1009 CARD_BUSY`…). Aucune écriture concurrente n'est possible, car **le serveur est le seul écrivain** (ENF-05) et inscrit chaque action dans un bloc en série.

**Reporté au cahier des charges** : §5.3 (ENF-22), §12.1, journal **v0.17**.

### Q6.4 — Journalisation : bibliothèques et centralisation des journaux ✅ **décidée en interne**
**Question** : comment journaliser proprement dans les deux langages, et comment rapprocher les journaux du serveur et des clients ?

**Statut** : **décision d'équipe**, comme l'outillage de couverture — **l'encadrant n'impose aucun outil**. Écrite au **§5.8** du cahier des charges (exigences **ENF-55 à ENF-58**, critère **REC-17**).

| Élément | Retenu | Pourquoi |
|---|---|---|
| Bibliothèque **C** | **`zlog`** (HardySimpson) | La plus complète : configuration externe (`zlog.conf`), niveaux libres, sorties multiples, **rotation native**, **MDC**, sûreté entre threads **et** entre processus, ≈ 250 000 msg/s, `zlog-chk-conf`. Repli documenté si le parc bloque l'installation : `rxi/log.c` (2 fichiers, MIT) |
| Bibliothèque **Java** | **SLF4J + Logback** (Maven) | Standard de fait, `MDC` pour la corrélation, appenders rotatif / asynchrone / syslog, configuration externe |
| **Format** | Ligne unique **identique aux deux langages** : `horodatage ISO 8601 UTC \| niveau \| service \| identifiant client \| identifiant de demande \| module \| message \| clés=valeurs` | C'est le format commun qui rend la fusion possible |
| **Corrélation** | L'**identifiant de demande du protocole (ENF-24)** porté par chaque ligne, **via le MDC des deux côtés** (`zlog` / Logback) | Relie une action du client Java à son traitement par le serveur C |
| **Centralisation (développement)** | Fichiers `logs/` + **une commande** : `sort -t'\|' -k1,1 logs/server.log logs/client-*.log` | 0 ligne de code |
| **Centralisation (démonstration)** | **syslog / `systemd-journald`** : `journalctl -f -t anicard-srv -t anicard-client` | Configuration système uniquement, aucun code applicatif |
| **Agrégateur central** (Graylog/GELF, Fluentd) | 🎁 **hors périmètre S3** | Surdimensionné : à citer en soutenance, pas à installer |
| Pièges traités | Non-blocage (ENF-18 / ENF-42), rotation bornée (10 Mo × 5), aucune donnée sensible | Voir §5.8.4 |

**Réponse en texte simple (à recopier telle quelle)**

```
Q6.4 - Journalisation : bibliotheques et centralisation des journaux

Reponse : une bibliotheque par langage, et un format de ligne identique des
deux cotes pour pouvoir fusionner les journaux.

Serveur (C) : zlog (HardySimpson), la bibliotheque de journalisation en C la
plus complete, et sans aucune dependance externe (POSIX et C99). Niveaux
definis librement, format et destinations pilotes par un fichier de
configuration (zlog.conf : fichier statique ou dynamique, stdout, stderr,
syslog, sortie personnalisee), rotation native par taille et par nombre de
fichiers, MDC pour attacher des paires cle-valeur aux lignes, surete entre
threads ET entre processus (indispensable avec le worker de minage),
rechargement de la configuration a chaud, environ 250 000 messages par seconde.
Le binaire zlog-chk-conf valide la configuration avant lancement. Elle se lie
avec -lzlog et s'installe avec make install ; si l'installation est impossible
sur les postes, deux parades : l'embarquer dans l'image Podman, ou basculer sur
le plan B rxi/log.c (deux fichiers C99 sous MIT, sans aucune dependance).

Client (Java) : SLF4J comme facade et Logback comme implementation, declares
dans le pom Maven. Configuration externe (logback.xml) pour changer le niveau
sans recompiler, MDC pour attacher l'identifiant client et l'identifiant de
demande a toutes les lignes, appenders console, fichier rotatif, asynchrone et
syslog.

Format unique, identique dans les deux langages, une ligne par evenement :
horodatage ISO 8601 en UTC a la milliseconde, puis niveau, service (srv ou
cli), identifiant client, identifiant de demande, module, message en anglais,
et enfin les cles=valeurs de contexte. Le message n'est jamais multiligne.
L'identifiant de demande est celui du protocole : c'est lui qui permet de
recoller une action du client Java a son traitement par le serveur C.

Centralisation, trois niveaux :
1. Meme machine (developpement) : le serveur ecrit dans logs/server.log, chaque
   client dans logs/client-<utilisateur>.log. Les formats etant identiques, la
   fusion tient en une commande : sort -t'|' -k1,1 logs/server.log
   logs/client-*.log. Un script scripts/logs.sh encapsule le tri et le suivi en
   direct.
2. Postes separes (demonstration) : on passe par l'outillage systeme, pas par
   l'application. Le serveur ecrit vers syslog, le client utilise le
   SyslogAppender de Logback, et systemd-journald centralise tout : journalctl
   -f -t anicard-srv -t anicard-client. Aucune ligne de code applicatif, c'est
   de la configuration.
3. Agregateur central type Graylog ou Fluentd : hors perimetre de la SAE, a
   citer seulement comme evolution possible.

Points de vigilance : la journalisation ne doit jamais bloquer la boucle
d'evenements du serveur ni le fil d'affichage JavaFX (file bornee et thread
d'ecriture dedie cote C, appender asynchrone cote Java ; si la file est pleine
on abandonne l'entree plutot que d'attendre). Les journaux sont rotes a 10 Mo
sur 5 fichiers, soit 50 Mo maximum par programme : la rotation est native a
zlog cote serveur, et assuree par RollingFileAppender cote client. Aucune donnee sensible n'est
journalisee : des identifiants, des compteurs et des durees, jamais la charge
utile complete ni un mot de passe.

Rien a demander a l'encadrant : aucun outil n'est impose, c'est un choix
d'equipe, ecrit au paragraphe 5.8 du cahier des charges (ENF-55 a ENF-58,
critere REC-17).
```

---

## Thème 7 — Paramètres chiffrés (calibrage)

> Ces valeurs ont été calibrées avec les données de l'annexe D du cahier des charges. Elles n'ont pas besoin d'être rediscutées sauf désaccord.

### Q7.1 — Difficulté de la preuve de travail ⚙️ 🟢 ✅ **tranchée par l'encadrant**
**Rappel du calcul** : le nombre moyen de hachages vaut `16^k` — k=3 → 4 096 ; k=4 → 65 536 ; k=5 → 1 048 576 ; k=6 → 16 777 216.

✅ **Recadrage de l'encadrant (18/09/2026)** : la tolérance **3 ou 4** existe précisément pour **s'adapter aux machines** et obtenir la preuve de travail en un temps acceptable. Il n'est **pas demandé** d'aller plus loin (ni ressources supplémentaires, ni parallélisation) : « vous mettre un défi supplémentaire avant même d'avoir conçu une première version me semble incohérent avec une démarche de production professionnelle ». En revanche, si **tous** les objectifs sont satisfaits en fin de temps imparti, une **exploration** est autorisée.

**Retenu** : **3 ou 4 zéros**, avec **journalisation et affichage** du nombre de hachages, du nonce et de la durée — c'est le travail qui est montré, pas l'attente. L'exploration (difficulté 5-6, parallélisation) devient un **bonus conditionnel de fin de projet** (annexe D.1 du cahier des charges).

🆕 **Ajout de l'équipe — calibrage automatique (DEC-22, §5.2.1 du CDC)** : plutôt qu'une difficulté choisie une fois à la main, le serveur **mesure son débit SHA-256 au démarrage** (banc ≤ 250 ms) et **choisit lui-même** la difficulté dans {3, 4} pour un temps de minage ≤ 1 s, avec **réévaluation continue** si la charge change. Modes : `auto` (défaut) ou `fixe:k` (tests, reproductibilité).

| Sous-question | Options | Proposition |
|---|---|---|
| Mode de détermination | (A) **auto-calibrage au démarrage** ✅ / (B) valeur fixe choisie manuellement avant la démo / (C) les deux, `auto` par défaut | **A** (avec C comme mise en œuvre : le mode `fixe:k` reste disponible) |
| Réévaluation en cours d'exécution | (A) **oui**, sur moyenne glissante des 20 derniers blocs ✅ / (B) non, une seule fois au démarrage | **A** |
| Bornes autorisées | (A) **{3, 4}** ✅ / (B) {3,4,5} / (C) aucune borne | **A** — au-delà : exploration de fin de projet |
| Où est stockée la difficulté | (A) **dans chaque bloc, incluse dans le hash** ✅ / (B) valeur globale courante | **A** — c'est une exigence d'intégrité (ENF-52) |
| Difficulté de repli si la mesure échoue | (A) **3** ✅ / (B) 4 / (C) échec du démarrage | **A** |

**Question liée — tranchée** : le motif se compte en **zéros HEXADÉCIMAUX**, pas binaires. Vérifications faites :

- SHA-256 produit 256 bits = 32 octets = **64 caractères hexadécimaux** ; c'est la forme sous laquelle le hash est manipulé et stocké.
- **1 caractère hexadécimal = 4 bits** : un zéro hex vaut 4 zéros binaires, la difficulté est donc ajustable 16 fois plus finement.
- Chaque caractère hexadécimal a une probabilité **1/16** d'être `0` → difficulté `k` ⟹ **`16^k` hachages** en moyenne (k=4 → 65 536).
- En binaire, « 4 zéros » ne représenterait que **16 hachages** : minage instantané, aucune preuve de travail mesurable. Il faudrait viser ≈ **16 zéros binaires** pour égaler une difficulté hex de 4.
- C'est la convention des blockchains et des implémentations pédagogiques de PoW (comparaison du préfixe de la chaîne, ex. `"0000"` → ≈ 65 536 tentatives).

**Implémentation** : comparaison du préfixe de la chaîne hexadécimale. Attention au cas de la difficulté **impaire (3)** : elle s'arrête au **nibble de poids fort** du deuxième octet.

**Réponse** : ✅ **difficulté 3 ou 4**, calibrée par mesure sur la machine de démonstration, sur des zéros **hexadécimaux** — détail et références en annexe D.1 du cahier des charges

### Q7.2 — Nombre maximum de clients connectés ⚙️ 🟢
| Option | Valeur | Remarque |
|---|---|---|
| A | 8 | Suffisant pour la démo |
| B | 16 (testé jusqu'à 32 en stress) | Proposition initiale de l'équipe — **non retenue** |
| C | 32 | Test de charge visible en démonstration |
| **D** ✅ **retenue** | **100** simultanés sur le serveur (limite paramétrable) | Marge confortable (≈ ×10 avant le plafond des 1 024 descripteurs de fichiers), testable avec un client de test automatisé, et laisse de la place à des démonstrations publiques |
| E | Illimité | Écarté : **« illimité » n'existe pas** — la limite serait reportée telle quelle sur le noyau (descripteurs, backlog) |

**Question liée** : que fait le serveur quand la limite est atteinte ? **A** *(refus explicite, socket fermée)* — **B** ✅ **retenue** *(mise en file d'attente)*.

**Réponse** ✅ **tranchée le 18/09/2026 (décision d'équipe)** : **100 clients connectés simultanés** sur le serveur. **Quand la limite est atteinte, le serveur met la nouvelle connexion en file d'attente** (option **B**) — il ne la refuse pas.

| Situation | Comportement |
|---|---|
| Moins de 100 connectés | Admission immédiate |
| Limite atteinte | Connexion acceptée et **placée en file d'attente** (FIFO) ; le client reçoit « **en attente, position N** » et n'a pas accès aux actions métier |
| Une place se libère | **Admission automatique** du premier de la file, sans reconnexion |
| File pleine (100 en attente) ou attente > 60 s | **Refus explicite** avec message d'erreur, socket fermée, refus journalisé |

**Pourquoi la file d'attente et non le refus** : à 100 clients, les déconnexions libèrent des places en continu ; refuser sèchement un client alors qu'une place se libère dans la seconde est un mauvais service. La file est **bornée** (100 places, 60 s) pour qu'elle ne devienne jamais une attente infinie : au bout du compte on refuse, mais explicitement et en dernier recours.

**Garde-fous techniques** : la file est une simple **liste de sockets en lecture différée** — elle ne consomme **aucun fil** et **ne bloque pas** la boucle d'événements (ENF-18, ENF-57). Elle est **journalisée** (ENF-40) et **affichée** côté client.

**Plafond à ne jamais oublier** : 1 024 descripteurs de fichiers par processus par défaut sous Linux. À 100 connectés + 100 en attente, la marge reste large (×5), mais c'est **le premier plafond du système** — d'où le `LimitNOFILE` documenté en annexe E.

**Pourquoi 100 — les trois raisons**

| Raison | Chiffre | Source |
|---|---|---|
| Largement suffisant pour l'usage visé (démonstration publique, 4 à 6 postes) | 100 connectés / 4 à 8 actifs en démonstration | §2.3 du cahier des charges |
| La marge avant le premier plafond système reste confortable | 1 024 descripteurs de fichiers par défaut vs 100 connectés + 100 en attente (**≈ ×5**) | `RLIMIT_NOFILE`, annexe E.1 |
| La file d'attente est **testable et démontrable** | 1 test de charge qui remplit les 100 places puis vérifie l'admission FIFO (REC-03) | §9.1 du cahier des charges |

**Réponse courte (à donner telle quelle si la question est posée)**

```
100 clients connectes simultanes sur le serveur (limite
parametrable).
Quand la limite est atteinte, le serveur ne refuse pas la
connexion : il la met en file d'attente. Le client recoit un
message "en attente, position N" et il est admis automatiquement
des qu'une place se libere, dans l'ordre d'arrivee. La file est
bornee a 100 places et l'attente a 60 secondes : au-dela, le
client recoit un message d'erreur explicite et la socket est
fermee. C'est le dernier recours, pas le comportement par defaut.

Pourquoi une file d'attente plutot qu'un refus : a 100 clients,
les deconnexions liberent des places en continu. Refuser
definitivement un client alors qu'une place se libere dans la
seconde suivante est un mauvais service. La file absorbe le pic
sans bloquer personne : elle ne consomme aucun fil et ne bloque
pas la boucle d'evenements du serveur.

Ce plafond n'est pas une limite de la machine : Linux accepte par
defaut 1024 descripteurs de fichiers par processus et un backlog
d'ecoute de 4096 connexions, et notre serveur utilise une boucle
d'evenements epoll qui gere des dizaines de milliers de
connexions avec quelques kilo-octets chacune. 100 est un choix de
dimensionnement, pas une contrainte technique : c'est le nombre
qu'on peut tester et demontrer avec un client de test automatise.

Si on voulait servir beaucoup plus d'utilisateurs, les leviers
seraient, dans l'ordre : les reglages noyau (nofile, somaxconn),
le format d'echange (binaire plutot que JSON), un pool de
connexions a la base (PgBouncer), puis plusieurs workers avec
SO_REUSEPORT et un chargeur de repartition. La comparaison
complete des solutions - sockets bruts, API REST, long polling,
SSE, WebSocket, gRPC, file de messages - est dans l'annexe E du
cahier des charges, avec les sources.
```

**Question liée — refus explicite plutôt que file d'attente**

| Critère | A. Refus explicite | **B. Mise en file d'attente** ✅ **retenue** |
|---|---|---|
| Expérience du client | Brutale : il est rejeté, même si une place se libère 1 s plus tard | **Fluide** : il est accepté tout de suite, informé de sa position, et entre automatiquement |
| Complexité | Faible : un test « trop de clients » → message d'erreur | Moyenne, mais **maîtrisée** : file bornée (100), horodatage d'entrée, **expiration à 60 s** — aucun réordonnancement, c'est du FIFO |
| Visibilité de la saturation | Immédiate | **Assurée** : position affichée au client, entrées et sorties de file journalisées (ENF-40) |
| Risque de saturation silencieuse | Aucun | **Couvert** par le bornage : au-delà de 100 en attente ou de 60 s, on **refuse explicitement** |
| Charge serveur | Nulle | Nulle : une liste de sockets en lecture différée, **aucun fil consommé** (ENF-18) |
| Démonstration | Difficile à montrer (un refus) | **Bonne scène** : 100 clients connectés, le 101ᵉ reçoit sa position, un client se déconnecte, il entre tout seul |

**Réponse développée (si l'on demande « et à beaucoup plus grande échelle ? »)** — voir l'**annexe E** du cahier des charges :

1. **La limite n'est pas la carte réseau** : Phoenix a démontré **2 millions de connexions WebSocket** sur une machine, WhatsApp jusqu'à **2,8 millions** par serveur.
2. **Les vrais goulots apparaissent dans cet ordre** : descripteurs de fichiers (1 024), puis modèle de concurrence (un thread par connexion ≈ 2 Mo de pile ⟹ 20 Go pour 10 000 clients), puis PostgreSQL (100 connexions par défaut), puis **la croissance de l'état** — notre blockchain ne se compacte jamais.
3. **Six familles de leviers** : réglages noyau, modèle de concurrence, format d'échange, base de données, architecture applicative, montée en charge horizontale. Les deux premières suffisent jusqu'à ≈ 10 000 clients.
4. **Les solutions de communication ne sont pas hiérarchisées** : sockets bruts = contrôle et latence, mais tout à écrire ; **API REST** = répartition triviale car sans état, mais **aucune poussée serveur** (1 000 clients en sondage toutes les 2 s = 500 requêtes/s permanentes pour souvent rien) ; SSE et WebSocket = poussée, au prix d'un état par connexion ; gRPC = binaire typé.
5. **Ce qu'on en garde dès maintenant, sans rien implémenter** : ne jamais bloquer la boucle d'événements, séparer les responsabilités, versionner le protocole, garder le serveur comme source de vérité, ne jamais faire confiance au client, diffuser sélectivement et paginer — **6 principes déjà couverts** par ENF-18, ENF-24, ENF-44, ENF-45, ENF-50, ENF-21.

**Reporté au cahier des charges** : **annexe E — Passage à l'échelle** (E.1 goulots, E.2 leviers, E.3 comparatif des solutions dont l'API REST, E.4 feuille de route par paliers, E.5 hors-périmètre + 6 principes, E.6 sources) et diagramme `docs/uml/07-montee-en-charge.puml`.

### Q7.3 — Volumes de test et couverture ⚙️ 🟢 ✅ **cadrée par l'encadrant**

**Question posée** : confirmer les cibles de tests et de couverture.

| Élément | Cible proposée | Origine | Quand elle s'applique |
|---|---|---|---|
| **Un test par nouvelle fonctionnalité livrée** | **règle de définition de fini** | ✅ **exigée par l'équipe (Q7.3, 18/09/2026)** | **dès la phase 2, et à chaque lot ensuite** |
| Tests côté serveur (C) | ≥ 30 (**plancher**, monte avec le code) | 🔧 proposé par l'équipe | mis en place en **phase 2**, consolidés en phase 3 |
| Tests côté client (Java) | ≥ 15 (**plancher**, monte avec le code) | 🔧 proposé par l'équipe | mis en place en **phase 2**, consolidés en phase 3 |
| Scénarios d'intégration bout en bout | ≥ 5 | 🔧 proposé par l'équipe | phase 2 |
| Couverture globale | ≥ 60 % | 🔧 proposé par l'équipe | **mesurée en phase 3** |
| Couverture des modules critiques (blockchain, validation, protocole) | ≥ 80 % | 🔧 proposé par l'équipe | **mesurée en phase 3** |
| Test d'altération d'un bloc | 1 | ✅ **imposé par le sujet** | phase 2 |
| Test d'altération de la **difficulté** du bloc | 1 (REC-16) | 🔧 ajouté par l'équipe (conséquence du calibrage automatique, §5.2.1) | phase 2 |

🧑‍🏫 **Réponse de l'encadrant (18/09/2026)** — verbatim :

> « Nous aborderons ces notions de couverture de test au second semestre en **R402 avec SonarQube**. Idem que sur la question précédente, partez du principe que vous allez **mettre en place vos tests et vérifier la couverture**. Vos objectifs sont **cohérents pour un produit abouti** mais vous verrez ces points **en phase 3 après avoir fait un premier prototype**. Ceci peut très bien être mis en place tout au long de votre développement et **le choix des outils utilisés vous appartient**. »

**Réponse de l'équipe (18/09/2026)** : **oui aux cibles**, avec **une règle ajoutée** — « **un test à chaque nouvelle fonction ou création** ». Concrètement : **chaque fonctionnalité livrée arrive avec son test**, c'est la **définition de fini** d'un lot, et cela vaut **dès la phase 2**. Les volumes (≥ 30 tests C, ≥ 15 tests Java) restent des **planchers**, qui montent mécaniquement avec le code. Le passage à un seuil plus ambitieux (**80 % global**) est **reporté au 4 janvier 2027**, en début de phase 3.

**Retenu et appliqué** :

| Point | Décision |
|---|---|
| Nature des seuils | Ce sont des **objectifs de fin de projet (phase 3)**, pas des conditions d'entrée de la phase 2 |
| Ce qui est exigé en phase 2 | **Un test pour chaque nouvelle fonctionnalité ou fonction livrée** (définition de fini) + **mise en place effective des tests** + exécution automatique par le pipeline CI + **vérification continue de la couverture** |
| Outillage | **Rien à installer, rien à choisir** : aucun outil de couverture n'est exigé en S3. Le sujet est traité en **R402 (S4) avec SonarQube**, « le choix des outils utilisés vous appart[ient] ». Pistes facultatives en phase 3 : `gcov` + `gcovr`/`lcov` (C), **JaCoCo** (Java), SonarQube si disponible |
| Rapport de couverture | Attendu avec le **livrable de phase 3** |
| Seuils plus ambitieux (ex. 80 % global) | **Reporté au 4 janvier 2027** (début de phase 3), selon l'état réel du code |
| Justification des valeurs | 60 % est la valeur « acceptable » chez Google, le consensus professionnel se situe à 70–80 % ; 80–90 % visé sur les modules critiques (blockchain, validation, protocole) |

**Réponse** : ✅ seuils conservés comme **objectifs de phase 3**, outils libres, tests mis en place dès la phase 2, **et un test pour chaque nouvelle fonctionnalité** — voir annexe D.4 et ENF-36 du cahier des charges.

### Q7.4 — Critères de recette chiffrés ✅ **confirmée par l'équipe (18/09/2026)**

**Réponse** : ✅ **confirmés en l'état** par l'équipe, sauf le **temps de minage**, qui suit le **calibrage automatique** : ✅ **arbitrage du 22/09/2026 — la cible est la fourchette 0,3 s – 2 s** (à la difficulté choisie dans {3, 4}). C'était la **seule valeur par défaut non retenue**, toutes les autres étant entérinées telles quelles.

**Précision importante** : ces critères ne sont **pas des questions à poser à l'encadrant**. Ce sont des **engagements que l'équipe prend envers elle-même** — la barre qu'elle se fixe pour s'auto-évaluer. Ils sont donc **validés par l'équipe** et déjà écrits dans le cahier des charges (**annexe D.5**), chacun relié à un critère de recette numéroté (§9.1).

| Critère | Valeur retenue | Défini dans | Origine |
|---|---|---|---|
| Temps de minage d'un bloc en démonstration | **0,3 s – 2 s** *(cible revue le 22/09/2026)*, à la difficulté **auto-calibrée** (3 ou 4), avec hachages, nonce et durée **affichés** | REC-06, annexe D.1 | 🔧 équipe |
| Restauration du contexte (≤ 1 000 blocs) | ≤ **10 s** | REC-02, REC-04 | 🔧 équipe |
| Fidélité de la restauration | **100 %** | REC-04 | 🔧 équipe |
| Détection des altérations testées | **100 %**, sur **2 cas** : donnée métier altérée **et** difficulté falsifiée | REC-07, REC-16 | 🔧 équipe (2ᵉ cas issu du calibrage automatique) |
| Clients simultanés en démonstration | ≥ **4** clients humains (un par membre) | REC-03, §9.2 | 🔧 équipe |
| Clients simultanés en charge | **16** testés, **32** en stress | REC-03, annexe D.2 | 🔧 équipe |
| Latence d'une action simple hors minage | ≤ **500 ms** | §2.3 | 🔧 équipe |
| Blocs du scénario de démonstration | ≤ **60** | §2.3, §9.2 | 🔧 équipe |
| Fonctions > 200 lignes | **0** | REC-11, ENF-33 | ✅ **imposé par le sujet** |

**Réponse** : ✅ critères **fixés par l'équipe** et déjà intégrés au cahier des charges (annexe D.5 et §9.1). Aucune validation externe nécessaire : c'est notre propre niveau d'exigence. Un seul critère vient du sujet (0 fonction > 200 lignes) ; le seuil de minage a été **fixé à la fourchette 0,3 s – 2 s le 22/09/2026**.

---

## Thème 8 — Organisation

### Q8.1 — Nom définitif de la plateforme ✅ **tranchée : AnimeTCG**
| Option | Nom | Remarque |
|---|---|---|
| A | ~~AniCard~~ | Nom provisoire — **abandonné** |
| B | OtakuDeck | Plus « communauté » |
| C | KartAnime | Plus ludique |
| D | SenpaiCards | Ton humoristique |
| E | NakamaCards | Clin d'œil, moins explicite |
| F | Autre : … | — |

**Réponse** ✅ **tranchée le 18/09/2026 (décision d'équipe)** : **AnimeTCG**.

**Reporté au cahier des charges** : en-tête du document, titre, tous les renvois internes, **diagrammes UML** (sources `.puml` + exports régénérés), journal **v0.16**. Le nom provisoire « AniCard » n'apparaît plus que dans l'historique des versions.

### Q8.2 — Attribution nominative des 4 pôles 🧑‍💻 🟢
À remplir (voir §6.1 du cahier des charges) :

| Pôle | Référent | Doublure |
|---|---|---|
| M1 — Chef de projet & socle serveur / sockets | ........................ | ........................ |
| M2 — Blockchain & cryptographie (+ spec technique, UML) | ........................ | ........................ |
| M3 — Persistance PostgreSQL & données (+ scénario de démo) | ........................ | ........................ |
| M4 — Clients Java / JavaFX (+ tests, présentation) | ........................ | ........................ |

**Question liée** — faut-il une **rotation** en cours de projet ? **A** ✅ **retenue** : **pas de rotation systématique** — chaque membre doit pouvoir expliquer le lot d'un autre pendant les questions-réponses — **B** : rotation à mi-parcours (écartée : perte de contexte).

**Réponse** ✅ **tranchée le 18/09/2026 (décision d'équipe)** : **référents encore à définir** (à compléter dans le tableau ci-dessus et au §6.1 du cahier des charges), **doublures permanentes**, et **rotation seulement « si besoin »** — une rotation n'est pas un principe du projet, mais reste possible ponctuellement (indisponibilité d'un membre, rééquilibrage de charge).

### Q8.3 — Volume horaire réel 🧑‍🏫 🟢
**Question** *(posée le 18/09/2026, quand nous estimions encore l'enveloppe à **≈ 20 h/étudiant**)* : cette hypothèse est-elle correcte, ou s'agit-il de 20 h **par semaine** pendant les périodes dédiées ?

| Option | Conséquence |
|---|---|
| A | 20 h **au total** par étudiant → plafond ≈ 80 h d'équipe : priorité absolue au fil conducteur, zéro bonus avant stabilité |
| B | 20 h **par semaine** pendant les semaines de SAÉ → enveloppe bien plus large, bonus et tests renforcés envisageables |
| C | Volume variable et non garanti → planification par lots courts, aucun engagement sur une semaine précise |

🧑🏫 **Cadré le 18/09/2026** : le temps total de SAÉ se répartit entre les trois ressources SAÉ ; **la ressource SAÉ 301 compte pour 60 %** de ce temps. **(Chiffre tranché par le sujet officiel le 22/09/2026 : 120 h/étudiant × 3 projets × 60 % = ≈ 72 h/étudiant, soit ≈ 288 h à 4 — le reste de la phrase décrit notre hypothèse du 18/09, conservée au titre de l'historique.)** Hypothèse retenue : ≈ 20 h/étudiant pour cette ressource, très irrégulières. L'équipe de **4 étudiants est acceptée** (groupes de 4 à 5).

**Réponse** ✅ **tranchée le 18/09/2026 (décision d'équipe)** : **option C — volume variable et non garanti**. Conséquence appliquée : **planification par lots courts**, **aucun engagement sur une semaine précise**, priorité absolue au fil conducteur de démonstration. L'hypothèse de **≈ 20 h/étudiant** (H8) reste une **base de dimensionnement, pas un engagement**. La question **R3** (« volume total ») reste posée à l'encadrant, mais **la planification n'en dépend plus**.

---

## Thème 9 — Diagrammes et livrables

### Q9.1 — Outil UML et format de rendu ✅ **déjà tranchée** (encadrant + cahier des charges)
| Option | Outil | Remarque |
|---|---|---|
| **A** ✅ proposée | **PlantUML** (sources `.puml` versionnées dans `docs/uml/`) + export PNG/SVG dans la spec technique | Diff-friendly, reproductible, gratuit |
| B | **StarUML** / **draw.io** (fichiers projet + exports) | Plus visuel, mais binaire et moins « versionnable » |
| C | Les deux : PlantUML pour les sources, exports graphiques pour la soutenance | Le plus sûr |

🧑🏫 **Validé par l'encadrant le 18/09/2026** : **PlantUML** convient (« un bon outil qui permet une version en fonction de l'avancée de votre code »). La modélisation est **itérative**, à granularité croissante. Correction importante : le sujet ne doit **pas** être lu comme limité aux diagrammes de séquence — il faut **tous les diagrammes jugés pertinents, dans la limite de 5 types**. Au premier rendu : le cahier des charges **+ les fonctionnalités singulières du projet**.

**Les 5 types retenus** (voir §5.7 du cahier des charges) : cas d'utilisation ☑ · classes (serveur ✅ + client ☐) · séquence (2 scénarios sur 5 ✅) · états d'une carte ✅ · composants/déploiement ✅.
**Hors décompte des 5 types** : modèle relationnel ☐ · Gantt ☐.

**Réponse** ✅ **tranchée le 18/09/2026 (équipe)** : **option C — plusieurs outils**. **PlantUML** reste la **référence pour les sources** `.puml` versionnées dans `docs/uml/` (c'est ce qui permet à l'encadrant de « voir une version en fonction de l'avancée de votre code »), et des **outils graphiques** (draw.io, StarUML, éditeur PlantUML) sont utilisés **en complément** pour des rendus plus soignés, notamment pour la soutenance. Le socle reste : **source + export commités ensemble**, exports **SVG + PNG** joints à la spécification technique. PlantUML lui-même avait été validé par l'encadrant le 18/09/2026.

**Réponse en texte simple (à recopier telle quelle)**

```
Q9.1 - Outil UML et format de rendu

Reponse : PlantUML (option A).

Les sources sont ecrites en PlantUML, en fichiers .puml, et versionnees dans
docs/uml/ au meme titre que le code. Les exports SVG et PNG sont deposes dans
docs/uml/exports/ et joints a la specification technique : le SVG pour la
projection et l'impression, le PNG pour le depot.

Cinq types de diagrammes au maximum, ceux retenus : cas d'utilisation, classes,
sequence, etats d'une carte, composants et deploiement. Mermaid a ete ecarte.

La modelisation est iterative : la granularite augmente a mesure que le code est
implemente. Les diagrammes sont enrichis a chaque phase et doivent etre
conformes au code livre en phase 3.

Format de rendu retenu : source PlantUML versionnee + export SVG et PNG
versionne dans le meme commit. La regeneration tient en une commande
(python3 docs/uml/render.py), sans rien installer.

Etat au 18 septembre 2026 : 7 diagrammes produits et exportes - cas
d'utilisation, classes serveur, etats d'une carte, deux sequences, composants
et deploiement, plus un diagramme de deploiement pour l'annexe "montee en
charge". Restent a produire : classes client, trois sequences, deploiement de
demonstration, modele relationnel et diagramme de Gantt.

Aucune question a poser a l'encadrant : ce choix est deja valide (18/09/2026).
```

**Où c'est déjà défini dans le cahier des charges** — c'est pour cela que la question est close :

| Élément | Référence |
|---|---|
| Outil retenu : **PlantUML** (sources versionnées + exports), **Mermaid écarté** | Note de lecture du CDC + §5.7 (note de bas de tableau) |
| Exigence correspondante | **ENF-43** — documentation graphique en UML, versionnée avec le code |
| Objectif mesurable | **O9** — 5 types maximum, granularité croissante, conformité au code livré |
| Point ouvert du sujet | **I11** — ✅ levé le 18/09/2026 (PlantUML, 5 types, modélisation itérative) |
| Ressources et budget | §6.3 (outils de documentation et UML) et **B6 — 0 €** |
| Mode opératoire, rendu, conventions de nommage | `docs/uml/README.md` |

**État de production au 18/09/2026** (sources `docs/uml/`, exports `docs/uml/exports/`) :

| Livrable | Fichier source | État |
|---|---|---|
| Cas d'utilisation (DOC-01) | `01-cas-utilisation.puml` | ✅ produit + exporté — *UC20 « Authentifier une carte » grisé : écarté, voir §3.2 du CDC* |
| Classes serveur (DOC-02) | `02-classes-serveur.puml` | ✅ produit + exporté |
| États d'une carte (DOC-05) | `03-etats-carte.puml` | ✅ produit + exporté |
| Séquence — connexion + reconstitution du contexte | `04-sequence-connexion-contexte.puml` | ✅ produit + exporté |
| Séquence — battle complète | `05-sequence-battle.puml` | ✅ produit + exporté |
| Composants / déploiement (DOC-06) | `06-composants-deploiement.puml` | ✅ produit + exporté |
| Déploiement — 3 paliers de montée en charge | `07-montee-en-charge.puml` | ✅ produit + exporté *(hors décompte des 5 types)* |
| Classes client — MVC (DOC-03) | `08-classes-client.puml` | ✅ produit + exporté |
| Séquence — création de carte | `09-sequence-creation-carte.puml` | ✅ produit + exporté |
| Séquence — recommandation et répudiation | `10-sequence-recommandation-repudiation.puml` | ✅ produit + exporté |
| Séquence — revendication | `11-sequence-revendication.puml` | ✅ produit + exporté |
| Modèle relationnel PostgreSQL (DOC-07) | `12-modele-relationnel.puml` + **`sql/schema.sql`** | ✅ produit + exporté |
| Déploiement — disposition des postes en démonstration | `13-deploiement-demo.puml` | ☐ à produire (phase 3) |
| Diagramme de Gantt (DOC-08) | — | ☐ à produire (phase 3) |

**Bilan (mis à jour le 22/09/2026)** : **14 diagrammes source produits, 28 exports (SVG + PNG)** — le n° 14 couvre le partage ciblé. **Les 5 types UML retenus sont complets.** Il ne reste que **2 artefacts**, tous deux **en phase 3** : le déploiement de démonstration et le diagramme de Gantt. Le **modèle relationnel** (DOC-07) est produit sous deux formes complémentaires : le diagramme `12-modele-relationnel.puml` **et** son **DDL exécutable** `sql/schema.sql` (11 tables, contraintes `CHECK`, index partiels). Les trois séquences manquantes (création de carte, recommandation/répudiation, revendication) et le diagramme de classes client ont été produits le 19/09/2026.
**Commande** : `python3 docs/uml/render.py` régénère les exports ; **toujours committer la source et son export dans le même commit**.

### Q9.2 — Arborescence du dépôt `sae-301` 🧑‍💻 🟢
**Proposition** :

```
sae-301/
├── server/                  # code C du serveur + Makefile
│   ├── net/                 # sockets, acceptation, diffusion
│   ├── protocol/            # frames, encodage/décodage, dispatch
│   ├── business/            # règles métier (§4 du cahier des charges)
│   ├── chain/               # blockchain, SHA-256, preuve de travail
│   └── persistence/         # libpq, sauvegarde, restauration
├── client/                  # Java + JavaFX + Maven  →  ARCHITECTURE MVC
│   └── src/main/java/.../{model, view, controller, net}/
├── tests/                   # tests d'intégration, scripts, client de test
├── docs/                    # cahier des charges, spec technique, journal
│   └── uml/                 # sources .puml + exports
├── sql/                     # schéma et scripts PostgreSQL
├── scripts/                 # lancement, démo, restauration, Podman, logs.sh
├── logs/                    # journaux locaux (NON versionné — .gitignore)
├── .gitlab-ci.yml           # pipeline : compilation + tests (ENF-49)
└── README.md
```

✅ **Cadre imposé par l'encadrant (18/09/2026)** : « il faut suivre le **MVC** ». La séparation modèle / vue / contrôleur doit être **visible dans les dossiers** du client (ENF-50) ; aucune règle métier dans une vue, aucun accès base hors du serveur.

**Question** : ce découpage de paquets vous convient-il dans le détail (nommage, granularité), une fois le MVC respecté ?
**Réponse** ✅ **validée le 18/09/2026** : « **ça convient** ». L'arborescence est retenue telle quelle, avec les évolutions apportées depuis : sous-dossiers de la **couche serveur** (`net/`, `protocol/`, `business/`, `chain/`, `persistence/`), **MVC côté client** (`model/`, `view/`, `controller/`, `net/`), **`third_party/`** (bibliothèques vendues, dont le plan B de journalisation) et **`logs/`** non versionné avec `scripts/logs.sh`. **Pas de dossier `bot/`** : le client automatique est écarté de la démonstration (**DEC-14**) et vit dans **`tests/`** comme outil de test de charge.

**Reporté au cahier des charges** : §5.7, ENF-50, I10 et journal **v0.16**.

---

## 📋 Réponses de l'équipe — relevé consolidé (22/09/2026)

> **Ce qui s'est passé.** Le questionnaire a été **rempli par l'équipe** et remis : les réponses figurent désormais **sous chaque question** (blocs « Réponse de l'équipe »). Le présent relevé les consolide, signale les **écarts** avec les propositions qui avaient été portées dans le cahier des charges, et liste **ce qui reste réellement ouvert**.
>
> **Règle appliquée** : la réponse de l'équipe **prime** sur la proposition par défaut. Quand les deux diffèrent, le cahier des charges, le protocole, le schéma et les diagrammes doivent être **mis à jour** — c'est le travail listé en §C ci-dessous.

### A. Ce qui est tranché par l'équipe (13 questions)

| Réf. | Question | Réponse de l'équipe | Écart avec la proposition | Retenue le |
|---|---|---|---|---|
| **DEC-02** | Q1.2-Q1.3 — champs et identifiant | **11 champs obligatoires** (`card_id`, `title_carte`, `sources_id`, `format`, `year`, `genres`, `themes`, `creator_id`, `owner_id`, `status`, `created_at`) ; `card_id` **numérique croissant** ; **doublons conservés** (battles, collections) | ⚠️ champs et doublons | 18/09/2026 |
| **DEC-21** | Q1.4 — cartes actives maximum | **100** ; carte active = mise au public + en deck + stockée côté serveur ; **archivage après défaite** ; à la limite, l'IHM **propose d'en désactiver une** | ⚠️ 10 → **100** | 18/09/2026 |
| **DEC-03** | Q2.1 — proximité | **Titre (sans casse)** et **format** — l'un ou l'autre suffit | ⚠️ P1…P5 **abandonnés** | 18/09/2026 |
| **DEC-05** | Q2.2 — conditions de battle | Cartes **actives**, **même format**, **rangs voisins** (fer ↔ bronze ↔ argent…), 2 cartes | ⚠️ **matchmaking par rang** | 18/09/2026 |
| **DEC-04** | Q2.3-Q2.5 — vote | **5 minutes** ; tout connecté **pendant** le vote ; créateur et propriétaires **exclus** ; **vote définitif** ; égalité → **la plus ancienne** ; quorum non atteint → **battle annulée** ; déconnexion → **vote conservé** | ⚠️ 90 s → **5 min** ; vote **non modifiable** | 18/09/2026 |
| **DEC-06** | Q2.6 — perdante | **Points de recommandation transférés**, carte **archivée** (plus affichée), victoire **comptée** pour la rareté ; **droits inchangés** | ⚠️ `FUSIONNEE` → **`ARCHIVEE`** | 18/09/2026 |
| **DEC-07** | Q3.1 — valeur d'une carte | **Points de recommandation × multiplicateur de rareté** (fer 0× / bronze 1,2× / argent 1,5× / or 1,7× / platine 2×) ; **deux compteurs** affichés ; répudiation **non récupérée** | ⚠️ formule **remplacée** | 18/09/2026 |
| **DEC-08** | Q3.2 — valeur d'un utilisateur | **Total des votes (points) des cartes possédées** | ⚠️ formule simplifiée | 18/09/2026 |
| **DEC-09** | Q4.1 — légitimation | **Au nombre de likes** ; 1 seul titulaire ; **retirable** (dépassement de likes ou victoire en battle) ; justificatif **public** | ⚠️ scrutin 60 s → **likes** | 18/09/2026 |
| **DEC-15** | Q5.2 — partage ciblé | **RETENU** : liste d'amis, acceptation → le demandeur se déplace, refus → rien, notification socket, **inscrit en blockchain** (option A du 22/09/2026 — la liste d'amis reste un réglage hors chaîne), anti-spam **3 messages puis 1/min** | ⚠️ « non retenu » → **retenu** | 22/09/2026 |
| **DEC-18** | Q6.2 — interface | Onglets `Mes cartes / Explorer / Mes recommandations / Battles / **Amis** / Profil` + barre d'état ; visuel distinct pour cartes inactives/fusionnées/**archivées** ; **image de couverture par l'utilisateur** ; **écran de connexion** ; **journal d'activité** | ⚠️ onglet `Amis`, image utilisateur, journal | 18/09/2026 |
| **DEC-19** | Q6.3 — cas d'erreur | **8 cas prioritaires** (1, 2, 4, 6, 9, 10, 13, 14) **+ cas 15** : *cartes rendues faute de votes* | ⚠️ liste prioritaire, pas « tous » | 18/09/2026 |
| **DEC-14** | Q5.1 — bot | **Bot de test + bot de maintenance** ; 1 maximum en démonstration ; votes **hors quorum** (en production) ; **affiché** dans la liste des connectés avec un **marqueur distinct** | ⚠️ bot **visible** | 18/09/2026 |

**Déjà tranchées par ailleurs** (rappel) : DEC-01 (animes — validé par l'encadrant), DEC-10 et DEC-11 (revendication et transaction, équipe du 18/09), DEC-13 (100 clients + file d'attente), DEC-16 (protocole, figé), DEC-17 (4 étudiants), DEC-20 (difficulté 3-4), DEC-22 (calibrage), DEC-23 (journalisation), DEC-24 (authentification), DEC-25 à DEC-29 (sujet §14), I1, I5, I8, I10, I11, I12.

### B. Ce qui restait ouvert — **tout est tranché** (sauf I2)

> 📄 **Liste consolidée de toutes les questions en attente** (équipe et technique uniquement — plus d'encadrant) : **`questions-a-repondre.md`**.

| # | Point | Nature | Qui |
|---|---|---|---|
| ~~1~~ | ~~Q4.1 vs Q2.6~~ ✅ **Tranché le 22/09/2026** : les **droits restent inchangés** (Q2.6 prime) — une victoire en battle ne retire pas la légitimation | ✅ clos | — |
| ~~2~~ | ~~Q4.2 : mois depuis la possession ou la demande~~ ✅ **Tranché le 22/09/2026** : **depuis la demande** | ✅ clos | — |
| ~~3~~ | ~~Q5.2 : carte transférée hors blockchain~~ ✅ **Tranché le 22/09/2026** : **option A — le partage est inscrit en blockchain** ; la liste d'amis reste un réglage hors chaîne | ✅ clos | — |
| ~~4~~ | ~~Q5.1 : bot de maintenance~~ ✅ **Tranché le 22/09/2026** : **test de charge + vérification de chaîne**, sans action métier | ✅ clos | — |
| ~~5~~ | ~~Q2.1 : titre et/ou format~~ ✅ **Tranché le 22/09/2026** : **l'un ou l'autre** (union) | ✅ clos | — |
| ~~6~~ | ~~Q6.3 cas 2 : pseudo déjà connecté~~ ✅ **Tranché le 22/09/2026** : **reprise de session**, aucun code ajouté | ✅ clos | — |
| **7** | **Q8.2 — attribution nominative des 4 pôles** (M1…M4) | organisation — ⚠️ **seul point encore ouvert** | équipe (5 min) |
| ~~8~~ | ~~Sign-off du protocole~~ ✅ **Donné le 22/09/2026** : **23 actions, 22 événements, 30 codes** (§16.3 du protocole) | ✅ clos | — |
| ~~9~~ | ~~Paramètres techniques §15.2~~ ✅ **Tranchés le 22/09/2026, aux valeurs par défaut** : file **100/60 s** (option A), 8 en vol, heartbeat 60 s, `JSON_MAX_DEPTH` 32, port **7001**, `linserv-info-01` en recette / Podman en développement — **seule exception : le minage à 0,3–2 s** (cf. le §2 de `questions-a-repondre.md`) | décisions finales | 📄 spec §15.2 |
| 10 | ~~R19 et R1, R2, R3, R5, R6~~ ✅ **tranchées en équipe le 22/09/2026** (l'équipe n'a plus d'encadrant) | décisions autonomes tracées | 📄 `questions-encadrant.md` v5 |

### C. Impacts à appliquer dans les autres documents

| Document | Ce qui doit changer |
|---|---|
| **Cahier des charges** | §4.2.1 (champs obligatoires, doublons autorisés), §4.4 (état **`ARCHIVEE`**), §4.5.2 (proximité **titre + format**, **rangs voisins**), §4.5.3 (vote **5 min**, **définitif**, quorum → annulation), §4.5.4 (archivage, **droits inchangés**), §4.6 (**formules** de valeur), §4.7.1 (légitimation **par likes**), §4.8 (**partage ciblé retenu**), §3.2/§3.3 (périmètre), §12 (DEC-02 à DEC-21), §12.1 (9 cas prioritaires) |
| **Protocole** | nouveau code **`1020 VOTE_ALREADY_CAST`** (vote définitif) ; action **`share.send`** + événement **`share.received`** activé ; sémantique `legitimation.*` = **likes** (fin du scrutin 60 s, `1013` à retirer ou redéfinir) ; `1001` = **100** cartes ; résultat de battle « **0 vote → cartes rendues** » |
| **Schéma PostgreSQL** | état `ARCHIVEE` (CHECK de `cards`), colonnes de **rareté/victoires**, **1 vote définitif** (`battle_votes`), tables **likes**, **amis**, **partages** |
| **Spec technique** | §5.2 (contrats), §6.1 (entités : like, ami, partage), §6.5 (charge utile des nouvelles actions), §7.4 (catalogue), §13 (tests) |
| **UML** | `02` (classes), `03` (états : + `ARCHIVEE`), `05` (battle : 5 min, rangs, 0 vote), `11` (revendication), nouveau diagramme de **partage / amis** |
| **Note de format** | événements `share.*`, `vote refused`, `like.*` ; catalogue **porté à 69 entrées** ✅ *(fait le 22/09/2026)* |

## Récapitulatif des décisions (mis à jour le 22/09/2026)

| Réf. | Question | Décision retenue | Qui | Date |
|---|---|---|---|---|
| DEC-01 | Q1.1 Nature des objets | ✅ **Animes** — validé par l'encadrant | Encadrant | 18/09/2026 |
| DEC-02 | Q1.2-Q1.3 Champs et identifiant | ✅ **11 champs obligatoires** ; `card_id` numérique croissant ; **doublons conservés** (battles, collections) | Équipe | 22/09/2026 |
| DEC-03 | Q2.1 Critères de proximité | ✅ **Titre (sans casse)** et **format**, l'un ou l'autre suffit | Équipe | 22/09/2026 |
| DEC-04 | Q2.3-Q2.5 Vote, égalité, quorum | ✅ **5 min**, vote **définitif**, créateur/propriétaires exclus ; égalité → **la plus ancienne** ; quorum non atteint → **battle annulée** ; déconnexion → vote conservé | Équipe | 22/09/2026 |
| DEC-05 | Q2.2 Conditions de battle | ✅ Cartes **actives**, **même format**, **rangs voisins** ; 2 cartes ; anti-abus 3/10 min | Équipe | 22/09/2026 |
| DEC-06 | Q2.6 Sort de la perdante | ✅ **Archivée** (plus affichée), recos transférées, victoire comptée ; **droits inchangés** | Équipe | 22/09/2026 |
| DEC-07 | Q3.1 Valeur d'une carte | ✅ **Points de reco × multiplicateur de rareté** (fer 0× → platine 2×) ; **deux compteurs** affichés | Équipe | 22/09/2026 |
| DEC-08 | Q3.2 Valeur d'un utilisateur | ✅ **Total des votes (points) des cartes possédées** | Équipe | 22/09/2026 |
| DEC-09 | Q4.1 Légitimation | ✅ **Au nombre de likes** ; 1 titulaire ; retirable (likes, battle) ; justificatif public | Équipe | 22/09/2026 |
| DEC-10 | Q4.2 Revendication | ✅ Légitimé + carte active + non déjà propriétaire + accord du propriétaire ; fenêtre de décision 120 s en démo ; **silence de 1 mois = transfert** | Équipe | 18/09/2026 |
| DEC-11 | Q4.3 Transaction et absence | ✅ **Demande de plus d'un mois → le demandeur devient propriétaire** ; refus = statu quo sans recours ; aucun litige | Équipe | 18/09/2026 |
| DEC-12 | ~~Q4.4 Authentification~~ | ❌ **Retirée le 18/09/2026, remplacée le 22/09/2026 par DEC-24** (le sujet officiel 2026-2027 exige l'authentification) : fonctionnalité écartée du périmètre (§3.2) | Encadrant | 18/09/2026 |
| DEC-13 | Q7.2 Clients connectés max | **100** simultanés ; au-delà **file d'attente FIFO** (100 places, 60 s) puis refus explicite | 🔧 décision d'équipe, adossée à l'annexe E | ✅ tranchée (18/09/2026) |
| DEC-14 | Q5.1 Bot | ✅ **Bot de test + bot de maintenance** ; 1 max en démo ; votes hors quorum (production) ; **affiché** avec marqueur distinct | Équipe (amendement) | 22/09/2026 |
| DEC-15 | Q5.2 Partage ciblé | ✅ **RETENU** : liste d'amis, acceptation → le demandeur se déplace, refus → rien, notification socket, **inscrit en blockchain** (option A — amitié hors chaîne), anti-spam 3 puis 1/min | Équipe | 22/09/2026 |
| DEC-16 | Q6.1 Protocole (100 % sockets) | ✅ **Figé** : trame `[4 octets][JSON UTF-8]`, 64 Kio, corrélation, événements, versionnage, **aucun chiffrement** (usage local) | Équipe | 22/09/2026 |
| DEC-17 | Q8.2 Rôles | ✅ **4 pôles doublés** ; 4 étudiants acceptés (attribution nominative à faire) | Encadrant + Équipe | 18/09/2026 |
| DEC-18 | Q6.2 Interface graphique | ✅ Onglets `Mes cartes / Explorer / Mes recommandations / Battles / **Amis** / Profil` + barre d'état ; visuel distinct inactives/fusionnées/**archivées** ; **image de couverture utilisateur** ; écran de connexion ; **journal d'activité** | Équipe | 22/09/2026 |
| DEC-19 | Q6.3 Cas d'erreur | ✅ **8 cas prioritaires** (1, 2, 4, 6, 9, 10, 13, 14) **+ cas 17** (« 15 » dans la réponse de l'équipe : cartes rendues faute de votes) ; le reste est couvert par les **30 codes** | Équipe | 22/09/2026 |
| DEC-20 | Q7.1 Difficulté PoW | ✅ **3 ou 4 zéros, calibré par mesure** ; exploration en bonus de fin de projet | Encadrant | 18/09/2026 |
| DEC-21 | Q1.4 Cartes actives max | ✅ **100** ; à la limite, l'IHM **propose de désactiver** une carte | Équipe | 22/09/2026 |
| **DEC-22** | **Q7.1 Calibrage automatique de la difficulté** | ✅ **Auto-calibrage** au démarrage, bornes {3, 4}, **cible 0,3 s – 2 s** (arbitrage du 22/09/2026), banc ≤ 250 ms | Équipe | 18/09/2026 |
| DEC-23 | Q6.4 Journalisation et centralisation des journaux | ✅ **`zlog`** (C) + **SLF4J/Logback** (Java) ; **format de ligne unique** aux deux langages ; fusion par tri ; syslog/journald pour la démo | 🔧 choix d'équipe, **aucun outil imposé** | ✅ tranchée (18/09/2026) |
| I1 | Q8.1 Nom de la plateforme | ✅ **AnimeTCG** (18/09/2026) — CDC, documents et diagrammes renommés | Équipe | 18/09/2026 |
| I5 | Q8.3 Volume horaire réel | ✅ **Chiffré par le sujet officiel** : 120 h/étudiant pour les trois projets → **60 %** pour la SAÉ projet = **≈ 72 h/étudiant** (≈ 288 h pour 4) ; ⚠️ volume réel non garanti (encadrant, 18/09/2026) | Sujet + Encadrant | 22/09/2026 |
| I10 | Q9.2 Arborescence GitLab | ✅ **Validée** (« ça convient », 18/09/2026) : cadre **MVC** côté client, **pas de dossier `bot/` à la racine** (DEC-14) ; *création = tâche 13 de la checklist* | Encadrant + Équipe | 18/09/2026 |
| **DEC-24** 🆕 | **Q4.4 Authentification (rétablie)** | ✅ **Label de confiance émis par un porteur légitime** : une seule active par carte, révocable, **aucun effet sur la valeur**, attribut et non état, suit la carte gagnante après une battle ; codes `1014`/`1018`/`1019` | Équipe (sujet officiel) | 22/09/2026 |
| **DEC-25** 🆕 *(révisée)* | **Sujet §14 — partage ciblé** | ✅ **RETENU (option A, arbitrage du 22/09/2026)** : actions `share.send` (22ᵉ) et `share.respond` (23ᵉ), événement `share.received` **activé**, partage **inscrit en blockchain** (`shares.block_index`) ; la liste d'amis reste un réglage hors chaîne | Équipe | 22/09/2026 |
| **DEC-26** 🆕 | **Sujet §14 — nombre de cartes par battle** | ✅ **2 cartes** (« deux ou plus » autorisé par le sujet) : vote non ambigu, quorum atteignable, démonstration lisible | Équipe | 22/09/2026 |
| **DEC-27** 🆕 | **Sujet §10 — bibliothèques au-delà de la liste blanche** | ✅ **Décision d'équipe (22/09/2026)** : le sujet prévoit une demande, **elle n'a plus de destinataire** → l'équipe **retient, déclare et justifie** (jansson, zlog, SLF4J/Logback, gcovr/JaCoCo), **un repli documenté par dépendance** ; DEC-27 reste ✅ « déclaration, aucune autorisation à demander**. En remplacement, **déclaration** des dépendances (**jansson**, **zlog**/SLF4J+Logback, **gcovr/JaCoCo**) : nom, version, rôle, licence, repli — §5.9.1 du CDC, §5.3.2 et §12.5 de la spec. Écart avec le §10 du sujet **tracé** (`docs/diff-sujet-2026.md`, écart n° 6) | Encadrant + Équipe | 22/09/2026 |
| **DEC-28** 🆕 | **Sujet §10 — images et crédits** | ✅ **Images créées par l'équipe ou sous licence libérale uniquement** (CC0 / CC BY / domaine public) ; crédits **visibles dans l'interface** et inventoriés dans **`CREDITS.md`** ; **aucun visuel officiel d'œuvre** (ENF-60) | Équipe | 22/09/2026 |
| **DEC-29** 🆕 | **Sujet §10 — hôte PostgreSQL** | ✅ **`linserv-info-01`** (LAN de l'IUT) en **recette** ; **PostgreSQL local via Podman** en **développement** et pour la démonstration hors réseau | Équipe | 22/09/2026 |
| I11 | Q9.1 Outil UML | ✅ **PlantUML** ; **5 types de diagrammes max** ; granularité croissante ; exports **SVG + PNG** en place (**14 diagrammes / 28 fichiers** au 22/09/2026) | Encadrant | 18/09/2026 |

**Décisions prises en autonomie le 22/09/2026** — ⛔ **l'équipe n'a plus d'encadrant** : les questions ci-dessous, **rédigées** le 22/09, sont **tranchées en interne** (`questions-encadrant.md` v5) :
1. 🔴 ~~**Barème détaillé**~~ → ✅ **le barème du sujet s'applique au mot** ; critères internes écrits pour le « dépôt GitLab organisé et exploitable ».
2. 🔴 ~~**libuv ou sockets BSD à la main ?**~~ → ✅ **sockets BSD + `epoll` écrits à la main**, libuv **écartée** (le sujet impose les sockets de A à Z et ne liste que POSIX). *(Historique : la bibliothèque pouvait être perçue comme un contournement de l'exercice de programmation système.
3. 🟠 ~~Format d'en-tête attendu pour les fonctions~~ → ✅ **bloc normalisé maison en anglais** (ENF-35).
4. 🟠 ~~Redémarrage à chaud~~ → ✅ **démontré en direct sur 1 000 blocs** (< 10 s), avec sauvegarde de secours.
5. 🟢 ~~Format du support, politique de branches, répétition~~ → ✅ **support Markdown + export PDF** (phase 4), **tronc + branche par fonctionnalité + MR relue**, **répétition demandée en janvier**.
6. 🔴 ~~**Relire le cahier des charges officiel mis à jour dans Git et Moodle**~~ → ✅ **fait (I12 clos)** : le sujet officiel 2026-2027 a été relu, **20 écarts** relevés et traités (`docs/diff-sujet-2026.md`).
7. 🔴 ~~**Confirmation de l'authentification** (CU-20)~~ → ✅ **tranchée en équipe (R19 close)** : le **sujet écrit fait autorité** → authentification **maintenue** (DEC-24). *(Les bibliothèques sont **tranchées depuis le 22/09** : hors liste = **retenues, déclarées et justifiées** avec repli, DEC-27.)*

---

*Questionnaire ouvert le 18/09/2026, **rempli le 22/09/2026 — il fait foi** (les réponses de l'équipe, relevées dans ce document, sont la référence) — à joindre au dépôt `sae-301` à côté du cahier des charges.*
