-- Pitic Bank schema.
-- Run once against the target database:
--   psql "host=localhost port=5432 dbname=postgres user=nailich" -f src/sql/schema.sql

-- Needed for crypt()/gen_salt(), used to hash and verify passwords server-side.
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS users (
    id            BIGSERIAL PRIMARY KEY,
    name          VARCHAR(128) NOT NULL,
    email         VARCHAR(256) NOT NULL UNIQUE,
    password_hash VARCHAR(128) NOT NULL,
    role          VARCHAR(32)  NOT NULL DEFAULT 'customer',
    created_at    TIMESTAMPTZ  NOT NULL DEFAULT now()
);
