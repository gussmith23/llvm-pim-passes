#include <iostream>

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Intrinsics.h"

namespace {

/***
 * Not a great name for this class.
 * Takes a number of inputs, which can be loads or immediates;
 * takes an operation;
 * takes a store.
 * Represents a chain which can be outsourced to PIM.
 */
struct LoadLoadOpStore {
    LoadLoadOpStore(std::vector<llvm::Value*> inputs, llvm::Instruction* op, std::vector<llvm::Value*> stores)
      : inputs(inputs), op(op), stores(stores) {};

    friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const LoadLoadOpStore& llos) {
      os << "Inputs:\n";
      for (llvm::Value* input : llos.inputs) os << "\t" << *input << "\n";
      os << "Operation:\n\t" << *llos.op << "\n";
      os << "Outputs:\n";
      for (llvm::Value* output : llos.stores) os << "\t" << *output << "\n";
      return os;
    }
  
    std::vector<llvm::Value*> inputs;
    llvm::Instruction* op;
    std::vector<llvm::Value*> stores;
};

std::vector<LoadLoadOpStore> findLoadLoadOpStore(llvm::Function& function);
bool replaceLoadLoadOpStore(std::vector<LoadLoadOpStore> loadLoadOpStores);

class FindLdLdOpSt : public llvm::FunctionPass {
  public:
    static char ID;

    FindLdLdOpSt() : llvm::FunctionPass(ID) {}

    bool runOnFunction(llvm::Function& function) override {
      for (LoadLoadOpStore llos : findLoadLoadOpStore(function)) 
        llvm::outs() << llos;
      return false;
    }
};

class ReplaceLdLdOpSt : public llvm::FunctionPass {
  public:
    static char ID;

    ReplaceLdLdOpSt() : llvm::FunctionPass(ID) {}

    bool runOnFunction(llvm::Function& function) override {
      std::vector<LoadLoadOpStore> lloss = findLoadLoadOpStore(function);
      // Do something
      return false;
    }
};

std::vector<LoadLoadOpStore> findLoadLoadOpStore(llvm::Function& function) {
  std::vector<llvm::Instruction*> worklist;
  for(llvm::inst_iterator I = inst_begin(function), E = inst_end(function); I != E; ++I){
    worklist.push_back(&*I);
  }

  std::vector<LoadLoadOpStore> foundLlos;

  for(std::vector<llvm::Instruction*>::iterator iter = worklist.begin(); iter != worklist.end(); ++iter) {
    llvm::Instruction* instr = *iter;

    if (!(instr->getOpcode() == llvm::Instruction::Add 
        || instr->getOpcode() == llvm::Instruction::Sub
        || instr->getOpcode() == llvm::Instruction::Mul
        || instr->getOpcode() == llvm::Instruction::And
        || instr->getOpcode() == llvm::Instruction::Or
        || instr->getOpcode() == llvm::Instruction::Xor))
      continue;

    // We use these to track different things about the operands.
    // We want to track whether the operands are all loads in the case
    // that we don't support immediates in memory (which we don't right
    // now).
    // We want to track whether te operands are all loads or immediates
    // once we support immediates.
    bool allLoads = true;
    bool allLoadsOrImmediates = true;
    bool allOperandsOnlyUsedByInstr = true;
    for (llvm::Use& use : instr->operands()) {
      llvm::errs() << "Use: " << *use << "\n";

      // First, before all else, check to see if this thing is used only by the op. 
      // If so, then we're good to replace it.
      // If not, then it could still be replaced, but requires a more complicated analysis.
      // TODO even in this simple state, i'm not sure this is actually right.
      // It could have 2 uses or more, but only becasue it's in two llos chains.
      if (use.get()->hasNUsesOrMore(2)) {
        allOperandsOnlyUsedByInstr = false;
        llvm::errs() << "One of the operands is used by at least one other instruction. Can't get rid of this operand.\n";
        break;
      }
      
      if (llvm::Instruction* useInstr = llvm::dyn_cast<llvm::Instruction>(use.get())) {
        if (useInstr->getOpcode() == llvm::Instruction::Load) {
        } else {
          llvm::errs() << "Found an operand that is an instruction other than Load: " << *use.get() << "\n";
          allLoadsOrImmediates = false;
          allLoads = false;
          break;
        }
      } else if (use->getType()->getTypeID() == llvm::Type::HalfTyID
                  || use->getType()->getTypeID() == llvm::Type::FloatTyID
                  || use->getType()->getTypeID() == llvm::Type::DoubleTyID
                  || use->getType()->getTypeID() == llvm::Type::IntegerTyID) {
        llvm::errs() << "Immediate found\n";
        allLoads = false;
      } else {
        llvm::errs() << "Found an operand that is of an unsupported type: " << use->getType() << "\n";
        allLoadsOrImmediates = false;
        allLoads = false;
        break;
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

    //if (allLoadsOrImmediates && allStores) 
    if (allLoads && allStores && allOperandsOnlyUsedByInstr) {
      llvm::errs() << "\nMaking load-load-op-store instruction sequence from\n"
                    << "Operands:\n";
      for (llvm::Use& use : instr->operands())
        llvm::errs() << "\t" << *use << "\n";
      llvm::errs() << "Operation:\n"
                    << "\t" << *instr << "\n";
      llvm::errs() << "Stores:\n";
      for (llvm::User* user : instr->users())
        llvm::errs() << "\t" << *user << "\n";

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

  // TODO i need to filter out by llos's whose inputs are only used by the op in the llos.
  // that is, if i'm going to replace l-l-o-s with one instruction in memory, i can't just
  // throw out loads whose values are used elsewhere.

  return foundLlos;
}

bool replaceLoadLoadOpStore(std::vector<LoadLoadOpStore> loadLoadOpStores) {
  
  // Replace instructions
  for (LoadLoadOpStore llos : loadLoadOpStores) {
    ///llvm::ReplaceInstWithInst(instr, //TODO add new instruction!
    
    // TODO we're indicating here that we expect it to ACTUALLY be a
    // load-load-op-store; that is, two loads and one store only.
    assert(llos.inputs.size() == 2);
    assert(llos.stores.size() == 1);

    llvm::Module* module = llos.op->getParent()->getParent()->getParent();

    llvm::Function* cacheOperandA = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::ID::cache_operand_a);
    llvm::Function* cacheOperandB = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::ID::cache_operand_b);
    llvm::Function* cacheDest = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::ID::cache_dest);
    
    llvm::CallInst* callCacheOperandA = llvm::CallInst::Create(cacheOperandA, llvm::ArrayRef<llvm::Value*>(llos.inputs[0]));
    llvm::CallInst* callCacheOperandB = llvm::CallInst::Create(cacheOperandB, llvm::ArrayRef<llvm::Value*>(llos.inputs[1]));
    llvm::CallInst* callCacheDest = llvm::CallInst::Create(cacheDest, llvm::ArrayRef<llvm::Value*>(llos.stores[0]));

    llos.op->getParent()->getInstList().insert(llvm::BasicBlock::iterator(llos.op), callCacheOperandA);
    llos.op->getParent()->getInstList().insert(llvm::BasicBlock::iterator(llos.op), callCacheOperandB);
    llvm::ReplaceInstWithInst(llos.op, callCacheDest);

    for (llvm::Value* input : llos.inputs) 
      if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(input)) inst->eraseFromParent();

    for (llvm::Value* output : llos.stores) 
      if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(output)) inst->eraseFromParent();
  }

  return loadLoadOpStores.size();
}

} // namespace

char FindLdLdOpSt::ID = 0;
static llvm::RegisterPass<FindLdLdOpSt> a("find-ld-ld-op-st","Find and print ld-ld-op-store patterns");

char ReplaceLdLdOpSt::ID = 1;
static llvm::RegisterPass<ReplaceLdLdOpSt> b("replace-ld-ld-op-st","Find and replace ld-ld-op-store patterns");
