#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void parse_inc_dec_target(const char *ctx, char *out, size_t out_sz)
{
	size_t i = 0;
	while (ctx[i] && isspace((unsigned char)ctx[i]))
		i++;
	size_t j = 0;
	while (ctx[i] && j + 1 < out_sz)
	{
		char c = ctx[i];
		if (!(isalnum((unsigned char)c) || c == '_'))
			break;
		out[j++] = c;
		i++;
	}
	out[j] = '\0';
}

void generate_llvm_compute_call(t_token *node, FILE *f, int *reg_count)
{
    if (strncmp(node->context, "print(", 6) == 0)
    {
        char p_arg[64];
        sscanf(node->context, "print(%63[^)])", p_arg);
        char ll_p[64];
        to_llvm_val_ex(f, reg_count, p_arg, ll_p);
        fprintf(f, "  %%print_res%d = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 %s)\n", *reg_count++, ll_p);
    }
    else
    {
        char dst[64], func[64], arg[64];
        int n = sscanf(node->context, "%s = %63[^(](%63[^)])", dst, func, arg);
        if (n == 3)
        {
            char ll_arg[64];
            char dst_ssa[96];
            memset(dst_ssa, 0, sizeof(dst_ssa));
            snprintf(dst_ssa, sizeof(dst_ssa), "%s", dst);
            set_var_version(dst, dst_ssa); 
            to_llvm_val_ex(f, reg_count, arg, ll_arg);
            if (dst_ssa[0] == '%') {
                fprintf(f, "  %s = call i32 @%s(i32 %s)\n", dst_ssa, trim_space(func), ll_arg);
            } else {
                fprintf(f, "  %%%s = call i32 @%s(i32 %s)\n", dst_ssa, trim_space(func), ll_arg);
            }
        }
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
			to_llvm_val_ex(f, reg_count, v1, ll_v1);
			to_llvm_val_ex(f, reg_count, v2, ll_v2);
			const char *op_nm = "add";
			if (strcmp(op, "-") == 0) op_nm = "sub nsw";
			fprintf(f, "  %%tmp_arg%d = %s i32 %s, %s\n", *reg_count, op_nm, ll_v1, ll_v2);
			sprintf(arg_val, "%%tmp_arg%d", *reg_count++);
		}
		else
			to_llvm_val_ex(f, reg_count, arg, arg_val);
		fprintf(f, "  %%call%d = call i32 @%s(i32 %s)\n", *reg_count, trim_space(func), arg_val);
		fprintf(f, "  ret i32 %%call%d\n", *reg_count);
		(*reg_count)++;
	}
	else
	{
		char c1[64];
		sscanf(node->context, "return %s", c1);
		char ll_v1[64];
		to_llvm_val_ex(f, reg_count, c1, ll_v1);
		fprintf(f, "  ret i32 %s\n", ll_v1);
	}
}

void generate_llvm_inc_dec(t_token *node, FILE *f, int *reg_count, bool is_increment)
{
	char name[64];
	parse_inc_dec_target(node->context, name, sizeof(name));
	if (name[0] == '\0')
		return;
	char ll_old[64];
	to_llvm_val_ex(f, reg_count, name, ll_old);
	char dst_ssa[96];
	snprintf(dst_ssa, sizeof(dst_ssa), "%s", name);
	set_var_version(name, dst_ssa);
	const char *op = is_increment ? "add nsw" : "sub nsw";
	fprintf(f, "  %%%s = %s i32 %s, 1\n", dst_ssa, op, ll_old);
}
