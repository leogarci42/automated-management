#include "header.h"

void print_token(t_token *token, int depth)
{
    if (!token) return;
    
    for (int i = 0; i < depth; i++)
        printf("  ");
        
    if (token->type == func)
    {
        printf("[\033[36mFUNC\033[0m] \033[32m%s\033[0m (Context: \033[33m%s\033[0m)\n", token->name ? token->name : "anon", token->context ? token->context : "none");
    }
    else if (token->type == ifelse)
        printf("[\033[36mIF/ELSE\033[0m] (Condition: \033[33m%s\033[0m)\n", token->context ? token->context : "none");
    else if (token->type == loop)
        printf("[\033[36mLOOP\033[0m] (Condition: \033[33m%s\033[0m)\n", token->context ? token->context : "none");
    else if (token->type == assignment)
        printf("[\033[36mASSIGNMENT\033[0m] \033[32m%s\033[0m = \033[35m%s\033[0m (Type: %s)\n", 
               token->name ? token->name : "anon", 
               token->context ? token->context : "none", 
               token->var_type == TYPE_INT ? "int32" : (token->var_type == TYPE_CHAR ? "char" : (token->var_type == TYPE_VAR ? "var" : "unknown")));
    else if (token->type == increment)
        printf("[\033[36mINCREMENT\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
    else if (token->type == decrement)
        printf("[\033[36mDECREMENT\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
    else if (token->type == func_call)
        printf("[\033[36mFUNC_CALL\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
    else if (token->type == ret_statement)
        printf("[\033[36mRETURN\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
    else if (token->type == statement)
        printf("[\033[36mSTATEMENT\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
        
    if (token->body)
    {
        for (int i = 0; i < depth; i++)
            printf("  ");
        printf("  |\n");
        for (int i = 0; i < depth; i++)
            printf("  ");
        printf("  \\-> [BODY]\n");
        print_token(token->body, depth + 1);
    }
    
    if (token->lhs)
    {
        for (int i = 0; i < depth; i++)
            printf("  ");
        printf("  |-> [LHS]\n");
        print_token(token->lhs, depth + 1);
    }

    if (token->rhs)
    {
        for (int i = 0; i < depth; i++)
            printf("  ");
        printf("  |-> [RHS]\n");
        print_token(token->rhs, depth + 1);
    }

    if (token->next)
    {
        print_token(token->next, depth);
    }
}
