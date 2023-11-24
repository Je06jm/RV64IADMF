LD = g++
LD_FLAGS = -Llibs -lglfw3dll

IMGUI_DIR = thirdparties/imgui

CXX = g++
CXX_FLAGS = -Iinclude -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -std=c++20 -Wall -Wextra -c

IMGUI = $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp $(IMGUI_DIR)/misc/cpp/imgui_stdlib.cpp
CXX_SOURCES = $(wildcard src/*.cpp) $(wildcard thirdparties/imgui/*.cpp) $(IMGUI)
CXX_OBJS = $(patsubst %.cpp,%.o,$(CXX_SOURCES))

CC_SOURCES = $(wildcard src/*.c)
CC_OBJS = $(patsubst %.c,%.o,$(CC_SOURCES))

CXX_HEADERS = $(wildcard src/*.hpp)

PROGRAM = RV32IMF

%.o: %.cpp $(CXX_HEADERS)
	$(CXX) $(CXX_FLAGS) $< -o $@

%.o: %.c $(CXX_HEADERS)
	$(CXX) $(CXX_FLAGS) $< -o $@

all: CXX_FLAGS += -O2
all: $(CXX_OBJS) $(CC_OBJS) $(CXX_HEADERS)
	$(LD) -o $(PROGRAM) $(CXX_OBJS) $(CC_OBJS) $(LD_FLAGS) -O2

debug: CXX_FLAGS += -g
debug: $(CXX_OBJS) $(CC_OBJS)  $(CXX_HEADERS)
	$(LD) -o $(PROGRAM) $(CXX_OBJS) $(CC_OBJS) $(LD_FLAGS) -g

clean:
	@-rm $(PROGRAM).*
	@-rm $(PROGRAM)
	@-rm $(CXX_OBJS)
	@-rm $(CC_OBJS)

PHONY: clean