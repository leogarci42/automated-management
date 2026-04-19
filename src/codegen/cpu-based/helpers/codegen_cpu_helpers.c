#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


void to_llvm_val(const char *var, char *out)
{
    int is_num = 1;
    for (int i = 0; var[i]; i++)
	{
        if (i == 0 && var[i] == '-')
			continue;
        if (var[i] < '0' || var[i] > '9')
		{
            is_num = 0;
			break;
        }
    }
    if (is_num)
        sprintf(out, "%s", var);
    else if (strcmp(var, current_arg) == 0 && strlen(current_arg) > 0)
        sprintf(out, "%%%s_arg", var);
    else
        sprintf(out, "%%%s", var);
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