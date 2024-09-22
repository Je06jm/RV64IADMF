#include <RV64.hpp>

#include "Test.hpp"

constexpr size_t ITERATIONS_PER_TESTCASE = 100;

int main() {
    RVInstruction::SetupCSRNames();
    
    __TestCase::RunTestCases(ITERATIONS_PER_TESTCASE);
}