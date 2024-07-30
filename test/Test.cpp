#include "Test.hpp"

std::vector<__TestCase*> __TestCase::test_cases;

void __TestCase::RunTestCases() {
    std::cout << std::format("Running {} test cases", test_cases.size()) << std::endl;
    std::vector<std::string> failed;
    std::vector<std::string> excepted;
    size_t passed = 0;

    for (auto& test_case : test_cases) {
        auto desc = test_case->GetDescription();
        try {
            auto result = test_case->Run();
            if (result.HasValue()) {
                std::cout << std::format("Test '{}' passed", desc) << std::endl;
                ++passed;
            }
            
            else {
                std::cout << std::format("Test '{}' failed: {}", desc, result.Error()) << std::endl;
                failed.push_back(desc);
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