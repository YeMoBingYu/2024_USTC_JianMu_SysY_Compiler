#include "SysY_AST.hpp"

#include <cstring>
#include <iostream>
#include <stack>
#include <queue>
#include <map>
#define _SYNTAX_TREE_ERROR_(name)                                     \
    std::cerr << "[syntax_tree]: " << name << " error." << std::endl; \
    std::abort()
#define _SEMANTICS_ERROR_(name)                                     \
    std::cerr << "[semantics]: " << name << " error." << std::endl; \
    std::abort()
#define _STR_EQ(a, b) (strcmp((a), (b)) == 0)

std::map<std::string, int> const_int_val;
std::map<std::string, float> const_float_val;
bool is_global = true;
bool is_const = false;
int tmp_ival;
float tmp_fval;
SysYType ast_tmp_type;
void AST::run_visitor(ASTVisitor &visitor) { root->accept(visitor); }

AST::AST(syntax_tree *s)
{
    if (s == nullptr)
    {
        std::cerr << "empty input tree!" << std::endl;
        std::abort();
    }
    auto node = transform_node_iter(s->root);
    del_syntax_tree(s);
    root = std::shared_ptr<ASTCompunit>(static_cast<ASTCompunit *>(node));
}

void AST::Getinitarray_const(syntax_tree_node *n, std::vector<std::shared_ptr<ASTInitarray>> &initvals, std::vector<unsigned> &content, unsigned &step, int current_div)
{
    if (n->children_num == 1)
    {
        is_const = true;
        transform_node_iter(n->children[0]);
        is_const = false;
        initvals[step]->is_exp = false;
        if (ast_tmp_type == TYPE_INT)
            initvals[step]->i_val = tmp_ival;
        else
            initvals[step]->f_val = tmp_fval;
        step++;
    }
    else if (n->children_num == 2)
    {
        step += content[current_div];
    }
    else if (n->children_num == 3)
    {
        unsigned next_step = step + content[current_div];
        Getinitarray_const(n->children[1], initvals, content, step, current_div + 1);
        step = next_step;
    }
    else if (n->children_num == 4)
    {
        std::stack<syntax_tree_node *> s;
        auto current = n->children[2];
        while (1)
        {
            if (current->children_num == 2)
            {
                s.push(current->children[1]);
                break;
            }
            else if (current->children_num == 3)
            {
                s.push(current->children[2]);
                current = current->children[0];
            }
        }
        s.push(n->children[1]);
        unsigned next_step = step + content[current_div];
        while (!s.empty())
        {
            Getinitarray_const(s.top(), initvals, content, step, current_div + 1);
            s.pop();
        }
        step = next_step;
    }
    return;
}

void AST::Getinitarray_var(syntax_tree_node *n, std::vector<std::shared_ptr<ASTInitarray>> &initvals, 
                           std::vector<unsigned> &content, unsigned &step, int current_div)
{
    if (n->children_num == 1)
    {
        if (!is_global)
        {
            auto val = static_cast<ASTExp *>(transform_node_iter(n->children[0]));
            auto val_shared = std::shared_ptr<ASTExp>(val);
            initvals[step]->is_exp = true;
            initvals[step]->exp = val_shared;
            step++;
        }
        else
        {
            is_const = true;
            transform_node_iter(n->children[0]);
            is_const = false;
            initvals[step]->is_exp = false;
            if (ast_tmp_type == TYPE_INT)
                initvals[step]->i_val = tmp_ival;
            else
                initvals[step]->f_val = tmp_fval;
            step++;
        }
    }
    else if (n->children_num == 2)
    {
        step += content[current_div];
    }
    else if (n->children_num == 3)
    {
        unsigned next_step = step + content[current_div];
        Getinitarray_var(n->children[1], initvals, content, step, current_div + 1);
        step = next_step;
    }
    else if (n->children_num == 4)
    {
        std::stack<syntax_tree_node *> s;
        auto current = n->children[2];
        while (1)
        {
            if (current->children_num == 2)
            {
                s.push(current->children[1]);
                break;
            }
            else if (current->children_num == 3)
            {
                s.push(current->children[2]);
                current = current->children[0];
            }
        }
        s.push(n->children[1]);
        unsigned next_step = step + content[current_div];
        while (!s.empty())
        {
            Getinitarray_var(s.top(), initvals, content, step, current_div + 1);
            s.pop();
        }
        step = next_step;
    }
    return;
}

ASTNode *AST::transform_node_iter(syntax_tree_node *n)
{
    if (_STR_EQ(n->name, "compunit"))
    {
        auto node = new ASTCompunit();
        std::stack<syntax_tree_node *> s;
        syntax_tree_node *current = n;
        while (1)
        {
            if (current->children_num == 1)
            {
                s.push(current->children[0]);
                break;
            }
            else if (current->children_num == 2)
            {
                s.push(current->children[1]);
                current = current->children[0];
            }
        }
        while (!s.empty())
        {
            auto child_node = static_cast<ASTDecl *>(transform_node_iter(s.top()));
            auto child_node_shared = std::shared_ptr<ASTDecl>(child_node);
            node->decls.push_back(child_node_shared);
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "decl"))
    {
        if (n->children_num == 1)
            return transform_node_iter(n->children[0]);
        return nullptr;
    }
    else if (_STR_EQ(n->name, "constdecl"))
    {
        auto node = new ASTConstdecl();
        node->datastay = CONST;
        if (_STR_EQ(n->children[1]->children[0]->name, "int"))
            node->type = TYPE_INT;
        else if (_STR_EQ(n->children[1]->children[0]->name, "float"))
            node->type = TYPE_FLOAT;
        ast_tmp_type = node->type;
        std::stack<syntax_tree_node *> s;
        if (n->children_num == 5)
        {
            syntax_tree_node *current = n->children[3];
            while (1)
            {
                if (current->children_num == 2)
                {
                    s.push(current->children[1]);
                    break;
                }
                else if (current->children_num == 3)
                {
                    s.push(current->children[2]);
                    current = current->children[0];
                }
            }
        }
        s.push(n->children[2]);
        while (!s.empty())
        {
            auto child_node = static_cast<ASTConstdef *>(transform_node_iter(s.top()));
            auto child_node_shared = std::shared_ptr<ASTConstdef>(child_node);
            node->constdefs.push_back(child_node_shared);
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "constdef"))
    {
        auto node = new ASTConstdef();
        node->id = n->children[0]->name;
        if (n->children_num == 3)
        {
            node->is_array = false;
            is_const = true;
            transform_node_iter(n->children[2]->children[0]);
            is_const = false;
            if (ast_tmp_type == TYPE_INT)
                node->constexp_i = tmp_ival;
            else
                node->constexp_f = tmp_fval;
            if (ast_tmp_type == TYPE_INT)
                const_int_val.insert({node->id, node->constexp_i});
            else
                const_float_val.insert({node->id, node->constexp_f});
        }
        else if (n->children_num == 4)
        {
            node->is_array = true;
            SysYType r_type = ast_tmp_type;
            // constexps
            syntax_tree_node *current = n->children[1];
            std::stack<syntax_tree_node *> s;
            while (1)
            {
                if (current->children_num == 3)
                {
                    s.push(current->children[1]);
                    break;
                }
                else if (current->children_num == 4)
                {
                    s.push(current->children[2]);
                    current = current->children[0];
                }
            }
            while (!s.empty())
            {
                is_const = true;
                ast_tmp_type = TYPE_INT;
                transform_node_iter(s.top());
                if (is_const && tmp_ival > 0)
                    node->constexps.push_back(tmp_ival);
                s.pop();
            }
            is_const = false;
            ast_tmp_type = r_type;
            // constinitvals
            unsigned element_num = 1;
            int now_div = node->constexps.size();
            std::vector<unsigned> content(now_div, 0);
            for (int i = 0; i < now_div; i++)
            {
                element_num = element_num * (unsigned)node->constexps[now_div - 1 - i];
                content[now_div - 1 - i] = element_num;
            }
            for (int i = 0; i <= element_num; i++)
            {
                auto const_init_array = new ASTInitarray();
                auto const_init_array_shared = std::shared_ptr<ASTInitarray>(const_init_array);
                node->constinitvals.push_back(const_init_array_shared);
            }
            auto start_initval = n->children[3];
            unsigned step = 1;
            int current_div = 0;
            Getinitarray_const(start_initval, node->constinitvals, content, step, current_div);
        }
        return node;
    }
    else if (_STR_EQ(n->name, "vardecl"))
    {
        auto node = new ASTVardecl();
        node->datastay = VAR;
        if (_STR_EQ(n->children[0]->children[0]->name, "int"))
            node->type = TYPE_INT;
        else if (_STR_EQ(n->children[0]->children[0]->name, "float"))
            node->type = TYPE_FLOAT;
        std::stack<syntax_tree_node *> s;
        if (n->children_num == 4)
        {
            syntax_tree_node *current = n->children[2];
            while (1)
            {
                if (current->children_num == 2)
                {
                    s.push(current->children[1]);
                    break;
                }
                else if (current->children_num == 3)
                {
                    s.push(current->children[2]);
                    current = current->children[0];
                }
            }
        }
        s.push(n->children[1]);
        while (!s.empty())
        {
            auto child_node = static_cast<ASTVardef *>(transform_node_iter(s.top()));
            auto child_node_shared = std::shared_ptr<ASTVardef>(child_node);
            node->vardefs.push_back(child_node_shared);
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "vardef"))
    {
        auto node = new ASTVardef();
        node->id = n->children[0]->name;
        if (n->children_num == 1 || n->children_num == 3)
        {
            node->is_array = false;
            if (n->children_num == 3)
            {
                if (!is_global)
                {
                    auto child_node = static_cast<ASTAddexp *>(transform_node_iter(n->children[2]->children[0]));
                    auto child_node_shared = std::shared_ptr<ASTAddexp>(child_node);
                    node->exp = child_node_shared;
                }
                else
                {
                    is_const = true;
                    transform_node_iter(n->children[2]->children[0]);
                    is_const = false;
                    if (ast_tmp_type == TYPE_INT)
                        node->global_ival = tmp_ival;
                    else
                        node->global_fval = tmp_fval;
                }
            }
        }
        else if (n->children_num == 2 || n->children_num == 4)
        {
            node->is_array = true;
            SysYType r_type = ast_tmp_type;
            // varexps
            syntax_tree_node *current = n->children[1];
            std::stack<syntax_tree_node *> s;
            while (1)
            {
                if (current->children_num == 3)
                {
                    s.push(current->children[1]);
                    break;
                }
                else if (current->children_num == 4)
                {
                    s.push(current->children[2]);
                    current = current->children[0];
                }
            }
            while (!s.empty())
            {
                is_const = true;
                ast_tmp_type = TYPE_INT;
                transform_node_iter(s.top());
                if (is_const && tmp_ival > 0)
                    node->varexps.push_back(tmp_ival);
                s.pop();
            }
            is_const = false;
            ast_tmp_type = r_type;
            // varinitvals
            if (n->children_num == 4)
            {
                unsigned element_num = 1;
                int now_div = node->varexps.size();
                std::vector<unsigned> content(now_div, 0);
                for (int i = 0; i < now_div; i++)
                {
                    element_num = element_num * node->varexps[now_div - 1 - i];
                    content[now_div - 1 - i] = element_num;
                }
                for (int i = 0; i <= element_num; i++)
                {
                    auto const_init_array = new ASTInitarray();
                    auto const_init_array_shared = std::shared_ptr<ASTInitarray>(const_init_array);
                    node->varinitvals.push_back(const_init_array_shared);
                }
                auto start_initval = n->children[3];
                unsigned step = 1;
                int current_div = 0;
                Getinitarray_var(start_initval, node->varinitvals, content, step, current_div);
            }
        }
        return node;
    }
    else if (_STR_EQ(n->name, "funcdef"))
    {
        is_global = false;
        auto node = new ASTFuncdef();
        if (_STR_EQ(n->children[0]->name, "void"))
            node->type = TYPE_VOID;
        else if (_STR_EQ(n->children[0]->children[0]->name, "int"))
            node->type = TYPE_INT;
        else if (_STR_EQ(n->children[0]->children[0]->name, "float"))
            node->type = TYPE_FLOAT;
        node->id = n->children[1]->name;
        std::stack<syntax_tree_node *> s;
        if (n->children_num == 6)
        {
            auto current = n->children[3];
            if (current->children_num == 2)
            {
                auto current_list = current->children[1];
                while (1)
                {
                    if (current_list->children_num == 2)
                    {
                        s.push(current_list->children[1]);
                        break;
                    }
                    else if (current_list->children_num == 3)
                    {
                        s.push(current_list->children[2]);
                        current_list = current_list->children[0];
                    }
                }
            }
            s.push(current->children[0]);
            while (!s.empty())
            {
                auto child_node = static_cast<ASTFuncfparam *>(transform_node_iter(s.top()));
                auto child_node_shared = std::shared_ptr<ASTFuncfparam>(child_node);
                node->funcfparams.push_back(child_node_shared);
                s.pop();
            }
            auto child_node_ = static_cast<ASTStmt *>(transform_node_iter(n->children[5]));
            auto child_node_shared_ = std::shared_ptr<ASTStmt>(child_node_);
            node->block = child_node_shared_;
        }
        else if (n->children_num == 5)
        {
            auto child_node = static_cast<ASTStmt *>(transform_node_iter(n->children[4]));
            auto child_node_shared = std::shared_ptr<ASTStmt>(child_node);
            node->block = child_node_shared;
        }
        is_global = true;
        return node;
    }
    else if (_STR_EQ(n->name, "funcfparam"))
    {
        auto node = new ASTFuncfparam();
        if (_STR_EQ(n->children[0]->children[0]->name, "int"))
            node->type = TYPE_INT;
        else if (_STR_EQ(n->children[0]->children[0]->name, "float"))
            node->type = TYPE_FLOAT;
        node->id = n->children[1]->name;
        if (n->children_num == 4 || n->children_num == 5)
            node->is_ptr = true;
        else if (n->children_num == 2)
            node->is_ptr = false;
        if (n->children_num == 5)
        {
            std::stack<syntax_tree_node *> s;
            auto current = n->children[4];
            while (1)
            {
                if (current->children_num == 3)
                {
                    s.push(current->children[1]);
                    break;
                }
                else if (current->children_num == 4)
                {
                    s.push(current->children[2]);
                    current = current->children[0];
                }
            }
            SysYType r_type = ast_tmp_type;
            while (!s.empty())
            {
                ast_tmp_type = TYPE_INT;
                is_const = true;
                transform_node_iter(s.top());
                if (is_const)
                    node->divs.push_back(tmp_ival);
                s.pop();
            }
            is_const = false;
            ast_tmp_type = r_type;
        }
        return node;
    }
    else if (_STR_EQ(n->name, "block"))
    {
        auto node = new ASTBlock();
        if (n->children_num == 3)
        {
            auto current = n->children[1];
            std::stack<syntax_tree_node *> s;
            while (1)
            {
                if (current->children_num == 1)
                {
                    s.push(current->children[0]);
                    break;
                }
                else if (current->children_num == 2)
                {
                    s.push(current->children[1]);
                    current = current->children[0];
                }
            }
            while (!s.empty())
            {
                auto child_node = static_cast<ASTStmt *>(transform_node_iter(s.top()->children[0]));
                auto child_node_shared = std::shared_ptr<ASTStmt>(child_node);
                node->stmts.push_back(child_node_shared);
                s.pop();
            }
        }
        return node;
    }
    else if (_STR_EQ(n->name, "stmt"))
    {
        if (_STR_EQ(n->children[0]->name, "lval"))
        {
            auto node = new ASTAssignstmt();
            if (n->children_num == 4)
            {
                auto child_node = static_cast<ASTLval *>(transform_node_iter(n->children[0]));
                auto child_node_shared = std::shared_ptr<ASTLval>(child_node);
                node->lval = child_node_shared;
                auto child_node_ = static_cast<ASTExp *>(transform_node_iter(n->children[2]));
                auto child_node_shared_ = std::shared_ptr<ASTExp>(child_node_);
                node->exp = child_node_shared_;
            }
            return node;
        }
        else if (_STR_EQ(n->children[0]->name, ";") || _STR_EQ(n->children[0]->name, "exp"))
        {
            auto node = new ASTSimplestmt();
            if (n->children_num == 2)
            {
                auto child_node = static_cast<ASTExp *>(transform_node_iter(n->children[0]));
                auto child_node_shared = std::shared_ptr<ASTExp>(child_node);
                node->exp = child_node_shared;
            }
            return node;
        }
        else if (_STR_EQ(n->children[0]->name, "block"))
        {
            return transform_node_iter(n->children[0]);
        }
        else if (_STR_EQ(n->children[0]->name, "if"))
        {
            auto node = new ASTIfstmt();
            auto child_node = static_cast<ASTLogicexp *>(transform_node_iter(n->children[2]));
            auto child_node_shared = std::shared_ptr<ASTLogicexp>(child_node);
            node->cond = child_node_shared;
            auto child_node_ = static_cast<ASTStmt *>(transform_node_iter(n->children[4]));
            auto child_node_shared_ = std::shared_ptr<ASTStmt>(child_node_);
            node->if_stmt = child_node_shared_;
            if (n->children_num == 7)
            {
                auto child_node__ = static_cast<ASTStmt *>(transform_node_iter(n->children[6]));
                auto child_node_shared__ = std::shared_ptr<ASTStmt>(child_node__);
                node->else_stmt = child_node_shared__;
            }
            return node;
        }
        else if (_STR_EQ(n->children[0]->name, "while"))
        {
            auto node = new ASTWhilestmt();
            if (n->children_num == 5)
            {
                auto child_node = static_cast<ASTLogicexp *>(transform_node_iter(n->children[2]));
                auto child_node_shared = std::shared_ptr<ASTLogicexp>(child_node);
                node->cond = child_node_shared;
                auto child_node_ = static_cast<ASTStmt *>(transform_node_iter(n->children[4]));
                auto child_node_shared_ = std::shared_ptr<ASTStmt>(child_node_);
                node->while_stmt = child_node_shared_;
            }
            return node;
        }
        else if (_STR_EQ(n->children[0]->name, "break"))
        {
            auto node = new ASTBreakstmt();
            return node;
        }
        else if (_STR_EQ(n->children[0]->name, "continue"))
        {
            auto node = new ASTContinuestmt();
            return node;
        }
        else if (_STR_EQ(n->children[0]->name, "return"))
        {
            auto node = new ASTReturnstmt();
            if (n->children_num == 3)
            {
                auto child_node = static_cast<ASTExp *>(transform_node_iter(n->children[1]));
                auto child_node_shared = std::shared_ptr<ASTExp>(child_node);
                node->exp = child_node_shared;
            }
            return node;
        }
        return nullptr;
    }
    else if (_STR_EQ(n->name, "exp"))
    {
        return transform_node_iter(n->children[0]);
    }
    else if (_STR_EQ(n->name, "cond"))
    {
        return transform_node_iter(n->children[0]);
    }
    else if (_STR_EQ(n->name, "lval"))
    {
        if (is_const)
        {
            if (ast_tmp_type == TYPE_INT)
            {
                if (const_int_val.find(n->children[0]->name) != const_int_val.end())
                    tmp_ival = const_int_val[n->children[0]->name];
            }
            else
            {
                if (const_float_val.find(n->children[0]->name) != const_float_val.end())
                    tmp_fval = const_float_val[n->children[0]->name];
            }
        }
        else
        {
            auto node = new ASTLval();
            node->id = n->children[0]->name;
            if (n->children_num == 2)
            {
                std::stack<syntax_tree_node *> s;
                auto current = n->children[1];
                while (1)
                {
                    if (current->children_num == 3)
                    {
                        s.push(current->children[1]);
                        break;
                    }
                    else if (current->children_num == 4)
                    {
                        s.push(current->children[2]);
                        current = current->children[0];
                    }
                }
                while (!s.empty())
                {
                    auto child_node = static_cast<ASTExp *>(transform_node_iter(s.top()));
                    auto child_node_shared = std::shared_ptr<ASTExp>(child_node);
                    node->exps.push_back(child_node_shared);
                    s.pop();
                }
            }
            return node;
        }
        return nullptr;
    }
    else if (_STR_EQ(n->name, "primaryexp"))
    {
        int choose;
        if (n->children_num == 3)
            choose = 1;
        else if (n->children_num == 1)
            choose = 0;
        if (is_const)
        {
            transform_node_iter(n->children[choose]);
        }
        else
        {
            auto node = new ASTPrimaryexp();
            auto child_node = static_cast<ASTExp *>(transform_node_iter(n->children[choose]));
            auto child_node_shared = std::shared_ptr<ASTExp>(child_node);
            node->exp = child_node_shared;
            return node;
        }
        return nullptr;
    }
    else if (_STR_EQ(n->name, "number"))
    {
        if (is_const)
        {
            if (n->children_num == 1)
            {
                if (n->children[0]->children_num == 1)
                {
                    if (_STR_EQ(n->children[0]->name, "integer"))
                    {
                        if (ast_tmp_type == TYPE_INT)
                        {
                            std::string num = n->children[0]->children[0]->name;
                            char *stop;
                            if (num.substr(0, 2) == "0x" || num.substr(0, 2) == "0x")
                                tmp_ival = strtol(num.c_str(), &stop, 16);
                            else if (num[0] == '0')
                                tmp_ival = strtol(num.c_str(), &stop, 8);
                            else
                                tmp_ival = strtol(num.c_str(), &stop, 10);
                        }
                        else
                            tmp_fval = std::stof(n->children[0]->children[0]->name);
                    }
                    else if (_STR_EQ(n->children[0]->name, "float"))
                    {
                        if (ast_tmp_type == TYPE_INT)
                        {
                            double f_val = std::stof(n->children[0]->children[0]->name);
                            tmp_ival = (int)(f_val);
                        }
                        else
                            tmp_fval = std::stof(n->children[0]->children[0]->name);
                    }
                }
            }
        }
        else
        {
            auto node = new ASTNum();
            if (n->children_num == 1)
            {
                if (n->children[0]->children_num == 1)
                {
                    if (_STR_EQ(n->children[0]->name, "integer"))
                    {
                        node->type = TYPE_INT;
                        std::string num = n->children[0]->children[0]->name;
                        char *stop;
                        if (num.substr(0, 2) == "0x" || num.substr(0, 2) == "0x")
                            node->int_val = strtol(num.c_str(), &stop, 16);
                        else if (num[0] == '0')
                            node->int_val = strtol(num.c_str(), &stop, 8);
                        else
                            node->int_val = strtol(num.c_str(), &stop, 10);
                    }
                    else if (_STR_EQ(n->children[0]->name, "float"))
                    {
                        node->type = TYPE_FLOAT;
                        node->float_val = std::stof(n->children[0]->children[0]->name);
                    }
                }
            }
            return node;
        }
        return nullptr;
    }
    else if (_STR_EQ(n->name, "unaryexp"))
    {
        if (is_const)
        {
            if (n->children_num == 3 || n->children_num == 4)
            {
                std::string error = "constexp_unaryexp";
                _SEMANTICS_ERROR_(error);
            }
            else if (n->children_num == 1)
            {
                transform_node_iter(n->children[0]);
            }
            else if (n->children_num == 2)
            {
                transform_node_iter(n->children[1]);
                if (is_const)
                {
                    if (ast_tmp_type == TYPE_INT)
                    {
                        if (_STR_EQ(n->children[0]->children[0]->name, "+"))
                            tmp_ival = +tmp_ival;
                        else if (_STR_EQ(n->children[0]->children[0]->name, "-"))
                            tmp_ival = -tmp_ival;
                        else if (_STR_EQ(n->children[0]->children[0]->name, "!"))
                            tmp_ival = (tmp_ival > 0) ? 1 : 0;
                    }
                    else
                    {
                        if (_STR_EQ(n->children[0]->children[0]->name, "+"))
                            tmp_fval = +tmp_fval;
                        else if (_STR_EQ(n->children[0]->children[0]->name, "-"))
                            tmp_fval = -tmp_fval;
                        else if (_STR_EQ(n->children[0]->children[0]->name, "!"))
                            tmp_fval = (tmp_fval > 0) ? 1 : 0;
                    }
                }
            }
        }
        else
        {
            auto node = new ASTUnaryexp();
            if (n->children_num == 3 || n->children_num == 4)
            {
                node->is_func = true;
                node->is_op = false;
                auto child_node = new ASTFuncexp();
                child_node->id = n->children[0]->name;
                if (n->children_num == 4)
                {
                    auto current = n->children[2];
                    std::stack<syntax_tree_node *> s;
                    if (current->children_num == 2)
                    {
                        auto current_list = current->children[1];
                        while (1)
                        {
                            if (current_list->children_num == 2)
                            {
                                s.push(current_list->children[1]);
                                break;
                            }
                            else if (current_list->children_num == 3)
                            {
                                s.push(current_list->children[2]);
                                current_list = current_list->children[0];
                            }
                        }
                    }
                    s.push(current->children[0]);
                    while (!s.empty())
                    {
                        auto child_node_ = static_cast<ASTExp *>(transform_node_iter(s.top()));
                        auto child_node_shared_ = std::shared_ptr<ASTExp>(child_node_);
                        child_node->funcrparams.push_back(child_node_shared_);
                        s.pop();
                    }
                }
                auto child_node_shared = std::shared_ptr<ASTFuncexp>(child_node);
                node->exp = child_node_shared;
            }
            else if (n->children_num == 1)
            {
                node->is_func = false;
                node->is_op = false;
                auto child_node = static_cast<ASTExp *>(transform_node_iter(n->children[0]));
                auto child_node_shared = std::shared_ptr<ASTExp>(child_node);
                node->exp = child_node_shared;
            }
            else if (n->children_num == 2)
            {
                node->is_func = false;
                node->is_op = true;
                if (_STR_EQ(n->children[0]->children[0]->name, "+"))
                    node->op = OP_PLUS;
                else if (_STR_EQ(n->children[0]->children[0]->name, "-"))
                    node->op = OP_UNPLUS;
                else if (_STR_EQ(n->children[0]->children[0]->name, "!"))
                    node->op = OP_NOT;
                auto child_node = static_cast<ASTExp *>(transform_node_iter(n->children[1]));
                auto child_node_shared = std::shared_ptr<ASTExp>(child_node);
                node->exp = child_node_shared;
            }
            return node;
        }
        return nullptr;
    }
    else if (_STR_EQ(n->name, "addexp"))
    {
        if (is_const)
        {
            int swap_ival;
            float swap_fval;
            if (n->children_num == 1)
            {
                transform_node_iter(n->children[0]);
            }
            else if (n->children_num == 3)
            {
                transform_node_iter(n->children[0]);
                if (ast_tmp_type == TYPE_INT)
                    swap_ival = tmp_ival;
                else
                    swap_fval = tmp_fval;
                transform_node_iter(n->children[2]);
                if (ast_tmp_type == TYPE_INT)
                {
                    if (_STR_EQ(n->children[1]->name, "+"))
                        tmp_ival = swap_ival + tmp_ival;
                    else if (_STR_EQ(n->children[1]->name, "-"))
                        tmp_ival = swap_ival - tmp_ival;
                }
                else
                {
                    if (_STR_EQ(n->children[1]->name, "+"))
                        tmp_fval = swap_fval + tmp_fval;
                    else if (_STR_EQ(n->children[1]->name, "-"))
                        tmp_fval = swap_fval - tmp_fval;
                }
            }
        }
        else
        {
            auto node = new ASTAddexp();
            if (n->children_num == 1)
            {
                auto child_node = static_cast<ASTMulexp *>(transform_node_iter(n->children[0]));
                auto child_node_shared = std::shared_ptr<ASTMulexp>(child_node);
                node->right_hand = child_node_shared;
            }
            else if (n->children_num == 3)
            {
                auto child_node = static_cast<ASTMulexp *>(transform_node_iter(n->children[2]));
                auto child_node_shared = std::shared_ptr<ASTMulexp>(child_node);
                node->right_hand = child_node_shared;
                auto child_node_ = static_cast<ASTAddexp *>(transform_node_iter(n->children[0]));
                auto child_node_shared_ = std::shared_ptr<ASTAddexp>(child_node_);
                node->left_hand = child_node_shared_;
                if (_STR_EQ(n->children[1]->name, "+"))
                    node->op = OP_ADD;
                else if (_STR_EQ(n->children[1]->name, "-"))
                    node->op = OP_SUB;
            }
            return node;
        }
        return nullptr;
    }
    else if (_STR_EQ(n->name, "mulexp"))
    {
        if (is_const)
        {
            int swap_ival;
            float swap_fval;
            if (n->children_num == 1)
            {
                transform_node_iter(n->children[0]);
            }
            else if (n->children_num == 3)
            {
                transform_node_iter(n->children[0]);
                if (ast_tmp_type == TYPE_INT)
                    swap_ival = tmp_ival;
                else
                    swap_fval = tmp_fval;
                transform_node_iter(n->children[2]);
                if (ast_tmp_type == TYPE_INT)
                {
                    if (_STR_EQ(n->children[1]->children[0]->name, "*"))
                        tmp_ival = swap_ival * tmp_ival;
                    else if (_STR_EQ(n->children[1]->children[0]->name, "/"))
                        tmp_ival = swap_ival / tmp_ival;
                    else if (_STR_EQ(n->children[1]->children[0]->name, "%"))
                        tmp_ival = swap_ival % tmp_ival;
                }
                else
                {
                    if (_STR_EQ(n->children[1]->children[0]->name, "*"))
                        tmp_fval = swap_fval * tmp_fval;
                    else if (_STR_EQ(n->children[1]->children[0]->name, "/"))
                        tmp_fval = swap_fval / tmp_fval;
                    else if (_STR_EQ(n->children[1]->children[0]->name, "%"))
                    {
                        std::string error = "mulexp : MOD cannot be float ! ";
                        _SEMANTICS_ERROR_(error);
                    }
                }
            }
        }
        else
        {
            auto node = new ASTMulexp();
            if (n->children_num == 1)
            {
                auto child_node = static_cast<ASTUnaryexp *>(transform_node_iter(n->children[0]));
                auto child_node_shared = std::shared_ptr<ASTUnaryexp>(child_node);
                node->right_hand = child_node_shared;
            }
            else if (n->children_num == 3)
            {
                auto child_node = static_cast<ASTUnaryexp *>(transform_node_iter(n->children[2]));
                auto child_node_shared = std::shared_ptr<ASTUnaryexp>(child_node);
                node->right_hand = child_node_shared;
                auto child_node_ = static_cast<ASTMulexp *>(transform_node_iter(n->children[0]));
                auto child_node_shared_ = std::shared_ptr<ASTMulexp>(child_node_);
                node->left_hand = child_node_shared_;
                if (n->children[1]->children_num == 1)
                {
                    if (_STR_EQ(n->children[1]->children[0]->name, "*"))
                        node->op = OP_MUL;
                    else if (_STR_EQ(n->children[1]->children[0]->name, "/"))
                        node->op = OP_DIV;
                    else if (_STR_EQ(n->children[1]->children[0]->name, "%"))
                        node->op = OP_MOD;
                }
            }
            return node;
        }
        return nullptr;
    }
    else if (_STR_EQ(n->name, "relexp"))
    {
        auto node = new ASTRelexp();
        if (n->children_num == 1)
        {
            auto child_node = static_cast<ASTAddexp *>(transform_node_iter(n->children[0]));
            auto child_node_shared = std::shared_ptr<ASTAddexp>(child_node);
            node->right_hand = child_node_shared;
        }
        else if (n->children_num == 3)
        {
            auto child_node = static_cast<ASTAddexp *>(transform_node_iter(n->children[2]));
            auto child_node_shared = std::shared_ptr<ASTAddexp>(child_node);
            node->right_hand = child_node_shared;
            auto child_node_ = static_cast<ASTRelexp *>(transform_node_iter(n->children[0]));
            auto child_node_shared_ = std::shared_ptr<ASTRelexp>(child_node_);
            node->left_hand = child_node_shared_;
            if (n->children[1]->children_num == 1)
            {
                if (_STR_EQ(n->children[1]->children[0]->name, "<"))
                    node->op = OP_LT;
                else if (_STR_EQ(n->children[1]->children[0]->name, ">"))
                    node->op = OP_GT;
                else if (_STR_EQ(n->children[1]->children[0]->name, "<="))
                    node->op = OP_LE;
                else if (_STR_EQ(n->children[1]->children[0]->name, ">="))
                    node->op = OP_GE;
            }
        }
        return node;
    }
    else if (_STR_EQ(n->name, "eqexp"))
    {
        auto node = new ASTEqexp();
        if (n->children_num == 1)
        {
            auto child_node = static_cast<ASTRelexp *>(transform_node_iter(n->children[0]));
            auto child_node_shared = std::shared_ptr<ASTRelexp>(child_node);
            node->right_hand = child_node_shared;
        }
        else if (n->children_num == 3)
        {
            auto child_node = static_cast<ASTRelexp *>(transform_node_iter(n->children[2]));
            auto child_node_shared = std::shared_ptr<ASTRelexp>(child_node);
            node->right_hand = child_node_shared;
            auto child_node_ = static_cast<ASTEqexp *>(transform_node_iter(n->children[0]));
            auto child_node_shared_ = std::shared_ptr<ASTEqexp>(child_node_);
            node->left_hand = child_node_shared_;
            if (n->children[1]->children_num == 1)
            {
                if (_STR_EQ(n->children[1]->children[0]->name, "=="))
                    node->op = OP_EQ;
                else if (_STR_EQ(n->children[1]->children[0]->name, "!="))
                    node->op = OP_NEQ;
            }
        }
        return node;
    }
    else if (_STR_EQ(n->name, "landexp"))
    {
        auto node = new ASTLandexp();
        std::stack<syntax_tree_node *> s;
        auto current = n;
        while (1)
        {
            if (current->children_num == 1)
            {
                s.push(current->children[0]);
                break;
            }
            else if (current->children_num == 3)
            {
                s.push(current->children[2]);
                current = current->children[0];
            }
        }
        while (!s.empty())
        {
            auto child_node = static_cast<ASTEqexp *>(transform_node_iter(s.top()));
            auto child_node_shared = std::shared_ptr<ASTEqexp>(child_node);
            node->eqexps.push_back(child_node_shared);
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "lorexp"))
    {
        auto node = new ASTLorexp();
        std::stack<syntax_tree_node *> s;
        auto current = n;
        while (1)
        {
            if (current->children_num == 1)
            {
                s.push(current->children[0]);
                break;
            }
            else if (current->children_num == 3)
            {
                s.push(current->children[2]);
                current = current->children[0];
            }
        }
        while (!s.empty())
        {
            auto child_node = static_cast<ASTLandexp *>(transform_node_iter(s.top()));
            auto child_node_shared = std::shared_ptr<ASTLandexp>(child_node);
            node->landexps.push_back(child_node_shared);
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "constexp"))
    {
        if (n->children_num != 1)
        {
            _SYNTAX_TREE_ERROR_(n->name);
        }
        return transform_node_iter(n->children[0]);
    }
    else
    {
        std::string error_name = n->name;
        std::string error = "not exist";
        error_name += error;
        _SYNTAX_TREE_ERROR_(error_name);
        return nullptr;
    }
    return nullptr;
}

Value *ASTCompunit ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTDecl ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTConstdecl ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTVardecl ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTConstdef ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTVardef ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTInitarray ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTFuncdef ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTFuncfparam ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTStmt ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTBlock ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTAssignstmt ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTIfstmt ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTSimplestmt ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTWhilestmt ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTBreakstmt ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTContinuestmt ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTReturnstmt ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTExp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTAddexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTMulexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTUnaryexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTFuncexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTPrimaryexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTRelexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTEqexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTLogicexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTLandexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTLorexp ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTLval ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTNum ::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
