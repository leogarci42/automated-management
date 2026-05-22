#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void trim_token(const char *in, char *out, size_t out_sz)
{
	size_t i = 0;
	while (in[i] && isspace((unsigned char)in[i]))
		i++;
	size_t j = 0;
	while (in[i] && j + 1 < out_sz)
	{
		if (isspace((unsigned char)in[i]))
			break;
		out[j++] = in[i++];
	}
	out[j] = '\0';
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

bool to_llvm_val_ex(FILE *f, int *reg_count, const char *var, char *out)
{
    char token[128];
    trim_token(var, token, sizeof(token));
    int is_num = 1;
    for (int i = 0; token[i]; i++)
    {
        if (i == 0 && token[i] == '-')
            continue;
        if (token[i] < '0' || token[i] > '9')
        {
            is_num = 0;
            break;
        }
    }
    if (is_num)
    {
        snprintf(out, 64, "%s", token);
        return true;
    }
    char arr_name[64];
    char idx_expr[64];
    if (parse_array_access(token, arr_name, sizeof(arr_name), idx_expr, sizeof(idx_expr)))
    {
        int arr_size = 0;
        t_var_type elem_type = TYPE_UNKNOWN;
        if (!get_array_meta(arr_name, &arr_size, &elem_type))
        {
            to_llvm_val(arr_name, out);
            return true;
        }
        char llvm_arr_name[128];
        to_llvm_val(arr_name, llvm_arr_name);

        char idx_val[64];
        to_llvm_val_ex(f, reg_count, idx_expr, idx_val);
        fprintf(f, "  call void @cucpp_bounds_check(i32 %s, i32 %d)\n", idx_val, arr_size);
        int ptr_id = (*reg_count)++;
        int load_id = (*reg_count)++;
        if (elem_type == TYPE_CHAR)
        {
            fprintf(f, "  %%ptr%d = getelementptr inbounds [%d x i8], [%d x i8]* %s, i32 0, i32 %s\n", ptr_id, arr_size, arr_size, llvm_arr_name, idx_val);
            fprintf(f, "  %%ld8_%d = load i8, i8* %%ptr%d\n", load_id, ptr_id);
            fprintf(f, "  %%ld%d = zext i8 %%ld8_%d to i32\n", load_id, load_id);
            snprintf(out, 64, "%%ld%d", load_id);
        }
        else
        {
            fprintf(f, "  %%ptr%d = getelementptr inbounds [%d x i32], [%d x i32]* %s, i32 0, i32 %s\n", ptr_id, arr_size, arr_size, llvm_arr_name, idx_val);
            fprintf(f, "  %%ld%d = load i32, i32* %%ptr%d\n", load_id, ptr_id);
            snprintf(out, 64, "%%ld%d", load_id);
        }
        return true;
    }
    to_llvm_val(token, out);
    return true;
}

char *trim_space(char *str)
{
	while(isspace(*str))
		str++;
	char *end = str + strlen(str) - 1;
	while(end > str && isspace(*end))
	{
		*end = '\0';
		end--;
	}
	return str;
}
