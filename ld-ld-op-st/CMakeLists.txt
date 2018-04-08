set(PASS_SOURCES ld-ld-op-st.cc)
add_pass(LdLdOpSt ${PASS_SOURCES})
target_link_libraries(LdLdOpSt PRIVATE OffloadableInstructions)
target_include_directories(LdLdOpSt PRIVATE 
                              "${CMAKE_SOURCE_DIR}/util/offloadable-instructions")
