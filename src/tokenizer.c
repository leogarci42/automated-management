#include "header.h"

void print_token(t_token *token, int depth)
{
    if (!token) return;
    
    for (int i = 0; i < depth; i++)
        printf("  ");
        
    if (token->type == func)
    {
        printf("[\033[36mFUNC\033[0m] \033[32m%s\033[0m (Context: \033[33m%s\033[0m)\n", token->name ? token->name : "anon", token->context ? token->context : "none");
    }
    else if (token->type == ifelse)
        printf("[\033[36mIF/ELSE\033[0m] (Condition: \033[33m%s\033[0m)\n", token->context ? token->context : "none");
    else if (token->type == loop)
        printf("[\033[36mLOOP\033[0m] (Condition: \033[33m%s\033[0m)\n", token->context ? token->context : "none");
    else if (token->type == assignment)
        printf("[\033[36mASSIGNMENT\033[0m] \033[32m%s\033[0m = \033[35m%s\033[0m (Type: %s)\n", 
               token->name ? token->name : "anon", 
               token->context ? token->context : "none", 
               token->var_type == TYPE_INT ? "int32" : (token->var_type == TYPE_CHAR ? "char" : (token->var_type == TYPE_VAR ? "var" : "unknown")));
    else if (token->type == increment)
        printf("[\033[36mINCREMENT\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
    else if (token->type == decrement)
        printf("[\033[36mDECREMENT\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
    else if (token->type == func_call)
        printf("[\033[36mFUNC_CALL\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
    else if (token->type == ret_statement)
        printf("[\033[36mRETURN\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
    else if (token->type == statement)
        printf("[\033[36mSTATEMENT\033[0m] \033[35m%s\033[0m\n", token->context ? token->context : "none");
        
    if (token->body)
    {
        for (int i = 0; i < depth; i++)
            printf("  ");
        printf("  |\n");
        for (int i = 0; i < depth; i++)
            printf("  ");
        printf("  \\-> [BODY]\n");
        print_token(token->body, depth + 1);
    }
    
    if (token->next)
    {
        print_token(token->next, depth);
    }
}

void free_token(t_token *token)
{
	if (!token) return;
	
	if (token->name)
		free(token->name);
	if (token->context)
		free(token->context);
	if (token->body)
		free_token(token->body);
	if (token->next)
		free_token(token->next);
		
	free(token);
}

static void skip_whitespace(char *buff, int *i)
{
	while (buff[*i] == ' ' || buff[*i] == '\t' || buff[*i] == '\n' || buff[*i] == '\r')
		(*i)++;
}

int	isprint(int c)
{
	return ((c >= 32) && (c <= 126));
}

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

static t_token* set_func_token(char *to_tokenize, int *global_i)
{
	t_token *token = (t_token *)malloc(sizeof(t_token));
	if (!token)
		return (NULL);
	token->type = func;
    token->body = NULL;
    token->next = NULL;
    token->context = NULL;
    token->name = NULL;
    
	int i = 0;
	skip_whitespace(to_tokenize, &i);
	if (to_tokenize[i] == '\0')
	{
		err->err_str = strdup("no function name\n");
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
	t_token *token = (t_token *)malloc(sizeof(t_token));
	if (!token)
		return (NULL);
	token->type = type;
    token->body = NULL;
    token->next = NULL;
    token->context = NULL;
    token->name = NULL;
    
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

#include "../patch_all.c"

static t_token* get_statement_token(char *buff, int *i)
{
	t_token *token = (t_token *)malloc(sizeof(t_token));
	if (!token) return (NULL);
	token->type = statement;
	token->body = NULL;
	token->next = NULL;
	token->name = NULL;
	token->context = NULL;
	
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
		token->type = func_call;

	return (token);
}

static t_token* get_token_data(char *buff, int *i)
{
	skip_whitespace(buff, i);
	if (!buff[*i] || buff[*i] == '}')
		return (NULL);

	if (strncmp(buff + *i, "func ", 5) == 0 || strncmp(buff + *i, "func\t", 5) == 0 || strncmp(buff + *i, "func(", 5) == 0)
	{
		if (buff[*i + 4] == '(') *i += 4; else *i += 5;
		return (set_func_token(buff + *i, i));
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

bool generate_token(int fd, char *buff, t_token **out_token)
{
	(void)fd; //TO HANDLE
	int i = 0;
	skip_whitespace(buff, &i);
	if (buff[i] == '\0')
	{
		return (true);
	}
	*out_token = get_token_data(buff, &i);
	if (!*out_token)
	{
        if (err->valid && !err->err_str)
        {
		    err->err_str = strdup("Token generation failed or missing token");
            err->valid = false;
        }
		return (false);
	}
	return (err->valid);
}
