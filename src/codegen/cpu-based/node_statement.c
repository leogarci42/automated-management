#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static bool parse_array_decl(const char *ctx, t_var_type *elem_type, char *name, size_t name_sz, int *size, const char **init_start)
{
	const char *p = ctx;
	while (*p && isspace((unsigned char)*p))
		p++;
	*elem_type = TYPE_INT;
	if (strncmp(p, "int ", 4) == 0)
	{
		*elem_type = TYPE_INT;
		p += 4;
	}
	else if (strncmp(p, "char ", 5) == 0)
	{
		*elem_type = TYPE_CHAR;
		p += 5;
	}
	while (*p && isspace((unsigned char)*p))
		p++;
	if (!isalpha((unsigned char)*p) && *p != '_')
		return false;
	size_t n = 0;
	while (*p && (isalnum((unsigned char)*p) || *p == '_'))
	{
		if (n + 1 >= name_sz)
			return false;
		name[n++] = *p++;
	}
	name[n] = '\0';
	while (*p && isspace((unsigned char)*p))
		p++;
	if (*p != '[')
		return false;
	p++;
	while (*p && isspace((unsigned char)*p))
		p++;
	char *endptr = NULL;
	long parsed = strtol(p, &endptr, 10);
	if (endptr == p || parsed <= 0)
		return false;
	*size = (int)parsed;
	p = endptr;
	while (*p && isspace((unsigned char)*p))
		p++;
	if (*p != ']')
		return false;
	p++;
	while (*p && isspace((unsigned char)*p))
		p++;
	if (*p == '=')
	{
		p++;
		while (*p && isspace((unsigned char)*p))
			p++;
		*init_start = p;
	}
	else
		*init_start = NULL;
	return true;
}

static bool parse_array_access(const char *expr, char *name, size_t name_sz, char *index_expr, size_t index_sz)
{
	const char *p = expr;
	while (*p && isspace((unsigned char)*p))
		p++;
	if (!isalpha((unsigned char)*p) && *p != '_')
		return false;
	size_t n = 0;
	while (*p && (isalnum((unsigned char)*p) || *p == '_'))
	{
		if (n + 1 >= name_sz)
			return false;
		name[n++] = *p++;
	}
	name[n] = '\0';
	while (*p && isspace((unsigned char)*p))
		p++;
	if (*p != '[')
		return false;
	p++;
	while (*p && isspace((unsigned char)*p))
		p++;
	size_t m = 0;
	while (*p && *p != ']' && m + 1 < index_sz)
		index_expr[m++] = *p++;
	index_expr[m] = '\0';
	if (*p != ']')
		return false;
	return true;
}

static bool parse_array_literal(const char *ctx, char *name, size_t name_sz, const char **list_start)
{
	const char *p = ctx;
	while (*p && isspace((unsigned char)*p))
		p++;
	if (!isalpha((unsigned char)*p) && *p != '_')
		return false;
	size_t n = 0;
	while (*p && (isalnum((unsigned char)*p) || *p == '_'))
	{
		if (n + 1 >= name_sz)
			return false;
		name[n++] = *p++;
	}
	name[n] = '\0';
	while (*p && isspace((unsigned char)*p))
		p++;
	if (*p != '=')
		return false;
	p++;
	while (*p && isspace((unsigned char)*p))
		p++;
	if (*p != '[')
		return false;
	*list_start = p;
	return true;
}

static bool parse_array_access_lhs(const char *ctx, char *name, size_t name_sz, char *index_expr, size_t index_sz)
{
	const char *eq = strchr(ctx, '=');
	if (!eq)
		return false;
	char lhs[128];
	size_t len = (size_t)(eq - ctx);
	if (len >= sizeof(lhs))
		return false;
	memcpy(lhs, ctx, len);
	lhs[len] = '\0';
	return parse_array_access(lhs, name, name_sz, index_expr, index_sz);
}

static bool parse_char_literal(const char *p, int *out_val, const char **out_next)
{
	if (p[0] != '\'')
		return false;
	int val = 0;
	const char *q = p + 1;
	if (*q == '\\')
	{
		q++;
		if (*q == 'n') val = '\n';
		else if (*q == 't') val = '\t';
		else if (*q == '\\') val = '\\';
		else if (*q == '\'') val = '\'';
		else return false;
		q++;
	}
	else
	{
		val = (unsigned char)*q;
		q++;
	}
	if (*q != '\'')
		return false;
	q++;
	*out_val = val;
	*out_next = q;
	return true;
}

static bool parse_literal_list(const char *list, int **values, int *count, bool *has_char)
{
	const char *p = list;
	*values = NULL;
	*count = 0;
	*has_char = false;
	if (*p != '[')
		return false;
	p++;
	while (*p)
	{
		while (*p && isspace((unsigned char)*p))
			p++;
		if (*p == ']')
		{
			p++;
			return true;
		}
		int val = 0;
		const char *next = NULL;
		if (parse_char_literal(p, &val, &next))
		{
			*has_char = true;
			p = next;
		}
		else
		{
			char *endptr = NULL;
			long parsed = strtol(p, &endptr, 10);
			if (endptr == p)
				return false;
			val = (int)parsed;
			p = endptr;
		}
		int *tmp = (int *)realloc(*values, sizeof(int) * (*count + 1));
		if (!tmp)
			return false;
		*values = tmp;
		(*values)[*count] = val;
		(*count)++;
		while (*p && isspace((unsigned char)*p))
			p++;
		if (*p == ',')
			p++;
		else if (*p == ']')
			continue;
		else if (*p == '\0')
			return false;
	}
	return false;
}

void generate_llvm_statement(t_token *node, FILE *f, int *reg_count)
{
	char arr_name[64];
	char idx_expr[64];
	const char *init_start = NULL;
	int arr_size = 0;
	t_var_type elem_type = TYPE_INT;
	if (parse_array_decl(node->context, &elem_type, arr_name, sizeof(arr_name), &arr_size, &init_start))
	{
		if (!add_array_meta(arr_name, arr_size, elem_type))
			return;
		const char *elem_ir = (elem_type == TYPE_CHAR) ? "i8" : "i32";
		fprintf(f, "  %%%s = alloca [%d x %s]\n", arr_name, arr_size, elem_ir);
		if (init_start)
		{
			int *vals = NULL;
			int count = 0;
			bool has_char = false;
			if (parse_literal_list(init_start, &vals, &count, &has_char))
			{
				if (count > arr_size)
					count = arr_size;
				for (int i = 0; i < count; i++)
				{
					int ptr_id = (*reg_count)++;
					fprintf(f, "  %%ptr%d = getelementptr inbounds [%d x %s], [%d x %s]* %%%s, i32 0, i32 %d\n", ptr_id, arr_size, elem_ir, arr_size, elem_ir, arr_name, i);
					if (elem_type == TYPE_CHAR)
						fprintf(f, "  store i8 %d, i8* %%ptr%d\n", vals[i] & 0xff, ptr_id);
					else
						fprintf(f, "  store i32 %d, i32* %%ptr%d\n", vals[i], ptr_id);
				}
			}
			free(vals);
		}
		return;
	}
	const char *list_start = NULL;
	if (parse_array_literal(node->context, arr_name, sizeof(arr_name), &list_start))
	{
		int *vals = NULL;
		int count = 0;
		bool has_char = false;
		if (!parse_literal_list(list_start, &vals, &count, &has_char))
		{
			free(vals);
			return;
		}
		int existing_size = 0;
		t_var_type existing_type = TYPE_UNKNOWN;
		bool exists = get_array_meta(arr_name, &existing_size, &existing_type);
		if (!exists)
		{
			elem_type = has_char ? TYPE_CHAR : TYPE_INT;
			arr_size = count;
			if (!add_array_meta(arr_name, arr_size, elem_type))
			{
				free(vals);
				return;
			}
			const char *elem_ir = (elem_type == TYPE_CHAR) ? "i8" : "i32";
			fprintf(f, "  %%%s = alloca [%d x %s]\n", arr_name, arr_size, elem_ir);
		}
		else
		{
			arr_size = existing_size;
			elem_type = existing_type;
		}
		if (count > arr_size)
			count = arr_size;
		const char *elem_ir = (elem_type == TYPE_CHAR) ? "i8" : "i32";
		for (int i = 0; i < count; i++)
		{
			int ptr_id = (*reg_count)++;
			fprintf(f, "  %%ptr%d = getelementptr inbounds [%d x %s], [%d x %s]* %%%s, i32 0, i32 %d\n", ptr_id, arr_size, elem_ir, arr_size, elem_ir, arr_name, i);
			if (elem_type == TYPE_CHAR)
				fprintf(f, "  store i8 %d, i8* %%ptr%d\n", vals[i] & 0xff, ptr_id);
			else
				fprintf(f, "  store i32 %d, i32* %%ptr%d\n", vals[i], ptr_id);
		}
		free(vals);
		return;
	}
	if (parse_array_access_lhs(node->context, arr_name, sizeof(arr_name), idx_expr, sizeof(idx_expr)))
	{
		int existing_size = 0;
		t_var_type existing_type = TYPE_UNKNOWN;
		if (!get_array_meta(arr_name, &existing_size, &existing_type))
			return;
		char rhs1[64], rhs2[64], op[8];
		char lhs_tmp[64];
		int n = sscanf(node->context, "%63s = %63s %7s %63s", lhs_tmp, rhs1, op, rhs2);
		char rhs_val[64];
		if (n == 4)
		{
			char v1[64], v2[64];
			to_llvm_val_ex(f, reg_count, rhs1, v1);
			to_llvm_val_ex(f, reg_count, rhs2, v2);
			const char *op_nm = "add";
			if (strcmp(op, "-") == 0)
				op_nm = "sub nsw";
			else if (strcmp(op, "*") == 0)
				op_nm = "mul nsw";
			int tmp_id = (*reg_count)++;
			fprintf(f, "  %%tmp%d = %s i32 %s, %s\n", tmp_id, op_nm, v1, v2);
			snprintf(rhs_val, sizeof(rhs_val), "%%tmp%d", tmp_id);
		}
		else if (n == 2)
		{
			to_llvm_val_ex(f, reg_count, rhs1, rhs_val);
		}
		else
		{
			return;
		}
		char idx_val[64];
		to_llvm_val_ex(f, reg_count, idx_expr, idx_val);
		fprintf(f, "  call void @cucpp_bounds_check(i32 %s, i32 %d)\n", idx_val, existing_size);
		int ptr_id = (*reg_count)++;
		if (existing_type == TYPE_CHAR)
		{
			fprintf(f, "  %%ptr%d = getelementptr inbounds [%d x i8], [%d x i8]* %%%s, i32 0, i32 %s\n", ptr_id, existing_size, existing_size, arr_name, idx_val);
			int trunc_id = (*reg_count)++;
			fprintf(f, "  %%tr%d = trunc i32 %s to i8\n", trunc_id, rhs_val);
			fprintf(f, "  store i8 %%tr%d, i8* %%ptr%d\n", trunc_id, ptr_id);
		}
		else
		{
			fprintf(f, "  %%ptr%d = getelementptr inbounds [%d x i32], [%d x i32]* %%%s, i32 0, i32 %s\n", ptr_id, existing_size, existing_size, arr_name, idx_val);
			fprintf(f, "  store i32 %s, i32* %%ptr%d\n", rhs_val, ptr_id);
		}
		return;
	}
	char c1[64], c2[64], c3[64], c4[64];
	int n = sscanf(node->context, "%s = %s %s %s", c1, c2, c3, c4);
	if (n == 4)
	{
		char ll_v2[64], ll_v4[64], ll_v1[64];
		to_llvm_val_ex(f, reg_count, c2, ll_v2);
		to_llvm_val_ex(f, reg_count, c4, ll_v4);
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
		to_llvm_val_ex(f, reg_count, c2, ll_v2);
		fprintf(f, "  %s = add i32 %s, 0\n", ll_v1, ll_v2);
	}
}
