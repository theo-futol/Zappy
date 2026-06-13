## ZAPPY SERVER MAKEFILE


CC = clang++

BINARY = zappy_server

CXXFLAGS = -Wall -Wextra -Werror -std=c++20

SRC = $(shell find . -type f -name "*.cpp")

OBJ = $(SRC:.cpp=.o)

all: $(OBJ)
	$(CC) $(CXXFLAGS) -o $(BINARY) $(OBJ)

clean:
	rm -f $(OBJ) $(BINARY)

fclean: clean
	rm -f $(BINARY)

re: fclean all
