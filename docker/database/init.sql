CREATE EXTENSION IF NOT EXISTS pg_trgm;
CREATE EXTENSION IF NOT EXISTS pg_prewarm;

CREATE UNLOGGED TABLE IF NOT EXISTS people (
    id uuid PRIMARY KEY DEFAULT uuidv7(),
    nickname VARCHAR(32) UNIQUE NOT NULL,
    "name" VARCHAR(100) NOT NULL,
    birth_date DATE NOT NULL,
    stack VARCHAR(32)[] DEFAULT '{}' NOT NULL,
    searchable text
);

CREATE INDEX CONCURRENTLY IF NOT EXISTS people_search_idx ON people USING GIN(searchable gin_trgm_ops);

SELECT pg_prewarm('people_search_idx');
