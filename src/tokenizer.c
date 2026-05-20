#include "codegen.h"

static char* get_name(char *buff, int *i)
{
	int start = *i;
	while (buff[*i] && isprint(buff[*i]) && buff[*i] != '(' && buff[*i] != ' ')
		(*i)++;
	int len = *i - start;
	char *name = (char *)malloc(sizeof(char) * (len + 1));
	if (!name)
		return (NULL);
	for (int tmp = 0; tmp < len; tmp++)
		name[tmp] = buff[start + tmp];
	name[len] = '\0';
	return (name); 
}

static char* get_context(char *buff, int *i)
{
	if (buff[*i] != '(')
	{
		err->valid = false;
		err->err_str = strdup("invalid context");
		return (NULL);
	}
	(*i)++;
	int start = *i;
	while (buff[*i] && isprint(buff[*i]) && buff[*i] != ')')
		(*i)++;
	if (buff[*i] != ')')
		return (NULL);
	int len = *i - start;
	char *context = (char *)malloc(sizeof(char) * (len + 1));
	if (!context)
		return (NULL);
	for (int tmp = 0; tmp < len; tmp++)
		context[tmp] = buff[start + tmp];
	context[len] = '\0';
	if (buff[*i] == ')')
		(*i)++;
	return (context);
}

static t_token* get_token_data(char *buff, int *i);

static t_exec_target pending_target = EXEC_CPU;

static bool parse_exec_annotation(char *buff, int *i)
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

static t_token* parse_body(char *buff, int *i)
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
		
	return (body_head);
}

static t_token* set_compute_token(char *to_tokenize, int *global_i)
{
	t_token *token = (t_token *)calloc(1, sizeof(t_token));
	if (!token)
		return (NULL);
	token->type = compute;
	token->exec_target = pending_target;
	pending_target = EXEC_CPU;
    
	int i = 0;
	skip_whitespace(to_tokenize, &i);
	if (to_tokenize[i] == '\0')
	{
		err->err_str = strdup("no computetion name\n");
		free(token);
		return (NULL);
	}
	token->name = get_name(to_tokenize, &i);
	if (token->name == NULL)
    {
        free(token);
		return (NULL);
    }
	token->context = get_context(to_tokenize, &i);
	if (!token->context)
	{
		free(token->name);
		free(token);
		err->valid = false;
		return (NULL);
	}
	token->body = parse_body(to_tokenize, &i);
	*global_i += i;
	return (token);
}

static t_token* set_generic_token(char *to_tokenize, int *global_i, t_token_type type)
{
	t_token *token = (t_token *)calloc(1, sizeof(t_token));
	if (!token)
		return (NULL);
	token->type = type;
	token->exec_target = EXEC_CPU;
    
	int i = 0;
	skip_whitespace(to_tokenize, &i);
	token->context = get_context(to_tokenize, &i);
	if (!token->context)
	{
		free(token);
		err->valid = false;
		return (NULL);
	}
	token->body = parse_body(to_tokenize, &i);
	*global_i += i;
	return (token);
}

static t_token *try_get_assignment_token(char *buff, int *i) {
    (void)buff;
    (void)i;
    return (NULL);
}

static t_token* get_statement_token(char *buff, int *i)
{
	t_token *token = (t_token *)calloc(1, sizeof(t_token));
	if (!token) return (NULL);
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

	return (token);
}

static t_token* get_token_data(char *buff, int *i)
{
	skip_whitespace(buff, i);
	if (!buff[*i] || buff[*i] == '}')
		return (NULL);

	if (strncmp(buff + *i, "compute ", 8) == 0 || strncmp(buff + *i, "compute\t", 8) == 0 || strncmp(buff + *i, "compute(", 8) == 0)
	{
		if (buff[*i + 7] == '(') *i += 7; else *i += 8;
		return (set_compute_token(buff + *i, i));
	}
	else if (strncmp(buff + *i, "ifelse ", 7) == 0 || strncmp(buff + *i, "ifelse\t", 7) == 0 || strncmp(buff + *i, "ifelse(", 7) == 0 ||
			 strncmp(buff + *i, "if ", 3) == 0 || strncmp(buff + *i, "if\t", 3) == 0 || strncmp(buff + *i, "if(", 3) == 0)
	{
		if (strncmp(buff + *i, "ifelse", 6) == 0)
			*i += (buff[*i + 6] == '(') ? 6 : 7;
		else
			*i += (buff[*i + 2] == '(') ? 2 : 3;
		return (set_generic_token(buff + *i, i, ifelse));
	}
	else if (strncmp(buff + *i, "loop ", 5) == 0 || strncmp(buff + *i, "loop\t", 5) == 0 || strncmp(buff + *i, "loop(", 5) == 0 ||
			 strncmp(buff + *i, "while ", 6) == 0 || strncmp(buff + *i, "while\t", 6) == 0 || strncmp(buff + *i, "while(", 6) == 0)
	{
		if (strncmp(buff + *i, "loop", 4) == 0)
			*i += (buff[*i + 4] == '(') ? 4 : 5;
		else
			*i += (buff[*i + 5] == '(') ? 5 : 6;
		return (set_generic_token(buff + *i, i, loop));
	}

	t_token *assign_token = try_get_assignment_token(buff, i);
	if (assign_token)
		return (assign_token);

	return (get_statement_token(buff, i));
}

bool generate_token(char *buff, t_token **out_token)
{
    int i = 0;
    t_token *head = NULL;
    t_token *current = NULL;
    
    skip_whitespace(buff, &i);
    if (buff[i] == '\0') {
        return (true);
    }
    
    while (buff[i]) {
        skip_whitespace(buff, &i);
        if (buff[i] == '\0') break;
		if (buff[i] == '@') {
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
        if (!new_token) {
            if (err->valid && !err->err_str) {
                err->err_str = strdup("Token generation failed or missing token");
                err->valid = false;
            }
            return false;
        }
        
        if (!head) head = new_token;
        else current->next = new_token;
        current = new_token;
    }
    *out_token = head;
    return err->valid;
}
