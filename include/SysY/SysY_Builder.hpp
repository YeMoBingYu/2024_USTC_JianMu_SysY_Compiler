#pragma once

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"
#include "SysY_AST.hpp"

#include <map>
#include <memory>

class Scope
{
public:
    // enter a new scope
    void enter() { inner.emplace_back(); }

    // exit a scope
    void exit() { inner.pop_back(); }

    bool in_global() { return inner.size() == 1; }

    // push a name to scope
    // return true if successful
    // return false if this name already exits
    bool push(const std::string &name, Value *val)
    {
        auto result = inner[inner.size() - 1].insert({name, val});
        return result.second;
    }

    Value *find(const std::string &name)
    {
        for (auto s = inner.rbegin(); s != inner.rend(); s++)
        {
            auto iter = s->find(name);
            if (iter != s->end())
            {
                return iter->second;
            }
        }

        // Name not found: handled here?
        std::string error = name + "not found in scope";
        std::cerr << error << std::endl;
        std::abort();

        // assert(false && error);

        return nullptr;
    }

private:
    std::vector<std::map<std::string, Value *>> inner;
};

class SysYBuilder : public ASTVisitor
{
public:
    SysYBuilder()
    {
        module = std::make_unique<Module>();
        builder = std::make_unique<IRBuilder>(nullptr, module.get());
        auto *TyVoid = module->get_void_type();
        auto *TyInt32 = module->get_int32_type();
        auto *TyFloat = module->get_float_type();
        auto *TyIntPtr = module->get_int32_ptr_type();
        auto *TyFloatPtr = module->get_float_ptr_type();

        // int getint()
        auto *getint_type = FunctionType::get(TyInt32, {});
        auto *getint_fun = Function::create(getint_type, "getint", module.get());

        // int getch()
        auto *getch_type = FunctionType::get(TyInt32, {});
        auto *getch_fun = Function::create(getch_type, "getch", module.get());

        // float getfloat()
        auto *getfloat_type = FunctionType::get(TyFloat, {});
        auto *getfloat_fun = Function::create(getfloat_type, "getfloat", module.get());

        // int getarray(int[])
        std::vector<Type *> getarray_params;
        getarray_params.push_back(TyIntPtr);
        auto *getarray_type = FunctionType::get(TyInt32, getarray_params);
        auto *getarray_fun = Function::create(getarray_type, "getarray", module.get());

        // int getfarray(float[])
        std::vector<Type *> getfarray_params;
        getfarray_params.push_back(TyFloatPtr);
        auto *getfarray_type = FunctionType::get(TyInt32, getfarray_params);
        auto *getfarray_fun = Function::create(getfarray_type, "getfarray", module.get());

        // void putint(int)
        std::vector<Type *> putint_params;
        putint_params.push_back(TyInt32);
        auto *putint_type = FunctionType::get(TyVoid, putint_params);
        auto *putint_fun = Function::create(putint_type, "putint", module.get());

        // void putch(int)
        std::vector<Type *> putch_params;
        putch_params.push_back(TyInt32);
        auto *putch_type = FunctionType::get(TyVoid, putch_params);
        auto *putch_fun = Function::create(putch_type, "putch", module.get());

        // void putfloat(float)
        std::vector<Type *> putfloat_params;
        putfloat_params.push_back(TyFloat);
        auto *putfloat_type = FunctionType::get(TyVoid, putfloat_params);
        auto *putfloat_fun = Function::create(putfloat_type, "putfloat", module.get());

        // void putarray(int , int[])
        std::vector<Type *> putarray_params;
        putarray_params.push_back(TyInt32);
        putarray_params.push_back(TyIntPtr);
        auto *putarray_type = FunctionType::get(TyVoid, putarray_params);
        auto *putarray_fun = Function::create(putarray_type, "putarray", module.get());

        // void putfarray(int , float[])
        std::vector<Type *> putfarray_params;
        putfarray_params.push_back(TyInt32);
        putfarray_params.push_back(TyFloatPtr);
        auto *putfarray_type = FunctionType::get(TyVoid, putfarray_params);
        auto *putfarray_fun = Function::create(putfarray_type, "putfarray", module.get());

        // void starttime()
        std::vector<Type *> starttime_params;
        starttime_params.push_back(TyInt32);
        auto *starttime_type = FunctionType::get(TyVoid, starttime_params);
        auto *starttime_fun = Function::create(starttime_type, "starttime", module.get());

        // void stoptime()
        std::vector<Type *> stoptime_params;
        stoptime_params.push_back(TyInt32);
        auto *stoptime_type = FunctionType::get(TyVoid, stoptime_params);
        auto *stoptime_fun = Function::create(stoptime_type, "stoptime", module.get());

        scope.enter();
        scope.push("getint", getint_fun);
        scope.push("getch", getch_fun);
        scope.push("getfloat", getfloat_fun);
        scope.push("getarray", getarray_fun);
        scope.push("getfarray", getfarray_fun);
        scope.push("putint", putint_fun);
        scope.push("putch", putch_fun);
        scope.push("putfloat", putfloat_fun);
        scope.push("putarray", putarray_fun);
        scope.push("putfarray", putfarray_fun);
        scope.push("starttime", starttime_fun);
        scope.push("stoptime", stoptime_fun);
    }

    std::unique_ptr<Module> getModule() { return std::move(module); }

private:
    virtual Value *visit(ASTCompunit &) override final;
    virtual Value *visit(ASTDecl &) override final;
    virtual Value *visit(ASTConstdecl &) override final;
    virtual Value *visit(ASTVardecl &) override final;
    virtual Value *visit(ASTConstdef &) override final;
    virtual Value *visit(ASTVardef &) override final;
    virtual Value *visit(ASTInitarray &) override final;
    virtual Value *visit(ASTFuncdef &) override final;
    virtual Value *visit(ASTFuncfparam &) override final;
    virtual Value *visit(ASTStmt &) override final;
    virtual Value *visit(ASTBlock &) override final;
    virtual Value *visit(ASTAssignstmt &) override final;
    virtual Value *visit(ASTIfstmt &) override final;
    virtual Value *visit(ASTSimplestmt &) override final;
    virtual Value *visit(ASTWhilestmt &) override final;
    virtual Value *visit(ASTBreakstmt &) override final;
    virtual Value *visit(ASTContinuestmt &) override final;
    virtual Value *visit(ASTReturnstmt &) override final;
    virtual Value *visit(ASTExp &) override final;
    virtual Value *visit(ASTAddexp &) override final;
    virtual Value *visit(ASTMulexp &) override final;
    virtual Value *visit(ASTUnaryexp &) override final;
    virtual Value *visit(ASTFuncexp &) override final;
    virtual Value *visit(ASTPrimaryexp &) override final;
    virtual Value *visit(ASTRelexp &) override final;
    virtual Value *visit(ASTEqexp &) override final;
    virtual Value *visit(ASTLogicexp &) override final;
    virtual Value *visit(ASTLandexp &) override final;
    virtual Value *visit(ASTLorexp &) override final;
    virtual Value *visit(ASTLval &) override final;
    virtual Value *visit(ASTNum &) override final;

    std::unique_ptr<IRBuilder> builder;
    Scope scope;
    std::unique_ptr<Module> module;
};
