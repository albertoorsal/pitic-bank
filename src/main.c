/**
 * @file main.c
 * @author Alberto Ornelas
 * @brief Pitic Bank — A Terminal Banking System Written in C
 * @date 19-July-2026
 */
#include "database.h"
#include "cli/cli.h"
#include <libpq-fe.h>
#include <stdio.h>

#include "user/user_service.h"
#include "user/pg_user_repository.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Resolve the libpq connection string: CLI argument wins, else the
 * DATABASE_URL environment variable, else the same local default used by
 * database.c's getConnection(). */
static const char *resolve_conn_info(int argc, char **argv)
{
    if (argc > 1) {
        return argv[1];
    }

    const char *from_env = getenv("DATABASE_URL");
    if (from_env != NULL) {
        return from_env;
    }

    return DEFAULT_CONN_INFO;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "seed-admin") == 0) {
        return seed_admin();
    }

    const char *conn_info = resolve_conn_info(argc, argv);

    UserRepository repository;
    if (pg_user_repository_create(conn_info, &repository) != REPO_OK) {
        fprintf(stderr, "Could not open the user store. Check DATABASE_URL.\n");
        return EXIT_FAILURE;
    }

    UserService service;
    user_service_init(&service, &repository);

    int exit_code = cli_run(&service);

    repo_destroy(&repository);
    return exit_code;
}
