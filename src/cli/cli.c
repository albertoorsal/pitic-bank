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

static void print_user(const User *user)
{
    printf("  #%ld  %-20s  %-30s  %s\n", user->id, user->name, user->email, user->role);
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

/* Forward declarations: role menus dispatch into their own loops below. */
static void run_admin_menu(const UserService *service);
static void run_customer_menu(const UserService *service);

static void action_login(const UserService *service)
{
    char email[INPUT_MAX];
    char password[INPUT_MAX];

    if (read_line("Email   : ", email, sizeof(email)) != 0 ||
        read_line("Password: ", password, sizeof(password)) != 0) {
        return;
    }

    User authenticated;
    ServiceStatus status = user_service_login(service, email, password, &authenticated);

    if (status != SERVICE_OK) {
        printf("  ! %s\n", service_status_text(status));
        return;
    }

    printf("  + welcome back, %s (%s)\n", authenticated.name, authenticated.role);

    if (strcmp(authenticated.role, "admin") == 0) {
        run_admin_menu(service);
    } else {
        run_customer_menu(service);
    }
}


static void print_menu(void) {
    printf("\n"
        "   Hello, Welcome to Pitic Bank Terminal System\n"
        "   --------------------------------------------\n"
        "   1) Login\n"
        "   2) Open Account\n"
        "   0) Quit\n");
}

static void print_logged_admin_menu(void)
{
    printf("\n"
        "   Hello, Welcome to Pitic Bank Terminal System Admin Panel\n"
        "   --------------------------------------------\n"
        "   1) Register User Account\n"
        "   2) Find User Account\n"
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

/* ---- role menu loops ---------------------------------------------------- */
static void run_admin_menu(const UserService *service)
{
    for (;;) {
        print_logged_admin_menu();

        char choice[INPUT_MAX];
        if (read_line("Choose: ", choice, sizeof(choice)) != 0) {
            return;
        }

        if      (strcmp(choice, "1") == 0) action_create(service);
        else if (strcmp(choice, "2") == 0) run_find_user_menu(service);
        else if (strcmp(choice, "0") == 0) return;
        else printf("  ! unknown option\n");
    }
}

static void run_customer_menu(const UserService *service)
{
    (void)service; /* balance/withdraw/deposit are not implemented yet */

    for (;;) {
        print_logged_normal_user();

        char choice[INPUT_MAX];
        if (read_line("Choose: ", choice, sizeof(choice)) != 0) {
            return;
        }

        if      (strcmp(choice, "1") == 0) printf("  ! not implemented yet\n");
        else if (strcmp(choice, "2") == 0) printf("  ! not implemented yet\n");
        else if (strcmp(choice, "3") == 0) printf("  ! not implemented yet\n");
        else if (strcmp(choice, "0") == 0) return;
        else printf("  ! unknown option\n");
    }
}

int cli_run(const UserService *service)
{
    for (;;) {
        print_menu();

        char choice[INPUT_MAX];

        if (read_line("Choose: ", choice, sizeof(choice)) != 0) {
            break;
        }

        if      (strcmp(choice, "1") == 0) action_login(service);
        else if (strcmp(choice, "2") == 0) action_create(service);
        else if (strcmp(choice, "0") == 0) break;
        else printf("  ! unknown option\n");
    }

    printf("Goodbye.\n");
    return 0;
}
