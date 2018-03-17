#include <iostream>

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"

namespace {

  /***
   * Not a great name for this class.
   * Takes a number of inputs, which can be loads or immediates;
   * takes an operation;
   * takes a store.
   * Represents a chain which can be outsourced to PIM.
   */
  class LoadLoadOpStore {
    public:
      LoadLoadOpStore(std::vector<llvm::Value*> inputs, llvm::Value* op, std::vector<llvm::Value*> stores)
        : inputs(inputs), op(op), stores(stores) {};

      friend llvm::raw_ostream& operator<<(llvm::raw_ostream& ostream, const LoadLoadOpStore& llos) {
        ostream << "Inputs:\n";
        for (llvm::Value* input : llos.inputs) ostream << "\t" << *input << "\n";
        ostream << "Operation:\n\t" << *llos.op << "\n";
        ostream << "Outputs:\n";
        for (llvm::Value* output : llos.stores) ostream << "\t" << *output << "\n";
        return ostream;
      }
    
    private:
      std::vector<llvm::Value*> inputs;
      llvm::Value* op;
      std::vector<llvm::Value*> stores;
  };

  class DefUse : public llvm::FunctionPass {
    public:
      static char ID;
      DefUse() : llvm::FunctionPass(ID) {}

      bool runOnFunction(llvm::Function& function) {
        std::vector<llvm::Instruction*> worklist;
        for(llvm::inst_iterator I = inst_begin(function), E = inst_end(function); I != E; ++I){
          worklist.push_back(&*I);
        }

        std::vector<LoadLoadOpStore> foundLlos;

        for(std::vector<llvm::Instruction*>::iterator iter = worklist.begin(); iter != worklist.end(); ++iter){
          llvm::Instruction* instr = *iter;

          if (!(instr->getOpcode() == llvm::Instruction::Add 
              || instr->getOpcode() == llvm::Instruction::Sub
              || instr->getOpcode() == llvm::Instruction::Mul
              || instr->getOpcode() == llvm::Instruction::And
              || instr->getOpcode() == llvm::Instruction::Or
              || instr->getOpcode() == llvm::Instruction::Xor))
            continue;

          bool allLoadsOrImmediates = true;
          for (llvm::Use& use : instr->operands()) {
            if (llvm::Instruction* useInstr = llvm::dyn_cast<llvm::Instruction>(use.get())) {
              if (useInstr->getOpcode() == llvm::Instruction::Load) {
              } else {
                llvm::errs() << "Found an operand that is an instruction other than Load: " << *use.get() << "\n";
                allLoadsOrImmediates = false;
              }
            } else if (use->getType()->getTypeID() == llvm::Type::HalfTyID
                        || use->getType()->getTypeID() == llvm::Type::FloatTyID
                        || use->getType()->getTypeID() == llvm::Type::DoubleTyID
                        || use->getType()->getTypeID() == llvm::Type::IntegerTyID) {
              
            } else {
              llvm::errs() << "Found an operand that is of an unsupported type: " << use->getType() << "\n";
              allLoadsOrImmediates = false;
            }
          }

          bool allStores = true;
          for (llvm::User* user : instr->users()) {
            if (llvm::dyn_cast<llvm::StoreInst>(user)) {
            } else {
              llvm::errs() << "Found a user which isn't a store: " << *user << "\n"; 
              allStores = false;
            }
          }

          if (allLoadsOrImmediates && allStores) {
            foundLlos.push_back(
                LoadLoadOpStore(
                  std::vector<llvm::Value*>(instr->operands().begin(), instr->operands().end()),
                  instr,
                  std::vector<llvm::Value*>(instr->users().begin(), instr->users().end())) 
                );
          }
        } 

        for (LoadLoadOpStore llos : foundLlos) {
          llvm::errs() << llos << "\n";
        }

        return false;
      }
  };
}

char DefUse::ID = 0;
static llvm::RegisterPass<DefUse> unused("def-use","DefUse");
// TODO I still don't understand the optional params to RegisterPass
