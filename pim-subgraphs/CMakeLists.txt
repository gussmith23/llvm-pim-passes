set(PASS_SOURCES "./pim-subgraphs.cc")
add_pass(PimSubgraphs ${PASS_SOURCES})
target_link_libraries(PimSubgraphs PRIVATE PimInstructionsFlag)
target_include_directories(PimSubgraphs PRIVATE 
                              "${CMAKE_SOURCE_DIR}/util/pim-instructions-flag")
