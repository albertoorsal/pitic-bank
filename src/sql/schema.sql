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
    rfc           VARCHAR(13),
    created_at    TIMESTAMPTZ  NOT NULL DEFAULT now()
);

-- Balances are stored as integer cents (BIGINT) to avoid floating-point
-- rounding: 10050 means $100.50.
CREATE TABLE IF NOT EXISTS accounts (
    id              BIGSERIAL PRIMARY KEY,
    user_id         BIGINT NOT NULL REFERENCES users(id),
    account_number  VARCHAR(16) NOT NULL UNIQUE,
    account_type    VARCHAR(16) NOT NULL DEFAULT 'checking'
                    CHECK (account_type IN ('checking', 'savings')),
    balance_cents   BIGINT NOT NULL DEFAULT 0
                    CHECK (balance_cents >= 0),
    status          VARCHAR(16) NOT NULL DEFAULT 'active'
                    CHECK (status IN ('active', 'frozen', 'closed')),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS accounts_user_id_idx ON accounts(user_id);

-- One row per transfer between two accounts. Deposits/withdrawals are not
-- transfers and live only as balance changes on `accounts`; only
-- account-to-account movements are recorded here.
CREATE TABLE IF NOT EXISTS transfers (
    id                  BIGSERIAL PRIMARY KEY,
    from_account_id     BIGINT NOT NULL REFERENCES accounts(id),
    to_account_id       BIGINT NOT NULL REFERENCES accounts(id),
    amount_cents        BIGINT NOT NULL CHECK (amount_cents > 0),
    status              VARCHAR(16) NOT NULL DEFAULT 'completed'
                        CHECK (status IN ('completed', 'failed')),
    created_at          TIMESTAMPTZ NOT NULL DEFAULT now(),

    CHECK (from_account_id <> to_account_id)
);

CREATE INDEX IF NOT EXISTS transfers_from_account_idx ON transfers(from_account_id);
CREATE INDEX IF NOT EXISTS transfers_to_account_idx ON transfers(to_account_id);
