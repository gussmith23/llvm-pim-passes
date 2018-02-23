#include <iostream>

#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"

namespace {
  class PimPass : public llvm::BasicBlockPass {
    public:
      static char ID;
      PimPass() : llvm::BasicBlockPass(ID) {}

      bool runOnBasicBlock(llvm::BasicBlock& basicBlock) {
        llvm::errs() << basicBlock << "\n";
        return true;
      }
  };
}

char PimPass::ID = 0;
static llvm::RegisterPass<PimPass> unused("pim-pass", "PIM Pass");
// TODO I still don't understand the optional params to RegisterPass
