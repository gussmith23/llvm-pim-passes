#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;


namespace {
    struct opcode_count : public FunctionPass 
    {
    static char ID; 
    opcode_count() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
     std::map<std::string,int> opcode_map;
	errs() << "Hello: ";
        errs() << F.getName() <<"\n";
        for (Function::iterator bb = F.begin(), e = F.end();bb!=e;bb++)
        {
          for (BasicBlock::iterator i = bb->begin(), i2 = bb->end(); i!=i2;i++)
          {
	      if (opcode_map.find(i->getOpcodeName()) == opcode_map.end())
              {
                 opcode_map[i->getOpcodeName()] = 1;
              }
              else
              { 
                 opcode_map[i->getOpcodeName()]+=1;
              }
           }
	}

        std::map<std::string,int>::iterator i=opcode_map.begin();
        std::map<std::string,int>::iterator e = opcode_map.end();
        while (i!=e)
        {
           errs()<<i->first << " ::: " <<i->second<<"\n";
           i++;

        }
        opcode_map.clear();

      }
 };
}

      
char opcode_count::ID = 0;
static RegisterPass<opcode_count> X("opcode_count", "opcode_count of the program");


