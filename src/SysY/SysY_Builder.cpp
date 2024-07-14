#include "SysY_Builder.hpp"
#include <cstring>
#include <stack>
#define MAX 10e3

// if string a == b
#define _STR_EQ(a, b) (strcmp((a), (b)) == 0)

// get CONST
#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())

// break and continue to BB
std::vector<BasicBlock *> break_BB;
std::vector<BasicBlock *> continue_BB;

// define_type
Type *tmp_ptr_type = nullptr;
Type *tmp_type = nullptr;

// val
Value *tmp_val = nullptr;
Value *tmp_ptr_val = nullptr;

// fun
Function *cur_fun = nullptr;

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;
Type *INTPTRPTR_T;
Type *FLOATPTRPTR_T;
Value *putint;
Value *putfloat;
Value *putch;

bool promote(std::unique_ptr<IRBuilder> &builder, Value **l_val_p, Value **r_val_p)
{
    bool is_int;
    auto &l_val = *l_val_p;
    auto &r_val = *r_val_p;
    if (l_val->get_type() == r_val->get_type())
    {
        is_int = l_val->get_type()->is_integer_type();
    }
    else
    {
        is_int = false;
        if (l_val->get_type()->is_integer_type())
            l_val = builder->create_sitofp(l_val, FLOAT_T);
        else
            r_val = builder->create_sitofp(r_val, FLOAT_T);
    }
    return is_int;
}

Value *SysYBuilder::visit(ASTCompunit &node)
{
    VOID_T = module->get_void_type();
    INT1_T = module->get_int1_type();
    INT32_T = module->get_int32_type();
    INT32PTR_T = module->get_int32_ptr_type();
    FLOAT_T = module->get_float_type();
    FLOATPTR_T = module->get_float_ptr_type();
    INTPTRPTR_T = module->get_pointer_type(INT32PTR_T);
    FLOATPTRPTR_T = module->get_pointer_type(FLOATPTR_T);
    putint = scope.find("putint");
    putfloat = scope.find("putfloat");
    putch = scope.find("putch");
    Value *ret_val = nullptr;
    for (auto &decl : node.decls)
    {
        ret_val = decl->accept(*this);
    }
    return ret_val;
}

Value *SysYBuilder::visit(ASTNum &node)
{
    if (node.type == TYPE_INT)
    {
        tmp_val = CONST_INT(node.int_val);
    }
    else
    {
        tmp_val = CONST_FP(node.float_val);
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTConstdecl &node)
{
    if (node.type == TYPE_INT)
        tmp_type = INT32_T;
    else
        tmp_type = FLOAT_T;
    for (auto &constdef : node.constdefs)
    {
        constdef->accept(*this);
    }
    tmp_type = nullptr;
    return nullptr;
}

Value *SysYBuilder::visit(ASTVardecl &node)
{
    if (node.type == TYPE_INT)
    {
        tmp_type = INT32_T;
        tmp_ptr_type = INT32PTR_T;
    }
    else
    {
        tmp_type = FLOAT_T;
        tmp_ptr_type = FLOATPTR_T;
    }
    for (auto &vardef : node.vardefs)
    {
        vardef->accept(*this);
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTConstdef &node)
{
    if (tmp_type != nullptr)
    {
        if (node.is_array)
        {
            if (scope.in_global())
            {
                int div = node.constexps.size() - 1;
                unsigned all_num = 1;
                for (int i = 0; i <= div; i++)
                    all_num = node.constexps[i] * all_num;
                std::vector<Constant *> val;
                std::vector<Constant *> rel_val;
                std::vector<Constant *> array_val;
                std::vector<Constant *> next_array_val;
                std::vector<int> div_num(node.constexps);
                div_num.pop_back();
                Type *current_type = tmp_type;
                unsigned base = node.constexps[div];
                Type *next_type = ArrayType::get(current_type, base);
                Constant *const_val;
                for (int i = 1; i <= all_num; i++)
                {
                    if (tmp_type == INT32_T)
                        const_val = CONST_INT(node.constinitvals[i]->i_val);
                    else
                        const_val = CONST_FP(node.constinitvals[i]->f_val);
                    val.push_back(const_val);
                    if (i % base == 0)
                    {
                        int j;
                        for (j = 0; j < base; j++)
                        {
                            if (tmp_type == INT32_T)
                            {
                                if (static_cast<ConstantInt *>(val[j])->get_value() != 0)
                                    break;
                            }
                            else if (tmp_type == FLOAT_T)
                            {
                                if (static_cast<ConstantFP *>(val[j])->get_value() != 0)
                                    break;
                            }
                        }
                        if (j == base)
                        {
                            val.clear();
                        }
                        else
                        {
                            rel_val.assign(val.begin(), val.end());
                        }
                        auto array_constant = ConstantArray::get(static_cast<ArrayType *>(next_type), rel_val);
                        array_val.push_back(array_constant);
                        if (!rel_val.empty())
                            rel_val.clear();
                    }
                }
                std::vector<Constant *> zero_array;
                if (tmp_type == INT32_T)
                    zero_array.push_back(ConstantZero::get(INT32_T, module.get()));
                else
                    zero_array.push_back(ConstantZero::get(FLOAT_T, module.get()));
                if (!rel_val.empty())
                    rel_val.clear();
                if (!val.empty())
                    val.clear();
                if (div_num.empty() && static_cast<ConstantArray *>(array_val[0])->get_size_of_array() == 0)
                    array_val[0] = ConstantArray::get(static_cast<ArrayType *>(next_type), zero_array);
                while (!div_num.empty())
                {
                    base = div_num.back();
                    current_type = next_type;
                    next_type = ArrayType::get(current_type, base);
                    for (int i = 0; i < array_val.size(); i++)
                    {
                        val.push_back(array_val[i]);
                        if ((i + 1) % base == 0)
                        {
                            int j;
                            for (j = 0; j < base; j++)
                            {
                                if (static_cast<ConstantArray *>(val[j])->get_size_of_array() != 0)
                                    break;
                            }
                            if (j != base)
                            {
                                for (int k = 0; k < base; k++)
                                {
                                    if (static_cast<ConstantArray *>(val[k])->get_size_of_array() == 0)
                                    {
                                        auto zero_constant = ConstantArray::get(static_cast<ArrayType *>(current_type), zero_array);
                                        val[k] = zero_constant;
                                    }
                                }
                                rel_val.assign(val.begin(), val.end());
                                auto array_constant = ConstantArray::get(static_cast<ArrayType *>(next_type), rel_val);
                                next_array_val.push_back(array_constant);
                            }
                            else
                            {
                                rel_val.assign(val.begin(), val.end());
                                auto array_constant = ConstantArray::get(static_cast<ArrayType *>(next_type), rel_val);
                                next_array_val.push_back(array_constant);
                            }
                            val.clear();
                        }
                    }
                    array_val.assign(next_array_val.begin(), next_array_val.end());
                    if (!next_array_val.empty())
                        next_array_val.clear();
                    if (!rel_val.empty())
                        rel_val.clear();
                    if (!val.empty())
                        val.clear();
                    div_num.pop_back();
                }
                if (array_val.size() != 1)
                    assert(false && "init array const val error");
                auto constarrayinit = GlobalVariable::create(node.id, module.get(), next_type, true, array_val[0]);
                scope.push(node.id, constarrayinit);
            }
            else
            {
                Type *current_type;
                Type *next_type;
                next_type = tmp_type;
                for (int i = node.constexps.size() - 1; i >= 0; i--)
                {
                    current_type = next_type;
                    next_type = ArrayType::get(current_type, node.constexps[i]);
                }
                auto const_array_val = builder->create_alloca(next_type);
                std::vector<int> div_num(node.constexps);
                std::vector<Value *> idxs;
                idxs.push_back(CONST_INT(0));
                std::vector<unsigned> content(node.constexps.size(), 1);
                for (int i = node.constexps.size() - 1; i >= 1; i--)
                    content[i - 1] = content[i] * node.constexps[i];
                for (int i = 1; i < node.constinitvals.size(); i++)
                {
                    if (tmp_type == INT32_T)
                    {
                        if (node.constinitvals[i]->i_val != 0)
                        {
                            Value *gep = const_array_val;
                            int next = i - 1;
                            for (int j = 0; j < content.size(); j++)
                            {
                                int swap = next / content[j];
                                idxs.push_back(CONST_INT((int)(swap)));
                                gep = builder->create_gep(gep, idxs);
                                idxs.pop_back();
                                next = next % content[j];
                            }
                            builder->create_store(CONST_INT(node.constinitvals[i]->i_val), gep);
                        }
                    }
                    else if (tmp_type == FLOAT_T)
                    {
                        if (node.constinitvals[i]->f_val != 0)
                        {
                            Value *gep = const_array_val;
                            int next = i - 1;
                            for (int j = 0; j < content.size(); j++)
                            {
                                int swap = next / content[j];
                                idxs.push_back(CONST_INT((int)(swap)));
                                gep = builder->create_gep(gep, idxs);
                                idxs.pop_back();
                                next = next % content[j];
                            }
                            builder->create_store(CONST_FP(node.constinitvals[i]->f_val), gep);
                        }
                    }
                }
                scope.push(node.id, const_array_val);
            }
        }
        else
        {
            if (scope.in_global())
            {
                if (tmp_type == INT32_T)
                {
                    auto const_val = GlobalVariable::create(node.id, module.get(), tmp_type, true, CONST_INT(node.constexp_i));
                    scope.push(node.id, const_val);
                }
                else
                {
                    auto const_val = GlobalVariable::create(node.id, module.get(), tmp_type, true, CONST_FP(node.constexp_f));
                    scope.push(node.id, const_val);
                }
            }
            else
            {
                if (tmp_type == INT32_T)
                {
                    auto const_val = builder->create_alloca(tmp_type);
                    builder->create_store(CONST_INT(node.constexp_i), const_val);
                    scope.push(node.id, const_val);
                }
                else
                {
                    auto const_val = builder->create_alloca(tmp_type);
                    builder->create_store(CONST_FP(node.constexp_f), const_val);
                    scope.push(node.id, const_val);
                }
            }
        }
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTVardef &node)
{
    if (tmp_type != nullptr)
    {
        if (node.is_array)
        {
            if (scope.in_global())
            {
                if (!node.varinitvals.empty())
                {
                    int div = node.varexps.size() - 1;
                    unsigned all_num = 1;
                    for (int i = 0; i <= div; i++)
                        all_num = node.varexps[i] * all_num;
                    std::vector<Constant *> val;
                    std::vector<Constant *> rel_val;
                    std::vector<Constant *> array_val;
                    std::vector<Constant *> next_array_val;
                    std::vector<int> div_num(node.varexps);
                    div_num.pop_back();
                    Type *current_type = tmp_type;
                    unsigned base = node.varexps[div];
                    Type *next_type = ArrayType::get(current_type, base);
                    Constant *var_val;
                    for (int i = 1; i <= all_num; i++)
                    {
                        if (tmp_type == INT32_T)
                            var_val = CONST_INT(node.varinitvals[i]->i_val);
                        else
                            var_val = CONST_FP(node.varinitvals[i]->f_val);
                        val.push_back(var_val);
                        if (i % base == 0)
                        {
                            int j;
                            for (j = 0; j < base; j++)
                            {
                                if (tmp_type == INT32_T)
                                {
                                    if (static_cast<ConstantInt *>(val[j])->get_value() != 0)
                                        break;
                                }
                                else if (tmp_type == FLOAT_T)
                                {
                                    if (static_cast<ConstantFP *>(val[j])->get_value() != 0)
                                        break;
                                }
                            }
                            if (j == base)
                            {
                                val.clear();
                            }
                            else
                            {
                                rel_val.assign(val.begin(), val.end());
                                val.clear();
                            }
                            auto array_constant = ConstantArray::get(static_cast<ArrayType *>(next_type), rel_val);
                            array_val.push_back(array_constant);
                            if (!rel_val.empty())
                                rel_val.clear();
                        }
                    }
                    std::vector<Constant *> zero_array;
                    if (tmp_type == INT32_T)
                        zero_array.push_back(ConstantZero::get(INT32_T, module.get()));
                    else
                        zero_array.push_back(ConstantZero::get(FLOAT_T, module.get()));
                    if (!rel_val.empty())
                        rel_val.clear();
                    if (!val.empty())
                        val.clear();
                    if (div_num.empty() && static_cast<ConstantArray *>(array_val[0])->get_size_of_array() == 0)
                        array_val[0] = ConstantArray::get(static_cast<ArrayType *>(next_type), zero_array);
                    while (!div_num.empty())
                    {
                        base = div_num.back();
                        current_type = next_type;
                        next_type = ArrayType::get(current_type, base);
                        for (int i = 0; i < array_val.size(); i++)
                        {
                            val.push_back(array_val[i]);
                            if ((i + 1) % base == 0)
                            {
                                int j;
                                for (j = 0; j < base; j++)
                                {
                                    if (static_cast<ConstantArray *>(val[j])->get_size_of_array() != 0)
                                        break;
                                }
                                if (j != base)
                                {
                                    for (int k = 0; k < base; k++)
                                    {
                                        if (static_cast<ConstantArray *>(val[k])->get_size_of_array() == 0)
                                        {
                                            auto zero_constant = ConstantArray::get(static_cast<ArrayType *>(current_type), zero_array);
                                            val[k] = zero_constant;
                                        }
                                    }
                                    rel_val.assign(val.begin(), val.end());
                                    auto array_constant = ConstantArray::get(static_cast<ArrayType *>(next_type), rel_val);
                                    next_array_val.push_back(array_constant);
                                }
                                else
                                {
                                    auto array_constant = ConstantArray::get(static_cast<ArrayType *>(next_type), rel_val);
                                    next_array_val.push_back(array_constant);
                                }
                                val.clear();
                            }
                        }
                        array_val.assign(next_array_val.begin(), next_array_val.end());
                        if (!next_array_val.empty())
                            next_array_val.clear();
                        if (!rel_val.empty())
                            rel_val.clear();
                        if (!val.empty())
                            val.clear();
                        div_num.pop_back();
                    }
                    if (array_val.size() != 1)
                        assert(false && "init array const val error");
                    if (dynamic_cast<ConstantArray *>(array_val[0]) && static_cast<ConstantArray *>(array_val[0])->get_size_of_array() == 0)
                        array_val[0] = ConstantArray::get(static_cast<ArrayType *>(next_type), zero_array);
                    auto vararrayinit = GlobalVariable::create(node.id, module.get(), next_type, false, array_val[0]);
                    scope.push(node.id, vararrayinit);
                }
                else
                {
                    int div = node.varexps.size() - 1;
                    Type *array_type = tmp_type;
                    for (int i = div; i >= 0; i--)
                        array_type = ArrayType::get(array_type, node.varexps[i]);
                    GlobalVariable *vararrayinit;
                    if (tmp_type == INT32_T)
                        vararrayinit = GlobalVariable::create(node.id, module.get(), array_type, false, ConstantZero::get(INT32_T, module.get()));
                    else if (tmp_type == FLOAT_T)
                        vararrayinit = GlobalVariable::create(node.id, module.get(), array_type, false, ConstantZero::get(FLOAT_T, module.get()));
                    scope.push(node.id, vararrayinit);
                }
            }
            else
            {
                if (!node.varinitvals.empty())
                {
                    Type *current_type;
                    Type *next_type;
                    next_type = tmp_type;
                    for (int i = node.varexps.size() - 1; i >= 0; i--)
                    {
                        current_type = next_type;
                        next_type = ArrayType::get(current_type, node.varexps[i]);
                    }
                    auto var_array_val = builder->create_alloca(next_type);
                    std::vector<int> div_num(node.varexps);
                    std::vector<Value *> idxs;
                    idxs.push_back(CONST_INT(0));
                    std::vector<unsigned>
                        content(node.varexps.size(), 1);
                    for (int i = node.varexps.size() - 1; i >= 1; i--)
                        content[i - 1] = content[i] * node.varexps[i];
                    for (int i = 1; i < node.varinitvals.size(); i++)
                    {
                        if (tmp_type == INT32_T)
                        {
                            if (node.varinitvals[i]->is_exp || node.varinitvals[i]->i_val != 0 || node.varinitvals.size() < MAX)
                            {
                                Value *gep = var_array_val;
                                int next = i - 1;
                                for (int j = 0; j < content.size(); j++)
                                {
                                    int swap = next / content[j];
                                    idxs.push_back(CONST_INT((int)(swap)));
                                    gep = builder->create_gep(gep, idxs);
                                    idxs.pop_back();
                                    next = next % content[j];
                                }
                                if (node.varinitvals[i]->is_exp && node.varinitvals[i]->exp != nullptr)
                                {
                                    node.varinitvals[i]->exp->accept(*this);
                                    builder->create_store(tmp_val, gep);
                                }
                                else
                                {
                                    builder->create_store(CONST_INT(node.varinitvals[i]->i_val), gep);
                                }
                            }
                        }
                        else if (tmp_type == FLOAT_T)
                        {
                            if (node.varinitvals[i]->is_exp || node.varinitvals[i]->f_val != 0 || node.varinitvals.size() < MAX)
                            {
                                Value *gep = var_array_val;
                                int next = i - 1;
                                for (int j = 0; j < content.size(); j++)
                                {
                                    int swap = next / content[j];
                                    idxs.push_back(CONST_INT((int)(swap)));
                                    gep = builder->create_gep(gep, idxs);
                                    idxs.pop_back();
                                    next = next % content[j];
                                }
                                if (node.varinitvals[i]->is_exp && node.varinitvals[i]->exp != nullptr)
                                {
                                    node.varinitvals[i]->exp->accept(*this);
                                    if (tmp_val->get_type() == INT1_T)
                                    {
                                        tmp_val = builder->create_zext(tmp_val, INT32_T);
                                        tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
                                    }
                                    else if (tmp_val->get_type() == INT32_T)
                                        tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
                                    builder->create_store(tmp_val, gep);
                                }
                                else
                                {
                                    builder->create_store(CONST_FP(node.varinitvals[i]->f_val), gep);
                                }
                            }
                        }
                    }
                    scope.push(node.id, var_array_val);
                }
                else
                {
                    int div = node.varexps.size() - 1;
                    Type *array_type = tmp_type;
                    for (int i = div; i >= 0; i--)
                        array_type = ArrayType::get(array_type, node.varexps[i]);
                    auto var_array = builder->create_alloca(array_type);
                    scope.push(node.id, var_array);
                }
            }
        }
        else
        {
            if (scope.in_global())
            {
                if (tmp_type == INT32_T)
                {
                    auto var_val = GlobalVariable::create(node.id, module.get(), tmp_type, false, CONST_INT(node.global_ival));
                    scope.push(node.id, var_val);
                }
                else
                {
                    auto var_val = GlobalVariable::create(node.id, module.get(), tmp_type, true, CONST_FP(node.global_fval));
                    scope.push(node.id, var_val);
                }
            }
            else
            {
                auto var_val = builder->create_alloca(tmp_type);
                if (node.exp != nullptr)
                {
                    node.exp->accept(*this);
                    if (tmp_type == INT32_T)
                    {
                        if (tmp_val->get_type() == INT1_T)
                            tmp_val = builder->create_zext(tmp_val, INT32_T);
                        else if (tmp_val->get_type() == FLOAT_T)
                            tmp_val = builder->create_fptosi(tmp_val, INT32_T);
                        builder->create_store(tmp_val, var_val);
                    }
                    else if (tmp_type == FLOAT_T)
                    {
                        if (tmp_val->get_type() == INT1_T)
                        {
                            tmp_val = builder->create_zext(tmp_val, INT32_T);
                            tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
                        }
                        else if (tmp_val->get_type() == INT32_T)
                            tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
                        builder->create_store(tmp_val, var_val);
                    }
                }
                else
                {
                    if (tmp_type == INT32_T)
                    {
                        builder->create_store(CONST_INT(0), var_val);
                    }
                    else if (tmp_type == FLOAT_T)
                    {
                        builder->create_store(CONST_FP(0.), var_val);
                    }
                }
                scope.push(node.id, var_val);
            }
        }
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTFuncdef &node)
{
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> params_type;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;
    if (!node.funcfparams.empty())
    {
        for (auto &param : node.funcfparams)
        {
            Type *swap_type;
            if (param->type == TYPE_INT)
                swap_type = INT32_T;
            else if (param->type == TYPE_FLOAT)
                swap_type = FLOAT_T;
            if (param->is_ptr)
            {
                if (param->divs.empty())
                {
                    if (param->type == TYPE_INT)
                        params_type.push_back(INT32PTR_T);
                    else if (param->type == TYPE_FLOAT)
                        params_type.push_back(FLOATPTR_T);
                }
                else
                {
                    int n = param->divs.size() - 1;
                    for (int i = n; i >= 0; i--)
                        swap_type = ArrayType::get(swap_type, (unsigned)param->divs[i]);
                    auto swap_type_ptr = module->get_pointer_type(swap_type);
                    params_type.push_back(swap_type_ptr);
                }
            }
            else
            {
                params_type.push_back(swap_type);
            }
        }
    }
    fun_type = FunctionType::get(ret_type, params_type);
    auto func = Function::create(fun_type, node.id, module.get());
    scope.push(node.id, func);
    cur_fun = func;
    auto funBB = BasicBlock::create(module.get(), "entry", func);
    builder->set_insert_point(funBB);
    scope.enter();
    std::vector<Value *> args;
    for (auto &arg : func->get_args())
    {
        args.push_back(&arg);
    }
    Value *alloc;
    if (args.size() == params_type.size())
    {
        for (int i = 0; i < args.size(); i++)
        {
            alloc = builder->create_alloca(params_type[i]);
            builder->create_store(args[i], alloc);
            scope.push(node.funcfparams[i]->id, alloc);
        }
    }
    else
        assert(false && "function args num wrong");
    node.block->accept(*this);
    if (not builder->get_insert_block()->is_terminated())
    {
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type())
        {
            /*if (_STR_EQ(cur_fun->get_name().c_str(), "main"))
            {
                builder->create_call(putfloat, {CONST_FP(0.)});
                builder->create_call(putch, {CONST_INT(10)});
            }*/
            builder->create_ret(CONST_FP(0.));
        }
        else
        {
            /*if (_STR_EQ(cur_fun->get_name().c_str(), "main"))
            {
                builder->create_call(putint, {CONST_INT(0)});
                builder->create_call(putch, {CONST_INT(10)});
            }*/
            builder->create_ret(CONST_INT(0));
        }
    }
    scope.exit();
    return nullptr;
}

Value *SysYBuilder::visit(ASTFuncfparam &node)
{
    return nullptr;
}

Value *SysYBuilder::visit(ASTBlock &node)
{
    if (!node.stmts.empty())
    {
        scope.enter();
        for (int i = 0; i < node.stmts.size(); i++)
        {
            node.stmts[i]->accept(*this);
            if (builder->get_insert_block()->is_terminated())
                break;
        }
        scope.exit();
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTExp &node)
{
    return nullptr;
}

Value *SysYBuilder::visit(ASTStmt &node)
{
    return nullptr;
}

Value *SysYBuilder::visit(ASTDecl &node)
{
    return nullptr;
}

Value *SysYBuilder::visit(ASTInitarray &node)
{
    return nullptr;
}

Value *SysYBuilder::visit(ASTIfstmt &node)
{
    node.cond->accept(*this);
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    BasicBlock *falseBB;
    if (node.else_stmt != nullptr)
    {
        falseBB = BasicBlock::create(module.get(), "", cur_fun);
    }
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    if (node.else_stmt == nullptr)
    {
        builder->create_cond_br(ret_val, trueBB, contBB);
    }
    else
    {
        builder->create_cond_br(ret_val, trueBB, falseBB);
    }
    builder->set_insert_point(trueBB);
    scope.enter();
    node.if_stmt->accept(*this);
    if (not builder->get_insert_block()->is_terminated())
    {
        builder->create_br(contBB);
    }
    scope.exit();
    if (node.else_stmt != nullptr)
    {
        builder->set_insert_point(falseBB);
        scope.enter();
        node.else_stmt->accept(*this);
        if (not builder->get_insert_block()->is_terminated())
        {
            builder->create_br(contBB);
        }
        scope.exit();
    }
    builder->set_insert_point(contBB);
    return nullptr;
}

Value *SysYBuilder::visit(ASTWhilestmt &node)
{

    auto exprBB = BasicBlock::create(module.get(), "", cur_fun);
    builder->create_br(exprBB);
    builder->set_insert_point(exprBB);
    node.cond->accept(*this);
    auto ret_val = tmp_val;
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto contBB = BasicBlock::create(module.get(), "", cur_fun);
    builder->create_cond_br(ret_val, trueBB, contBB);
    break_BB.push_back(contBB);
    continue_BB.push_back(exprBB);
    builder->set_insert_point(trueBB);
    scope.enter();
    node.while_stmt->accept(*this);
    if (not builder->get_insert_block()->is_terminated())
    {
        builder->create_br(exprBB);
    }
    break_BB.pop_back();
    continue_BB.pop_back();
    scope.exit();
    builder->set_insert_point(contBB);
    return nullptr;
}

Value *SysYBuilder::visit(ASTReturnstmt &node)
{
    if (node.exp == nullptr)
    {
        builder->create_void_ret();
    }
    else
    {
        auto fun_ret_type = cur_fun->get_function_type()->get_return_type();
        node.exp->accept(*this);
        if (fun_ret_type != tmp_val->get_type())
        {
            if (fun_ret_type->is_integer_type())
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
            else
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
        }
        /*if (tmp_val->get_type() == INT32_T)
        {
            if (_STR_EQ(cur_fun->get_name().c_str(), "main"))
            {
                builder->create_call(putint, {tmp_val});
                builder->create_call(putch, {CONST_INT(10)});
            }
        }
        else if (tmp_val->get_type() == FLOAT_T)
        {
            if (_STR_EQ(cur_fun->get_name().c_str(), "main"))
            {
                builder->create_call(putfloat, {tmp_val});
                builder->create_call(putch, {CONST_INT(10)});
            }
        }*/
        builder->create_ret(tmp_val);
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTBreakstmt &node)
{
    if (!break_BB.empty())
        builder->create_br(break_BB.back());
    return nullptr;
}

Value *SysYBuilder::visit(ASTContinuestmt &node)
{
    if (!continue_BB.empty())
        builder->create_br(continue_BB.back());
    return nullptr;
}

Value *SysYBuilder::visit(ASTLval &node)
{
    auto var = scope.find(node.id);
    assert(var != nullptr && "lval not found");
    auto is_int = var->get_type()->get_pointer_element_type()->is_integer_type();
    auto is_float = var->get_type()->get_pointer_element_type()->is_float_type();
    auto is_ptr = var->get_type()->get_pointer_element_type()->is_pointer_type();
    auto is_array = var->get_type()->get_pointer_element_type()->is_array_type();
    if (node.exps.empty())
    {
        if (is_int || is_float)
            tmp_ptr_val = var;
        else if (is_ptr)
            tmp_ptr_val = builder->create_load(var);
        else if (is_array)
        {
            std::vector<Value *> idxs;
            idxs.push_back(CONST_INT(0));
            idxs.push_back(CONST_INT(0));
            tmp_ptr_val = builder->create_gep(var, idxs);
        }
    }
    else
    {
        Value *array_addr;
        if (is_int || is_float)
            assert(false && "lval array type error");
        else if (is_ptr)
        {
            array_addr = builder->create_load(var);
            node.exps[0]->accept(*this);
            if (tmp_val->get_type()->is_float_type())
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
            array_addr = builder->create_gep(array_addr, {tmp_val});
            for (int i = 1; i < node.exps.size(); i++)
            {
                node.exps[i]->accept(*this);
                if (tmp_val->get_type()->is_float_type())
                    tmp_val = builder->create_fptosi(tmp_val, INT32_T);
                array_addr = builder->create_gep(array_addr, {CONST_INT(0), tmp_val});
            }
            if (array_addr->get_type()->get_pointer_element_type()->is_array_type())
                array_addr = builder->create_gep(array_addr, {CONST_INT(0), CONST_INT(0)});
        }
        else
        {
            array_addr = var;
            for (auto &exp : node.exps)
            {
                exp->accept(*this);
                auto val = tmp_val;
                if (val->get_type()->is_float_type())
                    val = builder->create_fptosi(val, INT32_T);
                array_addr = builder->create_gep(array_addr, {CONST_INT(0), val});
            }
            if (array_addr->get_type()->get_pointer_element_type()->is_array_type())
                array_addr = builder->create_gep(array_addr, {CONST_INT(0), CONST_INT(0)});
        }
        tmp_ptr_val = array_addr;
    }
    if (tmp_ptr_val->get_type()->is_pointer_type())
        if (tmp_ptr_val->get_type()->get_pointer_element_type()->is_integer_type() || 
            tmp_ptr_val->get_type()->get_pointer_element_type()->is_float_type() || 
            tmp_ptr_val->get_type()->get_pointer_element_type()->is_pointer_type())
            tmp_val = builder->create_load(tmp_ptr_val);
    return nullptr;
}

Value *SysYBuilder::visit(ASTAssignstmt &node)
{
    node.exp->accept(*this);
    auto expr_result = tmp_val;
    node.lval->accept(*this);
    auto var_addr = tmp_ptr_val;
    if (var_addr->get_type()->get_pointer_element_type() != expr_result->get_type())
    {
        if (var_addr->get_type()->get_pointer_element_type() == INT32_T)
        {
            if (expr_result->get_type() == INT1_T)
                expr_result = builder->create_zext(expr_result, INT32_T);
            else if (expr_result->get_type() == FLOAT_T)
                expr_result = builder->create_fptosi(expr_result, INT32_T);
        }
        else if (var_addr->get_type()->get_pointer_element_type() == FLOAT_T)
        {
            if (expr_result->get_type() == INT1_T)
            {
                expr_result = builder->create_zext(expr_result, INT32_T);
                expr_result = builder->create_sitofp(expr_result, FLOAT_T);
            }
            else if (expr_result->get_type() == INT32_T)
                expr_result = builder->create_sitofp(expr_result, FLOAT_T);
        }
        else
            assert(false && "assignstmt addr type error");
    }
    builder->create_store(expr_result, var_addr);
    tmp_val = expr_result;
    return nullptr;
}

Value *SysYBuilder ::visit(ASTSimplestmt &node)
{
    if (node.exp != nullptr)
        node.exp->accept(*this);
    return nullptr;
}

Value *SysYBuilder::visit(ASTRelexp &node)
{
    if (node.left_hand == nullptr)
    {
        node.right_hand->accept(*this);
    }
    else
    {
        node.left_hand->accept(*this);
        auto l_val = tmp_val;
        node.right_hand->accept(*this);
        auto r_val = tmp_val;
        if (l_val->get_type() == INT1_T)
            l_val = builder->create_zext(l_val, INT32_T);
        if (r_val->get_type() == INT1_T)
            r_val = builder->create_zext(r_val, INT32_T);
        bool is_int = promote(builder, &l_val, &r_val);
        Value *cmp;
        switch (node.op)
        {
        case OP_LT:
            if (is_int)
                cmp = builder->create_icmp_lt(l_val, r_val);
            else
                cmp = builder->create_fcmp_lt(l_val, r_val);
            break;
        case OP_LE:
            if (is_int)
                cmp = builder->create_icmp_le(l_val, r_val);
            else
                cmp = builder->create_fcmp_le(l_val, r_val);
            break;
        case OP_GE:
            if (is_int)
                cmp = builder->create_icmp_ge(l_val, r_val);
            else
                cmp = builder->create_fcmp_ge(l_val, r_val);
            break;
        case OP_GT:
            if (is_int)
                cmp = builder->create_icmp_gt(l_val, r_val);
            else
                cmp = builder->create_fcmp_gt(l_val, r_val);
            break;
        case OP_EQ:
        case OP_NEQ:
            assert(false && "relexp error");
            break;
        }
        tmp_val = cmp;
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTEqexp &node)
{
    if (node.left_hand == nullptr)
    {
        node.right_hand->accept(*this);
    }
    else
    {
        node.left_hand->accept(*this);
        auto l_val = tmp_val;
        node.right_hand->accept(*this);
        auto r_val = tmp_val;
        if (l_val->get_type() == INT1_T)
            l_val = builder->create_zext(l_val, INT32_T);
        if (r_val->get_type() == INT1_T)
            r_val = builder->create_zext(r_val, INT32_T);
        bool is_int = promote(builder, &l_val, &r_val);
        Value *cmp;
        switch (node.op)
        {
        case OP_LT:
        case OP_LE:
        case OP_GE:
        case OP_GT:
            assert(false && "eqexp error");
            break;
        case OP_EQ:
            if (is_int)
                cmp = builder->create_icmp_eq(l_val, r_val);
            else
                cmp = builder->create_fcmp_eq(l_val, r_val);
            break;
        case OP_NEQ:
            if (is_int)
                cmp = builder->create_icmp_ne(l_val, r_val);
            else
                cmp = builder->create_fcmp_ne(l_val, r_val);
            break;
        }
        tmp_val = cmp;
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTAddexp &node)
{
    if (node.left_hand == nullptr)
    {
        node.right_hand->accept(*this);
    }
    else
    {
        node.left_hand->accept(*this);
        auto l_val = tmp_val;
        node.right_hand->accept(*this);
        auto r_val = tmp_val;
        if (l_val->get_type() == INT1_T)
            l_val = builder->create_zext(l_val, INT32_T);
        if (r_val->get_type() == INT1_T)
            r_val = builder->create_zext(r_val, INT32_T);
        bool is_int = promote(builder, &l_val, &r_val);
        switch (node.op)
        {
        case OP_ADD:
            if (is_int)
                tmp_val = builder->create_iadd(l_val, r_val);
            else
                tmp_val = builder->create_fadd(l_val, r_val);
            break;
        case OP_SUB:
            if (is_int)
                tmp_val = builder->create_isub(l_val, r_val);
            else
                tmp_val = builder->create_fsub(l_val, r_val);
            break;
        default:
            break;
        }
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTMulexp &node)
{
    if (node.left_hand == nullptr)
    {
        node.right_hand->accept(*this);
    }
    else
    {
        node.left_hand->accept(*this);
        auto l_val = tmp_val;
        node.right_hand->accept(*this);
        auto r_val = tmp_val;
        if (l_val->get_type() == INT1_T)
            l_val = builder->create_zext(l_val, INT32_T);
        if (r_val->get_type() == INT1_T)
            r_val = builder->create_zext(r_val, INT32_T);
        if (node.op == OP_MOD)
        {
            if (!l_val->get_type()->is_integer_type() || !r_val->get_type()->is_integer_type())
                assert(false && "urem error");
            else
                tmp_val = builder->create_srem(l_val, r_val);
        }
        bool is_int = promote(builder, &l_val, &r_val);
        switch (node.op)
        {
        case OP_MUL:
            if (is_int)
                tmp_val = builder->create_imul(l_val, r_val);
            else
                tmp_val = builder->create_fmul(l_val, r_val);
            break;
        case OP_DIV:
            if (is_int)
                tmp_val = builder->create_isdiv(l_val, r_val);
            else
                tmp_val = builder->create_fdiv(l_val, r_val);
            break;
        case OP_MOD:
            break;
        }
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTLogicexp &node)
{
    return nullptr;
}

Value *SysYBuilder::visit(ASTLandexp &node)
{
    std::vector<BasicBlock *> bb_vec;
    std::vector<Value *> bb_val;
    bb_vec.push_back(builder->get_insert_block());
    node.eqexps[0]->accept(*this);
    if (tmp_val->get_type() == INT32_T)
        tmp_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    else if (tmp_val->get_type() == FLOAT_T)
    {
        tmp_val = builder->create_fcmp_ne(tmp_val, CONST_FP(0.));
    }
    if (node.eqexps.size() > 1)
    {
        bb_val.push_back(tmp_val);
        for (int i = 1; i < node.eqexps.size(); i++)
        {
            auto true_BB = BasicBlock::create(module.get(), "", cur_fun);
            builder->set_insert_point(true_BB);
            node.eqexps[i]->accept(*this);
            if (tmp_val->get_type() == INT32_T)
                tmp_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
            else if (tmp_val->get_type() == FLOAT_T)
            {
                tmp_val = builder->create_fcmp_ne(tmp_val, CONST_FP(0.));
            }
            bb_vec.push_back(true_BB);
            bb_val.push_back(tmp_val);
        }
        auto cont_BB = BasicBlock::create(module.get(), "", cur_fun);
        for (int i = 0; i < node.eqexps.size() - 1; i++)
        {
            builder->set_insert_point(bb_vec[i]);
            builder->create_cond_br(bb_val[i], bb_vec[i + 1], cont_BB);
        }
        builder->set_insert_point(bb_vec.back());
        builder->create_br(cont_BB);
        builder->set_insert_point(cont_BB);
        for (int i = 0; i < bb_val.size() - 1; i++)
            bb_val[i] = ConstantInt::get(false, module.get());
        auto phi1 = PhiInst::create_phi(INT1_T, cont_BB, bb_val, bb_vec);
        auto phi = static_cast<Instruction *>(phi1);
        cont_BB->add_instr_begin(phi);
        tmp_val = phi;
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTLorexp &node)
{
    std::vector<BasicBlock *> bb_start;
    std::vector<BasicBlock *> bb_end;
    std::vector<Value *> bb_val;
    bb_start.push_back(builder->get_insert_block());
    node.landexps[0]->accept(*this);
    if (node.landexps.size() > 1)
    {
        bb_end.push_back(builder->get_insert_block());
        bb_val.push_back(tmp_val);
        for (int i = 1; i < node.landexps.size(); i++)
        {
            auto true_BB = BasicBlock::create(module.get(), "", cur_fun);
            builder->set_insert_point(true_BB);
            node.landexps[i]->accept(*this);
            bb_start.push_back(true_BB);
            bb_val.push_back(tmp_val);
            bb_end.push_back(builder->get_insert_block());
        }
        auto cont_BB = BasicBlock::create(module.get(), "", cur_fun);
        for (int i = 0; i < node.landexps.size() - 1; i++)
        {
            builder->set_insert_point(bb_end[i]);
            builder->create_cond_br(bb_val[i], cont_BB, bb_start[i + 1]);
        }
        builder->set_insert_point(bb_end.back());
        builder->create_br(cont_BB);
        builder->set_insert_point(cont_BB);
        for (int i = 0; i < bb_val.size() - 1; i++)
            bb_val[i] = ConstantInt::get(true, module.get());
        if (bb_val.size() == bb_end.size())
        {
            auto phi1 = PhiInst::create_phi(INT1_T, cont_BB, bb_val, bb_end);
            auto phi = static_cast<Instruction *>(phi1);
            cont_BB->add_instr_begin(phi);
            tmp_val = phi;
        }
        else
            assert(false && "lorexp num error");
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTPrimaryexp &node)
{
    if (node.exp != nullptr)
        node.exp->accept(*this);
    return nullptr;
}

Value *SysYBuilder::visit(ASTFuncexp &node)
{
    auto fun = static_cast<Function *>(scope.find(node.id));
    std::vector<Value *> args;
    auto param_type = fun->get_function_type()->param_begin();
    for (auto &arg : node.funcrparams)
    {
        Type *ptr = *param_type;
        arg->accept(*this);
        if (!tmp_val->get_type()->is_pointer_type() && *param_type != tmp_val->get_type())
        {
            if (tmp_val->get_type()->is_integer_type())
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            else if (tmp_val->get_type()->is_float_type())
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
        }
        if (ptr->is_pointer_type())
            args.push_back(tmp_ptr_val);
        else
            args.push_back(tmp_val);
        param_type++;
    }
    if (_STR_EQ(node.id.c_str(), "starttime") || _STR_EQ(node.id.c_str(), "stoptime"))
        tmp_val = builder->create_call(static_cast<Function *>(fun), {CONST_INT(0)});
    else
        tmp_val = builder->create_call(static_cast<Function *>(fun), args);
    return nullptr;
}

Value *SysYBuilder::visit(ASTUnaryexp &node)
{
    node.exp->accept(*this);
    if (node.is_op)
    {
        if (tmp_val->get_type() == INT1_T)
        {
            if (node.op == OP_NOT)
            {
                tmp_val = builder->create_zext(tmp_val, INT32_T);
                tmp_val = builder->create_isub(CONST_INT(1), tmp_val);
                tmp_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
            }
            else if (node.op == OP_UNPLUS)
            {
                tmp_val = builder->create_zext(tmp_val, INT32_T);
                tmp_val = builder->create_isub(CONST_INT(0), tmp_val);
            }
        }
        else if (tmp_val->get_type() == INT32_T)
        {
            if (node.op == OP_UNPLUS)
                tmp_val = builder->create_isub(CONST_INT(0), tmp_val);
            else if (node.op == OP_NOT)
                tmp_val = builder->create_icmp_eq(tmp_val, CONST_INT(0));
        }
        else if (tmp_val->get_type() == FLOAT_T)
        {
            if (node.op == OP_UNPLUS)
                tmp_val = builder->create_fsub(CONST_FP(0.), tmp_val);
            else if (node.op == OP_NOT)
                tmp_val = builder->create_fcmp_eq(tmp_val, CONST_FP(0.));
        }
    }
    return nullptr;
}
