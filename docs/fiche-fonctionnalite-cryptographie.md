# Fiche de fonctionnalité — Cryptographie de la chaîne AnimeTCG

| Métadonnée | Valeur |
|---|---|
| **Identifiant** | `F-CRYPTO-01` |
| **Version** | 1.0 — 14 octobre 2026 |
| **Statut** | Contrat fonctionnel et noyau C implémenté ; intégration serveur complète à venir |
| **Périmètre** | Empreinte SHA-256, preuve de travail, chaînage et vérification d'intégrité des blocs |
| **Références internes** | Cahier des charges §5.2.1 ; spécification technique §5.3.1, §6.5 et §8.6–8.9 ; `sql/schema.sql` ; guide d’intégration `docs/guide-integration-crypto-c.md` |
| **Code** | `server/chain/crypto.h`, `server/chain/crypto_internal.h`, `server/chain/crypto.c`, `server/chain/sha256.c`, `server/chain/header.c`, `server/chain/pow.c`, `server/chain/verify.c`, `server/chain/test_crypto.c`, `server/chain/Makefile` |

## 1. Objet et résultat attendu

Cette fonctionnalité protège l'intégrité et l'ordre des actions validées par le serveur AnimeTCG. Elle produit une empreinte cryptographique déterministe pour chaque bloc, impose une preuve de travail pédagogique et permet de recalculer les empreintes et les liens de la chaîne afin de signaler une altération.

**Résultat nominal :** une action métier déjà validée est représentée par un bloc dont la charge utile, l'identité, l'horodatage, l'action, le nonce, la difficulté et le hash précédent sont engagés dans un hash SHA-256 reproductible. Le nouveau bloc ne peut être accepté que si son hash respecte la difficulté qui lui est propre et si son lien vers le bloc précédent est correct.

> Cette fiche suit une structure de spécification fonctionnelle : objectif, acteurs, préconditions, scénarios, règles métier, résultats, critères de recette et traçabilité. Cette forme évite de confondre le besoin observable avec le détail d'implémentation et rend chaque règle vérifiable [1].

## 2. Périmètre et exclusions

### Inclus

- SHA-256 de la charge utile canonique et de l'en-tête binaire du bloc ;
- format canonique fixe de l'en-tête version 1 : 96 octets big-endian ;
- preuve de travail avec 3 ou 4 zéros hexadécimaux en tête ;
- nonce 64 bits, budget de tentatives borné ;
- vérification d'un bloc et d'une séquence complète de blocs ;
- sélection de difficulté 3/4 à partir d'un débit SHA-256 mesuré ;
- tests unitaires de vecteurs, de frontières et d'altérations.

### Exclus

- **Chiffrement** des données : SHA-256 est un hachage à sens unique, pas un mécanisme de chiffrement et il n'existe aucune opération de déchiffrement ;
- TLS, chiffrement du canal socket, signature numérique, gestion de clés ou HMAC ;
- consensus distribué, réseau pair-à-pair, rémunération du minage ou garantie qu'une action métier est vraie ;
- parsing et canonicalisation JSON RFC 8785 eux-mêmes ;
- lecture/écriture PostgreSQL, contrôle de cohérence des projections métier et orchestration de threads.

La chaîne est une **blockchain privée simplifiée**, à écrivain unique, de finalité pédagogique. La preuve de travail de difficulté 3/4 sert à démontrer un coût de calcul mesurable ; elle ne constitue pas une sécurité de production.

## 3. Acteurs et composants

| Acteur / composant | Rôle dans la fonctionnalité |
|---|---|
| **Service métier** | Valide droits et règles métier avant d'appeler le module de chaîne. Une action refusée ne crée aucun bloc. |
| **Module C `chain`** | Calcule les empreintes, mine le nonce et vérifie les blocs. Il ne connaît ni les sockets ni PostgreSQL. |
| **Couche JSON / protocole** | Vérifie la forme de la requête et doit fournir les octets JCS exacts à hacher. |
| **Persistance** | Enregistre le bloc, son `payload_canon`, sa difficulté et sa projection métier dans une transaction atomique. |
| **Opérateur / tests** | Déclenche une vérification complète, notamment après restauration ou avant une démonstration d'altération. |

## 4. Glossaire

| Terme | Définition dans cette fonctionnalité |
|---|---|
| **SHA-256** | Fonction de hachage normalisée produisant 256 bits, soit 32 octets. Son résultat est affiché en 64 caractères hexadécimaux minuscules [2]. |
| **Empreinte / hash** | Résultat de SHA-256. Elle dépend des octets exacts reçus ; un changement d'octet change le résultat avec une probabilité écrasante. |
| **JCS** | JSON Canonicalization Scheme, RFC 8785 : forme UTF-8 déterministe d'une valeur JSON, notamment sans espaces insignifiants et avec les propriétés d'objet triées. |
| **Nonce** | Compteur 64 bits que le mineur fait varier afin de chercher un hash respectant le préfixe cible. |
| **Difficulté `k`** | Nombre de caractères hexadécimaux `0` exigés au début du hash. Chaque caractère apporte 4 bits ; une difficulté `k` demande en moyenne `16^k` essais. |
| **Charge utile canonique** | Octets JSON JCS exacts conservés dans `payload_canon`. Ils ne sont jamais reconstruits à partir de JSONB pour vérifier la chaîne. |
| **Projection** | Table SQL dérivée de la chaîne (cartes, recommandations, etc.). Elle accélère les lectures, mais n'est pas elle-même hachée par le présent module. |

## 5. Déclenchement, préconditions et garanties

### Déclenchement

1. Une action métier a passé les contrôles d'autorisation et de validation.
2. Le serveur construit une charge utile immuable et canonique, puis prépare les champs du bloc.
3. Le service de chaîne mine le bloc avant l'écriture atomique bloc + projection.
4. Au démarrage, après restauration, ou sur demande opérateur, le vérificateur parcourt les blocs dans l'ordre.

### Préconditions

- La charge utile fournie au module est déjà une sérialisation JSON JCS UTF-8 ; elle mesure entre **2 et 65 536 octets**.
- Le champ `version` vaut `1`, `timestamp_ms` est non négatif et `difficulty` vaut **3 ou 4**.
- `previous_hash` et `payload_hash` sont fournis comme 32 octets bruts ; le hash stocké est lui aussi comparé sous forme de 32 octets.
- Les blocs de version 1 ont un index compris entre `0` et `UINT32_MAX` ; l'index du genesis est `0`.
- Le code OpenSSL `libcrypto` est disponible. SHA-256 est confié à la bibliothèque, conformément au choix technique du projet [3][4].

### Garanties en cas de succès

- L'en-tête encodé mesure exactement 96 octets ; aucune dépendance à l'endianness de la machine.
- Les tentatives de minage n'allouent pas de mémoire dynamique ; seuls le nonce et le digest changent dans la boucle.
- Une vérification compare le payload hash, le hash de bloc, la preuve de travail et le lien précédent.
- Le module n'altère pas un bloc donné en entrée si son minage échoue ; les résultats de minage sont écrits uniquement en cas de succès.

### Garanties en cas d'échec

- Une erreur est retournée sous forme de `crypto_status_t` explicite ; aucun refus cryptographique n'est assimilé à un succès.
- La recherche de nonce est bornée par un nombre maximal d'essais.
- Une chaîne vide, un genesis incohérent, un index discontinu, un lien cassé, un digest erroné, une difficulté invalide ou un payload hors limites sont refusés.

## 6. Contrat du bloc

### 6.1 Charge utile canonique

Le JSON reçu sur la socket est d'abord validé selon les règles du protocole. La couche JSON doit produire une sérialisation **RFC 8785/JCS**, puis conserver exactement ces octets dans `payload_canon`. Le module C de cryptographie reçoit un pointeur et une longueur ; il ne reparcourt pas le JSON et ne tente pas de le normaliser.

JCS fixe une représentation stable pour le hachage interopérable : propriétés triées par unités de code UTF-16, absence d'espaces insignifiants, sérialisation déterministe des nombres et échappement normalisé des caractères de contrôle [2]. **JCS n'applique pas de normalisation Unicode.** Si la règle de validation métier normalise des chaînes en NFC, cette transformation doit arriver une seule fois **avant** de figer la charge utile ; le sérialiseur JCS conserve ensuite ces points de code tels quels.

Les deux représentations stockées dans PostgreSQL ont des rôles différents :

- `payload JSONB` : recherche et interrogation ;
- `payload_canon BYTEA` : source immuable des octets exacts hachés.

> **Limite explicite du code livré :** `crypto_payload_sha256()` vérifie la longueur et hache les octets fournis, mais ne prouve pas qu'ils sont conformes à JCS. Le serveur doit intégrer un canonicaliseur RFC 8785 testé séparément avant d'utiliser cette API en production.

### 6.2 En-tête version 1 — 96 octets

Tous les entiers sont encodés en **big-endian**. Les hashes sont les octets bruts de 32 octets, pas leur forme hexadécimale.

| Décalage | Octets | Champ | Représentation |
|---:|---:|---|---|
| 0 | 4 | `version` | entier non signé 32 bits ; valeur `1` |
| 4 | 4 | `block_index` | entier non signé 32 bits ; `0` pour le genesis |
| 8 | 8 | `timestamp_ms` | entier signé 64 bits ; millisecondes UTC, figé avant le minage |
| 16 | 32 | `previous_hash` | empreinte brute du bloc précédent ; 32 octets nuls pour le genesis |
| 48 | 4 | `action_id` | identifiant numérique de l'action du protocole |
| 52 | 4 | `difficulty` | entier non signé ; difficulté appliquée à **ce bloc** (3 ou 4) |
| 56 | 8 | `nonce` | entier non signé 64 bits |
| 64 | 32 | `payload_hash` | SHA-256 de `payload_canon` |
| **Total** | **96** | | `block_hash = SHA-256(header[0..95])` |

La difficulté appartient à l'en-tête haché. La vérification utilise toujours la difficulté enregistrée sur le bloc ; elle ne relit pas seulement la difficulté courante du serveur. Le pointeur mémoire vers le bloc précédent et les mesures de temps ne sont pas hashés.

### 6.3 Deux empreintes distinctes

```text
payload_hash = SHA-256(payload_canon)
block_hash   = SHA-256(header_96_octets)
```

Le hash de bloc n'est **pas** `SHA256(SHA256(header))` : il s'agit d'un seul SHA-256, conformément au contrat AnimeTCG. Les durées, le nombre d'essais et la difficulté courante utilisée par le calibrateur sont des mesures d'exploitation ; ils ne sont pas des champs du hash.

## 7. Scénario nominal — création d'un bloc

1. Le service métier rejette toute action non autorisée ou invalide ; aucun bloc n'est créé pour un refus.
2. Le service fige l'action, l'identifiant de bloc, l'horodatage et le hash précédent. Les octets JCS de la charge utile sont conservés tels quels.
3. Le module calcule `payload_hash` sur les octets canoniques.
4. Il construit l'en-tête 96 octets en big-endian, difficulté incluse.
5. Il part du nonce `0`, incrémente d'une unité et calcule un SHA-256 pour chaque tentative.
6. Il accepte le premier hash dont les `k` premiers chiffres hexadécimaux sont `0`.
7. Le nonce trouvé, le hash, la difficulté et `payload_canon` accompagnent le bloc vers la persistance.
8. La persistance valide la transaction bloc + projection ; la diffusion aux clients n'intervient qu'après validation de l'écriture selon le contrat applicatif.

### Exemple de preuve reproductible

Pour l'action de création de carte décrite ci-dessous :

```text
payload_canon = {"episodes":26,"title_romaji":"Cowboy Bebop","year":1998}
payload_hash  = a79a3d87a822afbd43f4191ae858bf03525779cc2f092362ec83953e3a792a23
difficulty    = 3
nonce         = 1484 (1 485 essais depuis 0)
block_hash    = 000e857b87a46fb7617d396f1a60c941e09318da4ba5ea9d3c37ac64e30daf54
```

L'objet JSON est écrit dans l'ordre JCS. La vérification de ce vecteur exerce la charge utile, le hash SHA-256, l'encodage big-endian, la recherche du nonce et le comptage du nibble de difficulté impaire.

## 8. Scénario de vérification de chaîne

Pour une chaîne ordonnée `blocks[0..n-1]`, le vérificateur :

1. refuse `n = 0` ;
2. exige que le premier bloc ait l'index `0` et un `previous_hash` de 32 octets nuls ;
3. exige les indices contigus `0, 1, 2, …` ;
4. pour chaque bloc, recalcule le SHA-256 de `payload_canon` et le compare à `payload_hash` ;
5. reconstruit les 96 octets, recalcule le hash de bloc et le compare au hash stocké ;
6. confirme le préfixe de preuve de travail en utilisant la difficulté de ce bloc ;
7. compare le `previous_hash` du bloc suivant au hash du bloc précédent.

La vérification s'arrête au premier défaut et renseigne dans `crypto_verify_report_t` l'indice de position et l'index du bloc fautif. Un code d'erreur distinct permet de différencier le hash du payload, le hash du bloc, la preuve de travail, le lien et l'index.

**Ce que cette vérification ne fait pas :** elle ne compare pas `cards.value_active` ou les autres tables de projection à l'historique. Une altération d'une projection seulement n'est pas détectée par `crypto_verify_chain()` ; il faut un contrôle distinct qui rejoue/reconstruit les projections à partir des actions de la chaîne. L'intégrité des octets de la chaîne et la cohérence de ses vues SQL sont deux contrôles différents.

## 9. Règles fonctionnelles et techniques

| ID | Règle normative | Contrôle / preuve |
|---|---|---|
| `RC-01` | Le hash de payload porte sur les octets JCS exacts et non sur un JSON re-sérialisé depuis JSONB. | Vecteur du payload canonique + conservation de `payload_canon`. |
| `RC-02` | Le payload de bloc mesure de 2 à 65 536 octets. | Tests de bornes en C ; rejet avant le hachage hors limites. |
| `RC-03` | La sortie SHA-256 mesure toujours 32 octets ; le texte hexadécimal minuscule de stockage mesure 64 caractères. | Vecteurs connus, dont chaîne vide et `abc` [2]. |
| `RC-04` | L'en-tête version 1 mesure exactement 96 octets et tous les nombres sont big-endian. | Test de comparaison des 96 octets au vecteur doré. |
| `RC-05` | Un horodatage négatif, une version inconnue et une difficulté autre que 3 ou 4 sont refusés. | Tests d'arguments invalides. |
| `RC-06` | La difficulté appliquée est incluse dans chaque en-tête et dans son hash. | Test d'altération `difficulty` puis vérification. |
| `RC-07` | La cible est un préfixe de zéros **hexadécimaux** ; pour `k=3`, le nibble haut du deuxième octet doit aussi être nul. | Tests spécifiques aux difficultés impaires. |
| `RC-08` | Le nonce est un entier non signé 64 bits, recherché de façon croissante sans dépassement. | Tests de frontière `UINT64_MAX` et de budget épuisé. |
| `RC-09` | Le budget par défaut vaut `16^(k+1)` : 65 536 essais pour `k=3`, 1 048 576 pour `k=4`. | Valeurs exactes testées ; budget d'appel jamais non borné. |
| `RC-10` | Le vérificateur valide les liens depuis le genesis et la difficulté propre à chaque bloc. | Chaîne synthétique valide, puis variations d'un octet, de l'index ou du lien. |
| `RC-11` | Le noyau C ne fait aucun `malloc`/`free` et n'alloue pas dans la boucle de minage. | Revue du module, tests ASan/UBSan et Memcheck quand disponible. |
| `RC-12` | Un hash ne chiffre pas, ne signe pas et n'authentifie pas l'auteur d'une action. | Limite de sécurité documentée ; TLS et signatures sont hors périmètre. |

### Sélection de difficulté

La politique déterministe implémentée choisit la difficulté 4 à partir de **32 768 hachages/s**, seuil obtenu par `16^4 / 2 s`. En dessous, elle choisit 3. Sous **2 048 hachages/s**, elle conserve 3 et indique à l'appelant que la machine est sous la cible de performance, car la difficulté 2 est hors périmètre. Cette fonction choisit la valeur à partir d'un débit déjà mesuré ; le banc réel (passes chronométrées, médiane, horloge monotone), la réévaluation glissante sur 20 blocs et le délai de garde appartiennent à l'orchestrateur serveur et ne sont pas implémentés dans ce module.

## 10. Erreurs observables

| Statut C | Signification | Traitement attendu par le serveur |
|---|---|---|
| `CRYPTO_ERR_ARGUMENT` | Pointeur obligatoire absent ou entrée incohérente | Erreur interne ; ne pas écrire le bloc. |
| `CRYPTO_ERR_PAYLOAD_LENGTH` | Charge utile hors limites | Refus avant minage ; aucune projection ni bloc. |
| `CRYPTO_ERR_HEADER_VERSION` | Version d'en-tête inconnue | Refus fermé ; aucune interprétation implicite. |
| `CRYPTO_ERR_TIMESTAMP` | Timestamp négatif | Refus de construction/vérification. |
| `CRYPTO_ERR_DIFFICULTY` | Difficulté hors de `{3, 4}` | Refus ; ne jamais remplacer par la difficulté courante. |
| `CRYPTO_ERR_ATTEMPT_BUDGET` / `CRYPTO_ERR_NO_WORK` | Budget invalide ou aucune solution trouvée dans le budget | Ne pas committer ; l'adaptateur serveur peut le traduire en `-32001 SERVER_BUSY` selon le protocole. |
| `CRYPTO_ERR_NONCE_OVERFLOW` | La plage de nonce demandée dépasserait `UINT64_MAX` | Arrêt sans wrap-around. |
| `CRYPTO_ERR_PAYLOAD_HASH_MISMATCH` | Octets du payload différents de ceux engagés | Déclarer le bloc altéré. |
| `CRYPTO_ERR_BLOCK_HASH_MISMATCH` | Hash d'en-tête recalculé différent du hash stocké | Déclarer le bloc altéré. |
| `CRYPTO_ERR_POW_INVALID` | Hash stocké ne respecte pas la difficulté déclarée | Déclarer la preuve insuffisante. |
| `CRYPTO_ERR_CHAIN_LINK` / `CRYPTO_ERR_BLOCK_INDEX` | Genesis, index ou lien incohérent | Arrêter au premier bloc fautif et rapporter sa position. |

Un échec d'allocation mémoire n'existe pas dans le module cryptographique lui-même car il ne fait pas d'allocation dynamique. Un échec de la bibliothèque OpenSSL est néanmoins retourné explicitement.

## 11. Gestion mémoire et robustesse C

Le noyau emploie seulement des tampons bornés sur pile : 96 octets pour l'en-tête, 32 octets pour les digests et une structure de résultat. La taille de payload est contrôlée **avant** l'appel à SHA-256 ; la boucle de minage réutilise le même en-tête et ne crée pas de chaîne hexadécimale à chaque tentative.

Le pointeur `payload_canon` de `crypto_block_t` est **emprunté** : le module n'en prend pas possession et ne le libère jamais. L'appelant doit garder les octets vivants et immuables pendant le minage et la vérification. Les sorties sont des tableaux fixes fournis par l'appelant. Aucun `malloc`, `calloc`, `realloc` ou `free` ne figure dans le code de production du module.

La suite de tests vérifie les pointeurs nuls, les tailles limites, les champs invalides, le nibble haut pour `k=3`, l'épuisement et le dépassement du nonce, la non-modification d'un bloc en échec, les hashes divergents et les liens rompus. AddressSanitizer et UndefinedBehaviorSanitizer sont activés avec `make -C server/chain sanitize`. Memcheck peut compléter cette vérification en détectant notamment accès illégaux, valeurs non initialisées, mauvais `free` et fuites [5].

## 12. Critères d'acceptation et tests

| ID | Étant donné | Quand | Alors |
|---|---|---|---|
| `AC-01` | Message vide puis `abc` | SHA-256 est calculé | Les deux digests correspondent aux vecteurs FIPS 180-4. |
| `AC-02` | Payload JCS canonique du vecteur de création de carte | Payload hashé | `payload_hash = a79a3d87…e3a792a23`. |
| `AC-03` | En-tête du même vecteur | Sérialisation et hash sont calculés | 96 octets exacts, nonce `1484`, hash `000e857b…e30daf54`. |
| `AC-04` | Hash commençant par `00 0f` | Test `k=3`, puis `k=4` | `k=3` passe, `k=4` échoue ; le nibble haut est contrôlé. |
| `AC-05` | Chaîne de deux blocs correctement minés | `crypto_verify_chain()` est appelé | La chaîne est déclarée valide. |
| `AC-06` | Payload, difficulté, hash, index ou lien précédent modifié | La chaîne est vérifiée | Le bloc fautif et la catégorie d'erreur sont retournés. |
| `AC-07` | Budget nul, trop grand ou plage de nonce qui déborde | Le minage est appelé | Erreur déterministe, aucune mutation du bloc et aucune boucle non bornée. |
| `AC-08` | Projet compilé avec drapeaux stricts | `make -C server/chain test` | `-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow` passent et tous les tests réussissent. |
| `AC-09` | Même suite compilée avec ASan/UBSan | `make -C server/chain sanitize` | Tous les tests réussissent sans diagnostic mémoire ou comportement indéfini. |
| `AC-10` | Environnement disposant de Valgrind | `make -C server/chain memcheck` | Aucun bloc « definitely lost » ou « possibly lost » et aucune erreur Memcheck. |

**Commande de recette :**

```sh
make -C server/chain test
make -C server/chain sanitize
# Sur une machine où Valgrind est installé :
make -C server/chain memcheck
```

Le harnais maison contient actuellement **96 vérifications** et s'exécute sans base de données ni serveur réseau.

## 13. Décisions d'arbitrage documentaire

Les documents du dépôt sont riches, mais deux exemples historiques auraient rendu le hash non conforme à la règle JCS si on les copiait littéralement. Les arbitrages suivants sont appliqués dans cette fiche, le vecteur de test et la spécification technique mise à jour :

1. **Ordre JCS des propriétés.** L'ancien exemple `{"title_romaji":"Cowboy Bebop","episodes":26,"year":1998}` n'est pas canonique RFC 8785, dont les clés sont triées. Son `payload_hash` historique correspondait à ces octets non triés. Le vecteur corrigé utilise `{"episodes":26,"title_romaji":"Cowboy Bebop","year":1998}`, son hash `a79a3d87…e3a792a23`, le nonce `1484` et le hash de bloc `000e857b…e30daf54`.
2. **NFC et RFC 8785.** Le protocole peut imposer une normalisation NFC au stade de la validation métier. RFC 8785, lui, préserve les chaînes et ne normalise pas Unicode. La règle retenue sépare ces deux opérations : normaliser une fois avant de figer l'action si le protocole l'exige, puis sérialiser JCS sans transformation supplémentaire.
3. **Taille de `block_index`.** L'en-tête version 1 encode un `uint32`, alors que le schéma utilise `BIGINT`. Le schéma v1.6 conserve `BIGINT` pour les relations, mais ajoute une contrainte `0..4 294 967 295`. Dépasser cette limite exige une nouvelle version de l'en-tête ; un cast silencieux est interdit.
4. **Chaîne et projections.** Modifier `cards.value_active` seul ne change pas les octets de `blocks` et n'est donc pas détecté par le vérificateur de chaîne. La chaîne cryptographique protège l'historique ; un replay ou une comparaison des projections est une vérification séparée.
5. **Portée du code.** Le code fourni implémente SHA-256, sérialisation d'en-tête, PoW, choix de difficulté depuis un débit donné et vérification de chaîne en mémoire. Il n'implémente pas encore le canonicaliseur RFC 8785, le banc chronométré complet, l'écriture PostgreSQL ou les contrôles de projections. Ces limites sont explicites et ne doivent pas être présentées comme déjà livrées.

## 14. Traçabilité

| Source / exigence | Élément de cette fiche | Preuve / implémentation |
|---|---|---|
| CDC §5.2.1, ENF-47 | Hash, chaîne, détection d'altération | `crypto_verify_block()` / `crypto_verify_chain()` |
| ENF-52, REC-16 | Difficulté de chaque bloc dans l'en-tête hashé | Vecteur de bloc et test de falsification |
| Spécification technique §5.3.1 | SHA-256 fourni par OpenSSL, pas une implémentation maison | `server/chain/sha256.c` |
| Spécification technique §6.5 | Header de 96 octets, payload canonique, vérification | `server/chain/header.c`, `server/chain/pow.c`, `server/chain/verify.c`, vecteur doré de `test_crypto.c` |
| Spécification technique §8.7–8.9 | Bornes `{3,4}`, seuils de débit, budget de nonce | `crypto_difficulty_for_rate()` et `crypto_default_attempt_budget()` |
| SQL DOC-07 | Payload exact et index version 1 borné | `blocks.payload_canon`, `blocks_index_range` |
| REC-07 | Vérification de cohérence de la chaîne | Suite C `test_crypto.c` |

## 15. Références externes consultées

1. Université de Lorraine / Télécom Nancy, **« De l’analyse à la spécification : SRS, exigences testables et critères d’acceptation »** — structure de spécification, cas d'utilisation, pré/postconditions, critères d'acceptation et traçabilité : <https://gibson-pages.telecomnancy.univ-lorraine.fr/francois.charoy/complement/cours/chapitre04/>.
2. RFC Editor, **RFC 8785 — JSON Canonicalization Scheme (JCS)** — représentation invariante pour les opérations de hachage/signature, règles de sérialisation et ordre des propriétés : <https://www.rfc-editor.org/rfc/rfc8785>.
3. NIST, **FIPS 180-4 — Secure Hash Standard** — standard SHA-256 et vecteurs de référence : <https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=910977>.
4. OpenSSL, **SHA256 / EVP digest documentation** — API SHA-256 et distinction entre fonction one-shot et anciennes fonctions de contexte : <https://docs.openssl.org/3.2/man3/SHA256_Init/> ; <https://docs.openssl.org/3.3/man3/EVP_DigestInit/>.
5. Valgrind, **Memcheck User Manual** — détection d'accès invalides, de valeurs non initialisées, de libérations incorrectes et de fuites : <https://valgrind.org/docs/manual/mc-manual.html>.
