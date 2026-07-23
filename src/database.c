#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Admin seed defaults; override at seed time with ADMIN_EMAIL / ADMIN_PASSWORD. */
#define DEFAULT_ADMIN_NAME     "Admin"
#define DEFAULT_ADMIN_EMAIL    "admin@piticbank.com"
#define DEFAULT_ADMIN_PASSWORD "Admin123!"

static const char *resolve_conn_info(void)
{
    const char *from_env = getenv("DATABASE_URL");
    return (from_env != NULL) ? from_env : DEFAULT_CONN_INFO;
}

PGconn *getConnection()
{
    PGconn *conn = PQconnectdb(resolve_conn_info());

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    printf("Successfully connected to PostgreSQL!\n");

    return conn;
}

/* Run one statement with no result rows expected (DDL). Exits the process
 * on failure since a broken schema means nothing downstream can work. */
static void exec_or_die(PGconn *conn, const char *sql)
{
    PGresult *result = PQexec(conn, sql);
    ExecStatusType status = PQresultStatus(result);

    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        fprintf(stderr, "[seed] statement failed: %s\n", PQerrorMessage(conn));
        fprintf(stderr, "[seed] statement was: %s\n", sql);
        PQclear(result);
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }
    PQclear(result);
}

static void ensure_schema(PGconn *conn)
{
    exec_or_die(conn, "CREATE EXTENSION IF NOT EXISTS pgcrypto;");
    exec_or_die(conn,
        "CREATE TABLE IF NOT EXISTS users ("
        "    id            BIGSERIAL PRIMARY KEY,"
        "    name          VARCHAR(128) NOT NULL,"
        "    email         VARCHAR(256) NOT NULL UNIQUE,"
        "    password_hash VARCHAR(128) NOT NULL,"
        "    role          VARCHAR(32)  NOT NULL DEFAULT 'customer',"
        "    rfc           VARCHAR(13),"
        "    created_at    TIMESTAMPTZ  NOT NULL DEFAULT now()"
        ");"
    );
    exec_or_die(conn,
        "CREATE TABLE IF NOT EXISTS accounts ("
        "    id              BIGSERIAL PRIMARY KEY,"
        "    user_id         BIGINT NOT NULL REFERENCES users(id),"
        "    account_number  VARCHAR(16) NOT NULL UNIQUE,"
        "    account_type    VARCHAR(16) NOT NULL DEFAULT 'checking'"
        "                    CHECK (account_type IN ('checking', 'savings')),"
        "    balance_cents   BIGINT NOT NULL DEFAULT 0"
        "                    CHECK (balance_cents >= 0),"
        "    status          VARCHAR(16) NOT NULL DEFAULT 'active'"
        "                    CHECK (status IN ('active', 'frozen', 'closed')),"
        "    created_at      TIMESTAMPTZ NOT NULL DEFAULT now()"
        ");"
    );
    exec_or_die(conn, "CREATE INDEX IF NOT EXISTS accounts_user_id_idx ON accounts(user_id);");
    exec_or_die(conn,
        "CREATE TABLE IF NOT EXISTS transfers ("
        "    id                  BIGSERIAL PRIMARY KEY,"
        "    from_account_id     BIGINT NOT NULL REFERENCES accounts(id),"
        "    to_account_id       BIGINT NOT NULL REFERENCES accounts(id),"
        "    amount_cents        BIGINT NOT NULL CHECK (amount_cents > 0),"
        "    status              VARCHAR(16) NOT NULL DEFAULT 'completed'"
        "                        CHECK (status IN ('completed', 'failed')),"
        "    created_at          TIMESTAMPTZ NOT NULL DEFAULT now(),"
        "    CHECK (from_account_id <> to_account_id)"
        ");"
    );
    exec_or_die(conn, "CREATE INDEX IF NOT EXISTS transfers_from_account_idx ON transfers(from_account_id);");
    exec_or_die(conn, "CREATE INDEX IF NOT EXISTS transfers_to_account_idx ON transfers(to_account_id);");
}

/* Insert the admin user if absent, or promote+reset an existing row with the
 * same email to admin. Idempotent: safe to run on every deploy. */
static int upsert_admin(PGconn *conn, const char *name, const char *email, const char *password)
{
    const char *sql =
        "INSERT INTO users (name, email, password_hash, role) "
        "VALUES ($1, $2, crypt($3, gen_salt('bf')), 'admin') "
        "ON CONFLICT (email) DO UPDATE "
        "   SET password_hash = crypt($3, gen_salt('bf')), "
        "       role = 'admin' "
        "RETURNING id;";
    const char *params[] = { name, email, password };

    PGresult *result = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[seed] could not upsert admin: %s\n", PQerrorMessage(conn));
        PQclear(result);
        return -1;
    }

    printf("Admin user ready: %s (id=%s)\n", email, PQgetvalue(result, 0, 0));
    PQclear(result);
    return 0;
}

int seed_admin(void)
{
    PGconn *conn = getConnection();

    ensure_schema(conn);

    const char *name = DEFAULT_ADMIN_NAME;
    const char *email = getenv("ADMIN_EMAIL");
    const char *password = getenv("ADMIN_PASSWORD");
    if (email == NULL) email = DEFAULT_ADMIN_EMAIL;
    if (password == NULL) password = DEFAULT_ADMIN_PASSWORD;

    int status = upsert_admin(conn, name, email, password);

    PQfinish(conn);
    return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
