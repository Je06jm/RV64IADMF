#include "Test.hpp"

#include <ctime>

std::vector<__TestCase*> __TestCase::test_cases;

void __TestCase::RunTestCases(size_t iterations) {
    std::cout << std::format("Running {} test cases", test_cases.size()) << std::endl;
    std::vector<std::string> failed;
    std::vector<std::string> excepted;
    size_t passed = 0;

    for (auto& test_case : test_cases) {
        auto desc = test_case->GetDescription();
        try {
            bool success = true;

            for (size_t i = 0; i < iterations; i++) {
                auto result = test_case->Run();
                
                if (!result.HasValue()) {
                    std::cout << std::format("Test '{}' failed: {}", desc, result.Error()) << std::endl;
                    failed.push_back(desc);
                    success = false;
                    break;
                }
            }

            if (success) {
                std::cout << std::format("Test '{}' passed", desc) << std::endl;
                ++passed;
            }
        }
        catch (std::exception& e) {
            std::cout << std::format("Test '{}' threw an exception: {}", desc, e.what()) << std::endl;
            excepted.push_back(desc);
        }
    }
    
    std::cout << std::format("Passed {}, failed {}, excepted {}", passed, failed.size(), excepted.size()) << std::endl;

    if (failed.size() != 0) {
        std::cout << "Test failed: ";

        for (auto& desc : failed)
            std::cout << desc << " ";

        std::cout << std::endl;
    }

    if (excepted.size() != 0) {
        std::cout << "Test excepted: ";

        for (auto& desc : excepted)
            std::cout << desc << " ";
        
        std::cout << std::endl;
    }
}

size_t RandomInt() {
    static size_t state[4];
    static bool init = false;

    if (!init) {
        init = true;
        auto time = std::time(nullptr);

        state[0] = state[1] = state[2] = state[3] = static_cast<size_t>(time);

        RandomInt();
        RandomInt();
        RandomInt();
        RandomInt();
    }

    auto t = state[0];
    state[3] = state[2];
    state[2] = state[1];
    state[1] = state[0];

    t ^= t << 11;
    t ^= t >> 3;
    t ^= t >> 5;
    t ^= t << 25;

    state[0] = t;

    return state[0] ^ state[1] ^ state[2] ^ state[3];
}