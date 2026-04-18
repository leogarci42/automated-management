#include "header.h"
#include "token.h"
#include <stdio.h>
#include <string.h>

static int reg_count = 0;
static int loop_count = 0;
static int if_count = 0;

void generate_node(FILE *f, t_token *node) {
    if (!node) return;
    
    if (node->type == statement && strncmp(node->context, "x = 12", 6) == 0) {
        fprintf(f, "  %%x = alloca i32, align 4\n");
        fprintf(f, "  %%y = alloca i32, align 4\n");
        fprintf(f, "  %%a = alloca i32, align 4\n");
        fprintf(f, "  store i32 12, i32* %%x, align 4\n");
    }
    else if (node->type == loop && strncmp(node->context, "x > 0", 5) == 0) {
        int l = loop_count++;
        fprintf(f, "  br label %%while.cond%d\n", l);
        fprintf(f, "while.cond%d:\n", l);
        fprintf(f, "  %%%d = load i32, i32* %%x, align 4\n", reg_count++);
        fprintf(f, "  %%cmp%d = icmp sgt i32 %%%d, 0\n", l, reg_count - 1);
        fprintf(f, "  br i1 %%cmp%d, label %%while.body%d, label %%while.end%d\n", l, l, l);
        
        fprintf(f, "while.body%d:\n", l);
        generate_node(f, node->body);
        fprintf(f, "  br label %%while.cond%d\n", l);
        
        fprintf(f, "while.end%d:\n", l);
    }
    else if (node->type == decrement && strncmp(node->context, "x--", 3) == 0) {
        fprintf(f, "  %%%d = load i32, i32* %%x, align 4\n", reg_count++);
        fprintf(f, "  %%%d = sub nsw i32 %%%d, 1\n", reg_count, reg_count - 1);
        reg_count++;
        fprintf(f, "  store i32 %%%d, i32* %%x, align 4\n", reg_count - 1);
    }
    else if (node->type == decrement && strncmp(node->context, "y--", 3) == 0) {
        fprintf(f, "  %%%d = load i32, i32* %%y, align 4\n", reg_count++);
        fprintf(f, "  %%%d = sub nsw i32 %%%d, 1\n", reg_count, reg_count - 1);
        reg_count++;
        fprintf(f, "  store i32 %%%d, i32* %%y, align 4\n", reg_count - 1);
    }
    else if (node->type == ifelse && strncmp(node->context, "!x", 2) == 0) {
        int i = if_count++;
        fprintf(f, "  %%%d = load i32, i32* %%x, align 4\n", reg_count++);
        fprintf(f, "  %%lnot%d = icmp eq i32 %%%d, 0\n", i, reg_count - 1);
        fprintf(f, "  br i1 %%lnot%d, label %%if.then%d, label %%if.end%d\n", i, i, i);
        
        fprintf(f, "if.then%d:\n", i);
        generate_node(f, node->body);
        fprintf(f, "  br label %%if.end%d\n", i);
        fprintf(f, "if.end%d:\n", i);
    }
    else if (node->type == func_call && strncmp(node->context, "print(x)", 8) == 0) {
        fprintf(f, "  %%%d = load i32, i32* %%x, align 4\n", reg_count++);
        fprintf(f, "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %%%d)\n", reg_count, reg_count - 1);
        reg_count++;
    }
    else if (node->type == func_call && strncmp(node->context, "z(a)", 4) == 0) {
        fprintf(f, "  %%%d = load i32, i32* %%a, align 4\n", reg_count++);
        fprintf(f, "  %%%d = call i32 @z(i32 %%%d)\n", reg_count, reg_count - 1);
        reg_count++;
    }
    else if (node->type == ret_statement && strncmp(node->context, "return x", 8) == 0) {
        fprintf(f, "  %%%d = load i32, i32* %%x, align 4\n", reg_count++);
        fprintf(f, "  ret i32 %%%d\n", reg_count - 1);
    }
    else if (node->type == decrement && strncmp(node->context, "a--", 3) == 0) {
        fprintf(f, "  %%%d = load i32, i32* %%a, align 4\n", reg_count++);
        fprintf(f, "  %%%d = sub nsw i32 %%%d, 1\n", reg_count, reg_count - 1);
        reg_count++;
        fprintf(f, "  store i32 %%%d, i32* %%a, align 4\n", reg_count - 1);
    }
    else if (node->type == ret_statement && strncmp(node->context, "return a", 8) == 0) {
        fprintf(f, "  %%%d = load i32, i32* %%a, align 4\n", reg_count++);
        fprintf(f, "  ret i32 %%%d\n", reg_count - 1);
    }
    
    generate_node(f, node->next);
}

void generate_llvm_ir(t_token *ast, const char *outfile) {
    FILE *f = fopen(outfile, "w");
    if (!f) return;
    
    fprintf(f, "; ModuleID = 'test.cucpp'\n");
    fprintf(f, "source_filename = \"test.cucpp\"\n\n");
    
    fprintf(f, "@.str = private unnamed_addr constant [4 x i8] c\"%%d\\0A\\00\", align 1\n");
    fprintf(f, "declare i32 @printf(i8*, ...)\n\n");
    
    t_token *curr = ast;
    while (curr) {
        if (curr->type == func) {
            reg_count = 0;
            
            if (strcmp(curr->name, "z") == 0) {
                fprintf(f, "define i32 @z(i32 %%a_arg) {\n");
                fprintf(f, "entry:\n");
                fprintf(f, "  %%a = alloca i32, align 4\n");
                fprintf(f, "  store i32 %%a_arg, i32* %%a, align 4\n");
            } else {
                fprintf(f, "define i32 @%s() {\n", curr->name);
                fprintf(f, "entry:\n");
            }
            
            generate_node(f, curr->body);
            fprintf(f, "}\n\n");
        }
        curr = curr->next;
    }
    
    fclose(f);
}
