#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int reg_count = 0;
static int if_count = 0;
static bool g_has_return = false;
char current_arg[256];

void generate_node(FILE *f, t_token *node) 
{
	if (!node)
		return;
	if (node->type == statement)
		generate_llvm_statement(node, f, &reg_count);
	else if (node->type == ret_statement)
	{
		g_has_return = true;
		generate_llvm_ret_statement(node, f, &reg_count);
	}
	else if (node->type == ifelse)
		generate_llvm_ifelse(node, f, &if_count, &reg_count);
	else if (node->type == compute_call)
		generate_llvm_compute_call(node, f, &reg_count);
	else if (node->type == loop)
		generate_llvm_loop(node, f, &reg_count);
	else if (node->type == increment)
		generate_llvm_inc_dec(node, f, &reg_count, true);
	else if (node->type == decrement)
		generate_llvm_inc_dec(node, f, &reg_count, false);

	generate_node(f, node->next);
}

void generate_llvm_ir_cpu(t_token *ast, const char *outfile)
{
	FILE *f = fopen(outfile, "w");
	if (!f) return;

	fprintf(f, "; ModuleID = 'test.cucpp'\n");
	fprintf(f, "source_filename = \"test.cucpp\"\n\n");

	fprintf(f, "@.str = private unnamed_addr constant [4 x i8] c\"%%d\\0A\\00\", align 1\n");
	fprintf(f, "declare i32 @printf(i8*, ...)\n");
	fprintf(f, "declare void @exit(i32)\n");
	fprintf(f, "@.arr_oob = private unnamed_addr constant [27 x i8] c\"array index out of bounds\\0A\\00\", align 1\n\n");
	fprintf(f, "define void @cucpp_bounds_check(i32 %%idx, i32 %%size) {\n");
	fprintf(f, "entry:\n");
	fprintf(f, "  %%cmp0 = icmp slt i32 %%idx, 0\n");
	fprintf(f, "  %%cmp1 = icmp sge i32 %%idx, %%size\n");
	fprintf(f, "  %%bad = or i1 %%cmp0, %%cmp1\n");
	fprintf(f, "  br i1 %%bad, label %%oob, label %%ok\n");
	fprintf(f, "oob:\n");
	fprintf(f, "  %%msg = getelementptr inbounds [27 x i8], [27 x i8]* @.arr_oob, i64 0, i64 0\n");
	fprintf(f, "  call i32 (i8*, ...) @printf(i8* %%msg)\n");
	fprintf(f, "  call void @exit(i32 1)\n");
	fprintf(f, "  br label %%ok\n");
	fprintf(f, "ok:\n");
	fprintf(f, "  ret void\n");
	fprintf(f, "}\n\n");

	t_token *curr = ast;
	while (curr)
	{
		if (curr->type == compute && curr->exec_target == EXEC_CPU)
		{
			reg_count = 0;
			reset_var_versions();
			reset_arrays();
			if (curr->context)
			{
				strcpy(current_arg, curr->context);
				current_arg[strcspn(current_arg, "\r\n ")] = 0;
			}
			else
				current_arg[0] = '\0';
			if (strlen(current_arg) > 0) 
				fprintf(f, "define i32 @%s(i32 %%%s_arg) {\n", curr->name, current_arg);
			else
				fprintf(f, "define i32 @%s() {\n", curr->name);
			fprintf(f, "entry:\n");
			
			g_has_return = false;
			generate_node(f, curr->body);
			if (!g_has_return)
				fprintf(f, "  ret i32 0\n");
			fprintf(f, "}\n\n");
		}
		curr = curr->next;
	}
	fclose(f);
}
