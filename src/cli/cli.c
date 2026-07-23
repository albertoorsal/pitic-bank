#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { INPUT_MAX = 512 };

/* Read one line into `buffer`, stripping the trailing newline.
* Returns 0 on success, -1 on EOF.
*/
static int read_line(const char *prompt, char *buffer, size_t size)
{
    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return -1;
    }

    buffer[strcspn(buffer, "\n")] = '\0';
    return 0;
}

/* Parse a decimal amount like "12.34" into integer cents. Returns 0 on
 * success, -1 if the text is not a valid amount. */
static int parse_amount_cents(const char *text, long long *out)
{
    char *end = NULL;
    double value = strtod(text, &end);
    if (end == text || *end != '\0' || value < 0) {
        return -1;
    }
    *out = (long long)(value * 100.0 + 0.5);
    return 0;
}

static void print_cents(long long cents)
{
    long long whole = cents / 100;
    long long fraction = cents % 100;
    if (fraction < 0) fraction = -fraction;
    printf("$%lld.%02lld", whole, fraction);
}

static void print_user(const User *user)
{
    printf("  #%ld  %-20s  %-30s  %s\n", user->id, user->name, user->email, user->role);
}

static void print_account(const Account *account)
{
    printf("  #%-6ld user=%-6ld %-16s %-10s balance=",
        account->id, account->user_id, account->account_number, account->account_type);
    print_cents(account->balance_cents);
    printf(" [%s]\n", account->status);
}

static void print_transfer(const Transfer *transfer)
{
    printf("  #%-6ld %s -> %s  ", transfer->id, transfer->from_account_number, transfer->to_account_number);
    print_cents(transfer->amount_cents);
    printf("  %s\n", transfer->created_at);
}

/** Menu actions (one function per use case) */
static void action_create(const UserService *service)
{
    char name[INPUT_MAX];
    char email[INPUT_MAX];
    char password[INPUT_MAX];
    char rfc[INPUT_MAX];

    if (read_line("Name    : ", name, sizeof(name)) != 0 ||
        read_line("Email   : ", email, sizeof(email)) != 0 ||
        read_line("Password: ", password, sizeof(password)) != 0 ||
        read_line("RFC: ", rfc, sizeof(rfc))) {
        return;
    }

    User created;
    ServiceStatus status = user_service_register(service, name, email, password, rfc, &created);

    if (status == SERVICE_OK) {
        printf("  + created:\n");
        print_user(&created);
    } else {
        printf("  ! %s\n", service_status_text(status));
    }
}

/* Register a new customer and immediately open their first bank account so
 * they can start banking without a separate "open account" step. */
static void action_register_customer(const AppContext *app)
{
    char name[INPUT_MAX];
    char email[INPUT_MAX];
    char password[INPUT_MAX];
    char rfc[INPUT_MAX];

    if (read_line("Name    : ", name, sizeof(name)) != 0 ||
        read_line("Email   : ", email, sizeof(email)) != 0 ||
        read_line("Password: ", password, sizeof(password)) != 0 ||
        read_line("RFC: ", rfc, sizeof(rfc))) {
        return;
    }

    User created;
    ServiceStatus status = user_service_register(app->users, name, email, password, rfc, &created);
    if (status != SERVICE_OK) {
        printf("  ! %s\n", service_status_text(status));
        return;
    }

    printf("  + welcome, %s\n", created.name);

    Account account;
    AccountServiceStatus account_status = account_service_open(app->accounts, created.id, &account);
    if (account_status == ACCOUNT_SERVICE_OK) {
        printf("  + account opened:\n");
        print_account(&account);
    } else {
        printf("  ! could not open an account: %s\n", account_service_status_text(account_status));
    }
}

/* Forward declarations: role menus dispatch into their own loops below. */
static void run_admin_menu(const AppContext *app);
static void run_customer_menu(const AppContext *app, const User *current_user);

static void action_login(const AppContext *app)
{
    char email[INPUT_MAX];
    char password[INPUT_MAX];

    if (read_line("Email   : ", email, sizeof(email)) != 0 ||
        read_line("Password: ", password, sizeof(password)) != 0) {
        return;
    }

    User authenticated;
    ServiceStatus status = user_service_login(app->users, email, password, &authenticated);

    if (status != SERVICE_OK) {
        printf("  ! %s\n", service_status_text(status));
        return;
    }

    printf("  + welcome back, %s (%s)\n", authenticated.name, authenticated.role);

    if (strcmp(authenticated.role, "admin") == 0) {
        run_admin_menu(app);
    } else {
        run_customer_menu(app, &authenticated);
    }
}


static void print_menu(void) {
    printf("\n"
        "   Hello, Welcome to Pitic Bank Terminal System\n"
        "   --------------------------------------------\n"
        "   1) Login\n"
        "   2) Register\n"
        "   0) Quit\n");
}

static void print_logged_admin_menu(void)
{
    printf("\n"
        "   Hello, Welcome to Pitic Bank Terminal System Admin Panel\n"
        "   --------------------------------------------\n"
        "   1) Register User Account\n"
        "   2) Find User Account\n"
        "   3) List Users\n"
        "   4) List Accounts\n"
        "   5) List Transfer History\n"
        "   0) Quit\n");
}

static void print_logged_normal_user(void)
{
    printf("\n"
        "   Hello, Welcome to Pitic Bank Terminal System\n"
        "   --------------------------------------------\n"
        "   1) Show current balance \n"
        "   2) Retire money\n"
        "   3) Deposit money\n"
        "   4) Transfer money\n"
        "   5) Open another account\n"
        "   0) Quit\n");
}

/* ---- admin submenu: "Find User Account" -------------------------------- */

static void print_find_user_menu(void)
{
    printf("\n"
        "   Find User Account\n"
        "   --------------------------------------------\n"
        "   1) By id\n"
        "   2) By email\n"
        "   0) Back\n");
}

static void action_find_by_id(const UserService *service)
{
    char id_text[INPUT_MAX];
    if (read_line("Id      : ", id_text, sizeof(id_text)) != 0) {
        return;
    }

    char *end = NULL;
    long id = strtol(id_text, &end, 10);
    if (end == id_text || *end != '\0') {
        printf("  ! not a valid id\n");
        return;
    }

    User found;
    ServiceStatus status = user_service_get(service, id, &found);
    if (status == SERVICE_OK) {
        print_user(&found);
    } else {
        printf("  ! %s\n", service_status_text(status));
    }
}

static void action_find_by_email(const UserService *service)
{
    char email[INPUT_MAX];
    if (read_line("Email   : ", email, sizeof(email)) != 0) {
        return;
    }

    UserList users;
    user_list_init(&users);
    ServiceStatus status = user_service_list(service, &users);
    if (status != SERVICE_OK) {
        printf("  ! %s\n", service_status_text(status));
        user_list_free(&users);
        return;
    }

    int found = 0;
    for (size_t i = 0; i < users.count; i++) {
        if (strcmp(users.items[i].email, email) == 0) {
            print_user(&users.items[i]);
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("  ! %s\n", service_status_text(SERVICE_NOT_FOUND));
    }
    user_list_free(&users);
}

/* Nested submenu, entered from the admin menu's "Find User Account" option. */
static void run_find_user_menu(const UserService *service)
{
    for (;;) {
        print_find_user_menu();

        char choice[INPUT_MAX];
        if (read_line("Choose: ", choice, sizeof(choice)) != 0) {
            return;
        }

        if      (strcmp(choice, "1") == 0) action_find_by_id(service);
        else if (strcmp(choice, "2") == 0) action_find_by_email(service);
        else if (strcmp(choice, "0") == 0) return;
        else printf("  ! unknown option\n");
    }
}

/* ---- admin actions: users / accounts / transfer history ---------------- */

static void action_list_users(const UserService *service)
{
    UserList users;
    user_list_init(&users);
    ServiceStatus status = user_service_list(service, &users);
    if (status != SERVICE_OK) {
        printf("  ! %s\n", service_status_text(status));
        user_list_free(&users);
        return;
    }

    for (size_t i = 0; i < users.count; i++) {
        print_user(&users.items[i]);
    }
    user_list_free(&users);
}

static void action_list_accounts(const AccountService *service)
{
    AccountList accounts;
    account_list_init(&accounts);
    AccountServiceStatus status = account_service_list_all(service, &accounts);
    if (status != ACCOUNT_SERVICE_OK) {
        printf("  ! %s\n", account_service_status_text(status));
        account_list_free(&accounts);
        return;
    }

    for (size_t i = 0; i < accounts.count; i++) {
        print_account(&accounts.items[i]);
    }
    account_list_free(&accounts);
}

static void action_list_transfer_history(const TransferService *service)
{
    TransferList transfers;
    transfer_list_init(&transfers);
    TransferServiceStatus status = transfer_service_list_all(service, &transfers);
    if (status != TRANSFER_SERVICE_OK) {
        printf("  ! %s\n", transfer_service_status_text(status));
        transfer_list_free(&transfers);
        return;
    }

    for (size_t i = 0; i < transfers.count; i++) {
        print_transfer(&transfers.items[i]);
    }
    transfer_list_free(&transfers);
}

/* ---- customer actions: balance / deposit / withdraw / transfer --------- */

/* A customer may hold several accounts; ask which one this action applies
 * to and load it, confirming ownership along the way. Returns 0 on success. */
static int choose_own_account(const AccountService *service, const User *current_user, Account *out)
{
    AccountList accounts;
    account_list_init(&accounts);
    AccountServiceStatus status = account_service_list_for_user(service, current_user->id, &accounts);
    if (status != ACCOUNT_SERVICE_OK) {
        printf("  ! %s\n", account_service_status_text(status));
        account_list_free(&accounts);
        return -1;
    }
    if (accounts.count == 0) {
        printf("  ! you have no accounts yet\n");
        account_list_free(&accounts);
        return -1;
    }
    if (accounts.count == 1) {
        *out = accounts.items[0];
        account_list_free(&accounts);
        return 0;
    }

    printf("  Your accounts:\n");
    for (size_t i = 0; i < accounts.count; i++) {
        print_account(&accounts.items[i]);
    }

    char id_text[INPUT_MAX];
    if (read_line("Account id: ", id_text, sizeof(id_text)) != 0) {
        account_list_free(&accounts);
        return -1;
    }
    long id = strtol(id_text, NULL, 10);

    for (size_t i = 0; i < accounts.count; i++) {
        if (accounts.items[i].id == id) {
            *out = accounts.items[i];
            account_list_free(&accounts);
            return 0;
        }
    }

    printf("  ! that account is not yours\n");
    account_list_free(&accounts);
    return -1;
}

static void action_show_balance(const AccountService *service, const User *current_user)
{
    Account account;
    if (choose_own_account(service, current_user, &account) != 0) {
        return;
    }
    printf("  Account %s balance: ", account.account_number);
    print_cents(account.balance_cents);
    printf("\n");
}

static void action_deposit(const AccountService *service, const User *current_user)
{
    Account account;
    if (choose_own_account(service, current_user, &account) != 0) {
        return;
    }

    char amount_text[INPUT_MAX];
    if (read_line("Amount  : ", amount_text, sizeof(amount_text)) != 0) {
        return;
    }

    long long amount_cents;
    if (parse_amount_cents(amount_text, &amount_cents) != 0) {
        printf("  ! not a valid amount\n");
        return;
    }

    Account updated;
    AccountServiceStatus status = account_service_deposit(
        service, account.id, current_user->id, amount_cents, &updated);

    if (status == ACCOUNT_SERVICE_OK) {
        printf("  + new balance: ");
        print_cents(updated.balance_cents);
        printf("\n");
    } else {
        printf("  ! %s\n", account_service_status_text(status));
    }
}

static void action_withdraw(const AccountService *service, const User *current_user)
{
    Account account;
    if (choose_own_account(service, current_user, &account) != 0) {
        return;
    }

    char amount_text[INPUT_MAX];
    if (read_line("Amount  : ", amount_text, sizeof(amount_text)) != 0) {
        return;
    }

    long long amount_cents;
    if (parse_amount_cents(amount_text, &amount_cents) != 0) {
        printf("  ! not a valid amount\n");
        return;
    }

    Account updated;
    AccountServiceStatus status = account_service_withdraw(
        service, account.id, current_user->id, amount_cents, &updated);

    if (status == ACCOUNT_SERVICE_OK) {
        printf("  + new balance: ");
        print_cents(updated.balance_cents);
        printf("\n");
    } else {
        printf("  ! %s\n", account_service_status_text(status));
    }
}

static void action_transfer(const AccountService *service, const User *current_user)
{
    Account from_account;
    if (choose_own_account(service, current_user, &from_account) != 0) {
        return;
    }

    char to_account_number[INPUT_MAX];
    char amount_text[INPUT_MAX];
    if (read_line("To account number: ", to_account_number, sizeof(to_account_number)) != 0 ||
        read_line("Amount           : ", amount_text, sizeof(amount_text)) != 0) {
        return;
    }

    long long amount_cents;
    if (parse_amount_cents(amount_text, &amount_cents) != 0) {
        printf("  ! not a valid amount\n");
        return;
    }

    long transfer_id;
    AccountServiceStatus status = account_service_transfer(
        service, from_account.account_number, current_user->id,
        to_account_number, amount_cents, &transfer_id);

    if (status == ACCOUNT_SERVICE_OK) {
        printf("  + transfer #%ld completed\n", transfer_id);
    } else {
        printf("  ! %s\n", account_service_status_text(status));
    }
}

/* ---- open another account (from the logged-in customer menu) ----------- */

static void action_open_account(const AccountService *service, const User *current_user)
{
    Account created;
    AccountServiceStatus status = account_service_open(service, current_user->id, &created);
    if (status == ACCOUNT_SERVICE_OK) {
        printf("  + account opened:\n");
        print_account(&created);
    } else {
        printf("  ! %s\n", account_service_status_text(status));
    }
}

/* ---- role menu loops ---------------------------------------------------- */
static void run_admin_menu(const AppContext *app)
{
    for (;;) {
        print_logged_admin_menu();

        char choice[INPUT_MAX];
        if (read_line("Choose: ", choice, sizeof(choice)) != 0) {
            return;
        }

        if      (strcmp(choice, "1") == 0) action_create(app->users);
        else if (strcmp(choice, "2") == 0) run_find_user_menu(app->users);
        else if (strcmp(choice, "3") == 0) action_list_users(app->users);
        else if (strcmp(choice, "4") == 0) action_list_accounts(app->accounts);
        else if (strcmp(choice, "5") == 0) action_list_transfer_history(app->transfers);
        else if (strcmp(choice, "0") == 0) return;
        else printf("  ! unknown option\n");
    }
}

static void run_customer_menu(const AppContext *app, const User *current_user)
{
    for (;;) {
        print_logged_normal_user();

        char choice[INPUT_MAX];
        if (read_line("Choose: ", choice, sizeof(choice)) != 0) {
            return;
        }

        if      (strcmp(choice, "1") == 0) action_show_balance(app->accounts, current_user);
        else if (strcmp(choice, "2") == 0) action_withdraw(app->accounts, current_user);
        else if (strcmp(choice, "3") == 0) action_deposit(app->accounts, current_user);
        else if (strcmp(choice, "4") == 0) action_transfer(app->accounts, current_user);
        else if (strcmp(choice, "5") == 0) action_open_account(app->accounts, current_user);
        else if (strcmp(choice, "0") == 0) return;
        else printf("  ! unknown option\n");
    }
}

int cli_run(const AppContext *app)
{
    for (;;) {
        print_menu();

        char choice[INPUT_MAX];

        if (read_line("Choose: ", choice, sizeof(choice)) != 0) {
            break;
        }

        if      (strcmp(choice, "1") == 0) action_login(app);
        else if (strcmp(choice, "2") == 0) action_register_customer(app);
        else if (strcmp(choice, "0") == 0) break;
        else printf("  ! unknown option\n");
    }

    printf("Goodbye.\n");
    return 0;
}
