SERVER=../../src/server

SRC = $(shell find . -type f -name "tests/server/*.cpp")
OBJ = $(SRC:.cpp=.o)

CXX = clang++
CXXFLAGS = -Wall -Wextra -Werror -std=c++20

all:
	make -C $(SERVER) all
	$(CXX) $(CXXFLAGS) $(OBJ) -o zappy_server_tests -lcriterion

fclean: clean
	make -C $(SERVER) fclean
	rm -f zappy_server_tests

clean:
	make -C $(SERVER) clean
	rm -rf $(OBJ)
	rm -rf $(shell find ../ -name "*.gcda")
	rm -rf $(shell find ../ -name "*.gcno")

tests_run:
	./zappy_server_tests
	gcovr -r ../../src/server --exclude tests
	gcovr -r ../../src/server --exclude tests --txt-metric branch
