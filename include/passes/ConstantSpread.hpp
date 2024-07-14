#pragma once
#include "PassManager.hpp"
#include "Function.hpp"
#include <map>
#include <set>

class ConstantSpread : public Pass
{
public:
    ConstantSpread(Module *m) : Pass(m) {}
    void SpreadConstant(Function *);
    void run();

private:
};