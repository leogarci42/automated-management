#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


void generate_llvm_ifelse(t_token *node, FILE *f, int *if_count)
{
	char v1[64], op[8], v2[64];
	sscanf(node->context, "%s %s %s", v1, op, v2);
	char ll_v1[64], ll_v2[64];
	to_llvm_val(v1, ll_v1);
	to_llvm_val(v2, ll_v2);
	int i = *if_count++;
	const char *icmp_op = "sgt";
	if (strcmp(op, "<") == 0)
		icmp_op = "slt";
	else if (strcmp(op, "==") == 0)
		icmp_op = "eq";
	fprintf(f, "  %%cmp%d = icmp %s i32 %s, %s\n", i, icmp_op, ll_v1, ll_v2);
	fprintf(f, "  br i1 %%cmp%d, label %%if.then%d, label %%if.end%d\n", i, i, i);
	fprintf(f, "if.then%d:\n", i);
	generate_node(f, node->body); 
	fprintf(f, "if.end%d:\n", i);
}

void generate_llvm_compute_call(t_token *node, FILE *f, int *reg_count)
{
	if (strncmp(node->context, "print(", 6) == 0)
	{
		char p_arg[64];
		sscanf(node->context, "print(%63[^)])", p_arg);
		char ll_p[64];
		to_llvm_val(p_arg, ll_p);
		fprintf(f, "  %%print_res%d = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %s)\n", *reg_count++, ll_p);
	}
	else
	{
		char dst[64], func[64], arg[64];
		int n = sscanf(node->context, "%s = %63[^(](%63[^)])", dst, func, arg);
		if (n == 3)
		{
			char ll_dst[64], ll_arg[64];
			to_llvm_val(dst, ll_dst);
			to_llvm_val(arg, ll_arg);
			fprintf(f, "  %s = call i32 @%s(i32 %s)\n", ll_dst, trim_space(func), ll_arg);
		}
	}	
}

void generate_llvm_statement(t_token *node, FILE *f)
{
	char c1[64], c2[64], c3[64], c4[64];
	int n = sscanf(node->context, "%s = %s %s %s", c1, c2, c3, c4);
	if (n == 4)
	{
		char ll_v2[64], ll_v4[64], ll_v1[64];
		to_llvm_val(c2, ll_v2);
		to_llvm_val(c4, ll_v4);
		to_llvm_val(c1, ll_v1);
		
		const char *op = "add";
		if (strcmp(c3, "-") == 0)
			op = "sub nsw";
		else if (strcmp(c3, "*") == 0)
			op = "mul nsw";
		fprintf(f, "  %s = %s i32 %s, %s\n", ll_v1, op, ll_v2, ll_v4);
	}
	else if (n == 2)
	{
		char ll_v1[64], ll_v2[64];
		to_llvm_val(c1, ll_v1);
		to_llvm_val(c2, ll_v2);
		fprintf(f, "  %s = add i32 %s, 0\n", ll_v1, ll_v2);
	}
}

void generate_llvm_ret_statement(t_token *node, FILE *f, int *reg_count)
{
	char func[64], arg[64];
	if (sscanf(node->context, "return %63[^(](%63[^)])", func, arg) == 2)
	{
		char v1[64], op[8], v2[64];
		int n = sscanf(arg, "%s %s %s", v1, op, v2);
		char arg_val[64];
		if (n == 3)
		{
			char ll_v1[64], ll_v2[64];
			to_llvm_val(v1, ll_v1);
			to_llvm_val(v2, ll_v2);
			const char *op_nm = "add";
			if (strcmp(op, "-") == 0) op_nm = "sub nsw";
			fprintf(f, "  %%tmp_arg%d = %s i32 %s, %s\n", *reg_count, op_nm, ll_v1, ll_v2);
			sprintf(arg_val, "%%tmp_arg%d", *reg_count++);
		} 
		else
		to_llvm_val(arg, arg_val);
		fprintf(f, "  %%call%d = call i32 @%s(i32 %s)\n", *reg_count, trim_space(func), arg_val);
		fprintf(f, "  ret i32 %%call%d\n", *reg_count);
		(*reg_count)++;
	}
	else
	{
		char c1[64];
		sscanf(node->context, "return %s", c1);
		char ll_v1[64];
		to_llvm_val(c1, ll_v1);
		fprintf(f, "  ret i32 %s\n", ll_v1);
	}
}
