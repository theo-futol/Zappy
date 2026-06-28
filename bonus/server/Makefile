## ZAPPY SERVER MAKEFILE


CC = clang++

BINARY_LOCATION ?= zappy_server

CXXFLAGS = -Wall -Wextra -Werror -std=c++20

SRC = $(shell find . -type f -name "*.cpp")

OBJ = $(SRC:.cpp=.o)

all: $(OBJ)
	$(CC) $(CXXFLAGS) -o $(BINARY_LOCATION) $(OBJ)

clean:
	rm -f $(OBJ) $(BINARY_LOCATION)

fclean: clean
	rm -f $(BINARY_LOCATION)

re: fclean all
