all:
	g++ src/*.cpp src/*.c -Wall -Wextra -std=c++20 -Iinclude -Llibs -lglfw3dll -o RV32IMF.exe
	