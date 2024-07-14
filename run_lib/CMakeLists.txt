add_library(sysy_lib SysY_lib.c)

install(
    TARGETS sysy_lib
    ARCHIVE DESTINATION lib
)
