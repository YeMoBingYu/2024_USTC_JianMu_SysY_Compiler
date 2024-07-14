add_library(common STATIC
    syntax_tree.c
    SysY_AST.cpp
    logging.cpp
)

target_link_libraries(common)

