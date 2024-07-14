add_library(
    opt_lib STATIC
    Dominators.cpp
    Mem2Reg.cpp
    FuncInfo.cpp
    DeadCode.cpp
    FindLoop.cpp
    LoopHoist.cpp
    ConstantSpread.cpp
    LiveVar.cpp
    RegAlloca.cpp
)
