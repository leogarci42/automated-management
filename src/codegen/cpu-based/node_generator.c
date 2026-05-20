#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int loop_count = 0;


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
			char dst_ssa[96];
			snprintf(dst_ssa, sizeof(dst_ssa), "%s_v%d", dst, (*reg_count)++);
			set_var_version(dst, dst_ssa);
			snprintf(ll_dst, sizeof(ll_dst), "%%%s", dst_ssa);
			to_llvm_val(arg, ll_arg);
			fprintf(f, "  %s = call i32 @%s(i32 %s)\n", ll_dst, trim_space(func), ll_arg);
		}
	}	
}

void generate_llvm_statement(t_token *node, FILE *f, int *reg_count)
{
	char c1[64], c2[64], c3[64], c4[64];
	int n = sscanf(node->context, "%s = %s %s %s", c1, c2, c3, c4);
	if (n == 4)
	{
		char ll_v2[64], ll_v4[64], ll_v1[64];
		to_llvm_val(c2, ll_v2);
		to_llvm_val(c4, ll_v4);
		char dst_ssa[96];
		snprintf(dst_ssa, sizeof(dst_ssa), "%s_v%d", c1, (*reg_count)++);
		set_var_version(c1, dst_ssa);
		snprintf(ll_v1, sizeof(ll_v1), "%%%s", dst_ssa);
		
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
		char dst_ssa[96];
		snprintf(dst_ssa, sizeof(dst_ssa), "%s_v%d", c1, (*reg_count)++);
		set_var_version(c1, dst_ssa);
		snprintf(ll_v1, sizeof(ll_v1), "%%%s", dst_ssa);
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

void generate_llvm_loop(t_token *node, FILE *f, int *reg_count)
{
	char v1[64], op[8], v2[64];
	int n = sscanf(node->context, "%63s %7s %63s", v1, op, v2);
	if (n < 3)
		return;
	char ll_init[64], ll_v2[64];
	to_llvm_val(v1, ll_init);
	to_llvm_val(v2, ll_v2);
	int id = loop_count++;
	const char *icmp_op = "sgt";
	if (strcmp(op, "<") == 0)
		icmp_op = "slt";
	else if (strcmp(op, "==") == 0)
		icmp_op = "eq";

	char phi_name[96];
	snprintf(phi_name, sizeof(phi_name), "%s_phi%d", v1, id);
	set_var_version(v1, phi_name);

	char *body_buf = NULL;
	size_t body_size = 0;
	FILE *body_f = open_memstream(&body_buf, &body_size);
	if (!body_f)
		return;
	generate_node(body_f, node->body);
	fclose(body_f);

	const char *next_ver = get_var_version(v1);
	if (!next_ver)
		next_ver = phi_name;

	fprintf(f, "  br label %%loop.pre%d\n", id);
	fprintf(f, "loop.pre%d:\n", id);
	fprintf(f, "  br label %%loop.cond%d\n", id);
	fprintf(f, "loop.cond%d:\n", id);
	fprintf(f, "  %%%s = phi i32 [%s, %%loop.pre%d], [%%%s, %%loop.body%d]\n", phi_name, ll_init, id, next_ver, id);
	fprintf(f, "  %%cmp_loop%d = icmp %s i32 %%%s, %s\n", id, icmp_op, phi_name, ll_v2);
	fprintf(f, "  br i1 %%cmp_loop%d, label %%loop.body%d, label %%loop.end%d\n", id, id, id);
	fprintf(f, "loop.body%d:\n", id);
	if (body_buf && body_size > 0)
		fwrite(body_buf, 1, body_size, f);
	fprintf(f, "  br label %%loop.cond%d\n", id);
	fprintf(f, "loop.end%d:\n", id);
	set_var_version(v1, phi_name);
	(*reg_count)++;
	free(body_buf);
}
