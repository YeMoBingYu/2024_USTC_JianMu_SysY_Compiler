add_executable(
    SysY
    SysY.cpp
    SysY_Builder.cpp
)

target_link_libraries(
    SysY
    IR_lib
    common
    syntax
    codegen
    stdc++fs
    opt_lib
)

install(
    TARGETS SysY
    RUNTIME DESTINATION bin
)
