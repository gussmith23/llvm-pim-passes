#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;


namespace {
    struct defuse : public FunctionPass 
    {
    static char ID; 
    defuse() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
	errs() << "Hello: ";
        errs() << F.getName() <<"\n";
        int counter=0;
    for (Function::iterator bb = F.begin(), e=F.end();bb!=e; bb++)
        {
        for (BasicBlock::iterator i = bb->begin(), i2=bb->end(); i!=i2; i++)
            {
             Instruction* inst = dyn_cast<Instruction>(i);
             if (inst->getOpcode() == Instruction::Add)
              {
                  int succeeding;

                 for(User *U: inst->users())
                     {
                      
                       if(Instruction *Inst = dyn_cast<Instruction>(U))
                       {
                         errs()<<"Dependent instruction  --> \n";
                         errs()<< *Inst << "\n";
                         succeeding = Inst->getOpcode();

                          // if (succeeding == Instruction::Store)
                          //    {
                          //     errs()<<"In memory instruction \n";
                          //    }
                          // else 
                          //    { 
                          //     errs()<<"Not In memory instruction \n";
                          //    }
 
                       }
                     }
                 for(Use &U :inst->operands())
                     {
                      Value *v = U.get();
	             
                      errs()<<"source instructions"<< *v << "\n";
                      //int *BBB = *v;


                        if(Instruction *Inst1 = dyn_cast<Instruction>(U))
                       {
                         //errs()<<"Instruction:::::: ";
                         //errs()<< *Inst1 << "\n";
                         int preceeding = Inst1->getOpcode();

                          if ((succeeding == Instruction::Store) && (preceeding == Instruction::Load))
                              {
                               errs()<<"In memory instruction \n\n\n\n";
                               counter = counter + 3;
                               errs()<<"counter"<< counter << "\n";

                             }
                           else 
                              { 
                               errs()<<"Not In memory instruction \n\n\n\n";
                               break;
                              }
 
                       }
                               
            
                      }
                                //counter = counter + 3;
                               //errs()<<"counter"<< counter << "\n"; 
                 
                } 
                               
            }
        }
        
     return false; 

     }
    
    
    
 };
}

      
char defuse::ID = 0;
static RegisterPass<defuse> X("defuse", "defuse of the program");


