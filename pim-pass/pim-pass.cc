#include <iostream>

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

namespace {
  class PimPass : public llvm::FunctionPass {
    public:
      static char ID;
      PimPass() : llvm::FunctionPass(ID) {}

      bool runOnFunction(llvm::Function& function) {
        llvm::errs() << function << "\n";
        return true;
      }
  };
}

char PimPass::ID = 0;
static llvm::RegisterPass<PimPass> unused("pim-pass", "PIM Pass");
// TODO I still don't understand the optional params to RegisterPass
