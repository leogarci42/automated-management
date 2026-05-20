#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct s_var_version {
	char *name;
	char *ssa_name;
	struct s_var_version *next;
} t_var_version;

static t_var_version *g_versions = NULL;

void reset_var_versions(void)
{
	while (g_versions)
	{
		t_var_version *next = g_versions->next;
		free(g_versions->name);
		free(g_versions->ssa_name);
		free(g_versions);
		g_versions = next;
	}
}

const char *get_var_version(const char *name)
{
	for (t_var_version *it = g_versions; it; it = it->next)
	{
		if (strcmp(it->name, name) == 0)
			return it->ssa_name;
	}
	return NULL;
}

void set_var_version(const char *name, const char *ssa_name)
{
	for (t_var_version *it = g_versions; it; it = it->next)
	{
		if (strcmp(it->name, name) == 0)
		{
			free(it->ssa_name);
			it->ssa_name = strdup(ssa_name);
			return;
		}
	}
	t_var_version *nv = (t_var_version *)calloc(1, sizeof(t_var_version));
	if (!nv)
		return;
	nv->name = strdup(name);
	nv->ssa_name = strdup(ssa_name);
	nv->next = g_versions;
	g_versions = nv;
}

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
	{
		sprintf(out, "%s", var);
		return;
	}
	const char *ver = get_var_version(var);
	if (ver)
		sprintf(out, "%%%s", ver);
	else if (strcmp(var, current_arg) == 0 && strlen(current_arg) > 0)
		sprintf(out, "%%%s_arg", var);
	else
		sprintf(out, "%%%s", var);
}
