#include "tokenizer_internal.h"

t_token *try_get_assignment_token(char *buff, int *i)
{
	(void)buff;
	(void)i;
	return NULL;
}

t_token *get_token_data(char *buff, int *i)
{
	skip_whitespace(buff, i);
	if (!buff[*i] || buff[*i] == '}')
		return NULL;

	if (strncmp(buff + *i, "compute ", 8) == 0 || strncmp(buff + *i, "compute\t", 8) == 0 || strncmp(buff + *i, "compute(", 8) == 0)
	{
		if (buff[*i + 7] == '(')
			*i += 7;
		else
			*i += 8;
		return set_compute_token(buff + *i, i);
	}
	else if (strncmp(buff + *i, "ifelse ", 7) == 0 || strncmp(buff + *i, "ifelse\t", 7) == 0 || strncmp(buff + *i, "ifelse(", 7) == 0 ||
			 strncmp(buff + *i, "if ", 3) == 0 || strncmp(buff + *i, "if\t", 3) == 0 || strncmp(buff + *i, "if(", 3) == 0)
	{
		if (strncmp(buff + *i, "ifelse", 6) == 0)
			*i += (buff[*i + 6] == '(') ? 6 : 7;
		else
			*i += (buff[*i + 2] == '(') ? 2 : 3;
		return set_generic_token(buff + *i, i, ifelse);
	}
	else if (strncmp(buff + *i, "loop ", 5) == 0 || strncmp(buff + *i, "loop\t", 5) == 0 || strncmp(buff + *i, "loop(", 5) == 0 ||
			 strncmp(buff + *i, "while ", 6) == 0 || strncmp(buff + *i, "while\t", 6) == 0 || strncmp(buff + *i, "while(", 6) == 0)
	{
		if (strncmp(buff + *i, "loop", 4) == 0)
			*i += (buff[*i + 4] == '(') ? 4 : 5;
		else
			*i += (buff[*i + 5] == '(') ? 5 : 6;
		return set_generic_token(buff + *i, i, loop);
	}

	t_token *assign_token = try_get_assignment_token(buff, i);
	if (assign_token)
		return assign_token;

	return get_statement_token(buff, i);
}

bool generate_token(char *buff, t_token **out_token)
{
	int i = 0;
	t_token *head = NULL;
	t_token *current = NULL;

	skip_whitespace(buff, &i);
	if (buff[i] == '\0')
		return true;

	while (buff[i])
	{
		skip_whitespace(buff, &i);
		if (buff[i] == '\0')
			break;
		if (buff[i] == '@')
		{
			if (!parse_exec_annotation(buff, &i))
			{
				err->valid = false;
				err->err_str = strdup("invalid execution annotation");
				return false;
			}
			if (!err->valid)
				return false;
			continue;
		}

		t_token *new_token = get_token_data(buff, &i);
		if (!new_token)
		{
			if (err->valid && !err->err_str)
			{
				err->err_str = strdup("Token generation failed or missing token");
				err->valid = false;
			}
			return false;
		}

		if (!head)
			head = new_token;
		else
			current->next = new_token;
		current = new_token;
	}
	*out_token = head;
	return err->valid;
}
