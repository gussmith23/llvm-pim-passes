#include "pim-instructions-flag.h"

#include <cstring>

#include <llvm/IR/Instructions.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

llvm::cl::opt<std::string> pimInstructionsFlag(
    "pi",
    llvm::cl::desc("Comma-separated list of LLVM instructions which can be "
                   "offloaded to memory"),
    llvm::cl::value_desc("inst1,inst2,..."));

std::set<unsigned int> getPimInstructions() {
  std::set<unsigned int> pimInstructions;

  if (pimInstructionsFlag.c_str()) {
    char* string = strdup(pimInstructionsFlag.c_str());
    char* token = strtok(string, ",");
    while (token != NULL) {
      if (strcmp(token, "add") == 0)
        pimInstructions.insert(llvm::Instruction::Add);
      else if (strcmp(token, "sub") == 0)
        pimInstructions.insert(llvm::Instruction::Sub);
      else if (strcmp(token, "and") == 0)
        pimInstructions.insert(llvm::Instruction::And);
      else if (strcmp(token, "or") == 0)
        pimInstructions.insert(llvm::Instruction::Or);
      else if (strcmp(token, "xor") == 0)
        pimInstructions.insert(llvm::Instruction::Xor);
      else if (strcmp(token, "mul") == 0)
        pimInstructions.insert(llvm::Instruction::Mul);
      else if (strcmp(token, "shl") == 0)
        pimInstructions.insert(llvm::Instruction::Shl);
      else if (strcmp(token, "ashr") == 0)
        pimInstructions.insert(llvm::Instruction::AShr);
      else if (strcmp(token, "lshr") == 0)
        pimInstructions.insert(llvm::Instruction::LShr);
      else if (strcmp(token, "sdiv") == 0)
        pimInstructions.insert(llvm::Instruction::SDiv);
      else if (strcmp(token, "udiv") == 0)
        pimInstructions.insert(llvm::Instruction::UDiv);
      else if (strcmp(token, "srem") == 0)
        pimInstructions.insert(llvm::Instruction::SRem);
      else if (strcmp(token, "urem") == 0)
        pimInstructions.insert(llvm::Instruction::URem);
      else if (strcmp(token, "sext") == 0)
        pimInstructions.insert(llvm::Instruction::SExt);
      else if (strcmp(token, "zext") == 0)
        pimInstructions.insert(llvm::Instruction::ZExt);
      else
        // If you get here, just add the instruction using the pattern above.
        llvm_unreachable("Unrecognized command in pim instruction flag.");
      token = strtok(NULL, ",");
    }
    free(string);
  }

  return pimInstructions;
}
