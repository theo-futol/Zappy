SRC	=	$(shell find ../../../src/server ./ -name "*.cpp" -not -path "../../../src/server/main.cpp")
OBJ = $(SRC:.cpp=.o)

CXX = g++
CXXFLAGS	=	-Wall -Wextra -Werror  -std=c++20 --coverage -O0 -g -I../../../src/server

all: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o zappy_server_tests -lcriterion

fclean: clean
	rm -f zappy_server_tests

clean:
	rm -rf $(OBJ)
	rm -rf $(shell find ../../../src/server ./ -name "*.gcda")
	rm -rf $(shell find ../../../src/server ./ -name "*.gcno")

tests_run: all
	./zappy_server_tests
	gcovr -r ../../../src/server
	gcovr -r ../../../src/server --txt-metric branch
