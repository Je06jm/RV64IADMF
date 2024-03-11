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

BIOS_LD = ld.lld
BIOS_LD_FLAGS = -T bios/linker.ld

BIOS_OBJ_COPY = llvm-objcopy
BIOS_OBJ_COPY_FLAGS = -O binary

BIOS_CC = clang
BIOS_CC_FLAGS = -target riscv32 -march=rv32imafd -ffreestanding

BIOS_CC_HEADERS = $(wildcard bios/*.h)

BIOS_CC_SOURCES = $(wildcard bios/*.c)
BIOS_CC_OBJS = $(patsubst %.c,%.rv_o,$(BIOS_CC_SOURCES))

BIOS_ASM_SOURCES = $(wildcard bios/*.S)
BIOS_ASM_OBJS = $(patsubst %.S,%.rv_o,$(BIOS_ASM_SOURCES))

BIOS = bios

%.o: %.cpp $(CXX_HEADERS)
	$(CXX) $(CXX_FLAGS) $< -o $@

%.o: %.c $(CXX_HEADERS)
	$(CXX) $(CXX_FLAGS) $< -o $@

%.rv_o: %.c $(BIOS_CC_HEADERS)
	$(BIOS_CC) $(BIOS_CC_FLAGS) -c $< -o $@

%.rv_o: %.S $(BIOS_CC_HEADERS)
	$(BIOS_CC) $(BIOS_CC_FLAGS) -c $< -o $@

all: CXX_FLAGS += -O2
all: $(CXX_OBJS) $(CC_OBJS) $(CXX_HEADERS)
	$(LD) -o $(PROGRAM) $(CXX_OBJS) $(CC_OBJS) $(LD_FLAGS) -O2

debug: CXX_FLAGS += -g
debug: $(CXX_OBJS) $(CC_OBJS) $(CXX_HEADERS)
	$(LD) -o $(PROGRAM) $(CXX_OBJS) $(CC_OBJS) $(LD_FLAGS) -g

bios: $(BIOS_CC_OBJS) $(BIOS_ASM_OBJS) $(BIOS_CC_HEADERS)
	$(BIOS_LD) $(BIOS_LD_FLAGS) -o $(BIOS).elf $(BIOS_CC_OBJS) $(BIOS_ASM_OBJS)
	$(BIOS_OBJ_COPY) $(BIOS_OBJ_COPY_FLAGS) $(BIOS).elf $(BIOS).bin

clean:
	@-rm $(PROGRAM).*
	@-rm $(PROGRAM)
	@-rm $(CXX_OBJS)
	@-rm $(CC_OBJS)

clean_bios:
	@-rm $(BIOS).*
	@-rm $(BIOS)
	@-rm $(BIOS_CC_OBJS)
	@-rm $(BIOS_ASM_OBJS)

PHONY: clean clean_bios