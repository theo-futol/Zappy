## EPITECH PROJECT - Sun, Jun, 2026                                                    ##
## Title           - zappy                                                             ##
## Description     -                                                                   ##
##     Makefile                                                                        ##
## ALL ZAPPY SERVICE MAKEFILE

CC = clang++

PROJECT_NAME = zappy
SERVER_BIN   = $(PROJECT_NAME)_server
AI_BIN       = $(PROJECT_NAME)_ai
AI_SOURCES   = $(shell find ia -type f -name "*.py")

all: $(SERVER_BIN) $(AI_BIN)

$(SERVER_BIN):
	$(MAKE) -C src/server BINARY_LOCATION=$(abspath $@)

$(AI_BIN): $(AI_SOURCES) Makefile
	@printf '%s\n' \
		'#!/bin/sh' \
		'APP_DIR=$$(CDPATH= cd -- "$$(dirname -- "$$0")" && pwd)' \
		'cd "$$APP_DIR" || exit 1' \
		'exec "$${PYTHON:-python3}" -m ia.client "$$@"' \
		> $@
	@chmod +x $@

clean:
	$(MAKE) -C src/server clean

fclean: clean 
	-rm -f $(SERVER_BIN) $(AI_BIN)

re: fclean all

.PHONY: all clean fclean re
