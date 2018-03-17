#include <iostream>

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

namespace {
  class DefUse : public llvm::FunctionPass {
    public:
      static char ID;
      DefUse() : llvm::FunctionPass(ID) {}

      bool runOnFunction(llvm::Function& function) {
        std::vector<llvm::Instruction*> worklist;
        for(llvm::inst_iterator I = inst_begin(function), E = inst_end(function); I != E; ++I){
          worklist.push_back(&*I);
        }

        // def-use chain for Instruction
        for(std::vector<llvm::Instruction*>::iterator iter = worklist.begin(); iter != worklist.end(); ++iter){
          llvm::Instruction* instr = *iter;

          if (!(instr->getOpcode() == llvm::Instruction::Add 
              || instr->getOpcode() == llvm::Instruction::Sub
              || instr->getOpcode() == llvm::Instruction::Mul
              || instr->getOpcode() == llvm::Instruction::And
              || instr->getOpcode() == llvm::Instruction::Or
              || instr->getOpcode() == llvm::Instruction::Xor))
            continue;

          llvm::errs() << "OPERANDS (live-ins):\n";
          for (llvm::Use& use : instr->operands()) {
            llvm::errs() << *use.get() << "\n"; 
          }

          llvm::errs() << "INSTRUCTION: " <<*instr << "\n"; 
          
          llvm::errs() << "USERS (live-outs):\n";
          for (llvm::User* user : instr->users()) {
            llvm::errs() << *user << "\n"; 
          }

          llvm::errs() << "\n";

          /*
          for(llvm::Value::use_iterator i = instr->use_begin(), ie = instr->use_end(); i!=ie; ++i){
            llvm::Value *v = *i;
            llvm::Instruction *vi = llvm::dyn_cast<llvm::Instruction>(*i);
            llvm::errs() << "\t\t" << *vi << "\n"; 
          }*/ 
        } 
       /* 
        // use-def chain for Instruction 
        for(std::vector<llvm::Instruction*>::iterator iter = worklist.begin(); iter != worklist.end(); ++iter){
          llvm::Instruction* instr = *iter;
          llvm::errs() << "use: " <<*instr << "\n"; 
          for (llvm::User::op_iterator i = instr -> op_begin(), e = instr -> op_end(); i != e; ++i) {
            llvm::Value *v = *i;
            llvm::Instruction *vi = llvm::dyn_cast<llvm::Instruction>(*i);
            llvm::errs() << "\t\t" << *vi << "\n"; 
          } 
        }*/

        return false;
      }
  };
}

char DefUse::ID = 0;
static llvm::RegisterPass<DefUse> unused("def-use","DefUse");
// TODO I still don't understand the optional params to RegisterPass
