-- ============================================================================
--  AnimeTCG — SAÉ BUT2 S3 — schéma PostgreSQL (DOC-07)
--  Version : 1.6 — 14/10/2026 — phase 1
--  (1.1 : charge utile canonique `payload_canon` ; 1.2 : table `authentications`,
--   rétablie par le sujet officiel 2026-2027 — CU-20, DEC-24 ; 1.3 : colonne
--   `revoked_block_index` ; 1.4 : réponses de l'équipe au questionnaire — état
--   `ARCHIVEE`, vote définitif, légitimation par likes, amis et partages, limite
--   de cartes actives à 100 ; 1.5 : partage inscrit en blockchain ; 1.6 : borne
--   `block_index <= UINT32_MAX`, cohérente avec l'en-tête binaire version 1.)
--
--  PRINCIPES
--   1. `blocks` est la SOURCE DE VÉRITÉ : table append-only, jamais modifiée,
--      jamais supprimée. Le serveur en est le SEUL écrivain (ENF-05).
--   2. Les autres tables sont des PROJECTIONS : un miroir indexé, écrit dans
--      LA MÊME TRANSACTION que le bloc correspondant. C'est ce qui permet la
--      restauration du contexte en quelques secondes (REC-02 : ≤ 10 s pour
--      1 000 blocs) au lieu de rejouer toute la chaîne à chaque démarrage.
--   3. Aucune donnée n'est supprimée : les états (« répudiée », « fusionnée »,
--      « refusée ») sont des colonnes, pas des DELETE.
--   4. Toutes les requêtes du serveur sont PARAMÉTRÉES (ENF-46) — aucune
--      concaténation de chaînes, jamais.
--   5. Les marqueurs d'authentification (CU-20, DEC-24) sont eux aussi des
--      PROJECTIONS : la vérité reste le bloc ; la table `authentications`
--      n'existe que pour afficher le badge sans reparcourir la chaîne.
--   6. La charge utile de chaque bloc est stockée DEUX FOIS, et ce n'est pas un
--      doublon : `payload` (JSONB) sert à interroger, `payload_canon` (BYTEA)
--      conserve les OCTETS EXACTEMENT hachés. Sans ces octets, la vérification
--      de chaîne serait impossible : JSONB réordonne les clés et supprime les
--      espaces, donc le recalcul du hash donnerait un résultat différent du
--      hash d'origine. Voir §6.5 de docs/specification-technique.md.
--
--  UTILISATION
--     psql -U anicard -d anicard -f sql/schema.sql
-- ============================================================================

BEGIN;

-- ----------------------------------------------------------------------------
-- 1. LA CHAÎNE — source de vérité, append-only
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS blocks (
    block_index     BIGINT       PRIMARY KEY,
    ts              TIMESTAMPTZ  NOT NULL DEFAULT now(),
    action_type     VARCHAR(32)  NOT NULL,          -- CREATION_CARTE, RECOMMANDATION, ...
    payload         JSONB        NOT NULL,          -- données d'action (interrogation)
    payload_canon   BYTEA        NOT NULL,          -- octets exacts hachés (RFC 8785 / JCS)
    previous_hash   CHAR(64),                       -- NULL uniquement pour le bloc genesis
    nonce           BIGINT       NOT NULL,
    difficulty      SMALLINT     NOT NULL,          -- difficulté APPLIQUÉE à ce bloc
    hash            CHAR(64)     NOT NULL,

    -- Le hash est un SHA-256 encodé en 64 caractères hexadécimaux minuscules.
    CONSTRAINT blocks_index_range   CHECK (block_index BETWEEN 0 AND 4294967295), -- header v1 encodes uint32
    CONSTRAINT blocks_hash_hex       CHECK (hash ~ '^[0-9a-f]{64}$'),
    CONSTRAINT blocks_prev_hash_hex  CHECK (previous_hash IS NULL OR previous_hash ~ '^[0-9a-f]{64}$'),
    -- Bornes du calibrage automatique (§5.2.1) : le serveur ne choisit que 3 ou 4.
    CONSTRAINT blocks_difficulty     CHECK (difficulty BETWEEN 3 AND 4),
    -- Seul le bloc genesis n'a pas de prédécesseur.
    CONSTRAINT blocks_genesis        CHECK ((block_index = 0) = (previous_hash IS NULL)),
    CONSTRAINT blocks_nonce          CHECK (nonce >= 0),
    -- La charge utile canonique est bornée : une trame fait au plus 64 Kio (ENF-23).
    CONSTRAINT blocks_payload_canon  CHECK (length(payload_canon) BETWEEN 2 AND 65536)
);

COMMENT ON TABLE  blocks IS 'Chaîne de blocs : source de vérité, append-only. La difficulté est incluse dans le hash (ENF-52).';
COMMENT ON COLUMN blocks.difficulty IS 'Difficulté appliquée lors du minage : l''altérer rend le bloc invalide (REC-16).';

-- Vérification de cohérence : on veut pouvoir parcourir la chaîne en ordre
-- sans tri, et retrouver un bloc par son hash lors de la détection d'altération.
CREATE INDEX IF NOT EXISTS idx_blocks_ts        ON blocks (ts DESC);
CREATE INDEX IF NOT EXISTS idx_blocks_action    ON blocks (action_type);

-- ----------------------------------------------------------------------------
-- 2. PROJECTIONS — utilisateurs
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS users (
    user_id          SERIAL       PRIMARY KEY,
    pseudo           VARCHAR(32)  NOT NULL,
    created_at       TIMESTAMPTZ  NOT NULL DEFAULT now(),
    last_seen_at     TIMESTAMPTZ,
    last_block_index BIGINT       REFERENCES blocks(block_index),

    CONSTRAINT users_pseudo_format CHECK (pseudo ~ '^[A-Za-z0-9_-]{3,32}$')
);
CREATE UNIQUE INDEX IF NOT EXISTS uq_users_pseudo ON users (lower(pseudo));

-- ----------------------------------------------------------------------------
-- 3. PROJECTIONS — cartes
--    Les 20 champs du §4.2.1 ; `genres` et `themes` sont normalisés (1re forme
--    normale) car un SGBD relationnel ne stocke pas de liste dans une colonne.
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS cards (
    card_id          SERIAL       PRIMARY KEY,
    title_romaji     VARCHAR(200) NOT NULL,
    title_english    VARCHAR(200),
    title_native     VARCHAR(200),
    format           VARCHAR(8)   NOT NULL,   -- TV, MOVIE, OVA, ONA, SPECIAL
    studio           VARCHAR(100),            -- facultatif (Q1.2 : 11 champs obligatoires seulement)
    year             SMALLINT     NOT NULL,
    season           VARCHAR(10),             -- HIVER, PRINTEMPS, ETE, AUTOMNE
    episodes         SMALLINT,                -- facultatif ; -1 si inconnu / en cours
    duration_min     SMALLINT,
    source           VARCHAR(12),             -- MANGA, LIGHT_NOVEL, ORIGINAL, JEU, AUTRE
    franchise        VARCHAR(100),            -- champ descriptif libre : PLUS un critère de proximité
                                              -- (Q2.1 : le titre sans casse ou le format, §4.5.1)
    synopsis         VARCHAR(500),
    rating           NUMERIC(3,1),            -- note déclarative sur 10
    creator_id       INT          NOT NULL REFERENCES users(user_id),
    owner_id         INT          NOT NULL REFERENCES users(user_id),
    status           VARCHAR(10)  NOT NULL DEFAULT 'ACTIVE',
    wins             SMALLINT     NOT NULL DEFAULT 0,   -- victoires validées : rang de rareté (Q3.1)
    rarity_rank      VARCHAR(8)   NOT NULL DEFAULT 'FER',
    value_active     NUMERIC(8,2) NOT NULL DEFAULT 0,
    created_at       TIMESTAMPTZ  NOT NULL DEFAULT now(),
    last_block_index BIGINT       REFERENCES blocks(block_index),

    CONSTRAINT cards_format   CHECK (format IN ('TV','MOVIE','OVA','ONA','SPECIAL')),
    CONSTRAINT cards_season   CHECK (season IS NULL OR season IN ('HIVER','PRINTEMPS','ETE','AUTOMNE')),
    CONSTRAINT cards_source   CHECK (source IS NULL OR source IN ('MANGA','LIGHT_NOVEL','ORIGINAL','JEU','AUTRE')),
    CONSTRAINT cards_status   CHECK (status IN ('ACTIVE','EN_BATTLE','INACTIVE','ARCHIVEE')),
    CONSTRAINT cards_episodes CHECK (episodes >= -1),
    CONSTRAINT cards_duration CHECK (duration_min IS NULL OR duration_min > 0),
    CONSTRAINT cards_rating   CHECK (rating IS NULL OR rating BETWEEN 0 AND 10),
    -- Bornes du §4.2.2 : année comprise entre 1950 et l'année courante + 1.
    -- NB : `now()` est une fonction STABLE, pas IMMUTABLE. PostgreSQL l'accepte
    -- dans un CHECK (évalué à chaque insertion), mais la borne « année courante »
    -- avance avec le temps : une carte acceptée aujourd'hui reste valide, elle
    -- ne sera simplement plus acceptable à la même date l'an prochain. Le serveur
    -- applique de toute façon la même règle (ENF-44), cette contrainte est une
    -- seconde ligne de défense, pas la seule.
    CONSTRAINT cards_year     CHECK (year BETWEEN 1950 AND (date_part('year', now())::int + 1)),
    CONSTRAINT cards_synopsis CHECK (synopsis IS NULL OR length(synopsis) <= 500),
    CONSTRAINT cards_wins     CHECK (wins >= 0),
    -- Rangs de rareté et multiplicateurs (réponse Q3.1) :
    --   FER 0× (0–10 victoires) · BRONZE 1,2× (11–30) · ARGENT 1,5× (31–60)
    --   OR 1,7× (61–100) · PLATINE 2× (> 100)
    CONSTRAINT cards_rank     CHECK (rarity_rank IN ('FER','BRONZE','ARGENT','OR','PLATINE')),
    CONSTRAINT cards_value    CHECK (value_active >= 0)
);

COMMENT ON COLUMN cards.value_active IS 'Valeur recalculée par le serveur : points de recommandation × multiplicateur de rareté (rang lu dans rarity_rank). Jamais fournie par un client (ENF-45). Deux compteurs affichés côté client (Q3.1).';
COMMENT ON COLUMN cards.wins         IS 'Compteur de victoires validées : détermine le rang de rareté et l''éligibilité aux battles (rangs voisins, Q2.2). Incrémenté quand la carte gagne une battle ; la perdante est ARCHIVEE.';
COMMENT ON COLUMN cards.rarity_rank  IS 'Rang dérivé des victoires (FER → PLATINE). Colonne dénormalisée pour l''affichage et le matchmaking ; la valeur de référence est recalculée depuis wins.';
COMMENT ON COLUMN cards.creator_id   IS 'Immuable (jamais mise à jour).';
COMMENT ON COLUMN cards.owner_id     IS 'Mutable par revendication ou par héritage après battle.';

-- Un utilisateur ne peut pas dépasser 100 cartes ACTIVE (§4.2.2, réponse Q1.4).
-- Le serveur refuse au 101e (`1001 CARD_LIMIT_REACHED`) et propose de désactiver
-- une carte ; l'index rend le contrôle instantané. Une carte quitte l'état ACTIVE
-- par désactivation volontaire (INACTIVE) ou par défaite (ARCHIVEE, définitive).
CREATE INDEX IF NOT EXISTS idx_cards_owner_active ON cards (owner_id) WHERE status = 'ACTIVE';
CREATE INDEX IF NOT EXISTS idx_cards_franchise    ON cards (lower(franchise));
CREATE INDEX IF NOT EXISTS idx_cards_studio       ON cards (lower(studio));
CREATE INDEX IF NOT EXISTS idx_cards_year         ON cards (year);
CREATE INDEX IF NOT EXISTS idx_cards_title        ON cards (lower(title_romaji));

-- Genres et thèmes : listes fermées, une ligne par valeur (1re forme normale).
CREATE TABLE IF NOT EXISTS card_genres (
    card_id INT         NOT NULL REFERENCES cards(card_id) ON DELETE CASCADE,
    genre   VARCHAR(24) NOT NULL,
    PRIMARY KEY (card_id, genre),
    CONSTRAINT card_genres_liste CHECK (genre IN (
        'action','aventure','comedie','drame','fantasy','horreur','mystere',
        'romance','science-fiction','slice of life','sport','thriller'))
);

CREATE TABLE IF NOT EXISTS card_themes (
    card_id INT         NOT NULL REFERENCES cards(card_id) ON DELETE CASCADE,
    theme   VARCHAR(24) NOT NULL,
    PRIMARY KEY (card_id, theme),
    CONSTRAINT card_themes_liste CHECK (theme IN (
        'isekai','mecha','school','sports','supernatural','military',
        'music','historical','psychological'))
);

-- ----------------------------------------------------------------------------
-- 4. PROJECTIONS — recommandations (§4.3, §4.6)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS recommendations (
    recommendation_id SERIAL       PRIMARY KEY,
    card_id           INT          NOT NULL REFERENCES cards(card_id),
    user_id           INT          NOT NULL REFERENCES users(user_id),
    weight            NUMERIC(3,1) NOT NULL,   -- 1.0 par un tiers, 0.5 par le créateur/propriétaire
    state             VARCHAR(10)  NOT NULL DEFAULT 'ACTIVE',
    block_index       BIGINT       NOT NULL REFERENCES blocks(block_index),
    created_at        TIMESTAMPTZ  NOT NULL DEFAULT now(),
    repudiated_at     TIMESTAMPTZ,

    CONSTRAINT reco_weight CHECK (weight IN (0.5, 1.0)),
    CONSTRAINT reco_state  CHECK (state IN ('ACTIVE','REPUDIEE')),
    CONSTRAINT reco_repud  CHECK ((state = 'REPUDIEE') = (repudiated_at IS NOT NULL))
);

-- Une seule recommandation ACTIVE par (carte, utilisateur) ; l'historique des
-- répudiations reste (index partiel plutôt qu'une contrainte d'unicité totale).
CREATE UNIQUE INDEX IF NOT EXISTS uq_reco_active
    ON recommendations (card_id, user_id) WHERE state = 'ACTIVE';

CREATE INDEX IF NOT EXISTS idx_reco_user ON recommendations (user_id, state);

-- ----------------------------------------------------------------------------
-- 5. PROJECTIONS — battles et votes (§4.5)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS battles (
    battle_id       SERIAL      PRIMARY KEY,
    card_a_id       INT         NOT NULL REFERENCES cards(card_id),
    card_b_id       INT         NOT NULL REFERENCES cards(card_id),
    opened_by       INT         NOT NULL REFERENCES users(user_id),
    state           VARCHAR(10) NOT NULL DEFAULT 'OUVERTE',
    opened_at       TIMESTAMPTZ NOT NULL DEFAULT now(),
    closes_at       TIMESTAMPTZ NOT NULL,
    winner_card_id  INT         REFERENCES cards(card_id),
    outcome         VARCHAR(16),
    last_block_index BIGINT     REFERENCES blocks(block_index),

    CONSTRAINT battle_state   CHECK (state IN ('OUVERTE','CLOSE')),
    -- Trois dénouements (Q2.5 et Q6.3 cas 17) : une gagnante, ou une battle
    -- ANNULÉE faute de quorum — ou faute de tout vote — et les cartes sont rendues.
    CONSTRAINT battle_outcome CHECK (outcome IS NULL OR outcome IN ('GAGNANTE_A','GAGNANTE_B','ANNULEE_QUORUM','ANNULEE_0_VOTE')),
    CONSTRAINT battle_deux    CHECK (card_a_id <> card_b_id),
    -- Durée du vote : 300 s / 5 min (§4.5.3, réponse Q2.3).
    CONSTRAINT battle_duree   CHECK (closes_at > opened_at)
);

-- Anti-abus : au plus 3 battles ouvertes par utilisateur et par tranche de 10 min.
CREATE INDEX IF NOT EXISTS idx_battles_open_by ON battles (opened_by, opened_at DESC);
CREATE INDEX IF NOT EXISTS idx_battles_state   ON battles (state) WHERE state = 'OUVERTE';

CREATE TABLE IF NOT EXISTS battle_votes (
    battle_id   INT         NOT NULL REFERENCES battles(battle_id) ON DELETE CASCADE,
    user_id     INT         NOT NULL REFERENCES users(user_id),
    card_id     INT         NOT NULL REFERENCES cards(card_id),
    block_index BIGINT      NOT NULL REFERENCES blocks(block_index),
    cast_at     TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (battle_id, user_id)      -- un seul vote par votant, DÉFINITIF (réponse Q2.4)
    -- Aucune mise à jour de cette table : un second vote est refusé côté serveur
    -- (1020 VOTE_ALREADY_CAST), il ne remplace pas la ligne. Une répudiation de
    -- vote n'existe pas : le vote déposé est un fait inscrit en blockchain.
);

-- ----------------------------------------------------------------------------
-- 6. PROJECTIONS — légitimation (§4.7.1)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS legitimations (
    legitimation_id SERIAL       PRIMARY KEY,
    card_id         INT          NOT NULL REFERENCES cards(card_id),
    user_id         INT          NOT NULL REFERENCES users(user_id),
    justification   VARCHAR(200) NOT NULL,
    state           VARCHAR(12)  NOT NULL DEFAULT 'EN_COURS',
    likes           SMALLINT     NOT NULL DEFAULT 0,   -- décompte des likes (Q4.1) ; détail dans legitimation_likes
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT now(),
    closes_at       TIMESTAMPTZ,               -- ⚠️ plus de scrutin de 60 s : NULL tant que la candidature est en tête
    closed_at       TIMESTAMPTZ,
    block_index     BIGINT       REFERENCES blocks(block_index),

    CONSTRAINT legit_state   CHECK (state IN ('EN_COURS','ACCEPTEE','REJETEE','EXPIREE')),
    CONSTRAINT legit_justif   CHECK (length(justification) BETWEEN 1 AND 200),
    -- Le mécanisme de scrutin (approbations / refus, clôture à 60 s) est REMPLACÉ
    -- par la comparaison continue du nombre de likes (réponse Q4.1) : une
    -- candidature reste EN_COURS jusqu'à ce qu'une autre la dépasse. Les états
    -- REJETEE et EXPIREE ne sont plus produits par le serveur (conservés pour
    -- ne pas invalider d'éventuelles lignes historiques).
    CONSTRAINT legit_likes    CHECK (likes >= 0)
);

-- Un seul porteur légitime actif à un instant donné (§4.7.1, point 4).
CREATE UNIQUE INDEX IF NOT EXISTS uq_legit_actif
    ON legitimations (card_id) WHERE state = 'ACCEPTEE';

-- ----------------------------------------------------------------------------
-- 7. PROJECTIONS — revendication (§4.7.2, DEC-10 et DEC-11)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS claims (
    claim_id          SERIAL       PRIMARY KEY,
    card_id           INT          NOT NULL REFERENCES cards(card_id),
    claimant_id       INT          NOT NULL REFERENCES users(user_id),
    owner_id          INT          NOT NULL REFERENCES users(user_id),
    justification     VARCHAR(200) NOT NULL,
    state             VARCHAR(12)  NOT NULL DEFAULT 'PROPOSEE',
    decision_deadline TIMESTAMPTZ  NOT NULL,   -- created_at + 120 s (fenêtre AFFICHÉE en démo)
    auto_transfer_at  TIMESTAMPTZ  NOT NULL,   -- created_at + 30 jours (la VRAIE règle : silence = accord)
    created_at        TIMESTAMPTZ  NOT NULL DEFAULT now(),
    decided_at        TIMESTAMPTZ,
    block_index       BIGINT       REFERENCES blocks(block_index),

    CONSTRAINT claim_state      CHECK (state IN ('PROPOSEE','ACCEPTEE','REFUSEE','TRANSFEREE')),
    CONSTRAINT claim_justif     CHECK (length(justification) BETWEEN 1 AND 200),
    CONSTRAINT claim_ordre      CHECK (auto_transfer_at > decision_deadline AND decision_deadline > created_at),
    CONSTRAINT claim_pas_soi    CHECK (claimant_id <> owner_id),
    -- Un refus explicite est définitif : il n'existe aucun état « contestée » (DEC-11).
    -- (v1.4) L'état `SANS_OBJET` introduit en v1.3 est RETIRÉ : la réponse de l'équipe
    -- (Q2.6) dit que les droits de la carte perdante restent INCHANGÉS après une battle
    -- — une revendication en attente n'a donc pas à être close par la défaite (§4.5.4).
    CONSTRAINT claim_clos       CHECK ((state = 'PROPOSEE') = (decided_at IS NULL))
);

-- Une seule revendication en attente par carte (§3.2 : pas de mécanisme de litige).
CREATE UNIQUE INDEX IF NOT EXISTS uq_claim_pending
    ON claims (card_id) WHERE state = 'PROPOSEE';

-- Index « balayage des échéances » : c'est la requête que le serveur exécute
-- périodiquement pour appliquer la règle du silence (30 jours).
CREATE INDEX IF NOT EXISTS idx_claims_echeance
    ON claims (auto_transfer_at) WHERE state = 'PROPOSEE';

-- ----------------------------------------------------------------------------
-- 8. PROJECTIONS — authentification d'une carte (§4.7.3, DEC-24)
--    Rétablie par le sujet officiel 2026-2027 : un utilisateur LÉGITIMÉ certifie
--    que les métadonnées de la carte décrivent fidèlement l'œuvre.
--    Aucun effet sur la valeur : c'est un label de confiance (DEC-24).
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS authentications (
    authentication_id SERIAL       PRIMARY KEY,
    card_id           INT          NOT NULL REFERENCES cards(card_id),
    user_id           INT          NOT NULL REFERENCES users(user_id),  -- porteur légitime
    legitimation_id   INT          NOT NULL REFERENCES legitimations(legitimation_id),
    justification     VARCHAR(200),
    state             VARCHAR(10)  NOT NULL DEFAULT 'ACTIVE',
    created_at        TIMESTAMPTZ  NOT NULL DEFAULT now(),
    revoked_at        TIMESTAMPTZ,
    block_index       BIGINT       REFERENCES blocks(block_index),          -- bloc de l'authentification
    revoked_block_index BIGINT     REFERENCES blocks(block_index),          -- bloc de la révocation (v1.3)

    CONSTRAINT auth_state    CHECK (state IN ('ACTIVE','REVOQUEE')),
    CONSTRAINT auth_justif   CHECK (justification IS NULL OR length(justification) BETWEEN 1 AND 200),
    -- Une révocation a toujours une date ET un bloc ; une authentification active n'a ni l'une ni l'autre.
    -- La révocation est une ACTION VALIDÉE (elle fait un bloc) : les deux index sont donc conservés,
    -- ce qui rend l'historique complet sans jamais effacer l'inscription d'origine (§4.7.3).
    CONSTRAINT auth_revoked  CHECK ((state = 'REVOQUEE') = (revoked_at IS NOT NULL)
                                AND (state = 'REVOQUEE') = (revoked_block_index IS NOT NULL))
);

-- Une seule authentification ACTIVE par carte (une nouvelle remplace l'ancienne,
-- qui reste dans l'historique — aucune écriture n'est effacée).
CREATE UNIQUE INDEX IF NOT EXISTS uq_auth_active
    ON authentications (card_id) WHERE state = 'ACTIVE';

COMMIT;


-- ----------------------------------------------------------------------------
-- 9. PROJECTIONS — likes de légitimation (§4.7.1, réponse Q4.1)
--    Le porteur légitime est le candidat qui totalise le PLUS de likes ;
--    il perd le titre dès qu'un autre le dépasse (mécanisme continu, plus de
--    scrutin de 60 s : `1013 LEGITIMATION_CLOSED` n'est plus émis).
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS legitimation_likes (
    legitimation_id INT         NOT NULL REFERENCES legitimations(legitimation_id),
    user_id         INT         NOT NULL REFERENCES users(user_id),
    liked_at        TIMESTAMPTZ NOT NULL DEFAULT now(),
    block_index     BIGINT      NOT NULL REFERENCES blocks(block_index),
    PRIMARY KEY (legitimation_id, user_id)   -- un seul like par utilisateur et par candidature
);

CREATE INDEX IF NOT EXISTS idx_legit_likes ON legitimation_likes (legitimation_id);

-- ----------------------------------------------------------------------------
-- 10. PROJECTIONS — liste d'amis (§4.8, réponse Q5.2)
--     Le partage ciblé n'est ouvert qu'aux amis. La ligne est écrite une fois,
--     l'amitié est symétrique (deux INSERT, ou une vue symétrique).
--     ⚠️ L'amitié est un RÉGLAGE d'utilisateur, pas une action métier : elle ne
--     déplace aucune carte et n'est donc PAS inscrite en blockchain (contrairement
--     au partage lui-même, table 11, qui l'est depuis le 22/09/2026).
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS friendships (
    user_id    INT         NOT NULL REFERENCES users(user_id),
    friend_id  INT         NOT NULL REFERENCES users(user_id),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    block_index BIGINT     REFERENCES blocks(block_index),   -- facultatif : l'amitié n'est pas une action métier
    PRIMARY KEY (user_id, friend_id),
    CONSTRAINT ami_pas_soi CHECK (user_id <> friend_id)
);

-- ----------------------------------------------------------------------------
-- 11. PROJECTIONS — partages ciblés (§4.8, réponse Q5.2)
--     ✅ Décision du 22/09/2026 (option A) : le partage EST inscrit en blockchain,
--     comme toute action métier — `share.send` et `share.respond` produisent
--     chacune un bloc. La chaîne reste donc la source de vérité, sans exception.
--     (La LISTE D'AMIS, elle, reste un réglage d'utilisateur : voir table 10.)
-- ----------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS shares (
    share_id     SERIAL       PRIMARY KEY,
    card_id      INT          NOT NULL REFERENCES cards(card_id),
    from_user_id INT          NOT NULL REFERENCES users(user_id),
    to_user_id   INT          NOT NULL REFERENCES users(user_id),
    state        VARCHAR(10)  NOT NULL DEFAULT 'ENVOYE',
    created_at   TIMESTAMPTZ  NOT NULL DEFAULT now(),
    decided_at   TIMESTAMPTZ,
    block_index  BIGINT       REFERENCES blocks(block_index),  -- bloc de l'envoi (puis de la décision)

    CONSTRAINT share_state  CHECK (state IN ('ENVOYE','ACCEPTE','REFUSE')),
    CONSTRAINT share_clos   CHECK ((state = 'ENVOYE') = (decided_at IS NULL)),
    -- Envoi et acceptation sont des actions métier : chacune a son bloc.
    CONSTRAINT share_bloc   CHECK (block_index IS NOT NULL),
    CONSTRAINT share_pas_soi CHECK (from_user_id <> to_user_id)
);

CREATE UNIQUE INDEX IF NOT EXISTS uq_share_pending
    ON shares (card_id, to_user_id) WHERE state = 'ENVOYE';   -- une seule offre en attente par carte et par destinataire

-- ============================================================================
--  ANNEXE — requêtes de service (non exécutées par ce script)
-- ============================================================================
--
-- A. RECONSTITUTION DU CONTEXTE D'UN CLIENT (CU-7, au moment de session.welcome)
--    Objectif : ≤ 10 s pour 1 000 blocs (REC-02). On lit les projections
--    (indexées) plutôt que de rejouer la chaîne bloc par bloc.
--
--    SELECT c.*, g.genre
--      FROM cards c
--      LEFT JOIN card_genres g USING (card_id)
--     WHERE c.owner_id = $1 OR c.creator_id = $1;
--
--    SELECT r.* FROM recommendations r WHERE r.user_id = $1;
--    SELECT v.*, b.* FROM battle_votes v JOIN battles b USING (battle_id) WHERE v.user_id = $1;
--
-- B. VÉRIFICATION DE COHÉRENCE (CU-26, EF-26)
--    Parcours complet de `blocks` en ordre croissant, recalcul du hash de
--    chaque bloc (avec son champ `difficulty`, ENF-52) et comparaison au
--    `previous_hash` du suivant. Toute rupture ⇒ altération détectée (REC-07),
--    y compris la falsification de la difficulté (REC-16).
--
--    SELECT block_index, hash, previous_hash, payload_canon, nonce, difficulty
--      FROM blocks ORDER BY block_index;
--    (On recalcule l'en-tête de 96 octets à partir de ces colonnes, puis son
--     SHA-256, et on compare au hash stocké — voir §6.5 de la spec technique.)
--
-- C. CONTRÔLE D'INTÉGRITÉ (facultatif, phase 3) — détecter une projection
--    qui ne correspondrait plus à la chaîne :
--    comparer, pour chaque bloc, l'action du `payload` à la ligne de
--    projection portant le même `block_index`.
--
-- C bis. AUTHENTIFICATIONS D'UN CLIENT (CU-20, contexte du §4.9)
--    SELECT a.* FROM authentications a
--     WHERE a.user_id = $1
--        OR a.card_id IN (SELECT card_id FROM cards WHERE owner_id = $1);
--
-- D. NETTOYAGE POUR LES TESTS (jamais exécuté sur la base de démonstration)
--    TRUNCATE shares, friendships, legitimation_likes, authentications, claims,
--             legitimations, battle_votes, battles, recommendations,
--             card_themes, card_genres, cards, users, blocks
--             RESTART IDENTITY CASCADE;
-- ============================================================================
