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

    if (read_line("Name    : ", name, sizeof(name)) != 0 ||
        read_line("Email   : ", email, sizeof(email)) != 0 ||
        read_line("Password: ", password, sizeof(password)) != 0) {
        return;
    }

    User created;
    ServiceStatus status = user_service_register(service, name, email, password, &created);

    if (status == SERVICE_OK) {
        printf("  + created:\n");
        print_user(&created);
    } else {
        printf("  ! %s\n", service_status_text(status));
    }
}

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

    if (status == SERVICE_OK) {
        printf("  + welcome back, %s (%s)\n", authenticated.name, authenticated.role);
    } else {
        printf("  ! %s\n", service_status_text(status));
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
