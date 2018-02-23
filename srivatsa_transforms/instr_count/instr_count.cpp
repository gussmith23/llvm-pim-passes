#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;


namespace {
    struct instr_count : public FunctionPass 
    {
    static char ID; 
    instr_count() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      errs() << "instr_count: ";
      errs() << F.getName() << "\n";
      for (Function::iterator bb = F.begin(), e = F.end(); bb!=e;bb++)
      {
        errs()<<"BasicBlock name ="<< bb ->getName()<<"\n";
        errs()<<"BasicBlock size ="<< bb->size()<<"\n\n";
      } 
    }
  };
}

char instr_count::ID = 0;
static RegisterPass<instr_count> X("instr_count", "instr_count of the program");


