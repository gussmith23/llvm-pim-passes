#include <iostream>

#include <llvm/Pass.h> 
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

namespace {
  class TestModulePass : public llvm::ModulePass {
    public:
      static char ID;
      TestModulePass() : llvm::ModulePass(ID) {}

      bool runOnModule(llvm::Module& module) {
        llvm::errs() << module << "\n";
        return true;
      }
  };
}

char TestModulePass::ID = 0;
static llvm::RegisterPass<TestModulePass> unused("test-module-pass", "Test Module Pass");
// TODO I still don't understand the optional params to RegisterPass
