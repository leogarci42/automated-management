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
        printf("[\033[36mIF/ELSE\033[0m]\n");
    else if (token->type == loop)
        printf("[\033[36mLOOP\033[0m]\n");
        
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
	if (buff[*i + 1] != '\n')
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

static t_token* set_func_token(char *to_tokenize)
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
	return (token);
}

static t_token* get_token_data(char *buff, int i)
{
	while (buff[i])
	{
		if (strncmp(buff + i, "func ", 5) == 0 || strncmp(buff + i, "func\t", 5) == 0)
			return (set_func_token(buff + i + 5));
		i++;
	}
	return (NULL);
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
	*out_token = get_token_data(buff, i);
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