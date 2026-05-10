#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct s_array_meta {
	char *name;
	int size;
	t_var_type elem_type;
	struct s_array_meta *next;
} t_array_meta;

static t_array_meta *g_arrays = NULL;

void reset_arrays(void)
{
	while (g_arrays)
	{
		t_array_meta *next = g_arrays->next;
		free(g_arrays->name);
		free(g_arrays);
		g_arrays = next;
	}
}

bool get_array_meta(const char *name, int *size, t_var_type *elem_type)
{
	for (t_array_meta *it = g_arrays; it; it = it->next)
	{
		if (strcmp(it->name, name) == 0)
		{
			if (size)
				*size = it->size;
			if (elem_type)
				*elem_type = it->elem_type;
			return true;
		}
	}
	return false;
}

bool add_array_meta(const char *name, int size, t_var_type elem_type)
{
	int existing_size = 0;
	t_var_type existing_type = TYPE_UNKNOWN;
	if (get_array_meta(name, &existing_size, &existing_type))
		return false;
	t_array_meta *nv = (t_array_meta *)calloc(1, sizeof(t_array_meta));
	if (!nv)
		return false;
	nv->name = strdup(name);
	nv->size = size;
	nv->elem_type = elem_type;
	nv->next = g_arrays;
	g_arrays = nv;
	return true;
}
