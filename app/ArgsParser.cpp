#include "ArgsParser.hpp"

#include <iostream>
#include <format>
#include <cstdlib>

ArgsParser::ArgsParser(const std::vector<std::string>& args) {
    auto Duplicate = [](const std::string& name) {
        std::cerr << std::format("Argument {} has already been defined", name) << std::endl;
        std::exit(EXIT_FAILURE);
    };

    for (auto& arg : args) {
        if (arg.starts_with("--")) {
            auto name = arg.substr(2);

            auto index = name.find('=');

            if (index != std::string::npos) {
                auto value = name.substr(index + 1);
                name = name.substr(0, index);

                if (HasValue(name))
                    Duplicate(name);
                
                values[name] = value;
            }
            else {
                if (HasFlag(name))
                    Duplicate(name);
                
                flags.insert(name);
            }
        }
        else if (arg.starts_with("-")) {
            if (arg.size() < 2) {
                std::cerr << std::format("Unknown argument at '{}'", arg) << std::endl;
                std::exit(EXIT_FAILURE);
            }
            else if (arg.size() > 2) {
                std::cerr << std::format("Single dash flags can only be a single letter '{}'", arg) << std::endl;
                std::exit(EXIT_FAILURE);
            }

            std::string name;
            name += arg[1];

            if (HasFlag(name))
                Duplicate(name);
            
            flags.insert(name);
        }
        else {
            ordered.push_back(arg);
        }
    }
}