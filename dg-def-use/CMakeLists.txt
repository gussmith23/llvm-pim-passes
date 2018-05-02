set(PASS_SOURCES ./dg-def-use.cc)
add_pass(DgDefUse ${PASS_SOURCES})

target_link_libraries(DgDefUse 
                        PRIVATE ${LLVMdg_LIBRARY} 
                        PRIVATE ${LLVMrd_LIBRARY} 
                        PRIVATE ${RD_LIBRARY})
target_include_directories(DgDefUse PRIVATE /home/mdl/hfs5022/usr/local/include/llvm-dg)
