## EPITECH PROJECT - Sun, Jun, 2026                                                    ##
## Title           - zappy                                                             ##
## Description     -                                                                   ##
##     Makefile                                                                        ##
## ALL ZAPPY SERVICE MAKEFILE

CC = clang++

PROJECT_NAME = zappy
SERVER_BIN   = $(PROJECT_NAME)_server
AI_BIN       = $(PROJECT_NAME)_ai
GUI_BIN 	 = $(PROJECT_NAME)_gui
AI_SOURCES   = $(shell find src/ia -type f -name "*.py")

all: $(SERVER_BIN) $(AI_BIN) $(GUI_BIN)

$(SERVER_BIN):
	$(MAKE) -C src/server BINARY_LOCATION=$(abspath $@)

$(AI_BIN): $(AI_SOURCES) Makefile
	@printf '%s\n' \
		'#!/bin/sh' \
		'APP_DIR=$$(CDPATH= cd -- "$$(dirname -- "$$0")" && pwd)' \
		'cd "$$APP_DIR" || exit 1' \
		'PYTHONPATH="$$APP_DIR/src$${PYTHONPATH:+:$$PYTHONPATH}"' \
		'export PYTHONPATH' \
		'exec "$${PYTHON:-python3}" -m ia.client "$$@"' \
		> $@
	@chmod +x $@
$(GUI_BIN):
	$(MAKE) -C src/gui BINARY_LOCATION=$(abspath $@)

clean:
	$(MAKE) -C src/server clean
	$(MAKE) -C src/gui clean

fclean: clean 
	-rm -f $(SERVER_BIN) $(AI_BIN) $(GUI_BIN)

re: fclean all

.PHONY: all clean fclean re
