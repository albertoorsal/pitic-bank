CC = clang
CFLAGS = -Wall -Wextra -std=c17 -Isrc -I$(shell pg_config --includedir)
LDFLAGS = -L$(shell pg_config --libdir) -lpq

SRC = src/main.c \
      src/database.c \
      src/cli/cli.c \
      src/user/user.c \
      src/user/user_repository.c \
      src/user/user_service.c \
      src/user/pg_user_repository.c
BIN = pitic-bank

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

seed-admin: $(BIN)
	./$(BIN) seed-admin

clean:
	rm -f $(BIN)

.PHONY: clean seed-admin
