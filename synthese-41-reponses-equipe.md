# Demande n° 41 — « on avait déjà tout répondu » : relevé, propagation, reste à faire

**Date** : 22/09/2026 · **Objet** : les **réponses de l'équipe** au questionnaire (13 questions) font foi ; elles remplacent les propositions par défaut partout dans le corpus.

---

## 1. Le constat de départ

`questionnaire-decisions.md` affichait, pour treize questions, la **proposition par défaut** de l'assistant suivie d'un « Réponse : ....... » — comme si l'équipe n'avait jamais répondu. Or l'équipe **avait répondu**, et ses réponses **divergent** sur huit points structurants. Le cahier des charges, le protocole, le schéma SQL, les diagrammes et la note de log portaient donc des **valeurs périmées**.

**Ce qui a été fait dans cette demande** : ① les réponses sont **intégrées** au questionnaire (fait juste avant, 1 119 l.) ; ② elles sont **propagées** dans le corpus (ci-dessous) ; ③ cette synthèse.

---

## 2. Les 13 réponses, et ce qu'elles ont changé

| Réf. | Question | Réponse de l'équipe | Le corpus disait | Corrigé |
|---|---|---|---|---|
| **Q1.1** | Objets d'une carte | anime : titre, **personnages, lieux, objets, thèmes, liens** (amour, rivalité, maître/apprenti, famille) | seuls titre/studio/genres modélisés | §4.2 CDC |
| **Q1.2** | Champs obligatoires | **11 champs** (`card_id`, `title_carte`, `sources_id`, `format`, `year`, `genres`, `themes`, `creator_id`, `owner_id`, `status`, `created_at`) | 15 obligatoires (dont `studio`, `episodes`) | §4.2.1 CDC, SQL (`studio`/`episodes` facultatifs), DOC-09 §7.4.2 |
| **Q1.3** | Identifiant, doublons | id **numérique croissant** ; **doublons autorisés** | unicité implicite | §4.2.2 CDC |
| **Q1.4** | Cartes actives max | **100** (au lieu de 10) ; désactivation **par l'utilisateur** ; à la limite, l'IHM **propose de désactiver** | 10, refus sec | §4.2.2, `1001` (protocole), SQL, DOC-09 §7.4.2/§7.2, UML `09`, `12` |
| **Q2.1** | Proximité | **titre (sans casse)** et **format** | P1…P5 (franchise, distance d'édition, studio…) | §4.5.1 CDC, UML `05`, DOC-09 `battle.c` |
| **Q2.2** | Ouvrir une battle | actives + **même format** + **rangs voisins** | format libre, pas de rang | §4.5.2 CDC, UML `05`, DOC-09 |
| **Q2.3** | Durée du vote | **5 min (300 s)** | 90 s | §4.5.3 CDC, SQL `battle_duree`, DOC-09, UML `05` |
| **Q2.4** | Droit de vote | tout connecté **pendant** le vote ; **créateur et propriétaires exclus** ; **vote définitif** ; bots hors quorum en production | vote modifiable, exclusion des seuls propriétaires | §4.5.3, **code `1020 VOTE_ALREADY_CAST` (30ᵉ)**, tableau de recette n° 10, note de log `vote refused` |
| **Q2.5** | Égalité, quorum, déconnexion | égalité → **la plus ancienne** ; quorum non atteint **ou 0 vote** → **battle annulée, cartes rendues** ; déconnexion → **vote conservé** | départage par valeur active ; « battle nulle » sans cas dédié | §4.5.3, §12.1 **cas 17**, SQL `outcome`, protocole `battle.closed`, UML `05` |
| **Q2.6** | Sort de la perdante | **archivée** (plus affichée), recos transférées, **+1 victoire**, **droits inchangés** | état `FUSIONNEE`, droits hérités par la gagnante, réactivation possible | §4.4 (état **`ARCHIVEE`**), §4.5.4, §4.7.3, **état `SANS_OBJET` retiré du SQL**, règle CU-17 du protocole retirée, note de log `card archived`, UML `02`, `03`, `05`, `08`, `12` |
| **Q3.1 / Q3.2** | Valeurs | `V(carte) = points de reco × multiplicateur de rang` (fer 0× → platine 2×) ; `V(user)` = **total des votes des cartes possédées** | `Σ poids + 3 × victoires` ; cartes + Σ valeurs | §4.6 CDC, SQL (`wins`, `rarity_rank`), DOC-09 §6.1, UML `02`, `12` |
| **Q4.1** | Légitimation | **au nombre de likes** (1 titulaire, retirable) | scrutin interne de 60 s (≥ 2 approbations) | §4.7.1 CDC, **`legitimation.respond` = un like**, `1013 LEGITIMATION_CLOSED` **réservé/non émis**, SQL (`likes`, table `legitimation_likes`), note de log (`legitimation liked`, `legitimation taken`), UML `12` |
| **Q5.1** | Bot | **bot de test + bot de maintenance**, 1 max en démo, votes **hors quorum**, **affiché avec un marqueur distinct** | DEC-14 « non retenu », bot invisible | §3.2 CDC (nuancé), DEC-14 amendée |
| **Q5.2** | Partage ciblé | **RETENU** : liste d'amis, acceptation → **le demandeur se déplace**, refus → rien, notification socket, **inscrit en blockchain** *(arbitrage du 22/09/2026 : option A)*, **hors blockchain**, anti-spam 3 puis 1/min | bonus **non retenu** | §4.8 CDC, §3.3 (bonus → périmètre), **actions `share.send`/`share.respond` (22ᵉ, 23ᵉ)**, événements `share.received`/`share.responded`, **tables `friendships` et `shares`**, note de log, **UML 14 (nouveau diagramme)** |
| **Q6.2** | Interface | onglets `Mes cartes / Explorer / Mes recommandations / Battles / **Amis** / Profil`, **image de couverture**, écran de connexion, **journal d'activité** | pas d'onglet `Amis`, pas de journal | §12 (DEC-18), UML `08`/`01` |
| **Q6.3** | Cas d'erreur | **8 prioritaires** (1, 2, 4, 6, 9, 10, 13, 14) **+ un cas « cartes rendues faute de vote »** | « 16 cas, tous au même niveau » | §12.1 CDC (priorités marquées, cas **17** ajouté), §12.2, DOC-09 (REC-13), diff-sujet |

**Nouveaux nombres du corpus** (après propagation) : **14 tables** · **23 actions** · **22 événements** · **30 codes d'erreur** · **17 cas de recette** (8 prioritaires) · **69 entrées de journal** · **14 diagrammes / 28 exports**.

---

## 3. État du corpus après correction

| Document | Version | Ce qui a changé |
|---|---|---|
| `cahier-des-charges-sae-s3.md` | **0.22** (1 867 l.) | cartes (§4.2), états (§4.4), battle (§4.5), valeurs (§4.6), légitimation (§4.7.1), partage (§4.8), périmètre (§3.2/§3.3), §12 (14 lignes tranchées), §12.1 (priorités + cas 17), §13 I4, annexe F, journal |
| `docs/protocole-applicatif.md` | **1.2** (572 l.) | `1020` ajouté, `share.send`/`share.respond` (§6.8) et événements associés, `legitimation.respond` = like, `1013` réservé, `1001`/`1003` corrigés, CU-17 retirée, §16.2 (journal) |
| `docs/specification-technique.md` | **0.7** (2 046 l.) | §6.1 (3 entités ajoutées, valeurs, revendication), §7.4.2 (lignes 22–23), §7.4.3 (30 codes), §6.4/§8.4, §15.2 (**paramètres tous tranchés le 22/09**), journal |
| `sql/schema.sql` | **1.5** (444 l.) | état `ARCHIVEE`, `wins`/`rarity_rank`, `studio`/`episodes` facultatifs, vote définitif, `likes`, **`legitimation_likes`**, **`friendships`**, **`shares`**, `SANS_OBJET` retiré, 3 dénouements — `pglast` revalide sans erreur |
| `docs/note-format-ligne.md` | **1.3** (568 l.) | `card archived`, `vote refused`, `battle closed` (outcome), `legitimation liked`/`taken`, `share sent`/`accepted`/`refused`, `claim closed` désactivé → **69 entrées** |
| `docs/uml/` | 14 sources / 28 exports | `02`, `03`, `05`, `08`, `09`, `12` corrigés + **`14-sequence-partage.puml`** créé ; exports SVG/PNG régénérés |
| `questionnaire-decisions.md` | 1 116 l. | réponses intégrées, **relevé consolidé** (§A), décisions (**§B — tout est tranché sauf I2**), impacts (§C), récapitulatif daté |
| `checklist-phase1-10-10.md` | 159 l. | tâche 4 (réunion) reformulée, risque « contradictions non arbitrées » |
| `docs/diff-sujet-2026.md` | 138 l. | comptes mis à jour, CU-17 marquée révisée |

---

## 4. Ce qui restait ouvert — ✅ **tranché le 22/09/2026** (sauf le n° 8)

1. ~~**Q4.1 ↔ Q2.6**~~ ✅ **Tranché : Q2.6 prime** — les droits de la perdante restent **inchangés** ; la légitimation ne se perd que par dépassement de likes.
2. ~~**Q1.4 ↔ Q2.6**~~ ✅ **Tranché : deux sorties distinctes** — `INACTIVE` réversible (choix de l'utilisateur) vs `ARCHIVEE` définitif (défaite), écrit au CDC §4.4.
3. ~~**Q4.2**~~ ✅ **Tranché : le délai court depuis la demande** (cohérent avec Q4.3 et DEC-11).
4. ~~**Q2.1**~~ ✅ **Tranché : l'un ou l'autre** (union) — proximité par titre **sans casse** ou par format.
5. ~~Q5.2 — carte transférée hors blockchain~~ ✅ **Tranché le 22/09/2026 : option A** — le partage est **inscrit en blockchain** (la liste d'amis reste un réglage hors chaîne). *(Ancien point de conception, spec §15.2 n° 11 : (A) inscrire le partage, (B) assumer l'exception, (C) bloc de régularisation.*
6. ~~**Q5.1**~~ ✅ **Tranché : bot = charge + vérification de chaîne**, sans action métier ; aucun des deux ne fabrique de valeur.
7. ~~**Q6.3 cas 2**~~ ✅ **Tranché : reprise de session**, aucun code ajouté au protocole.
8. **Q8.2** — attribution nominative des **4 pôles**.
9. ~~**Sign-off du protocole**~~ ✅ **Donné** : **23 actions / 22 événements / 30 codes** (protocole §16.3, spec §7.4.6).
10. ~~Paramètres techniques (spec §15.2)~~ ✅ **Tranchés le 22/09/2026 aux valeurs par défaut** (file 100/60 s, 8 en vol, heartbeat 60 s, `JSON_MAX_DEPTH` 32, port 7001.
11. ~~Encadrant : R19 + R1, R2, R3, R5, R6~~ ✅ **Tranchées en équipe le 22/09/2026** — l'équipe **n'a plus d'encadrant** : authentification maintenue, barème du sujet appliqué au mot, **sockets BSD + `epoll`** (libuv écartée), ≈ 72 h en hypothèse, rendu = spécification technique (Markdown + PDF), 4 clients humains minimum. **Aucune relance à envoyer** (`questions-encadrant.md` v5).

---

## 5. Plan de la réunion d'arbitrage (30 min, semaine du 28/09)

| Temps | Point | Sortie attendue |
|---|---|---|
**Mise à jour du 22/09/2026 — la réunion n'a plus d'objet d'arbitrage** : tous les points ci-dessus ont été **tranchés** (valeurs par défaut retenues comme définitives, **seule exception : le minage à 0,3–2 s**). Il ne reste qu'à :

| Temps | Point | Sortie attendue |
|---|---|---|
| 0–5 min | **Relevé de décisions** (`questions-a-repondre.md` §4, bloc copiable) | « qui s'y oppose ? » — silence = adoption |
| 5–10 min | **Noms sur les 4 pôles** (I2) | tableau §6.1 du CDC rempli |
| 10–15 min | **Sign-off du protocole** (déjà donné par défaut) et **calendrier des tâches 12, 13, 14** | répartition nominative |

*Hors réunion* : `THIRD-PARTY.md` (M3, avant `phase3`), `CREDITS.md` (M4, phase 2), figement v1.0 + tag `phase1` (M1, 8–9 octobre).
