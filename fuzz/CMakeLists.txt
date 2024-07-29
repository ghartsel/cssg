add_executable(cssg-fuzz cssg-fuzz.c)
cssg_add_compile_options(cssg-fuzz)
target_link_libraries(cssg-fuzz cssg)
