#include <iostream>
#include <set>

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/LLVMDependenceGraph.h>

namespace {
  class DgTest : public llvm::ModulePass {
    public:
      static char ID;
      DgTest() : llvm::ModulePass(ID) {}

      bool runOnModule(llvm::Module& module) {
        dg::LLVMDependenceGraph dg;
        dg.build(&module);

        llvm::errs() << "entry: ";
        dg.getEntry()->getValue()->print(llvm::errs());
        llvm::errs() << "\n";

        return false;
      }
  };
}

char DgTest::ID = 0;
static llvm::RegisterPass<DgTest> unused("dg-test", "DG Test");
// TODO I still don't understand the optional params to RegisterPass
