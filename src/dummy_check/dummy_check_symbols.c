#include "dummy_check_internal.h"

t_symbol *find_symbol(t_symbol *table, char *name)
{
	while (table)
	{
		if (strcmp(table->name, name) == 0)
			return table;
		table = table->next;
	}
	return NULL;
}

void free_symbol_table(t_symbol *table)
{
	while (table)
	{
		t_symbol *next = table->next;
		free(table->name);
		free(table);
		table = next;
	}
}

static void add_symbol(t_symbol **table, char *name, t_var_type type)
{
	t_symbol *sym = malloc(sizeof(t_symbol));
	sym->name = strdup(name);
	sym->type = type;
	sym->next = *table;
	*table = sym;
}

bool validate_types(t_token *token, t_symbol **table)
{
	if (!token)
		return true;

	if (token->type == assignment)
	{
		t_symbol *existing = find_symbol(*table, token->name);
		if (existing)
		{
			if (existing->type != token->var_type && token->var_type != TYPE_VAR && existing->type != TYPE_VAR)
			{
				if (err->err_str)
					free(err->err_str);
				char buf[256];
				snprintf(buf, sizeof(buf), "Error: Cannot reassign variable '%s' to a different type", token->name);
				err->err_str = strdup(buf);
				err->valid = false;
				return false;
			}
			if (existing->type == TYPE_VAR && token->var_type != TYPE_VAR)
				existing->type = token->var_type;
		}
		else
		{
			add_symbol(table, token->name, token->var_type);
		}
	}

	if (token->body && !validate_types(token->body, table))
		return false;
	if (token->next && !validate_types(token->next, table))
		return false;
	return true;
}

bool has_exec_target(t_token *token, t_exec_target target)
{
	if (!token)
		return false;
	if (token->type == compute && token->exec_target == target)
		return true;
	if (token->body && has_exec_target(token->body, target))
		return true;
	if (token->next && has_exec_target(token->next, target))
		return true;
	return false;
}
