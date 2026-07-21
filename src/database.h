#ifndef DATABASE_H
#define DATABASE_H
#include <libpq-fe.h>

/* Local default libpq connection string, shared by the app and the seeder.
 * Override with the DATABASE_URL environment variable or a CLI argument. */
#define DEFAULT_CONN_INFO "host=localhost port=5432 dbname=postgres user=nailich"

// Get Database Connection
PGconn *getConnection();

/* Ensure the schema exists and an admin user is present (idempotent).
 * Returns an EXIT_SUCCESS/EXIT_FAILURE-style process exit code. */
int seed_admin(void);

#endif
