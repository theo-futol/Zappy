## ZAPPY GUI MAKEFILE

CXX = clang++
CC  = clang

BINARY_LOCATION ?= zappy_gui

## Includes relative to this directory (src/gui), plus vendored libs and pkg-config deps.
CPPFLAGS = -I. -Ilib/glad/include -Ilib/stb $(shell pkg-config --cflags glfw3 freetype2)
## -Wno-unused-private-field: tolerate not-yet-used fields of WIP stub classes
## (Texture, TextRenderer, HudRenderer). Drop it once those classes are implemented.
CXXFLAGS = -Wall -Wextra -Werror -std=c++20 -Wno-unused-private-field
LDLIBS   = $(shell pkg-config --libs glfw3 freetype2) -lGL -ldl
DEPFLAGS = -MMD -MP

## C++ sources of the GUI + the vendored GLAD loader (C).
CXX_SRC = $(shell find . -type f -name "*.cpp")
C_SRC   = lib/glad/src/glad.c
OBJ     = $(CXX_SRC:.cpp=.o) $(C_SRC:.c=.o)
DEP     = $(OBJ:.o=.d)

all: $(BINARY_LOCATION)

$(BINARY_LOCATION): $(OBJ)
	$(CXX) -o $(BINARY_LOCATION) $(OBJ) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(DEP)

fclean: clean
	rm -f $(BINARY_LOCATION)

re: fclean all

-include $(DEP)

.PHONY: all clean fclean re
