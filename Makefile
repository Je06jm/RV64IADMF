all:
	g++ src/*.cpp src/*.c -Wall -Wextra -std=c++20 -Iinclude -Llibs -lglfw3dll -o RV32IMF.exe -O2

debug:
	g++ src/*.cpp src/*.c -Wall -Wextra -std=c++20 -Iinclude -Llibs -lglfw3dll -o RV32IMF.exe -g

clean:
	@-rm RV32IMF.exe

PHONY: clean