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
#include "accounts/account_service.h"
#include "accounts/pg_account_repository.h"
#include "transfers/transfer_service.h"
#include "transfers/pg_transfer_repository.h"
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


    // Init all repositories
    UserRepository user_repository;
    if (pg_user_repository_create(conn_info, &user_repository) != REPO_OK) {
        fprintf(stderr, "Could not open the user store. Check DATABASE_URL.\n");
        return EXIT_FAILURE;
    }

    AccountRepository account_repository;
    if (pg_account_repository_create(conn_info, &account_repository) != REPO_OK) {
        fprintf(stderr, "Could not open the account store. Check DATABASE_URL.\n");
        repo_destroy(&user_repository);
        return EXIT_FAILURE;
    }

    TransferRepository transfer_repository;
    if (pg_transfer_repository_create(conn_info, &transfer_repository) != REPO_OK) {
        fprintf(stderr, "Could not open the transfer store. Check DATABASE_URL.\n");
        repo_destroy(&user_repository);
        account_repo_destroy(&account_repository);
        return EXIT_FAILURE;
    }


    // Init Services
    UserService user_service;
    user_service_init(&user_service, &user_repository);

    AccountService account_service;
    account_service_init(&account_service, &account_repository);

    TransferService transfer_service;
    transfer_service_init(&transfer_service, &transfer_repository);

    AppContext app = {
        .users = &user_service,
        .accounts = &account_service,
        .transfers = &transfer_service
    };

    int exit_code = cli_run(&app);

    repo_destroy(&user_repository);
    account_repo_destroy(&account_repository);
    transfer_repo_destroy(&transfer_repository);
    return exit_code;
}
