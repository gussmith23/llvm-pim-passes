#include <iostream>
#include <set>
#include <memory>

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/LLVMDependenceGraph.h>
#include <llvm/analysis/PointsTo/PointsTo.h>
#include <llvm/analysis/ReachingDefinitions/ReachingDefinitions.h>
#include <llvm/analysis/DefUse.h>

void runOnDependenceGraph(dg::LLVMDependenceGraph* dg);
void expandOutwardsFromInstruction(dg::LLVMNode*);

std::set<unsigned int> supportedInstructions = {
  llvm::Instruction::Add,
  llvm::Instruction::Sub,
  llvm::Instruction::And,
  llvm::Instruction::Or,
  llvm::Instruction::Xor,
  llvm::Instruction::Mul,
};


namespace {

  /***
   * A more complex Def-Use pass built with the dg library.
   */
  class DgDefUse : public llvm::ModulePass {
    public:
      static char ID;
      DgDefUse() : llvm::ModulePass(ID) {}

      bool runOnModule(llvm::Module& module) {
        
        // We have to do pointer + reaching definition analysis before we can build a
        // graph with data dependencies.
        // Actually, it looks like the specific order is: pointer analysis, which
        // building the graph depends on. Then reaching definitions. Then def-use
        // analysis, which depends on built graph, pointer analysis, and reaching defs.
        
        std::unique_ptr<dg::LLVMPointerAnalysis> pta(new dg::LLVMPointerAnalysis(&module));
        // TODO just picked a setting at random -- which should we use?
        pta->run<dg::analysis::pta::PointsToFlowSensitive>();

        std::unique_ptr<dg::analysis::rd::LLVMReachingDefinitions> rd(new dg::analysis::rd::LLVMReachingDefinitions(&module, pta.get()));
        rd->run<dg::analysis::rd::ReachingDefinitionsAnalysis>();

        std::unique_ptr<dg::LLVMDependenceGraph> dg(new dg::LLVMDependenceGraph());
        dg->build(&module, pta.get());
        
        dg::LLVMDefUseAnalysis dua(dg.get(), rd.get(), pta.get());
        dua.run();

        runOnDependenceGraph(dg.get());

        return false;
      }
  };
}

std::set<dg::LLVMDependenceGraph*> seen;

void runOnDependenceGraph(dg::LLVMDependenceGraph* dg) {
  if (dg == nullptr) return;

  if (seen.find(dg) != seen.end())
    return;
  else
    seen.insert(dg);

  for (auto bb_it : dg->getBlocks() ) {
    for (dg::LLVMNode* node : bb_it.second->getNodes()) {
      
      // Depending on what kind of instruction it is, do something.
      
      // If it's a call, we have to recurse into its subgraphs.
      if (llvm::dyn_cast<llvm::CallInst>(node->getValue())) {
        for (auto subgraph : node->getSubgraphs())
          runOnDependenceGraph(subgraph);
      }

      else if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(node->getValue())) {
        if (supportedInstructions.find(inst->getOpcode()) != supportedInstructions.end()) {
          llvm::errs() << "Found supported instruction:\n" << *inst << "\n";
          expandOutwardsFromInstruction(node);
        }
      }
    }
  }
}

/***
 * "Expand" outwards from an instruction, "consuming" the adjacent instructions
 * which can be offloaded to memory as well.
 * This will create a subgraph of instructions which can be offloaded to
 * memory.
 */
void expandUpwards(dg::LLVMNode*);
void expandDownwards(dg::LLVMNode*);
void expandOutwardsFromInstruction(dg::LLVMNode* node) {
  expandUpwards(node);
}

void expandUpwards(dg::LLVMNode* node) {
  for (dg::LLVMNode::data_iterator it = node->rev_data_begin(); it != node->rev_data_end(); it++) {
    llvm::errs() << *(*it)->getValue() << "\n";
  }
}


char DgDefUse::ID = 0;
static llvm::RegisterPass<DgDefUse> unused("dg-def-use", "DG Def Use");
