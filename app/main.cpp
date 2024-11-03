#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#include <VirtualMachine.hpp>

#include "Machine.hpp"

int main(int argc, const char** argv) {
    std::vector<std::string> args;

    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }

    // Process command line args
    ArgsParser args_parser(args);
    
    return RunMachine(args_parser);
}