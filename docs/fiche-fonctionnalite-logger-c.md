# Fiche de fonctionnalité — Logger C maison AnimeTCG

| Métadonnée | Valeur |
|---|---|
| **Identifiant** | F-LOG-01 |
| **Version** | 1.0 — 14/10/2026 |
| **Statut** | **Décision prise ; composant C implémenté et testé de façon autonome.** Le raccordement aux couches applicatives reste à faire lorsque `server/main.c`, `net/`, `protocol/`, `business/` et `persistence/` seront présents. |
| **Documents normatifs** | `docs/note-format-ligne.md` §5–§13 ; `docs/specification-technique.md` §10 ; ENF-55 à ENF-58 et REC-17 du cahier des charges |
| **Implémentation** | `server/util/log.c`, `server/util/log.h`, `server/util/log_events.c`, `server/util/log_events.h` |
| **Guide d'intégration** | `docs/guide-integration-logger-c.md` |
| **Dépendances** | C11, POSIX threads et appels de fichiers POSIX ; aucune bibliothèque de journalisation tierce |

## 1. Résumé exécutif

F-LOG-01 fournit au serveur AnimeTCG un logger **écrit et maintenu dans le dépôt**, sans `printf` de diagnostic et sans moteur C tiers. Son interface est spécialisée pour le contrat AnimeTCG : événements et niveaux fermés, champs typés, contexte `c=`/`r=` local au fil, assainissement anti-injection, file bornée et écriture par un thread dédié.

Le choix **remplace** la décision historique `zlog` avec repli `rxi/log.c` consignée en septembre 2026. Cette dernière reste dans l'historique pour assurer la traçabilité ; elle n'est plus le choix d'implémentation. La modification est répercutée dans la spécification, la note de format et le cahier des charges afin qu'il ne subsiste pas deux décisions actives.

Le composant C est conçu comme un logger serveur **mono-processus, multithread**. Une instance possède un seul fichier actif et un seul thread écrivain. Les appels producteurs n'attendent ni l'écriture disque ni la rotation : ils construisent une ligne bornée sur leur pile, puis tentent une insertion dans la file. En cas de contention ou de saturation, l'événement est abandonné, compté et signalé par une synthèse `WARN`.

## 2. Comparaison des références C et décision

La comparaison porte sur des projets représentatifs et leurs documentations publiques officielles ; elle ne prétend pas mesurer un classement universel de popularité.

| Référence | Pratiques confirmées par la documentation du projet | Apport retenu | Limite au regard d'AnimeTCG |
|---|---|---|---|
| **zlog** | Modèle par catégories/règles, niveaux configurables, plusieurs destinations, format par configuration, MDC, rotation et protection multithread/multiprocessus annoncés par le projet [1][2] | Contexte MDC, séparation événement/configuration, niveaux, rotation, thread écrivain et traitement visible des pertes | Dépendance tierce à installer et à déclarer ; son API et son format ne garantissent pas à eux seuls notre vocabulaire fermé, nos clés typées ni la file non bloquante imposée par ENF-57 |
| **rxi/log.c** | Bibliothèque compacte C99, six macros de niveau, seuil, `FILE *`, callbacks et crochet de verrouillage exposés dans son README officiel [3] | API courte, petit coût d'entrée, contrôle du niveau, verrou explicite | Ne fournit pas le contrat AnimeTCG complet : schéma d'événements, MDC prêt à l'emploi, file asynchrone bornée, rotation/rétention intégrées |
| **log4c** | Catégories, appenders, layouts et configuration XML ; sortie vers fichiers, syslog et autres destinations décrite par le site du projet [4] | Découpage logique entre source, destination et rendu ; sortie système comme option d'exploitation | Modèle de configuration plus étendu que le besoin de cette SAÉ ; le projet devrait encore fournir notre format, nos règles de confidentialité et notre politique de saturation |
| **syslog(3) / RFC 5424** | Interface de transport/collecte et format système normalisé, non un catalogue métier AnimeTCG | Mapping de sévérité et centralisation au niveau exploitation | Ne remplace pas la façade structurée, les bornes de mémoire ou l'assainissement applicatif |

### Décision retenue

**Logger maison derrière une façade publique stable**, avec un seul format conforme à `docs/note-format-ligne.md` et aucune bibliothèque C de journalisation à installer.

Cette décision répond à la demande explicite d'un logger maison et réduit l'adaptation spécifique au projet. Le code reprend des pratiques éprouvées observées dans les références — filtrage par niveau, contexte par fil, formats déterministes, rotation, thread écrivain et compteurs — sans recopier leur API ni prétendre reproduire toute leur maturité.

**Coût assumé :** maintenance et revue de sécurité à la charge de l'équipe ; pas d'appenders configurables, pas de rechargement de fichier de configuration, pas de rotation multiprocessus ni de promesse de débit issue des benchmarks d'autres projets. Si le périmètre évolue vers un collecteur multiprocessus ou des sorties multiples, il faudra réexaminer la décision plutôt que d'étendre silencieusement cette API.

## 3. Problème, objectifs et hors-périmètre

### Problème

Les sorties `printf` sont difficiles à filtrer et à corréler, peuvent bloquer la boucle d'événements, et laissent ouverte l'injection de lignes via des valeurs utilisateur. Une bibliothèque générale ne connaît pas non plus les événements, champs et interdits métier propres à AnimeTCG.

### Objectifs mesurables

1. Produire une ligne UTF-8 valide, mono-ligne, au format partagé `HORODATAGE | NIVEAU | SERVICE | c=CLIENT | r=REQUÊTE | MODULE | message | clés=valeurs`.
2. N'accepter que les événements et clés du catalogue versionné ; ne jamais utiliser un message libre ni interpoler de valeur dans le nom d'événement.
3. Éviter toute attente d'E/S disque dans le fil producteur ; borner la file à 4096 enregistrements.
4. Rendre visible toute perte de file via des compteurs et une synthèse périodique `WARN log dropped`.
5. Limiter chaque journal C à 10 MiB par fichier et cinq fichiers au total, avec des permissions système restrictives.
6. Permettre le réglage du niveau sans recompilation, à l'initialisation (`ANIMETCG_LOG_LEVEL`) et à chaud (`animetcg_log_set_level`).
7. Éviter les allocations par événement ; libérer la mémoire possédée au shutdown.

### Hors-périmètre

- Le futur client Java et son backend SLF4J/Logback, visés mais absents de ce checkout ; ils devront partager la note de format.
- Les règles métier du serveur, la génération des `request_id`, les sockets, la configuration de `journald`, rsyslog et les tableaux de bord.
- La purge des journaux de plus de 180 jours n'est pas faite par le logger C : le script d'exploitation `scripts/logs-purge.sh` est livré avec `--dry-run`, mais sa planification (systemd/cron) reste à la charge du déploiement.
- L'intégrité cryptographique des journaux, la collecte distante intégrée dans le logger, les logs JSON et les messages multiprocessus.

## 4. Acteurs et scénario nominal

**Acteurs :** modules `net`, `protocol`, `business`, `chain`, `persistence`, le thread d'écriture interne, l'équipe d'exploitation et les lecteurs des journaux.

**Scénario nominal :**

1. Le programme initialise le logger une fois, avec `logs/`, `server.log` et le niveau `INFO`.
2. Le producteur pose le contexte de client et de requête pour la durée utile du traitement.
3. Il émet un identifiant d'événement du catalogue et des champs typés ; le message et le module sont dérivés du catalogue.
4. Le logger filtre le niveau, vérifie les clés/types et neutralise les octets dangereux sans conserver les pointeurs appelants.
5. Il tente `pthread_mutex_trylock`, copie le record dans le ring buffer et retourne ; aucune écriture disque n'est exécutée sur ce fil.
6. Le thread écrivain dépile, fixe l'horodatage UTC monotone, effectue éventuellement la rotation et écrit en append.
7. À l'arrêt ordonné, le serveur journalise `server stopped`, cesse les producteurs puis appelle `animetcg_log_shutdown`, qui draine, `fdatasync` et ferme le fichier.

## 5. Contrat fonctionnel et interface

### API publique

Le fichier `server/util/log.h` expose notamment :

- `animetcg_log_init(config)` et `animetcg_log_shutdown()` ;
- `animetcg_log_emit(event, fields, count)` ;
- `animetcg_log_set_level`, `animetcg_log_get_level` et `animetcg_log_is_enabled` ;
- `animetcg_log_set_context(client_id, request_id)` et `animetcg_log_clear_context` ;
- `animetcg_log_get_stats` et `animetcg_log_flush(timeout_ms)` ;
- constructeurs de champs texte, entier signé/non signé et décimal à une décimale ;
- `animetcg_log_sanitize`, testable séparément.

Le contrat C complet, dont les valeurs de retour, est documenté dans `server/util/log.h` et dans le guide d'intégration.

### Niveaux

| Niveau | Sens retenu | Seuil par défaut |
|---|---|---|
| `TRACE` | Détail de développement très fin | Non |
| `DEBUG` | Détail utile au diagnostic ; seule la ligne dédiée `client address` peut porter l'IP | Non |
| `INFO` | Faits métier et cycle de vie | **Oui** |
| `WARN` | Refus métier, anomalie récupérée, perte de log | Selon événement |
| `ERROR` | Échec d'une opération attendue | Selon événement |
| `FATAL` | Service indisponible | Selon événement |
| `OFF` | Seuil interne de désactivation ; ne constitue pas un niveau de ligne | Non |

Le niveau associé à un événement est fixé dans `log_events.c`. Le seuil filtre les événements ; il ne permet pas de requalifier arbitrairement un `WARN` en `INFO`.

### Événements et clés

- Le catalogue C contient **70 identifiants** et correspond au §8.2 de la note de format ; il inclut aussi des événements client-only non émis par ce composant serveur et l'identifiant historique `claim closed`, désormais obsolète.
- `client address` figure immédiatement après `client connected`, avec le niveau `DEBUG`; la clé `ip` est réservée à cet événement. L'adresse reste une donnée personnelle et doit être activée uniquement pour un diagnostic justifié.
- Le logger du serveur accepte uniquement les événements serveur actifs. Les événements marqués « client seulement » et l'événement obsolète `claim closed` sont rejetés.
- Le C émet `service=srv`; `module`, `message` et le niveau proviennent des métadonnées immuables de l'événement.
- Les clés sont une énumération C fermée avec contrôle du type générique. Les chaînes sont bornées à 64 points de code ; les clés dépassant la limite par ligne sont résumées sous `extra=N`. Les listes de valeurs sémantiques et la combinaison clé/événement restent à vérifier par le producteur (l'API ne spécialise que la politique `ip`).
- Les noms `backlog`, `queue_depth` et `outcome` sont présents dans le registre C et dans la note de format, car ils apparaissent dans les exemples/événements normatifs.

### Contexte de corrélation

Le contexte est **thread-local** et copié dans chaque record avant l'enqueue. `client_id` absent devient `c=-`; `request_id` absent devient `r=-`. Un identifiant de requête présent doit comporter exactement 12 chiffres hexadécimaux minuscules.

Le contexte n'est pas automatiquement propagé entre fils applicatifs : un travail transféré à un thread de travail doit inclure `client_id` et `request_id` dans sa structure de tâche, puis rétablir le contexte dans le fil destinataire.

## 6. Format, assainissement et confidentialité

### Format émis

```text
YYYY-MM-DDTHH:MM:SS.mmmZ | LEVEL | srv | c=ID|- | r=12hex|- | module | event | key=value ...
```

Une ligne de journal fait au plus **512 octets, LF compris**. Le format, la grammaire et le mapping normatifs restent ceux de `docs/note-format-ligne.md` v1.4. Le timestamp est généré par le fil écrivain au moment de l'écriture et reste strictement croissant à la milliseconde pendant la vie du processus ; si l'horloge système recule ou retourne le même milliseconde, le logger avance le timestamp logique. Cette garantie ne traverse pas un redémarrage du processus : synchroniser les hôtes et éviter les retours d'horloge restent des responsabilités de déploiement.

### Neutralisation

- CR, LF, TAB, autres contrôles, `|`, `"` et `\` sont échappés selon la note.
- Les séquences UTF-8 valides sont conservées ; chaque octet initial d'une séquence invalide devient `?`.
- Les limites de valeur et de ligne ajoutent `~` sans couper une séquence UTF-8 ni un escape complet.
- Les valeurs nécessitant un échappement sont citées façon logfmt ; une valeur absente est `-`.

### Données interdites et données de diagnostic

Aucune charge utile complète, aucun titre/synopsis/justification en clair, mot de passe, jeton, secret, requête SQL, chemin système ou variable d'environnement ne doit être passé au logger. On journalise l'action, les identifiants métier strictement utiles, les compteurs et les longueurs.

L'adresse IP, donnée personnelle, n'est permise que sur `client address` au niveau `DEBUG`; les autres événements contenant `ip` sont refusés par l'API. `INFO` et les autres événements ne doivent pas recopier cette adresse.

## 7. Architecture et gestion des ressources

| Élément | Propriété | Ressource / borne |
|---|---|---|
| File circulaire | Allouée une fois dans `animetcg_log_init`, possédée par le logger | 4096 records de 514 octets (`uint16_t` longueur + 512 octets ligne), soit **2 105 344 octets ≈ 2,01 MiB** |
| Producteur | Aucun `malloc`, aucune écriture fichier, aucun attente sur le mutex de file | Buffer temporaire borné de 512 octets sur la pile |
| Thread écrivain | Unique propriétaire des opérations `write`, `rename`, rotation et timestamp | Un thread POSIX par logger/processus |
| Fichier | `open(O_RDWR|O_APPEND|O_CREAT)`, régulier, mode forcé 0640 | Le mode lecture/écriture permet de vérifier la fin du fichier et de retirer seulement une ligne incomplète après arrêt brutal |
| Directory | Créé si absent puis mode 0750 | Le chemin de configuration doit être contrôlé par l'application |
| Shutdown | Draine la file, appelle `fdatasync`, ferme, détruit la condition, libère le ring buffer | Seul chemin de vidage synchronisé ; pas de `fsync` par ligne |

Un compteur de producteurs actifs protège la transition de shutdown. Le mutex de file est tenté avec `pthread_mutex_trylock`; toute contention et toute file pleine incrémentent `dropped` et `pending_dropped`, au lieu de bloquer un appelant.

## 8. Rotation, rétention et centralisation

- Fichier actif par défaut : `logs/server.log`.
- Taille seuil : **10 MiB** (`10 × 1024 × 1024` octets).
- **Cinq fichiers au total** : actif plus `server.log.1` à `.4`; le plus ancien est supprimé lors d'une rotation réussie. Plafond nominal : 50 MiB par processus.
- Les enregistrements complets sont append-only ; à l'initialisation, une terminaison sans `LF` est reconnue comme un éventuel record incomplet et seul ce suffixe est tronqué jusqu'au dernier `LF`. En cas d'écriture partielle, le writer tente le même rollback sur la fin active ; si le rollback échoue, il ferme le descripteur pour éviter d'ajouter des lignes derrière un fragment invalide. Chaque erreur d'E/S est comptée et déclenche au plus un message d'urgence fixe sur `stderr` ; le logger ne se rappelle pas récursivement.
- La rétention de 180 jours reste une tâche d'exploitation et n'est pas confondue avec la rotation par taille.
- Le code ne transporte pas les logs par le socket AnimeTCG. Pour la collecte inter-postes, une configuration système (par exemple rsyslog `imfile`) doit transférer le fichier ; cette configuration n'est pas incluse ni testée par le composant C.

## 9. Paramètres et erreurs

| Paramètre / erreur | Comportement |
|---|---|
| Niveau de base | `minimum_level` de `animetcg_log_config_t`, normalement `INFO` |
| Variable d'environnement | `ANIMETCG_LOG_LEVEL=TRACE|DEBUG|INFO|WARN|ERROR|FATAL|OFF`, lue à l'initialisation ; valeur invalide → niveau configuré + message fixe sur `stderr` |
| Réglage à chaud | `animetcg_log_set_level`; seuil atomique, sans recompilation |
| `ANIMETCG_LOG_OK` | Record accepté dans la file |
| `ANIMETCG_LOG_FILTERED` | Niveau inférieur au seuil ; aucun record produit |
| `ANIMETCG_LOG_DROPPED` | Contention ou file pleine ; compteur mis à jour |
| `ANIMETCG_LOG_ERR_ARGUMENT` / `...PRIVACY_POLICY` | Identifiant d'événement, clé/type générique ou contexte structurel invalide — ou politique `ip` violée ; aucune ligne enqueued. Les listes de valeurs sémantiques et la matrice clé/événement restent à la charge de l'appelant |
| `ANIMETCG_LOG_ERR_NOT_RUNNING` | Logger non initialisé ou déjà arrêté |
| Erreur d'E/S | Compteur `io_errors`, message fixe d'urgence une seule fois sur `stderr`; le serveur n'est pas terminé par le logger |

`animetcg_log_flush()` attend que le ring et l'écriture en cours soient drainés, avec délai borné. C'est une primitive de contrôle/tests : **elle est interdite dans la boucle d'événements et dans les fils de réponse**. `animetcg_log_shutdown()` est un appel bloquant réservé à l'arrêt ordonné.

## 10. Exigences de qualité et limites connues

1. **Thread-safety :** plusieurs producteurs dans un processus ; un seul thread touche le fichier.
2. **Non-blocage :** pas de mutex bloquant, disque ou `fsync` sur le chemin `emit` ; les producteurs peuvent perdre un record sous contention.
3. **Bornes :** queue, record, nombre de champs, valeur et chemin sont bornés ; aucune allocation par événement.
4. **Sûreté :** messages/keys fermés, typing de champs, assainissement, contrôle spécial de la clé `ip`.
5. **Compatibilité :** ordre des champs positionnels immuable ; noms d'événements non renommés ; nouveaux événements/keys documentés d'abord.
6. **Limitations explicites :** pas de support multi-processus partageant le même fichier ; ne pas appeler `fork()` après init ; pas de reprise/rotation multiprocessus ; pas de réouverture à chaud d'un fichier externalement déplacé ; fichier local uniquement ; livraison non garantie si queue pleine ou si disque défaillant.

## 11. Critères d'acceptation et état des preuves

| Critère | Test prévu | État observé pour le composant autonome |
|---|---|---|
| Catalogue (compte 70), entrée dépréciée, règles de niveau testées et grammaire | Compte, politique de dépréciation, filtrage représentatif et validation des lignes produites | **Exécuté** dans le harnais `server/util/test_log.c` ; la parité complète Markdown/code reste une revue explicite |
| Une émission hostile = une seule ligne | CR/LF/separateur assainis ; aucun timestamp forgé en début de nouvelle ligne | **Exécuté** ; le harnais compare aussi le delta de `written` à 1 |
| Clés et dépassement | Types validés ; 8 clés max ; surplus résumé par `extra=N` ; ligne ≤ 512 octets | **Exécuté** |
| Non-blocage / pertes | Point d'injection test-only suspend le writer, sature la file et vérifie le compteur/synthèse | **Exécuté** ; le test impose une durée producteur < 500 ms |
| Rotation | Écrit au-delà de 10 MiB ; compte fichiers, taille et chronologie | **Exécuté** ; test vérifie cinq fichiers max et 50 MiB nominal |
| Récupération d'E/S | Répare une terminaison sans LF au démarrage et injecte un short write pour vérifier rollback + poursuite | **Exécuté** dans le harnais C maison ; le scénario émet une seule alerte fixe `stderr` attendue |
| Niveau réglable | Défaut INFO, événement DEBUG filtré puis accepté après changement dynamique | **Exécuté** |
| Concurrence et mémoire | Plusieurs producteurs ; ASan/UBSan ; fermeture propre | **Exécuté** avec `make -C server/util sanitize` |
| Valgrind Memcheck | Fuites, invalid reads/writes, frees invalides | **Non exécuté** : Valgrind n'est pas disponible dans l'environnement ; ASan/UBSan passe. Le harnais fournit la cible `memcheck` |
| Client ↔ serveur, 20 corrélations bout en bout | Scénario multi-programme REC-17 | **Non exécuté** : l'application serveur et le client ne sont pas présents dans ce checkout |
| Rotation + purge de 180 jours sur un déploiement | REC-17 test 10 | **Purge exécutée** sur fichiers temporaires via `scripts/logs-purge.sh` (dry-run + suppression) ; planification et recette sur les journaux du déploiement restent à faire |

Commande de validation locale :

```bash
make -C server/util clean test
make -C server/util sanitize
```

## 12. Traçabilité

| Source | Exigence / décision | Couverture F-LOG-01 |
|---|---|---|
| Cahier des charges ENF-55 | Journal dédié, pas de sorties de diagnostic `printf` | API `animetcg_log_emit`; appels fonctionnels par événements typés |
| ENF-56 | Format unique documenté | Note de format v1.4, formatter C et tests de conformité |
| ENF-57 | Pas de blocage, 4096 records, pertes comptées/synthétisées | Ring buffer, `trylock`, thread dédié, test de saturation |
| ENF-58 | Niveau modifiable, rotation, confidentialité et conservation | Variable d'environnement/API, 10 MiB × 5, politique de données ; `scripts/logs-purge.sh` gère la rétention, planification à faire au déploiement |
| REC-17 | 10 preuves de recette | Format, injection, pertes, rotation/niveau couverts localement ; purge vérifiée sur répertoire temporaire ; intégration multi-programme et planification réelle restent ouvertes |
| ADR-05 (historique) / ADR-15 | `zlog` remplacé par logger maison C | Décision et conséquences documentées en DOC-09 §15 |

## 13. Sources externes — comparaison de conception

1. HardySimpson, **zlog**, README officiel — catégories/règles, sorties, MDC, rotation, sécurité des threads/processus : <https://github.com/HardySimpson/zlog>
2. HardySimpson, **zlog User's Guide** — configuration, rotation et APIs : <https://hardysimpson.github.io/zlog/UsersGuide-EN.html>
3. rxi, **log.c**, README officiel — C99, niveaux, sortie `FILE *`, callbacks et verrouillage : <https://github.com/rxi/log.c>
4. **log4c**, site et documentation officiels — catégories, appenders, layouts, fichier XML, sorties fichier/syslog : <https://log4c.sourceforge.net/>
5. IETF, **RFC 5424 — The Syslog Protocol** : <https://www.rfc-editor.org/rfc/rfc5424.html>
