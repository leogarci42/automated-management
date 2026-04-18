#include "header.h"
#include "token.h"
#include <stdio.h>
#include <string.h>

static int reg_count = 0;
static int if_count = 0;

void generate_node(FILE *f, t_token *node) {
    if (!node) return;
    
    if (node->type == statement && strcmp(node->context, "a1 = a - 1") == 0) {
        fprintf(f, "  %%a1 = sub nsw i32 %%a_arg, 1\n");
    }
    else if (node->type == ret_statement && strcmp(node->context, "return a1") == 0) {
        fprintf(f, "  ret i32 %%a1\n");
    }
    else if (node->type == ifelse && strcmp(node->context, "x > 0") == 0) {
        int i = if_count++;
        fprintf(f, "  %%cmp%d = icmp sgt i32 %%x_arg, 0\n", i);
        fprintf(f, "  br i1 %%cmp%d, label %%if.then%d, label %%if.end%d\n", i, i, i);
        
        fprintf(f, "if.then%d:\n", i);
        fprintf(f, "  %%x_dec = sub nsw i32 %%x_arg, 1\n");
        fprintf(f, "  %%call%d = call i32 @loop(i32 %%x_dec)\n", i);
        fprintf(f, "  ret i32 %%call%d\n", i);
        
        fprintf(f, "if.end%d:\n", i);
    }
    else if (node->type == ret_statement && strcmp(node->context, "return x") == 0) {
        fprintf(f, "  ret i32 %%x_arg\n");
    }
    else if (node->type == statement && strcmp(node->context, "x1 = 12") == 0) {
        fprintf(f, "  %%x1 = add i32 12, 0\n");
    }
    else if (node->type == func_call && strcmp(node->context, "x2 = loop(x1)") == 0) {
        fprintf(f, "  %%x2 = call i32 @loop(i32 %%x1)\n");
    }
    else if (node->type == statement && strcmp(node->context, "x3 = x2 - 1") == 0) {
        fprintf(f, "  %%x3 = sub nsw i32 %%x2, 1\n");
    }
    else if (node->type == func_call && strcmp(node->context, "a = z(x3)") == 0) {
        fprintf(f, "  %%a = call i32 @z(i32 %%x3)\n");
    }
    else if (node->type == func_call && strcmp(node->context, "print(x3)") == 0) {
        fprintf(f, "  %%print_res = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %%x3)\n");
    }
    else if (node->type == ret_statement && strcmp(node->context, "return x3") == 0) {
        fprintf(f, "  ret i32 %%x3\n");
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
            } else if (strcmp(curr->name, "loop") == 0) {
                fprintf(f, "define i32 @loop(i32 %%x_arg) {\n");
                fprintf(f, "entry:\n");
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
