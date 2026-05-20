#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void generate_node_gpu(FILE *f, t_token *node, int *reg_count, int *if_count)
{
	if (!node)
		return;
	if (node->type == statement)
		generate_llvm_statement(node, f, reg_count);
	else if (node->type == ret_statement)
		generate_llvm_ret_statement(node, f, reg_count);
	else if (node->type == ifelse)
		generate_llvm_ifelse(node, f, if_count, reg_count);
	else if (node->type == compute_call)
		generate_llvm_compute_call(node, f, reg_count);
	else if (node->type == loop)
		generate_llvm_loop(node, f, reg_count);
	generate_node_gpu(f, node->next, reg_count, if_count);
}

void generate_llvm_ir_gpu(t_token *ast, const char *outfile, const char *target_triple)
{
	FILE *f = fopen(outfile, "w");
	if (!f) return;

	fprintf(f, "; ModuleID = 'gpu_output'\n");
	fprintf(f, "source_filename = \"gpu_output.cucpp\"\n");
	if (target_triple && target_triple[0] != '\0')
		fprintf(f, "target triple = \"%s\"\n\n", target_triple);

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

	for (t_token *curr = ast; curr; curr = curr->next)
	{
		if (curr->type != compute || curr->exec_target != EXEC_GPU)
			continue;
		int reg_count = 0;
		int if_count = 0;
		reset_var_versions();
		reset_arrays();
		if (curr->context)
		{
			strncpy(current_arg, curr->context, sizeof(current_arg) - 1);
			current_arg[sizeof(current_arg) - 1] = '\0';
			current_arg[strcspn(current_arg, "\r\n ")] = 0;
		}
		else
			current_arg[0] = '\0';
		if (strlen(current_arg) > 0)
			fprintf(f, "define i32 @%s(i32 %%%s_arg) {\n", curr->name, current_arg);
		else
			fprintf(f, "define i32 @%s() {\n", curr->name);
		fprintf(f, "entry:\n");
		generate_node_gpu(f, curr->body, &reg_count, &if_count);
		fprintf(f, "}\n\n");
	}
	fclose(f);
}
