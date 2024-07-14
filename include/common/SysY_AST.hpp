#pragma once

extern "C"
{
#include "syntax_tree.h"
    extern syntax_tree *parse(const char *input);
}
#include "User.hpp"
#include <memory>
#include <string>
#include <vector>

enum SysYType
{
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_VOID
};

enum LogicOp
{
    OP_AND,
    OP_OR
};

enum RelOp
{
    // <=
    OP_LE,
    // <
    OP_LT,
    // >
    OP_GT,
    // >=
    OP_GE,
    // ==
    OP_EQ,
    // !=
    OP_NEQ
};

enum DataStay
{
    CONST,
    VAR
};

enum AddOp
{
    // +
    OP_ADD,
    // -
    OP_SUB
};

enum Unaryop
{
    // +
    OP_PLUS,
    // -
    OP_UNPLUS,
    // !
    OP_NOT
};
enum MulOp
{
    // *
    OP_MUL,
    // /
    OP_DIV,
    // %
    OP_MOD
};

class AST;

struct ASTNode;
struct ASTCompunit;
struct ASTDecl;
struct ASTConstdecl;
struct ASTVardecl;
struct ASTConstdef;
struct ASTVardef;
struct ASTInitarray;
struct ASTFuncdef;
struct ASTFuncfparam;
struct ASTStmt;
struct ASTBlock;
struct ASTAssignstmt;
struct ASTSimplestmt;
struct ASTIfstmt;
struct ASTWhilestmt;
struct ASTBreakstmt;
struct ASTContinuestmt;
struct ASTReturnstmt;
struct ASTExp;
struct ASTAddexp;
struct ASTMulexp;
struct ASTUnaryexp;
struct ASTPrimaryexp;
struct ASTFuncexp;
struct ASTRelexp;
struct ASTEqexp;
struct ASTLogicexp;
struct ASTLandexp;
struct ASTLorexp;
struct ASTLval;
struct ASTNum;

class ASTVisitor;

class AST
{
public:
    AST() = delete;
    AST(syntax_tree *);
    AST(AST &&tree)
    {
        root = tree.root;
        tree.root = nullptr;
    };
    ASTCompunit *get_root() { return root.get(); }
    void run_visitor(ASTVisitor &visitor);
    void Getinitarray_const(syntax_tree_node *, std::vector<std::shared_ptr<ASTInitarray>> &, std::vector<unsigned> &, unsigned &, int);
    void Getinitarray_var(syntax_tree_node *, std::vector<std::shared_ptr<ASTInitarray>> &, std::vector<unsigned> &, unsigned &, int);

private:
    std::shared_ptr<ASTCompunit> root = nullptr;
    ASTNode *transform_node_iter(syntax_tree_node *);
};

struct ASTNode
{
    virtual Value *accept(ASTVisitor &) = 0;
    virtual ~ASTNode() = default;
};

struct ASTCompunit : ASTNode
{
    virtual Value *accept(ASTVisitor &);
    std::vector<std::shared_ptr<ASTDecl>> decls;
};

struct ASTStmt : ASTNode
{
    virtual Value *accept(ASTVisitor &);
};

struct ASTDecl : ASTStmt
{
    virtual Value *accept(ASTVisitor &);
    SysYType type;
};

struct ASTConstdecl : ASTDecl
{
    virtual Value *accept(ASTVisitor &);
    DataStay datastay;
    std::vector<std::shared_ptr<ASTConstdef>> constdefs;
};

struct ASTConstdef : ASTNode
{
    virtual Value *accept(ASTVisitor &);
    std::string id = "";
    bool is_array;
    int constexp_i;
    float constexp_f;
    std::vector<int> constexps;
    std::vector<std::shared_ptr<ASTInitarray>> constinitvals;
};

struct ASTInitarray : ASTNode
{
    virtual Value *accept(ASTVisitor &);
    int i_val = 0;
    float f_val = 0;
    bool is_exp = false;
    std::shared_ptr<ASTExp> exp;
};

struct ASTVardecl : ASTDecl
{
    virtual Value *accept(ASTVisitor &);
    DataStay datastay;
    std::vector<std::shared_ptr<ASTVardef>> vardefs;
};

struct ASTVardef : ASTNode
{
    virtual Value *accept(ASTVisitor &);
    std::string id = "";
    bool is_array;
    int global_ival;
    float global_fval;
    std::shared_ptr<ASTAddexp> exp = nullptr;
    std::vector<int> varexps;
    std::vector<std::shared_ptr<ASTInitarray>> varinitvals;
};

struct ASTFuncdef : ASTDecl
{
    virtual Value *accept(ASTVisitor &);
    std::string id = "";
    std::vector<std::shared_ptr<ASTFuncfparam>> funcfparams;
    std::shared_ptr<ASTStmt> block = nullptr;
};

struct ASTFuncfparam : ASTNode
{
    virtual Value *accept(ASTVisitor &);
    SysYType type;
    bool is_ptr = false;
    std::string id = "";
    std::vector<int> divs;
};

struct ASTBlock : ASTStmt
{
    virtual Value *accept(ASTVisitor &);
    std::vector<std::shared_ptr<ASTStmt>> stmts;
};

struct ASTAssignstmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &);
    std::shared_ptr<ASTLval> lval = nullptr;
    std::shared_ptr<ASTExp> exp = nullptr;
};

struct ASTSimplestmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &);
    std::shared_ptr<ASTExp> exp = nullptr;
};

struct ASTIfstmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &);
    std::shared_ptr<ASTStmt> if_stmt = nullptr;
    std::shared_ptr<ASTStmt> else_stmt = nullptr;
    std::shared_ptr<ASTLogicexp> cond = nullptr;
};

struct ASTWhilestmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &);
    std::shared_ptr<ASTStmt> while_stmt = nullptr;
    std::shared_ptr<ASTLogicexp> cond = nullptr;
};

struct ASTBreakstmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &);
};

struct ASTContinuestmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &);
};

struct ASTReturnstmt : ASTStmt
{
    virtual Value *accept(ASTVisitor &);
    std::shared_ptr<ASTExp> exp;
};

struct ASTExp : ASTNode
{
    virtual Value *accept(ASTVisitor &);
};

struct ASTAddexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    AddOp op;
    std::shared_ptr<ASTMulexp> right_hand = nullptr;
    std::shared_ptr<ASTAddexp> left_hand = nullptr;
};

struct ASTMulexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    MulOp op;
    std::shared_ptr<ASTUnaryexp> right_hand = nullptr;
    std::shared_ptr<ASTMulexp> left_hand = nullptr;
};

struct ASTPrimaryexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    std::shared_ptr<ASTExp> exp = nullptr;
};

struct ASTUnaryexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    Unaryop op;
    bool is_func = false;
    bool is_op = false;
    std::shared_ptr<ASTExp> exp = nullptr;
};

struct ASTFuncexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    std::string id = "";
    std::vector<std::shared_ptr<ASTExp>> funcrparams;
};

struct ASTRelexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    RelOp op;
    std::shared_ptr<ASTAddexp> right_hand = nullptr;
    std::shared_ptr<ASTRelexp> left_hand = nullptr;
};

struct ASTEqexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    RelOp op;
    std::shared_ptr<ASTRelexp> right_hand = nullptr;
    std::shared_ptr<ASTEqexp> left_hand = nullptr;
};

struct ASTLogicexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
};

struct ASTLandexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    LogicOp op = OP_AND;
    std::vector<std::shared_ptr<ASTEqexp>> eqexps;
};

struct ASTLorexp : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    LogicOp op = OP_OR;
    std::vector<std::shared_ptr<ASTLandexp>> landexps;
};

struct ASTLval : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    std::string id = "";
    std::vector<std::shared_ptr<ASTExp>> exps;
};

struct ASTNum : ASTExp
{
    virtual Value *accept(ASTVisitor &);
    int int_val;
    float float_val;
    SysYType type;
};

class ASTVisitor
{
public:
    virtual Value *visit(ASTCompunit &) = 0;
    virtual Value *visit(ASTDecl &) = 0;
    virtual Value *visit(ASTConstdecl &) = 0;
    virtual Value *visit(ASTVardecl &) = 0;
    virtual Value *visit(ASTConstdef &) = 0;
    virtual Value *visit(ASTVardef &) = 0;
    virtual Value *visit(ASTInitarray &) = 0;
    virtual Value *visit(ASTFuncdef &) = 0;
    virtual Value *visit(ASTFuncfparam &) = 0;
    virtual Value *visit(ASTStmt &) = 0;
    virtual Value *visit(ASTBlock &) = 0;
    virtual Value *visit(ASTAssignstmt &) = 0;
    virtual Value *visit(ASTIfstmt &) = 0;
    virtual Value *visit(ASTSimplestmt &) = 0;
    virtual Value *visit(ASTWhilestmt &) = 0;
    virtual Value *visit(ASTBreakstmt &) = 0;
    virtual Value *visit(ASTContinuestmt &) = 0;
    virtual Value *visit(ASTReturnstmt &) = 0;
    virtual Value *visit(ASTExp &) = 0;
    virtual Value *visit(ASTAddexp &) = 0;
    virtual Value *visit(ASTMulexp &) = 0;
    virtual Value *visit(ASTUnaryexp &) = 0;
    virtual Value *visit(ASTFuncexp &) = 0;
    virtual Value *visit(ASTPrimaryexp &) = 0;
    virtual Value *visit(ASTRelexp &) = 0;
    virtual Value *visit(ASTEqexp &) = 0;
    virtual Value *visit(ASTLogicexp &) = 0;
    virtual Value *visit(ASTLandexp &) = 0;
    virtual Value *visit(ASTLorexp &) = 0;
    virtual Value *visit(ASTLval &) = 0;
    virtual Value *visit(ASTNum &) = 0;
};