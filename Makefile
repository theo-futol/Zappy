## EPITECH PROJECT - Sun, Jun, 2026                                                    ##
## Title           - zappy                                                             ##
## Description     -                                                                   ##
##     Makefile                                                                        ##
## ALL ZAPPY SERVICE MAKEFILE

PROJECT_NAME = zappy
BINARIES     = server gui ai

zappy_%:
	$(MAKE) -C src/$* BINARY_LOCATION=$(abspath $(PROJECT_NAME)_$*)

all: zappy_server zappy_gui zappy_ai

clean:
	@for bin in $(BINARIES); do \
		if ! $(MAKE) -C src/$$bin clean; then \
			echo "$$bin clean failed"; \
		fi; \
	done

fclean: clean
	-rm -f $(addprefix $(PROJECT_NAME)_,$(BINARIES))

.PHONY: all clean fclean
