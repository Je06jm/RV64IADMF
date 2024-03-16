#include <Memory.hpp>
#include <VirtualMachine.hpp>
#include <RV32I.hpp>
#include <ECalls.hpp>

int main() {
    RVInstruction::SetupCSRNames();
    
    Memory memory(2 * 1024 * 1024);
    VirtualMachine vm(memory, 0, 100000, 0);

    vm.Start();
}