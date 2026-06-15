## EPITECH PROJECT - Sun, Jun, 2026                                                    ##
## Title           - zappy                                                             ##
## Description     -                                                                   ##
##     Makefile                                                                        ##
## ALL ZAPPY SERVICE MAKEFILE

ZAPPY_SERVER = make -C src/server BINARY_LOCATION=$(abspath zappy_server)

all:
	$(ZAPPY_SERVER)

zappy_server:
	$(ZAPPY_SERVER)
zappy_gui:
	## inshallah un jour on aura le gui
zappy_ai:
	## inshallah un jour on aura l'ia
clean:
	$(ZAPPY_SERVER) clean

fclean: clean
	$(ZAPPY_SERVER) fclean

re:
	$(ZAPPY_SERVER) re
