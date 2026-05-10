#include "tokenizer_internal.h"

t_token *set_compute_token(char *to_tokenize, int *global_i)
{
	t_token *token = (t_token *)calloc(1, sizeof(t_token));
	if (!token)
		return NULL;
	token->type = compute;
	token->exec_target = pending_target;
	pending_target = EXEC_CPU;

	int i = 0;
	skip_whitespace(to_tokenize, &i);
	if (to_tokenize[i] == '\0')
	{
		err->err_str = strdup("no computetion name\n");
		free(token);
		return NULL;
	}
	token->name = get_name(to_tokenize, &i);
	if (token->name == NULL)
	{
		free(token);
		return NULL;
	}
	token->context = get_context(to_tokenize, &i);
	if (!token->context)
	{
		free(token->name);
		free(token);
		err->valid = false;
		return NULL;
	}
	token->body = parse_body(to_tokenize, &i);
	*global_i += i;
	return token;
}

t_token *set_generic_token(char *to_tokenize, int *global_i, t_token_type type)
{
	t_token *token = (t_token *)calloc(1, sizeof(t_token));
	if (!token)
		return NULL;
	token->type = type;
	token->exec_target = EXEC_CPU;

	int i = 0;
	skip_whitespace(to_tokenize, &i);
	token->context = get_context(to_tokenize, &i);
	if (!token->context)
	{
		free(token);
		err->valid = false;
		return NULL;
	}
	token->body = parse_body(to_tokenize, &i);
	*global_i += i;
	return token;
}

t_token *get_statement_token(char *buff, int *i)
{
	t_token *token = (t_token *)calloc(1, sizeof(t_token));
	if (!token)
		return NULL;
	token->type = statement;
	token->exec_target = EXEC_CPU;

	int start = *i;
	while (buff[*i] && buff[*i] != '\n' && buff[*i] != '}' && buff[*i] != '{')
		(*i)++;

	int len = *i - start;
	token->context = (char *)malloc(len + 1);
	for (int tmp = 0; tmp < len; tmp++)
		token->context[tmp] = buff[start + tmp];
	token->context[len] = '\0';

	for (int j = len - 1; j >= 0 && (token->context[j] == ' ' || token->context[j] == '\t' || token->context[j] == '\r'); j--)
		token->context[j] = '\0';

	if (buff[*i] == '\n')
		(*i)++;

	if (strstr(token->context, "++"))
		token->type = increment;
	else if (strstr(token->context, "--"))
		token->type = decrement;
	else if (strncmp(token->context, "return ", 7) == 0 || strncmp(token->context, "return\t", 7) == 0)
		token->type = ret_statement;
	else if (strchr(token->context, '(') && strchr(token->context, ')'))
		token->type = compute_call;

	return token;
}
