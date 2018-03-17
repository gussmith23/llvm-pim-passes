set(PASS_SOURCES dg-test.cc)
add_pass(DgTest ${PASS_SOURCES})

find_library(LLVMdg_LIBRARY LLVMdg /home/mdl/hfs5022/usr/local/lib/)
target_link_libraries(DgTest PRIVATE ${LLVMdg_LIBRARY})
target_include_directories(DgTest PRIVATE /home/mdl/hfs5022/usr/local/include/llvm-dg)
