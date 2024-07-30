#include <RV64.hpp>

#include "Test.hpp"

int main() {
    RVInstruction::SetupCSRNames();
    
    __TestCase::RunTestCases();
}