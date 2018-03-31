#include <functional>
#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <numeric>

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
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

/**
 * This is a collection of instructions which ends with an operation and a
 * store. Furthermore, the result of the operation is not used again.
 * 
 * We use this struct to represent a sequence of instructions that's
 * "almost" a LoadLoadOpStore; that is, it would be a LoadLoadOpStore if
 * its operands were loads. We'd like to track this so that we can
 * motivate our later search for more complex patterns. I.e., if we can
 * show  that there are a lot of OpStores -- many more than there are
 * LoadLoadOpStores -- then this shows us we shouldn't be looking at just
 * LoadLoadOpStores.
 */
struct OpStore {
  OpStore(std::vector<llvm::Value*> inputs, llvm::Instruction* op, std::vector<llvm::Value*> stores)
    : inputs(inputs), op(op), stores(stores) {};

  friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const OpStore& llos) {
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

/**
 * The instructions which our Processing in Memory architecture can support
 * in-memory.
 */
std::set<unsigned int> offloadableOpcodes = {
  llvm::Instruction::Add,
  llvm::Instruction::Sub,
  llvm::Instruction::Mul,
  llvm::Instruction::And,
  llvm::Instruction::Or,
  llvm::Instruction::Xor,
  llvm::Instruction::AShr,
  llvm::Instruction::LShr,
  llvm::Instruction::Shl,
  llvm::Instruction::SDiv,
  llvm::Instruction::SRem,
  llvm::Instruction::UDiv,
  llvm::Instruction::URem,
};

/**
 * Find ld-ld-op-st patterns in a function.
 *
 * A ld-ld-op-st gets turned into a LoadLoadOpStore and appended to
 * foundLlos.
 *
 * An op-st pattern which isn't a ld-ld-op-st gets turned into an OpStore
 * and appended to foundOs.
 */
void findLoadLoadOpStore(llvm::Function& function,
                          std::vector<LoadLoadOpStore>& foundLlos,
                          std::vector<OpStore>& foundOs);
bool replaceLoadLoadOpStore(std::vector<LoadLoadOpStore> loadLoadOpStores);

class FindLdLdOpSt : public llvm::ModulePass {
  public:
    static char ID;

    FindLdLdOpSt() : llvm::ModulePass(ID) {}

    bool runOnModule(llvm::Module& module) override {
      for (llvm::Function& function : module.functions())
        findLoadLoadOpStore(function, lloss, oss);
      return false;
    }

    void print(llvm::raw_ostream& os, const llvm::Module* m) const override {

      os << "LD-LD-OP-ST DATA\n";

      std::map<unsigned int, unsigned long> histogram;
      for (LoadLoadOpStore llos : lloss) 
        histogram[llos.op->getOpcode()]++;

      os << "OPCODE HISTOGRAM (the opcode of the op in a ld-ld-op-st)\n";
      for (auto hist_it : histogram)
        os << llvm::Instruction::getOpcodeName(hist_it.first) << "\t" << hist_it.second << "\n";

      os << "\n";
      os << "OP-ST DATA (op-st = group of instructions that was nearly a ld-ld-op-st)\n";
      os << "\n";

      
      // Classify the types of the operands.
      unsigned long oneImmediateOperand = 0;
      unsigned long twoImmediateOperands = 0;

      // Get more detail about the immediates used.
      std::map<llvm::Type*, unsigned long> immediateHistogram;

      // Get more detail about the instructions which produce operand values.
      std::map<unsigned int, unsigned long> operandInstructionHistogram;

      for (OpStore opStore : oss) {

        unsigned int numImmediates = 0;
        for (auto input : opStore.inputs) {
          if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(input))
            operandInstructionHistogram[inst->getOpcode()]++;
          // TODO we assume that it's an immediate otherwise, which I'm not
          // sure is a safe assumption.
          else {
            numImmediates++;
            immediateHistogram[input->getType()]++;
          }
        }

        switch (numImmediates) {
          case 1: oneImmediateOperand++; break;
          case 2: twoImmediateOperands++; break;
        }
      }


      os << "OPERAND OPCODE HISTOGRAM (for operands that aren't loads)\n";
      for (auto hist_it : operandInstructionHistogram)
        os << llvm::Instruction::getOpcodeName(hist_it.first) << "\t" << hist_it.second << "\n";

      os << "\n";

      os << "Number of patterns with one immediate operand:\n"<< oneImmediateOperand << "\n";
      os << "Number of patterns with two immediate operands:\n"<< twoImmediateOperands << "\n";

      os << "\n";

      os << "IMMEDIATE TYPE HISTOGRAM (for operands that are immediates)\n";
      for (auto hist_it : immediateHistogram) {
        hist_it.first->print(os);
        os << "\t" << hist_it.second << "\n";
      }

    }
  private:
    std::vector<LoadLoadOpStore> lloss;
    std::vector<OpStore> oss;
};

class ReplaceLdLdOpSt : public llvm::FunctionPass {
  public:
    static char ID;

    ReplaceLdLdOpSt() : llvm::FunctionPass(ID) {}

    bool runOnFunction(llvm::Function& function) override {
      std::vector<LoadLoadOpStore> lloss;
      std::vector<OpStore> oss;
      findLoadLoadOpStore(function, lloss, oss);
      // Do something
      return false;
    }
};

void findLoadLoadOpStore(llvm::Function& function,
                          std::vector<LoadLoadOpStore>& foundLlos,
                          std::vector<OpStore>& foundOs) {
  std::vector<llvm::Instruction*> worklist;
  for(llvm::inst_iterator I = inst_begin(function), E = inst_end(function); I != E; ++I){
    worklist.push_back(&*I);
  }

  for(std::vector<llvm::Instruction*>::iterator iter = worklist.begin(); iter != worklist.end(); ++iter) {
    llvm::Instruction* instr = *iter;

    if (offloadableOpcodes.find(instr->getOpcode()) == offloadableOpcodes.end())
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

    // In this basic analysis, if the result of op is used by any instruction
    // other than a store, we want to throw it out.
    /* A fun way to do this using functional-style C++17:
     * Though I'm not gonna argue that it's better. It's definitely not more readable.
    std::vector<bool> usersAreStores;
    std::transform(instr->users().begin(), instr->users().end(),
                    usersAreStores.begin(),
                    [](llvm::User* user){return llvm::dyn_cast<llvm::StoreInst>(user);});
    std::reduce(usersAreStores.begin(), usersAreStores.end(), true, std::multiplies<bool>);
    */
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

      foundLlos.emplace_back(
        std::vector<llvm::Value*>(instr->operands().begin(), instr->operands().end()),
        instr,
        std::vector<llvm::Value*>(instr->users().begin(), instr->users().end())
      );
    } 
    // Make an OpStore if the operands aren't just loads.
    // Note that we still want the store(s) at the end to be the only
    // instruction(s) using the result of op.
    else if (!allLoads && allStores) {
      foundOs.emplace_back(
        std::vector<llvm::Value*>(instr->operands().begin(), instr->operands().end()),
        instr,
        std::vector<llvm::Value*>(instr->users().begin(), instr->users().end())
      );
      
    }
  } 

  // TODO i need to filter out by llos's whose inputs are only used by the op in the llos.
  // that is, if i'm going to replace l-l-o-s with one instruction in memory, i can't just
  // throw out loads whose values are used elsewhere.

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
static llvm::RegisterPass<FindLdLdOpSt> a("find-ld-ld-op-st","Find and print ld-ld-op-store patterns",
                                          true, // CFG only (TODO what is this?)
                                          true // analysis pass
                                          );

char ReplaceLdLdOpSt::ID = 1;
static llvm::RegisterPass<ReplaceLdLdOpSt> b("replace-ld-ld-op-st","Find and replace ld-ld-op-store patterns");
