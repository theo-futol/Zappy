## ZAPPY GUI MAKEFILE

CXX = clang++
CC  = clang

BINARY_LOCATION ?= zappy_gui

## Includes relative to this directory (src/gui), plus vendored libs and pkg-config deps.
## openxr is a build-time dependency only (libopenxr-dev): the loader resolves an actual
## runtime/headset lazily at first API call, so the binary always builds and links even
## with no runtime installed; VR session creation then fails gracefully at runtime instead.
CPPFLAGS = -I. -Ilib/glad/include -Ilib/stb -Ilib/cgltf $(shell pkg-config --cflags glfw3 freetype2 openxr)
## -Wno-unused-private-field: tolerate not-yet-used fields of WIP stub classes
## (Texture, TextRenderer, HudRenderer). Drop it once those classes are implemented.
CXXFLAGS = -Wall -Wextra -Werror -std=c++20 -Wno-unused-private-field
## -lX11: the VR/GLX graphics-binding code (VRSession.cpp) calls Xlib functions (XFree, ...)
## directly, which GLX/GL alone don't pull in.
LDLIBS   = $(shell pkg-config --libs glfw3 freetype2 openxr) -lGL -lX11 -ldl
DEPFLAGS = -MMD -MP

## C++ sources of the GUI + the vendored C loaders (GLAD, cgltf).
CXX_SRC = $(shell find . -type f -name "*.cpp")
C_SRC   = lib/glad/src/glad.c lib/cgltf/cgltf.c
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
