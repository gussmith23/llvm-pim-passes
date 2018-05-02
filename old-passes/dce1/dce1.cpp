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
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.def"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/Local.h"


int flag=0;
using namespace llvm;

namespace 
{
       struct dce1:public FunctionPass
       {
          static char ID;
          dce1():FunctionPass(ID){}
          bool runOnFunction(Function &F) override{
            for(Function::iterator b = F.begin(), ee = F.end(); b!=ee;b++)
                	 {
                	   
                	   errs()<<*b<<"function\n";
                	   for (BasicBlock::iterator i3 = b->end(),i4 = b->begin();i3!=i4;)  
                	   //for (BasicBlock::iterator i3 = b->begin(),i4 = b->end();i3!=i4;)                              
              			{
              			
              			    //if (l==0)
                		    //	{
                 		  	 i3--;
                 		   //	 l=1;
              			   //	}
              			        errs()<<*i3<<"\n";
              				Instruction* inst4 = dyn_cast<Instruction>(i3); 
              			   //	i3--;
              				errs()<<*i3<<"\n"
;              			    
              			    if (isInstructionTriviallyDead(inst4))
              			    {
              			    
              			        errs()<<"test"<<"\n";
              			    	//if(deadInst && !inst4->isTerminator())
              			    	//if(deadInst)
                   		    	//{
                      		    	errs()<<*inst4<<"\n";
                      		    	inst4->eraseFromParent();
                      		    	//errs()<<*inst4<<"\n";
                      		    break;	 
                  		    }	
                  		    errs()<<"test123"<<"\n";
                  		   //break; 
                  		}
                  		break;
                  	 }
            
            
            	 
            return false;
          }
        };
}
  
  char dce1::ID = 0;
  static RegisterPass<dce1> X("dce1", "this is a pass");

