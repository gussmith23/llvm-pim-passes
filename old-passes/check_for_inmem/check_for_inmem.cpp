#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;


namespace {
    struct check_for_inmem : public FunctionPass 
    {
    static char ID; 
    check_for_inmem() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
	errs() << "Hello: ";
        errs() << F.getName() <<"\n";
        int counter=0;
        int counter_and=0;
        int counter_or=0;
        int counter_xor=0;
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
                               counter = counter + 2;
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
       

        for (Function::iterator bbb = F.begin(), ee=F.end();bbb!=ee; bbb++)
        {
        for (BasicBlock::iterator i3 = bbb->begin(), i4=bbb->end(); i3!=i4; i3++)
            {
             Instruction* inst_and = dyn_cast<Instruction>(i3);
             if (inst_and->getOpcode() == Instruction::And)
              {
                  int succeeding;

                 for(User *UU: inst_and->users())
                     {
                      
                       if(Instruction *Inst_and = dyn_cast<Instruction>(UU))
                       {
                         errs()<<"Dependent instruction for AND  --> \n";
                         errs()<< *Inst_and << "\n";
                         succeeding = Inst_and->getOpcode();

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
                 for(Use &UU :inst_and->operands())
                     {
                      Value *vv = UU.get();
	             
                      errs()<<"source instructions for AND"<< *vv << "\n";
                      //int *BBB = *v;


                        if(Instruction *Inst1_and = dyn_cast<Instruction>(UU))
                       {
                         //errs()<<"Instruction:::::: ";
                         //errs()<< *Inst1 << "\n";
                         int preceeding = Inst1_and->getOpcode();

                          if ((succeeding == Instruction::Store) && (preceeding == Instruction::Load))
                              {
                               errs()<<"In memory instruction \n\n\n\n";
                               counter_and = counter_and + 2;
                               errs()<<"counter_and"<< counter_and << "\n";

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
                       
 
                      for (Function::iterator bbbb = F.begin(), eee=F.end();bbbb!=eee; bbbb++)
        {
        for (BasicBlock::iterator i5 = bbbb->begin(), i6=bbbb->end(); i5!=i6; i5++)
            {
             Instruction* inst_or = dyn_cast<Instruction>(i5);
             if (inst_or->getOpcode() == Instruction::Or)
              {
                  int succeeding;

                 for(User *UUU: inst_or->users())
                     {
                      
                       if(Instruction *Inst_or = dyn_cast<Instruction>(UUU))
                       {
                         errs()<<"Dependent instruction for OR  --> \n";
                         errs()<< *Inst_or << "\n";
                         succeeding = Inst_or->getOpcode();
                        
                       }
                     }
                 for(Use &UUU :inst_or->operands())
                     {
                      Value *vvv = UUU.get();
	             
                      errs()<<"source instructions for OR"<< *vvv << "\n";
                      //int *BBB = *v;


                        if(Instruction *Inst1_or = dyn_cast<Instruction>(UUU))
                       {
                         //errs()<<"Instruction:::::: ";
                         //errs()<< *Inst1 << "\n";
                         int preceeding = Inst1_or->getOpcode();

                          if ((succeeding == Instruction::Store) && (preceeding == Instruction::Load))
                              {
                               errs()<<"In memory instruction \n\n\n\n";
                               counter_or = counter_or + 2;
                               errs()<<"counter_or"<< counter_or << "\n";

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


        for (Function::iterator bbbbb = F.begin(), eeee=F.end();bbbbb!=eeee; bbbbb++)
        {
        for (BasicBlock::iterator i6 = bbbbb->begin(), i7=bbbbb->end(); i6!=i7; i6++)
            {
             Instruction* inst_xor = dyn_cast<Instruction>(i6);
             if (inst_xor->getOpcode() == Instruction::Xor)
              {
                  int succeeding;

                 for(User *UUUU: inst_xor->users())
                     {
                      
                       if(Instruction *Inst_xor = dyn_cast<Instruction>(UUUU))
                       {
                         errs()<<"Dependent instruction for XOR  --> \n";
                         errs()<< *Inst_xor << "\n";
                         succeeding = Inst_xor->getOpcode();
                        
                       }
                     }
                 for(Use &UUUU :inst_xor->operands())
                     {
                      Value *vvvv = UUUU.get();
	             
                      errs()<<"source instructions for XOR"<< *vvvv << "\n";
                      //int *BBB = *v;


                        if(Instruction *Inst1_xor = dyn_cast<Instruction>(UUUU))
                       {
                         //errs()<<"Instruction:::::: ";
                         //errs()<< *Inst1 << "\n";
                         int preceeding = Inst1_xor->getOpcode();

                          if ((succeeding == Instruction::Store) && (preceeding == Instruction::Load))
                              {
                               errs()<<"In memory instruction \n\n\n\n";
                               counter_xor = counter_xor + 2;
                               errs()<<"counter_xor"<< counter_xor << "\n";

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

      
char check_for_inmem::ID = 0;
static RegisterPass<check_for_inmem> X("check_for_inmem", "check_for_inmem of the program");


