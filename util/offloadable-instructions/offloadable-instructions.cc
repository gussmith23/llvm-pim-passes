#include "offloadable-instructions.h"

#include <cstring>
#include <set>

#include <llvm/IR/Instructions.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

llvm::cl::opt<std::string> pimInstructionsFlag(
    "pi",
    llvm::cl::desc("Comma-separated list of LLVM instructions which can be "
                   "offloaded to memory"),
    llvm::cl::value_desc("inst1,inst2,..."));

/**
 * Default list of offloadable instructions.
 * This list is comprised of any instructions we could plausibly support in
 * memory.
 */
std::set<unsigned int> offloadableInstructions = {
    llvm::Instruction::Add,           llvm::Instruction::Sub,
    llvm::Instruction::Mul,           llvm::Instruction::UDiv,
    llvm::Instruction::SDiv,          llvm::Instruction::URem,
    llvm::Instruction::SRem,          llvm::Instruction::FAdd,
    llvm::Instruction::FSub,          llvm::Instruction::FMul,
    llvm::Instruction::FDiv,          llvm::Instruction::FRem,
    llvm::Instruction::GetElementPtr, llvm::Instruction::Shl,
    llvm::Instruction::LShr,          llvm::Instruction::AShr,
    llvm::Instruction::And,           llvm::Instruction::Or,
    llvm::Instruction::Xor,           llvm::Instruction::Trunc,
    llvm::Instruction::FPTrunc,       llvm::Instruction::ZExt,
    llvm::Instruction::SExt,          llvm::Instruction::FPToUI,
    llvm::Instruction::FPToSI,        llvm::Instruction::UIToFP,
    llvm::Instruction::SIToFP,        llvm::Instruction::PtrToInt,
    llvm::Instruction::IntToPtr,      llvm::Instruction::BitCast};

/**
 * Read the value of the -pi flag, parse the instructions, and return a list of
 * opcodes.
 * Alternatively, if no -pi flag is given, return the list of defaults.
 */
void getPimInstructions() {
  if (pimInstructionsFlag.c_str() && strlen(pimInstructionsFlag.c_str())) {
    // Remove defaults and parse the flag.
    offloadableInstructions.clear();
    char* string = strdup(pimInstructionsFlag.c_str());
    char* token = strtok(string, ",");
    while (token != NULL) {
      if (strcmp(token, "add") == 0)
        offloadableInstructions.insert(llvm::Instruction::Add);
      else if (strcmp(token, "sub") == 0)
        offloadableInstructions.insert(llvm::Instruction::Sub);
      else if (strcmp(token, "and") == 0)
        offloadableInstructions.insert(llvm::Instruction::And);
      else if (strcmp(token, "or") == 0)
        offloadableInstructions.insert(llvm::Instruction::Or);
      else if (strcmp(token, "xor") == 0)
        offloadableInstructions.insert(llvm::Instruction::Xor);
      else if (strcmp(token, "mul") == 0)
        offloadableInstructions.insert(llvm::Instruction::Mul);
      else if (strcmp(token, "shl") == 0)
        offloadableInstructions.insert(llvm::Instruction::Shl);
      else if (strcmp(token, "ashr") == 0)
        offloadableInstructions.insert(llvm::Instruction::AShr);
      else if (strcmp(token, "lshr") == 0)
        offloadableInstructions.insert(llvm::Instruction::LShr);
      else if (strcmp(token, "sdiv") == 0)
        offloadableInstructions.insert(llvm::Instruction::SDiv);
      else if (strcmp(token, "udiv") == 0)
        offloadableInstructions.insert(llvm::Instruction::UDiv);
      else if (strcmp(token, "srem") == 0)
        offloadableInstructions.insert(llvm::Instruction::SRem);
      else if (strcmp(token, "urem") == 0)
        offloadableInstructions.insert(llvm::Instruction::URem);
      else if (strcmp(token, "sext") == 0)
        offloadableInstructions.insert(llvm::Instruction::SExt);
      else if (strcmp(token, "zext") == 0)
        offloadableInstructions.insert(llvm::Instruction::ZExt);
      else
        // If you get here, just add the instruction using the pattern above.
        llvm_unreachable("Unrecognized command in pim instruction flag.");
      token = strtok(NULL, ",");
    }
    free(string);
  }
}

bool flagParsed = false;
bool canOffload(llvm::Value* value) {
  // Initial setup.
  if (!flagParsed) {
    getPimInstructions();
    flagParsed = true;
  }

  if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(value))
    if (offloadableInstructions.count(inst->getOpcode())) return true;
  return false;
}
