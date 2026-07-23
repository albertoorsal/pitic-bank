/* account_service.c - Business rules and orchestration for accounts. */
#include "account_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

enum { ACCOUNT_NUMBER_DIGITS = 12, ACCOUNT_OPEN_MAX_ATTEMPTS = 5 };

static int seeded = 0;

/* Fill `out` (must hold >= ACCOUNT_NUMBER_DIGITS + 1 bytes) with a random
 * numeric account number. Collisions are handled by retrying at the caller. */
static void generate_account_number(char *out, size_t size)
{
    if (!seeded) {
        srand((unsigned)time(NULL) ^ (unsigned)getpid());
        seeded = 1;
    }

    size_t digits = size - 1 < ACCOUNT_NUMBER_DIGITS ? size - 1 : ACCOUNT_NUMBER_DIGITS;
    for (size_t i = 0; i < digits; i++) {
        out[i] = (char)('0' + (rand() % 10));
    }
    out[digits] = '\0';
}

static AccountServiceStatus from_repo_status(RepoStatus status)
{
    switch (status) {
        case REPO_OK:        return ACCOUNT_SERVICE_OK;
        case REPO_NOT_FOUND: return ACCOUNT_SERVICE_NOT_FOUND;
        case REPO_CONFLICT:  return ACCOUNT_SERVICE_INSUFFICIENT_FUNDS;
        case REPO_ERROR:     return ACCOUNT_SERVICE_ERROR;
    }
    return ACCOUNT_SERVICE_ERROR;
}

void account_service_init(AccountService *service, const AccountRepository *repository)
{
    service->repository = repository;
}

AccountServiceStatus account_service_open(const AccountService *service, long user_id, Account *created)
{
    Account input = { .id = 0, .user_id = user_id, .balance_cents = 0 };
    snprintf(input.account_type, sizeof(input.account_type), "%s", "checking");

    for (int attempt = 0; attempt < ACCOUNT_OPEN_MAX_ATTEMPTS; attempt++) {
        generate_account_number(input.account_number, sizeof(input.account_number));

        RepoStatus status = account_repo_create(service->repository, &input, created);
        if (status == REPO_CONFLICT) {
            continue; /* account_number collision: try another number */
        }
        return from_repo_status(status);
    }

    return ACCOUNT_SERVICE_ERROR;
}

AccountServiceStatus account_service_get(const AccountService *service, long id, Account *out)
{
    return from_repo_status(account_repo_find_by_id(service->repository, id, out));
}

AccountServiceStatus account_service_list_for_user(
    const AccountService *service, long user_id, AccountList *out)
{
    return from_repo_status(account_repo_find_by_user_id(service->repository, user_id, out));
}

AccountServiceStatus account_service_list_all(const AccountService *service, AccountList *out)
{
    return from_repo_status(account_repo_find_all(service->repository, out));
}

/* Load `account_id` and confirm it is owned by `owner_user_id`. */
static AccountServiceStatus load_owned_account(
    const AccountService *service, long account_id, long owner_user_id, Account *out)
{
    RepoStatus status = account_repo_find_by_id(service->repository, account_id, out);
    if (status != REPO_OK) {
        return from_repo_status(status);
    }
    if (out->user_id != owner_user_id) {
        return ACCOUNT_SERVICE_FORBIDDEN;
    }
    return ACCOUNT_SERVICE_OK;
}

AccountServiceStatus account_service_deposit(
    const AccountService *service, long account_id, long owner_user_id,
    long long amount_cents, Account *out)
{
    if (amount_cents <= 0) {
        return ACCOUNT_SERVICE_INVALID_AMOUNT;
    }

    AccountServiceStatus owned = load_owned_account(service, account_id, owner_user_id, out);
    if (owned != ACCOUNT_SERVICE_OK) {
        return owned;
    }

    return from_repo_status(account_repo_adjust_balance(service->repository, account_id, amount_cents, out));
}

AccountServiceStatus account_service_withdraw(
    const AccountService *service, long account_id, long owner_user_id,
    long long amount_cents, Account *out)
{
    if (amount_cents <= 0) {
        return ACCOUNT_SERVICE_INVALID_AMOUNT;
    }

    AccountServiceStatus owned = load_owned_account(service, account_id, owner_user_id, out);
    if (owned != ACCOUNT_SERVICE_OK) {
        return owned;
    }

    RepoStatus status = account_repo_adjust_balance(service->repository, account_id, -amount_cents, out);
    if (status == REPO_CONFLICT) {
        return ACCOUNT_SERVICE_INSUFFICIENT_FUNDS;
    }
    return from_repo_status(status);
}

AccountServiceStatus account_service_transfer(
    const AccountService *service,
    const char *from_account_number, long owner_user_id,
    const char *to_account_number,
    long long amount_cents, long *transfer_id)
{
    if (amount_cents <= 0) {
        return ACCOUNT_SERVICE_INVALID_AMOUNT;
    }
    if (strcmp(from_account_number, to_account_number) == 0) {
        return ACCOUNT_SERVICE_SAME_ACCOUNT;
    }

    Account from;
    RepoStatus status = account_repo_find_by_account_number(service->repository, from_account_number, &from);
    if (status != REPO_OK) {
        return from_repo_status(status);
    }
    if (from.user_id != owner_user_id) {
        return ACCOUNT_SERVICE_FORBIDDEN;
    }

    Account to;
    status = account_repo_find_by_account_number(service->repository, to_account_number, &to);
    if (status != REPO_OK) {
        return from_repo_status(status);
    }

    status = account_repo_transfer(service->repository, from.id, to.id, amount_cents, transfer_id);
    if (status == REPO_CONFLICT) {
        return ACCOUNT_SERVICE_INSUFFICIENT_FUNDS;
    }
    return from_repo_status(status);
}

const char *account_service_status_text(AccountServiceStatus status)
{
    switch (status) {
        case ACCOUNT_SERVICE_OK:                  return "OK";
        case ACCOUNT_SERVICE_INVALID_AMOUNT:      return "amount must be greater than zero";
        case ACCOUNT_SERVICE_INSUFFICIENT_FUNDS:  return "insufficient funds";
        case ACCOUNT_SERVICE_NOT_FOUND:           return "account not found";
        case ACCOUNT_SERVICE_FORBIDDEN:           return "you do not own that account";
        case ACCOUNT_SERVICE_SAME_ACCOUNT:        return "cannot transfer to the same account";
        case ACCOUNT_SERVICE_ERROR:               return "storage error";
    }
    return "unknown status";
}
