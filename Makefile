#   This script is an improvement of:
# Thomas Daley
# September 13, 2021
# For the original version follow:
# https://gist.github.com/tomdaley92/190c68e8a84038cc91a5459409e007df

# A generic build template for C/C++ programs

# executable name
EXE := app
MAIN := main.c

# C compiler
CC := clang
# C++ compiler
CXX := clang
# linker
LD := clang

# C and C++ flags
CFLAGS := -O2 -Wall -g -Werror=pointer-arith
# C++ flags
CPPFLAGS :=

# # Remove all logging except for errors
# CFLAGS += -DBUILD_RELEASE

# # Remove references to source when logging
# CFLAGS += -DLOG_NO_SOURCE_POSITION

# # Remove special charaters in logging (Useful when using terminals that do not
#   support them
# CFLAGS += -DLOG_DISABLE_COLOR

# dependency-generation flags
DEPFLAGS := -MMD -MP -Isrc/
# linker flags
LDFLAGS := -g -dead_strip
# packages used
PKG :=
# build directories
BIN := bin
OBJ := obj
SRC := src
SHA := shaders
# Additional dependencies on which rebuild objects
# useful when changing stuff in the makefile to rebuild the project with new options
OBJDEPS := Makefile

################################################################################
# Ideally you should not go under this line
################################################################################

CFLAGS += $(shell [ "$(PKG)" ] && pkg-config --cflags $(PKG)) $(DEPFLAGS)
LDFLAGS += $(shell [ "$(PKG)" ] && pkg-config --libs $(PKG))

SOURCES.c := $(shell find $(SRC) -name '*.c')
SOURCES.cpp := $(shell find $(SRC) -name '*.cpp')

OBJECTS.o := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES.c))
OBJECTS.opp := $(patsubst $(SRC)/%.cpp, $(OBJ)/%.opp, $(SOURCES.cpp))

SHADERS := \
	$(patsubst $(SHA)/%.frag, $(SHA)/%Frag.spv, $(wildcard $(SHA)/*.frag)) \
	$(patsubst $(SHA)/%.vert, $(SHA)/%Vert.spv, $(wildcard $(SHA)/*.vert))

# include compiler-generated dependency rules
DEPENDS := $(OBJECTS.o:.o=.d) $(OBJECTS.opp:.opp=.d)

# compile C source
COMPILE.c = $(CC) $(CFLAGS) -c -o $@
# compile C++ source
COMPILE.cpp = $(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@
# link objects
LINK.o = $(LD) $(LDFLAGS) $(OBJECTS.opp) $(OBJECTS.o) -o $@
# shaders creation
COMPILE.spv = glslc -o $@

ENSURE = @mkdir -p $(dir $@) 2> /dev/null || true

.DEFAULT_GOAL = all

.PHONY: all
all: $(BIN)/$(EXE) $(SHADERS)

$(BIN)/$(EXE): $(SRC) $(OBJ) $(BIN) $(OBJECTS.o) $(OBJECTS.opp)
	$(LINK.o)

$(SRC):
	mkdir -p $(SRC)
	touch $(SRC)/$(MAIN)

$(OBJ):
	mkdir -p $(OBJ)

$(BIN):
	mkdir -p $(BIN)

$(SHA):
	mkdir -p $(SHA)

$(OBJ)/%.o:	$(SRC)/%.c $(OBJDEPS)
	$(ENSURE)
	$(COMPILE.c) $<

$(OBJ)/%.opp:	$(SRC)/%.cpp $(OBJDEPS)
	$(ENSURE)
	$(COMPILE.cpp) $<

# force rebuild
.PHONY: remake
remake:	clean $(BIN)/$(EXE)

# execute the program
.PHONY: run
run: $(BIN)/$(EXE) $(SHADERS)
	./$(BIN)/$(EXE)

# remove previous build and objects
.PHONY: clean
clean:
	$(RM) $(OBJECTS.o) $(OBJECTS.opp)
	$(RM) $(DEPENDS)
	$(RM) $(SHA)/*.spv
	$(RM) $(BIN)/$(EXE)

# compile shaders only
.PHONY: compile-shaders
compile-shaders: $(SHADERS)

# clean shaders only
.PHONY: clean-shaders
clean-shaders:
	$(RM) $(SHA)/*.spv

-include $(DEPENDS)
