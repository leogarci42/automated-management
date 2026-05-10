#include "tokenizer_internal.h"

char *get_name(char *buff, int *i)
{
	int start = *i;
	while (buff[*i] && isprint(buff[*i]) && buff[*i] != '(' && buff[*i] != ' ')
		(*i)++;
	int len = *i - start;
	char *name = (char *)malloc(sizeof(char) * (len + 1));
	if (!name)
		return NULL;
	for (int tmp = 0; tmp < len; tmp++)
		name[tmp] = buff[start + tmp];
	name[len] = '\0';
	return name;
}

char *get_context(char *buff, int *i)
{
	if (buff[*i] != '(')
	{
		err->valid = false;
		err->err_str = strdup("invalid context");
		return NULL;
	}
	(*i)++;
	int start = *i;
	while (buff[*i] && isprint(buff[*i]) && buff[*i] != ')')
		(*i)++;
	if (buff[*i] != ')')
		return NULL;
	int len = *i - start;
	char *context = (char *)malloc(sizeof(char) * (len + 1));
	if (!context)
		return NULL;
	for (int tmp = 0; tmp < len; tmp++)
		context[tmp] = buff[start + tmp];
	context[len] = '\0';
	if (buff[*i] == ')')
		(*i)++;
	return context;
}

bool parse_exec_annotation(char *buff, int *i)
{
	int start = *i;
	if (buff[*i] != '@')
		return false;
	(*i)++;
	int name_start = *i;
	while (buff[*i] && isprint(buff[*i]) && buff[*i] != ' ' && buff[*i] != '\t' && buff[*i] != '\n' && buff[*i] != '\r')
		(*i)++;
	int len = *i - name_start;
	if (len <= 0)
	{
		err->valid = false;
		err->err_str = strdup("invalid execution annotation");
		return true;
	}
	if (len == 3 && strncmp(buff + name_start, "gpu", 3) == 0)
		pending_target = EXEC_GPU;
	else if (len == 3 && strncmp(buff + name_start, "cpu", 3) == 0)
		pending_target = EXEC_CPU;
	else
	{
		err->valid = false;
		err->err_str = strdup("unknown execution annotation");
		return true;
	}
	while (buff[*i] && buff[*i] != '\n')
		(*i)++;
	if (buff[*i] == '\n')
		(*i)++;
	if (start == *i)
		(*i)++;
	return true;
}

t_token *parse_body(char *buff, int *i)
{
	t_token *body_head = NULL;
	t_token *current = NULL;

	skip_whitespace(buff, i);
	bool has_braces = false;
	if (buff[*i] == '{')
	{
		has_braces = true;
		(*i)++;
	}

	while (buff[*i])
	{
		skip_whitespace(buff, i);
		if (has_braces && buff[*i] == '}')
			break;
		if (!has_braces && body_head != NULL)
			break;

		t_token *new_token = get_token_data(buff, i);
		if (!new_token)
		{
			if (buff[*i] == '}')
				break;
			else if (buff[*i] && has_braces)
				(*i)++;
			else if (!has_braces)
				break;
			continue;
		}

		if (!body_head)
			body_head = new_token;
		else
			current->next = new_token;
		current = new_token;
	}

	if (has_braces && buff[*i] == '}')
		(*i)++;

	return body_head;
}
