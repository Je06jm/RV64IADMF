LD = g++
LD_FLAGS = -Llibs -lglfw3dll -lWs2_32

AR = ar
AR_FLAGS = rvs

IMGUI_DIR = thirdparties/imgui

CXX = g++
CXX_FLAGS = -Iinclude -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -std=c++20 -Wall -Wextra -c

IMGUI = $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp $(IMGUI_DIR)/misc/cpp/imgui_stdlib.cpp
CXX_SOURCES = $(wildcard src/*.cpp) $(wildcard thirdparties/imgui/*.cpp) $(IMGUI)
CXX_OBJS = $(patsubst %.cpp,%.o,$(CXX_SOURCES))

APP_SOURES = $(wildcard app/*.cpp)
APP_OBJS = $(patsubst %.cpp,%.o,$(APP_SOURES))

APP_CC_SOURCES = $(wildcard app/*.c)
APP_CC_OBJS = $(patsubst %.c,%.o,$(APP_CC_SOURCES))

APP_FLAGS = -Isrc

CXX_HEADERS = $(wildcard src/*.hpp)

LIBRARY = rv64adfima.a

PROGRAM = RV64ADFIMA.exe

BIOS_LD = ld.lld
BIOS_LD_FLAGS = -T bios/linker.ld

BIOS_OBJ_COPY = llvm-objcopy
BIOS_OBJ_COPY_FLAGS = -O binary

BIOS_CC = clang
BIOS_CC_FLAGS = -target riscv64 -march=rv64imafd -ffreestanding

BIOS_CC_HEADERS = $(wildcard bios/*.h)

BIOS_CC_SOURCES = $(wildcard bios/*.c)
BIOS_CC_OBJS = $(patsubst %.c,%.rv_o,$(BIOS_CC_SOURCES))

BIOS_ASM_SOURCES = $(wildcard bios/*.S)
BIOS_ASM_OBJS = $(patsubst %.S,%.rv_o,$(BIOS_ASM_SOURCES))

BIOS = bios

TEST_CXX = g++
TEST_CXX_FLAGS = -Iinclude -std=c++20 -Wall -Wextra -c

TEST_CXX_HEADERS = $(wildcard test/*.hpp)

TEST_CXX_SOURCES = $(wildcard test/*.cpp)
TEST_CXX_OBJS = $(patsubst %.cpp,%.t_o,$(TEST_CXX_SOURCES))

TEST_PROGRAM = test.exe

%.o: %.cpp $(CXX_HEADERS)
	$(CXX) $(CXX_FLAGS) $< -o $@

%.o: %.c $(CXX_HEADERS)
	$(CXX) $(CXX_FLAGS) $< -o $@

%.rv_o: %.c $(BIOS_CC_HEADERS)
	$(BIOS_CC) $(BIOS_CC_FLAGS) -c $< -o $@

%.rv_o: %.S $(BIOS_CC_HEADERS)
	$(BIOS_CC) $(BIOS_CC_FLAGS) -c $< -o $@

%.t_o: %.cpp $(TEST_CXX_HEADERS)
	$(TEST_CXX) $(TEST_CXX_FLAGS) $< -o $@

library: CXX_FLAGS += -O3
library: $(CXX_OBJS) $(CXX_HEADERS)
	$(AR) $(AR_FLAGS) $(LIBRARY) $(CXX_OBJS)

debug_library: CXX_FLAGS += -g
debug_library: $(CXX_OBJS) $(CXX_HEADERS)
	$(AR) $(AR_FLAGS) $(LIBRARY) $(CXX_OBJS)

all: CXX_FLAGS += $(APP_FLAGS)
all: library
all: CXX_FLAGS += -O3
all: $(APP_OBJS)  $(APP_CC_OBJS)
	$(LD) -o $(PROGRAM) $(APP_OBJS) $(CXX_OBJS) $(APP_CC_OBJS) $(LD_FLAGS) -O3

debug: CXX_FLAGS += $(APP_FLAGS)
debug: debug_library
debug: CXX_FLAGS += -g
debug: $(APP_OBJS) $(APP_CC_OBJS)
	$(LD) -o $(PROGRAM) $(APP_OBJS) $(CXX_OBJS) $(APP_CC_OBJS) $(LD_FLAGS) -O3

bios: $(BIOS_CC_OBJS) $(BIOS_ASM_OBJS) $(BIOS_CC_HEADERS)
	$(BIOS_LD) $(BIOS_LD_FLAGS) -o $(BIOS).elf $(BIOS_CC_OBJS) $(BIOS_ASM_OBJS)
	$(BIOS_OBJ_COPY) $(BIOS_OBJ_COPY_FLAGS) $(BIOS).elf $(BIOS).bin

test: TEST_CXX_FLAGS += -O3
test: library $(TEST_CXX_OBJS)
	$(LD) -o $(TEST_PROGRAM) $(TEST_CXX_OBJS) $(LIBRARY) -O3

debug_test: TEST_CXX_FLAGS += -g
debug_test: debug_library $(TEST_CXX_OBJS)
	$(LD) -o $(TEST_PROGRAM) $(TEST_CXX_OBJS) $(LIBRARY) -O3

clean:
	@-rm $(PROGRAM)
	@-rm $(CXX_OBJS)
	@-rm $(APP_CC_OBJS)
	@-rm $(APP_OBJS)
	@-rm $(LIBRARY)
	@-rm $(TEST_CXX_OBJS)
	@-rm $(TEST_PROGRAM)

clean_bios:
	@-rm $(BIOS).elf
	@-rm $(BIOS).bin
	@-rm $(BIOS_CC_OBJS)
	@-rm $(BIOS_ASM_OBJS)

PHONY: all debug clean clean_bios