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
       struct ori:public FunctionPass
       {
          static char ID;
          ori():FunctionPass(ID){}
          bool runOnFunction(Function &F) override{
            for(Function::iterator bb = F.begin(), e = F.end(); bb!=e;bb++)
            {
              int j = 0; 
              int k = 0;
              //bool deadInst = true;
              //errs()<<*bb<<"\n";
              //for (BasicBlock::iterator i = bb->begin(), i2 = bb->end();i!=i2;i++)
              //for (BasicBlock::iterator i2 = bb->end(), i = bb->begin();i2!=i;i--)
                for (BasicBlock::iterator i2 = bb->end(),i = bb->begin();i2!=i;)
                               
              {
                
                //if (j==0)
                //{
                  i2--;
                //  j=1;
                //}
                //errs()<<*i2<<"decrementing instructions  \n";
                Instruction* inst = dyn_cast<Instruction>(i2); 
                //i2--;
                //i3 =i2;
                //errs()<<*i2<<"decrementing instructions  \n";
                            
                if(inst->getOpcode() == Instruction::Or)
                   {
                       Value *a = &(*inst->getOperand(0));
                       Value *b = &(*inst->getOperand(1));
                       Instruction* a1 = dyn_cast<Instruction>(a);
                       Instruction* b1 = dyn_cast<Instruction>(b);
                         errs()<< *a<<"operands \n";
                         errs()<< *b<<"operands \n";
                  	                   	                   	 
                  	 //if(a1->getOpcode() == Instruction::And || a1->getOpcode() == Instruction::Load)
                  	 if((a1->getOpcode() == Instruction::Load || b1->getOpcode() == Instruction::Load) || (a1->getOpcode() == Instruction::Load || b1->getOpcode() == Instruction::And) || (a1->getOpcode() == Instruction::Load || b1->getOpcode() == Instruction::Mul)|| (a1->getOpcode() == Instruction::And || b1->getOpcode() == Instruction::Load) )
                  	   {
                  	    
                  	         errs()<<k<<"\n";
                  	         for (User *W :inst->users())
                       		 {
                       		        //deadInst = false;                                              
                        	        errs()<<"entering users for inst"<<*W<<"\n";
                       		        
                        		if(Instruction *Inst2 = dyn_cast<Instruction>(W))
                          		 {
                           		  //if(Inst2->getOpcode() == Instruction::Add || Inst2->getOpcode() == Instruction::Store)
                           		  if(Inst2->getOpcode() == Instruction::Store)
                               			{
                               			 
                               			 errs()<< "before delete Inst2"<<*Inst2 <<"\n";
                               			 errs()<< "before delete Inst"<<*inst <<"\n";
                               			 Inst2->eraseFromParent();
                                		 errs()<< "after erase Inst2"<<*Inst2 <<"\n";
                                		 //inst->replaceAllUsesWith(UndefValue::get(inst->getType()));
                                		 //errs()<< "before erase Inst"<<*inst <<"\n";
                                		 //inst->eraseFromParent();	 
                                		 //errs()<< "checkpoint"<<*inst <<"\n";
                                		                        	  		
                                		 k =1;
                                		 errs()<<k<<"\n";
                                		                                 		   
                              			 }
                              		                               	            
   					  }
                          		 
                          	     
   				       break;              	     
                        	  }
                        	  
                        	  if (k == 1)
                        	  {
                        	  inst->replaceAllUsesWith(UndefValue::get(inst->getType()));
                        	  errs()<< "checkpoint"<<*inst <<"\n";       
                  	          inst->eraseFromParent(); 
                  	          //a1->eraseFromParent(); 
                          	  //b1->eraseFromParent();
                          	  break;
                  	         k = 0;
                  	          
                        	  }
                  	           
                  	                             	       
                  	    }
                  	    
                  	                  	           
                   }
                    
                   	                         
                            
              }
             
              
            }
                	/*for(Function::iterator bbb = F.begin(), ee = F.end(); bbb!=ee;bbb++)
                	 {
                	   int l = 0;
                	   bool deadInst = true;
                	   for (BasicBlock::iterator i3 = bbb->end(),i4 = bbb->begin();i3!=i4;)                               
              			{
              			
              			    if (l==0)
                			{
                 			 i3--;
                 			 l=1;
              				}
              			    Instruction* inst4 = dyn_cast<Instruction>(i3); 
              			    if (isInstructionTriviallyDead(inst4) == true)
              			    {
              			    
              			        errs()<<"test"<<"";
              			    	//if(deadInst && !inst4->isTerminator())
              			    	//if(deadInst)
                   		    	//{
                      		    	//errs()<<"test"<<"";
                      		    	inst4->eraseFromParent();
                      		    	 
                  		    }	
                  		    errs()<<"test123"<<"\n";
                  		    
                  		}
                  	 }*/	     
              			 
            return false;
          }
        };
}
  
  char ori::ID = 0;
  static RegisterPass<ori> X("ori", "this is a pass");

