#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stack>
#include <unordered_map>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "pim-instructions-flag.h"

namespace {

/**
 * Determine whether an LLVM value (e.g. an instruction or immediate) can be
 * offloaded to memory.
 */
bool canOffload(llvm::Value* value);
std::set<unsigned int> offloadableInstructions = {
    llvm::Instruction::Add,           llvm::Instruction::Sub,
    llvm::Instruction::And,           llvm::Instruction::Or,
    llvm::Instruction::Xor,           llvm::Instruction::Mul,
    llvm::Instruction::BinaryOps::Shl};

/**
 * Represents a connected subgraph of nodes, where each of the nodes can be
 * offloaded to memory.
 */
class PimSubgraph {
 public:
  static std::unordered_map<llvm::Value*, std::shared_ptr<PimSubgraph>>
      pimSubgraphs;

  /**
   * First checks whether node is in pimSubgraphs, so as not to duplicate work.
   * If the node isn't in pimSubgraphs, this function constructs a new
   * PimSubgraph, inserts it into pimSubgraphs, and returns it.
   * Expands outward from node, consuming all offloadable instructions to create
   * a PimSubgraph.
   */
  static std::shared_ptr<PimSubgraph> getSubgraphFromValue(llvm::Value* value);

  // The offloadable instructions interior to the graph.
  std::set<llvm::Value*> values;

  // The nodes on the frontier of the graph.
  // Rear frontier nodes are nodes with at least one operand that cannot be
  // offloaded to memory.
  // Similarly, frontier nodes are nodes with at least one user that can't be
  // offloaded to memory.
  std::set<llvm::Value*> rearFrontier, frontier;

  friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                       const PimSubgraph& ps) {
    os << "Rear frontier:\n";
    for (auto i : ps.rearFrontier) {
      os << *i << "\n";
    }
    os << "Subgraph:\n";
    for (auto i : ps.values) {
      os << *i << "\n";
    }
    os << "Frontier:\n";
    for (auto i : ps.frontier) {
      os << *i << "\n";
    }
    return os;
  }

 private:
  PimSubgraph() {}

  /**
   * dir: the direction we were traveling when this call to probe() occured. If
   * this node can't be offloaded to memory, this determines whether the node
   * gets added to the frontier or the rear frontier.
   */
  enum TRAVERSE_DIR { BACKWARD, FORWARD, INIT };
  void probe(llvm::Value* value, TRAVERSE_DIR dir = INIT);
};
// See https://stackoverflow.com/questions/9282354/static-variable-link-error
std::unordered_map<llvm::Value*, std::shared_ptr<PimSubgraph>>
    PimSubgraph::pimSubgraphs;

/**
 * Implements a simple (and thus slow) graph isomorphism check between two
 * PimSubgraphs.
 */
bool operator==(const PimSubgraph& lhs, const PimSubgraph& rhs);
bool operator==(std::shared_ptr<PimSubgraph> lhs,
                std::shared_ptr<PimSubgraph> rhs) {
  return *rhs == *lhs;
}

/***
 * A more complex Def-Use pass built with the dg library.
 */
class PimSubgraphPass : public llvm::ModulePass {
 public:
  static char ID;
  PimSubgraphPass() : llvm::ModulePass(ID) {}

  bool runOnModule(llvm::Module& m) override {
    std::set<unsigned int> pimInstrsFromFlags = getPimInstructions();
    if (pimInstrsFromFlags.size())
      offloadableInstructions = std::move(pimInstrsFromFlags);

    for (auto& f : m) {
      for (auto& bb : f) {
        for (auto& i : bb) {
          if (canOffload(&i)) sgs.insert(PimSubgraph::getSubgraphFromValue(&i));
        }
      }
    }

    return false;
  }

  void print(llvm::raw_ostream& os, const llvm::Module* m) const override;

 private:
  std::set<std::shared_ptr<PimSubgraph>> sgs;
};

std::shared_ptr<PimSubgraph> PimSubgraph::getSubgraphFromValue(
    llvm::Value* value) {
  if (pimSubgraphs.count(value)) return pimSubgraphs[value];

  // Because constructor is private, we can't use make_shared.
  std::shared_ptr<PimSubgraph> sg =
      std::shared_ptr<PimSubgraph>(new PimSubgraph());
  sg->probe(value);

  for (llvm::Value* value : sg->values) pimSubgraphs[value] = sg;
  for (llvm::Value* value : sg->rearFrontier) pimSubgraphs[value] = sg;
  for (llvm::Value* value : sg->frontier) pimSubgraphs[value] = sg;

  return std::move(sg);
}

void PimSubgraph::probe(llvm::Value* value, TRAVERSE_DIR dir) {
  // TODO should this happen?
  if (values.count(value)) return;

  if (canOffload(value)) {
    values.insert(value);
  } else if (dir == BACKWARD) {
    rearFrontier.insert(value);
    return;
  } else if (dir == FORWARD) {
    frontier.insert(value);
    return;
  } else {
    // TODO should we be able to get here?
    // It depends on how we call the function. If it's called on a node that
    // can't be offloaded, then we can get here. I don't intend to use it that
    // way, so this should be unreachable.
    assert(false);
    return;
  }

  if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(value)) {
    for (auto& use : inst->operands()) {
      probe(use, BACKWARD);
    }
  }
  for (auto user : value->users()) {
    probe(user, FORWARD);
  }
}

bool canOffload(llvm::Value* value) {
  if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(value))
    if (offloadableInstructions.count(inst->getOpcode())) return true;
  return false;
}

/**
 * For a group of llvm::Value*, update two histograms:
 * - The opcode histogram, for those values which are instructions.
 * - the immediate type histogram, for the rest (which we assume are immediates)
 * Does not clear the histograms beforehand; thus, this can be used to either
 * create a new histogram, or update a previous histogram.
 */
void updateHists(const std::set<llvm::Value*>& values,
                 std::map<unsigned int, unsigned long>& opcodeHist,
                 std::map<llvm::Type*, unsigned long>& immedateTypeHist) {
  for (auto i : values) {
    if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(i))
      opcodeHist[inst->getOpcode()]++;
    else
      immedateTypeHist[i->getType()]++;
  }
}

void printHists(const std::map<unsigned int, unsigned long>& opcodeHist,
                const std::map<llvm::Type*, unsigned long>& immedateTypeHist,
                const std::string& opcodeHistHeading,
                const std::string& immedateTypeHistHeading,
                llvm::raw_ostream& os) {
  // Print
  os << opcodeHistHeading << "\n";
  for (auto hist_it : opcodeHist)
    os << llvm::Instruction::getOpcodeName(hist_it.first) << "\t"
       << hist_it.second << "\n";
  os << "\n";
  os << immedateTypeHistHeading << "\n";
  for (auto hist_it : immedateTypeHist) {
    hist_it.first->print(os);
    os << "\t" << hist_it.second << "\n";
  }
}

void sgToDotGraph(const std::shared_ptr<PimSubgraph> sg, llvm::raw_ostream& os);
void PimSubgraphPass::print(llvm::raw_ostream& os,
                            const llvm::Module* m) const {
  os << "TOTAL NUMBER OF SUBGRAPHS\n" << sgs.size() << "\n";

  std::map<unsigned int, unsigned long> rearFrontierOpcodeHist, opcodeHist,
      frontierOpcodeHist;
  std::map<llvm::Type*, unsigned long> rearFrontierImmedateTypeHist,
      immedateTypeHist, frontierImmediateTypeHist;

  // This histogram tracks the number of instructions actually offloaded (i.e.
  // sg->values) per subgraph.
  std::map<unsigned long, unsigned long> subgraphSizeHist;

  // Maps size of sg->values to PimSubgraphs, so that we can easily find the
  // largest/smallest subgraphs.
  std::multimap<unsigned long, std::shared_ptr<PimSubgraph>>
      offloadSizeToSubgraph;

  for (auto sg : sgs) {
    subgraphSizeHist[sg->values.size()]++;

    // TODO could be updated to newer C++14 syntax with make_pair
    std::pair<unsigned long, std::shared_ptr<PimSubgraph>> p(sg->values.size(),
                                                             sg);
    offloadSizeToSubgraph.insert(p);

    updateHists(sg->rearFrontier, rearFrontierOpcodeHist,
                rearFrontierImmedateTypeHist);
    updateHists(sg->values, opcodeHist, immedateTypeHist);
    updateHists(sg->frontier, frontierOpcodeHist, frontierImmediateTypeHist);
  }

  std::vector<std::pair<std::shared_ptr<PimSubgraph>, unsigned long>>
      subgraphClasses;
  for (auto sg : sgs) {
    bool matched = false;
    for (auto& sg_other_it : subgraphClasses) {
      if (*sg == *sg_other_it.first) {
        sg_other_it.second++;
        matched = true;
        break;
      }
    }
    if (!matched) {
      subgraphClasses.emplace_back(sg, 1);
    }
  }
  os << "\n";
  os << "SUBGRAPH CLASSES\n";
  os << "\n";
  os << "Total unique subgraph classes:\n";
  os << subgraphClasses.size() << "\n";
  os << "\n";

  auto cmpCount =
      [](std::pair<std::shared_ptr<PimSubgraph>, unsigned long>& lhs,
         std::pair<std::shared_ptr<PimSubgraph>, unsigned long>& rhs) {
        return lhs.second > rhs.second;
      };
  auto cmpSubgraphSize =
      [](std::pair<std::shared_ptr<PimSubgraph>, unsigned long>& lhs,
         std::pair<std::shared_ptr<PimSubgraph>, unsigned long>& rhs) {
        return (lhs.first->rearFrontier.size() + lhs.first->values.size() +
                lhs.first->frontier.size()) >
               (rhs.first->rearFrontier.size() + rhs.first->values.size() +
                rhs.first->frontier.size());
      };

  sort(subgraphClasses.begin(), subgraphClasses.end(), cmpCount);
  os << "BY NUMBER OF INSTANCES:\n";
  const int numToPrint = 10;
  int i = 0;
  for (const auto it : subgraphClasses) {
    os << "Count:\n" << it.second << "\nSubgraph:\n";
    sgToDotGraph(it.first, os);
    os << "\n";
    if (++i > numToPrint) break;
  }
  sort(subgraphClasses.begin(), subgraphClasses.end(), cmpSubgraphSize);
  os << "\n";
  os << "BY SUBGRAPH SIZE:\n";
  i = 0;
  for (const auto it : subgraphClasses) {
    os << "Count:\n" << it.second << "\nSubgraph:\n";
    sgToDotGraph(it.first, os);
    os << "\n";
    if (++i > numToPrint) break;
  }

  os << "SUBGRAPH SIZE HISTOGRAM\n";
  for (auto hist_it : subgraphSizeHist)
    os << hist_it.first << "\t" << hist_it.second << "\n";

  os << "\n";

  os << "LARGEST SUBGRAPHS:\n";
  constexpr int numSgToPrint = 10;
  auto it = offloadSizeToSubgraph.rbegin();
  for (int i = 0; it != offloadSizeToSubgraph.rend() && i < numSgToPrint;
       it++, i++)
    os << *it->second << "\n";

  os << "\n";

  printHists(rearFrontierOpcodeHist, rearFrontierImmedateTypeHist,
             "REAR FRONTIER OPERAND HISTOGRAM",
             "REAR FRONTIER IMMEDIATE TYPE HISTOGRAM", os);
  os << "\n";
  printHists(opcodeHist, immedateTypeHist, "SUBGRAPH OPERAND HISTOGRAM",
             "SUBGRAPH IMMEDIATE TYPE HISTOGRAM", os);
  os << "\n";
  // Note that there shouldn't be any immediates on the frontier (because
  // immediates aren't instructions, and don't take any operands)
  printHists(frontierOpcodeHist, frontierImmediateTypeHist,
             "FRONTIER OPERAND HISTOGRAM", "FRONTIER IMMEDIATE TYPE HISTOGRAM",
             os);
  os << "\n";
}

/**
 * Determines whether two values are equivalent for the purposes of finding
 * graph isomorphism.
 * Note that this is specific to our subgraphs, which should only have
 * instructions and immediates.
 * For our purposes, this just means:
 * - If they're instructions, they have the same opcode.
 * - If they're immediates, they have the same type.
 */
bool compareSubgraphValues(const llvm::Value* lhs, const llvm::Value* rhs) {
  // If they point to the same value, this is trivially true.
  // In practice, this check shouldn't ever be true, at least not in the
  // isomorphism algorithm we're implementing.
  if (lhs == rhs) return true;

  if (const llvm::Instruction* lhsInst =
          llvm::dyn_cast<llvm::Instruction>(lhs)) {
    if (const llvm::Instruction* rhsInst =
            llvm::dyn_cast<llvm::Instruction>(rhs)) {
      // If they're both instructions, just compare their opcodes.
      return lhsInst->getOpcode() == rhsInst->getOpcode();
    }
    // If one's an instruction and the other's not -- obviously not equal!
    else
      return false;
  }
  // Else, they should be Immediates. We use ConstantData here, because I think
  // that's the most specific subtype I can use.
  // I could also use ConstantInt here, assuming we don't offload FP
  // instructions, but who's to say we won't try to do that at some point?
  else if (const llvm::ConstantData* lhsImm =
               llvm::dyn_cast<llvm::ConstantData>(lhs)) {
    if (const llvm::ConstantData* rhsImm =
            llvm::dyn_cast<llvm::ConstantData>(rhs)) {
      return lhsImm->getType() == rhsImm->getType();
    } else
      return false;
  }
  llvm::errs() << *rhs << "\n" << *lhs;
  llvm_unreachable(
      "Tried to compare two values that aren't instructions or immediates!");
  return false;
}

// Recurrence relation:
// There's a compatible pairing of lhs values in worklist to rhs values iff
// we pop an item V from the worklist and, for at least one compatible pairing
// of V with a V' in rhs, there is a compatible pairing of
// worklist-V UNION {users of V}.
//
// Algorithm:
// 1. Pop lhs value off of the worklist.
// 2. Identify all potential values in rhs which match this value.
//      This is done using whoAddedMe to get this value's parent, using lhsToRhs
//      to map that parent to the corresponding rhs value, and then looking at
//      the rhs value's children (users). The users are the potential matches.
//      If the value is in the rear frontier (and thus doesn't have a parent who
//      added it), it instead iterates over all rhs rear frontier nodes.
// 3. For each potential match:
//    a. Identify whether the match is compatible. If not, continue to next
//        iteration.
//    b. Add the match to lhsToRhs.
//    c. Update usedRhsValues.
//    d. For each user of this lhs value:
//      i. Add user to worklist.
//      ii. Update whoAddedMe.
//    e. Recurse. Return true if recursion returns true.
// 4. If no match is found, return false. NOTE: if the lhs value is in the
//      rear frontier, it must add itself to the worklist again before
//      returning. All other values will be added to the worklist by their
//      parents, and thus visited in a later recursion. As the rear frontier
//      values don't have parents, they must do this manually.
bool recursivelyFindMatchingWalk2(
    const PimSubgraph& lhs, const PimSubgraph& rhs,
    std::set<std::tuple<const llvm::Value*, const llvm::Value*>>& worklist,
    std::map<const llvm::Value*, const llvm::Value*>& lhsToRhs,
    std::set<const llvm::Value*>& usedRhsValues) {
  // Success condition.
  if (worklist.empty()) return true;

  // Get the top worklist item and unpack it.
  std::tuple<const llvm::Value*, const llvm::Value*> worklistItem =
      *worklist.begin();
  worklist.erase(worklistItem);
  const llvm::Value* lhsVal = std::get<0>(worklistItem);
  const llvm::Value* lhsParent = std::get<1>(worklistItem);

  assert(lhsVal);

  // TODO remove
  llvm::errs() << "current lhsval:" << *lhsVal << "\n";

  // If we're matched already
  if (lhsToRhs.count(lhsVal)) {
    // TODO remove
    llvm::errs() << "matched\n";

    const llvm::Value* alreadyMatchedRhsVal = lhsToRhs[lhsVal];

    // If we've been matched already, it means one of our other parents (not
    // lhsParent) added us, and we were already processed.
    // We simply now need to confirm that whatever rhs value we were matched
    // to can be reached from the rhs value corresponding to our parent.

    // If we're already matched, we must have been added to the worklist at
    // least twice, meaning that we definitely have a parent.
    assert(lhsParent);
    // Furthermore, that parent must be matched (this is true of all parents,
    // if they exist.)
    const llvm::Value* rhsParent = lhsToRhs[lhsParent];

    for (const llvm::Value* rhsVal : rhsParent->users())
      // The potential match must actually be in rhs.
      if (rhs.values.count(const_cast<llvm::Value*>(rhsVal)) ||
          rhs.frontier.count(const_cast<llvm::Value*>(rhsVal)))
        if (rhsVal == alreadyMatchedRhsVal) return true;

    llvm::errs()
        << "Failed: wer're already matched but couldn't validate the match\n";
    return false;
  }

  // Otherwise, we're not matched
  else {
    // TODO remove
    llvm::errs() << "not matched\n";

    // To find a match, we have to search through potential matches in rhs.
    // If we're a rear frontier node, we must search the rear frontier.
    // Otherwise, if we're not a rear frontier node (and thus we have a
    // parent), we must search lhsToRhs[lhsParent]->users().
    //
    // TODO this is a really bad way to generate this list, but I couldn't
    // figure out how else to do it without duplicating the main for loop.
    std::vector<const llvm::Value*> potentialMatches;
    if (lhsParent) {
      llvm::errs() << "has parent, adding from rhs parent users\n";
      for (const llvm::Value* potentialMatch : lhsToRhs[lhsParent]->users()) {
        if (rhs.values.count(const_cast<llvm::Value*>(potentialMatch)) ||
            rhs.frontier.count(const_cast<llvm::Value*>(potentialMatch)))
          potentialMatches.push_back(potentialMatch);
      }
    } else {
      llvm::errs() << "no parent, adding from rhs rearfrontier\n";
      for (auto potentialMatch : rhs.rearFrontier) {
        potentialMatches.push_back(potentialMatch);
      }
    }

    for (const llvm::Value* rhsVal : potentialMatches) {
      // Cant match with a value that's already been used.
      if (usedRhsValues.count(rhsVal)) continue;
      // Can't match with a value if we aren't like them (same op, same
      // immediate)
      if (!compareSubgraphValues(lhsVal, rhsVal)) continue;

      // If we get here, then we can match with this value!

      // TODO delete
      llvm::errs() << "matched with: " << *rhsVal << "\n";

      // Match.
      lhsToRhs[lhsVal] = rhsVal;
      usedRhsValues.insert(rhsVal);
      std::vector<std::tuple<const llvm::Value*, const llvm::Value*>>
          worklistAdditions;
      // If we're not in the frontier, it means there are nodes which come
      // after us which must be added to the worklist.
      // We keep track of the items we add to the worklist, because they may
      // need to be removed later if this configuration doesn't work out.
      if (lhs.frontier.count(const_cast<llvm::Value*>(lhsVal)) == 0) {
        for (const llvm::Value* user : lhsVal->users()) {
          // If we're not on the frontier, then all of our users are either
          // internal or on the frontier. so we don'te have to check.
          // TODO is this right?
          // IT'S WRONG! Immediates are static, so their users are all over the
          // program. Thus, we have to make sure that we're only adding things
          // that are in the PimSubgraph.
          if (lhs.values.count(const_cast<llvm::Value*>(user)) ||
              lhs.frontier.count(const_cast<llvm::Value*>(user))) {
            std::tuple<const llvm::Value*, const llvm::Value*> newItem(user,
                                                                       lhsVal);
            worklistAdditions.push_back(newItem);
            worklist.insert(newItem);
          }
        }
      }

      // Recurse.
      if (recursivelyFindMatchingWalk2(lhs, rhs, worklist, lhsToRhs,
                                       usedRhsValues)) {
        llvm::errs() << "success!\n";
        return true;
      }

      // If it failed, we have to undo our state and then continue on to the
      // next rhsVal.
      lhsToRhs.erase(lhsVal);
      usedRhsValues.erase(rhsVal);
      for (auto item : worklistAdditions) worklist.erase(item);
    }
    // If we couldn't match, then return false.
    // First, though, if we're a rear frontier node, we have to add ourselves
    // back to the worklist.
    if (!lhsParent) worklist.emplace(lhsVal, nullptr);
    llvm::errs() << "failed: we couldn't match.\n";
    return false;
  }  // End else (not matched)
}

bool doIt(const PimSubgraph& lhs, const PimSubgraph& rhs) {
  // TODO debug, remove
  llvm::errs() << "comparing:\n" << lhs << "and\n" << rhs << "\n";

  std::set<std::tuple<const llvm::Value*, const llvm::Value*>> worklist;
  for (auto v : lhs.rearFrontier) worklist.emplace(v, nullptr);
  std::map<const llvm::Value*, const llvm::Value*> lhsToRhs;
  std::set<const llvm::Value*> usedRhsValues;
  std::map<const llvm::Value*, const llvm::Value*> whoAddedMe;

  return recursivelyFindMatchingWalk2(lhs, rhs, worklist, lhsToRhs,
                                      usedRhsValues);
}

bool operator==(const PimSubgraph& lhs, const PimSubgraph& rhs) {
  // STAGE 1: simple checks to try to weed out non-isomorphisms early.
  // Feel free to add more and more checks into this stage. Anything to weed
  // out early -- because otherwise, we just end up doing a brute-force
  // search.

  if (&lhs == &rhs) return true;

  if (lhs.values.size() != rhs.values.size() ||
      lhs.rearFrontier.size() != rhs.rearFrontier.size() ||
      lhs.rearFrontier.size() != rhs.rearFrontier.size())
    return false;

  {
    // Compare the distributions of opcodes and immediate types in the rear
    // frontier, core values, and frontier of the subgraph.
    // TODO i need to solidify my terminology...specifically on "values"
    std::map<unsigned int, unsigned long> opcodeHistLhs, opcodeHistRhs;
    std::map<llvm::Type*, unsigned long> immedateTypeHistLhs,
        immedateTypeHistRhs;

    updateHists(lhs.rearFrontier, opcodeHistLhs, immedateTypeHistLhs);
    updateHists(rhs.rearFrontier, opcodeHistRhs, immedateTypeHistRhs);
    if (opcodeHistLhs != opcodeHistRhs) return false;
    if (immedateTypeHistLhs != immedateTypeHistRhs) return false;

    opcodeHistLhs.clear();
    immedateTypeHistLhs.clear();
    opcodeHistRhs.clear();
    immedateTypeHistRhs.clear();

    updateHists(lhs.values, opcodeHistLhs, immedateTypeHistLhs);
    updateHists(rhs.values, opcodeHistRhs, immedateTypeHistRhs);
    if (opcodeHistLhs != opcodeHistRhs) return false;
    if (immedateTypeHistLhs != immedateTypeHistRhs) return false;

    opcodeHistLhs.clear();
    immedateTypeHistLhs.clear();
    opcodeHistRhs.clear();
    immedateTypeHistRhs.clear();

    updateHists(lhs.frontier, opcodeHistLhs, immedateTypeHistLhs);
    updateHists(rhs.frontier, opcodeHistRhs, immedateTypeHistRhs);
    if (opcodeHistLhs != opcodeHistRhs) return false;
    if (immedateTypeHistLhs != immedateTypeHistRhs) return false;
  }

  // Brute force isomorphism check
  return doIt(lhs, rhs);
}

void printValueDeclaration(const llvm::Value* val, llvm::raw_ostream& os) {
  if (const llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(val)) {
    std::stringstream ss;
    os << "\t" << (unsigned long)val << " [label=\"" << inst->getOpcodeName()
       << "\"];\n";
  } else if (const llvm::ConstantData* imm =
                 llvm::dyn_cast<llvm::ConstantData>(val)) {
    os << "\t" << (unsigned long)val << " [label=\"";
    imm->getType()->print(os);
    os << "\"];\n";
  } else
    llvm_unreachable(
        "shouldn't get here -- should only work with insts and immediates");
}
void sgToDotGraph(const std::shared_ptr<PimSubgraph> sg,
                  llvm::raw_ostream& os) {
  os << "digraph G {\n";
  for (const llvm::Value* val : sg->rearFrontier) {
    for (const llvm::Value* user : val->users()) {
      if (sg->frontier.count(const_cast<llvm::Value*>(user)) ||
          sg->values.count(const_cast<llvm::Value*>(user))) {
        printValueDeclaration(val, os);
        printValueDeclaration(user, os);
        os << "\t" << (unsigned long)val << " -> " << (unsigned long)user
           << ";\n";
      }
    }
  }
  for (const llvm::Value* val : sg->values) {
    for (const llvm::Value* user : val->users()) {
      if (sg->frontier.count(const_cast<llvm::Value*>(user)) ||
          sg->values.count(const_cast<llvm::Value*>(user))) {
        printValueDeclaration(val, os);
        printValueDeclaration(user, os);
        os << "\t" << (unsigned long)val << " -> " << (unsigned long)user
           << ";\n";
      }
    }
  }
  // Frontier should be printed  by the values.
  os << "}\n";
}

}  // namespace

char PimSubgraphPass::ID = 0;
static llvm::RegisterPass<PimSubgraphPass> unused("find-pim-subgraphs",
                                                  "Find PIM subgraphs", true,
                                                  true);
