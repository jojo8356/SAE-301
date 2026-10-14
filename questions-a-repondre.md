# Décisions finales — AnimeTCG (22/09/2026)

> ## ✅ Tout est tranché — une seule exception
>
> **Les valeurs par défaut de tous les points ouverts sont retenues comme valeurs finales** (décision d'équipe du 22/09/2026), **à une exception près : le temps de minage**, fixé à la **fourchette 0,3 s – 2 s** (et non « ≤ 1 s »).
>
> Conséquence pratique : **plus aucune valeur n'est provisoire** dans le corpus (CDC, spec, protocole, schéma, UML, note de log). Le dossier peut être figé en **v1.0** et taggé `phase1`.
> ⚠️ Toute modification ultérieure d'une de ces valeurs passe par une **version mineure** du document concerné, avec justification.

---

## 1. Les 11 points d'arbitrage — **décidés**

| # | Question | ✅ Valeur finale | Portée dans le corpus |
|---|---|---|---|
| **1** | **Q4.1 ↔ Q2.6** : la légitimation est-elle retirée par une victoire en battle ? | **Non** — les **droits de la carte perdante restent inchangés** (Q2.6 prime). Seul un **dépassement de likes** change de porteur légitime | CDC §4.5.4, §4.7.3 ; questionnaire (§4.7.3) |
| **2** | **Q1.4 ↔ Q2.6** : `INACTIVE` et `ARCHIVEE` sont-ils deux sorties distinctes ? | **Oui** : `INACTIVE` = **choix de l'utilisateur**, réversible ; `ARCHIVEE` = **défaite**, définitif, plus affichée | CDC §4.4 ; UML `03` |
| **3** | **Q4.2** : le délai d'un mois court depuis la **possession** ou la **demande** ? | **Depuis la demande** de revendication (cohérent avec Q4.3 et DEC-11) | CDC §4.7.2 |
| **4** | **Q2.1** : proximité = titre **et** format, ou titre **ou** format ? | **L'un ou l'autre** (union) — la variante « les deux » est écartée | CDC §4.5.1 |
| **5** | **Q5.2** : la carte transférée par un partage reste-t-elle **hors blockchain** ? | **Non — option A : le partage est inscrit en blockchain** (`share.send` et `share.respond` produisent chacune un bloc). La **liste d'amis** reste un réglage d'utilisateur, hors chaîne | CDC §4.8 ; protocole §6.8 ; **SQL v1.5** (`shares.block_index`) ; note de log `share accepted` ; UML `14` |
| **6** | **Q5.1** : que fait le **bot de maintenance** ? | **Test de charge + vérification de chaîne** en tâche de fond, **sans action métier** ; ses votes ne comptent pas au quorum | CDC §3.2, annexe F |
| **7** | **Q6.3 cas 2** : un pseudo déjà connecté ? | **Reprise de session** — aucun refus, **aucun code ajouté** | CDC §12.1 |
| **8** | **Q4.3** : l'« engagement symbolique » est-il un prix en points ? | **Non** — pas de prix : l'engagement porte sur la **conservation des recommandations** et du statut | CDC §4.7.2 ; questionnaire (Q4.3) |
| **9** | **Noms sur les 4 pôles** (Q8.2 / I2) | ⚠️ **SEUL POINT NON DÉCIDABLE PAR DÉFAUT** : il faut **4 noms**. Tant qu'ils ne sont pas donnés, le tableau §6.1 du CDC reste « à définir » | CDC §6.1, §13 (I2) ; checklist tâche 3 |
| **10** | **Sign-off du protocole** | ✅ **Validé** : **23 actions · 22 événements · 30 codes** ; trame `[4 o][JSON ≤ 64 Kio]`, 8 en vol, heartbeat 60 s, inactivité 300 s | protocole §16.3 |
| **11** | **Bibliothèques hors liste §10** | ✅ **On garde**, **déclaré et justifié** (jansson, zlog, SLF4J/Logback, gcovr/JaCoCo), **un repli documenté par dépendance** ; libuv **écartée** (`epoll` écrit à la main) | CDC §5.9.1 ; spec §5.3.2 ; annexe A.1 |

---

## 2. Paramètres techniques — **ratifiés aux valeurs par défaut**

| # | Paramètre | Valeur finale | Où |
|---|---|---|---|
| 1 | File d'attente des connexions | **100 places / attente 60 s** (option A) | spec §11.2, §11.13.3 |
| 2 | Demandes en vol par client | **8** | spec §7.1 |
| 3 | Battement de cœur | **60 s** (expiration 180 s) | protocole §6.7, spec §7.1 |
| 4 | **Temps de minage** | ⚠️ **0,3 s – 2 s** *(seule exception : le défaut antérieur « ≤ 1 s » est abandonné)* — `k = 4` dès 32 768 h/s, réévaluation hors [0,3 ; 2], **délai de garde 4 s** ; borne basse inatteignable au-delà de ≈ 218 000 h/s (difficulté plafonnée à 4 par le sujet) → le **temps mesuré est affiché** | CDC §2.2 O3, §5.2.1, annexe D.1 ; spec §8.5, §8.7, §8.9, ANC D |
| 5 | `JSON_MAX_DEPTH` | **32** | spec §5.3, §7.4 |
| 6 | Port d'écoute | **7001** | spec §8.1, protocole §6.1 |
| 7 | Hôte PostgreSQL | **`linserv-info-01` en recette**, **PostgreSQL local via Podman en développement** | DEC-29, CDC annexe A.5 |
| 8 | Licences et `THIRD-PARTY.md` | **Non rédigé** → 🗂️ **tâche** de M3 **avant le tag `phase3`** (ce n'est plus une question) | spec §15.2 |

---

## 3. Ce qu'il reste à faire — **ce ne sont plus des questions**

| # | Action | Qui | Quand |
|---|---|---|---|
| 1 | **Donner les 4 noms** des pôles (réseau & protocole · serveur & chaîne · persistance & données · client) et compléter le tableau §6.1 | Équipe | réunion / avant le 10/10 |
| 2 | Rédiger **`CREDITS.md`** (images sous licence libérale, crédits visibles — ENF-60) | M4 | phase 2 |
| 3 | Rédiger **`THIRD-PARTY.md`** + vérifier les licences (§12.5) | M3 | avant `phase3` |
| 4 | **Figer le cahier des charges en v1.0**, relire, puis poser le **tag `phase1`** | M1 | 8–9 octobre |
| 5 | Créer `logs/` (zlog) et l'**arborescence du dépôt** (README, `.gitlab-ci.yml`, `third_party/`) | M1 + M4 | semaine du 28/09 |

---

## 4. Bloc copiable (sans accents) — relevé de décisions à valider en 5 minutes

```
Releve de decisions - equipe AnimeTCG - 22/09/2026

Valeurs par defaut retenues COMME VALEURS FINALES, sauf une :
- minage : fourchette 0,3 s - 2 s (et non <= 1 s)

1.  Droits de la carte perdante : INCHANGES (une victoire ne retire pas la legitimite)
2.  INACTIVE (reversible, choix utilisateur) et ARCHIVEE (defaite, definitif) : deux sorties distinctes
3.  Le mois de silence court DEPUIS LA DEMANDE de revendication
4.  Proximite : titre OU format (l'un ou l'autre suffit)
5.  Partage cible : INSCRIT EN BLOCKCHAIN (option A) ; liste d'amis = reglage hors chaine
6.  Bot de maintenance : test de charge + verification de chaine, sans action metier
7.  Pseudo deja connecte : reprise de session, aucun code d'erreur
8.  Engagement symbolique : pas de prix en points
9.  Protocole : 23 actions, 22 evenements, 30 codes - sign-off donne
10. Bibliotheques hors liste : gardees, declarees, repli documente ; libuv ecartee (epoll a la main)
11. Parametres : file 100/60 s, 8 en vol, heartbeat 60 s, JSON_MAX_DEPTH 32, port 7001,
    PostgreSQL linserv-info-01 en recette / Podman en developpement

Reste a renseigner : les 4 noms sur les poles.
```

---

## 5. Rappel — ce qui a déjà été décidé avant (ne pas redemander)

- **13 réponses du questionnaire d'équipe** (Q1.1 → Q6.3) — intégrées le 22/09/2026 (`synthese-41-reponses-equipe.md`).
- **6 ex-questions bloquantes + 10 d'appoint** — tranchées en équipe le 22/09/2026 (`questions-encadrant.md` v5) : authentification maintenue, barème du sujet, `epoll` à la main, ≈ 72 h en hypothèse, rendu = spécification technique (Markdown + PDF), 4 clients humains minimum…
- **DEC-01 → DEC-29** : toutes tranchées ; **DEC-12** retirée, remplacée par **DEC-24**.
- **I1 → I16** : toutes closes, **sauf I2** (noms des pôles).
