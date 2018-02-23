// // This "for" loop will create a temporary variable before every constant integer used in the program.
// It also the copy the value of constant integer into that temporary variable. 
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Instructions.h"
//#include "string"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.def"

using namespace llvm;
using namespace std;
using std::string;

namespace {
    struct new_instruction_learn : public FunctionPass 
    {
    static char ID; 
    new_instruction_learn() : FunctionPass(ID) {}


bool runOnFunction(Function &F) override {
//bool runOnBasicBlock(BasicBlock &B) override {

	//errs() << "Hello: ";
        //errs() << F.getName() <<"\n";

    for(Function::iterator bb = F.begin(),e = F.end();bb!=e;++bb) 
     { 
        int counter=0;

        for(BasicBlock::iterator i = bb->begin(), i2 = bb->end(); i!=i2; i++) 
          {
             Instruction *inst = &*i; 
             if(i->getOpcode() == Instruction::Store)

            {
              Value *operand0 = inst->getOperand(0);
 
                     if(isa<ConstantInt>(operand0)) 
                     
                     {

                           int counter;
                           string temp = ((dyn_cast<ConstantInt>(operand0))->getValue()).toString(10,true);
                           Type* A = IntegerType::getInt32Ty(F.getContext()); 
                           string name = "t"+std::to_string(++counter); 


                         errs()<< "instruction address"<<&*inst <<"\n";

                           AllocaInst* variable = new AllocaInst(A,0x803c650,NULL,4,name,&*inst);
                           /*   BasicBlock::iterator i2 = i;     
                           i2--; 
                           Instruction *inst2 = &*i2; 
                           Value *tempzx = inst2; 
                           StoreInst *str = new StoreInst(operand0,variable); 
                           bb->getInstList().insert(i,str); 
               */
                    } 

           }

              /* else if(i->getOpcode() == Instruction::Add || i->getOpcode() == Instruction::Sub || i->getOpcode() == Instruction::Mul || i->getOpcode() == Instruction::SDiv || i->getOpcode() == Instruction::UDiv || i->getOpcode() == Instruction::SRem || i->getOpcode() == Instruction::URem )

                     { 
                        Instruction *inst = &*i; 
                        Value *operand0 = inst->getOperand(0); 
                        Value *operand1 = inst->getOperand(1); 
                          
                             if(isa<ConstantInt>(operand0))
                               {
                                 string temp = ((dyn_cast<ConstantInt>(operand0))->getValue()).toString(10,true); 
                                 Type* A = IntegerType::getInt32Ty(F.getContext()); 
                                 string name = "t"+std::to_string(++counter); 
                                 AllocaInst* variable = new AllocaInst(A,32,NULL,4,name,&*inst);
                                 StoreInst *str = new StoreInst(operand0,variable); 
                                 bb->getInstList().insert(i,str);
                               }

                                  if(isa<ConstantInt>(operand1))
                                   {
                                      string temp = ((dyn_cast<ConstantInt>(operand1))->getValue()).toString(10,true);
                                      Type* A = IntegerType::getInt32Ty(F.getContext()); 
                                      string name = "t"+std::to_string(++counter); 
                                      AllocaInst* variable = new AllocaInst(A,32,NULL,4,name,&*inst); 
                                      StoreInst *str = new StoreInst(operand1,variable); 
                                      bb->getInstList().insert(i,str);
                                   }
                     }   */

           }

       }
    }
  };
 }

char new_instruction_learn::ID = 0;
static RegisterPass<new_instruction_learn> X("new_instruction_learn", "new_instruction_learn of the program");

