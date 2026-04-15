#include "header.h"

static void skip_whitespace(char *buff, int i)
{
	for (i; buff[i]; i++)
	{
		if (buff[i] <= 9 || buff[i] >= 13 && buff[i] != 32)
			break ;
	}
}

int	isprint(int c)
{
	return ((c >= 32) && (c <= 126));
}

static char* get_name(char *buff, int i)
{
	int j = i;
	while (isprint(buff[i]))
		i++;
	int k = i - j;
	char *name = (char *)malloc(sizeof(char) * j);
	if (!name)
		return (NULL);
	int tmp = 0;
	while (j)
	{
		name[tmp++] = buff[k++];
		j--;
	}
	name[tmp + 1] = '\0';
	return (name); 
}

static t_token* set_func_token(char *to_tokenize)
{
	t_token *token = (t_token *)malloc(sizeof(t_token));
	if (!token)
		return (NULL);
	token->type = func;
	int i = 0;
	skip_whitespace(to_tokenize, &i);
	if (to_tokenize[i] == '\0')
	{
		err->err_str = strdup("no function name\n");
		return (false);
	}
	token->name = get_name(to_tokenize, &i);
}

static t_token* get_token_data(char *buff, int i)
{
	while (buff[i])
	{
		if (buff[i] == 'f')
			if (buff[i + 1] == 'u')
				if (buff[i + 2] == 'n')
					if (buff[i + 3] == 'c' && (buff[i + 4] == 9 || buff[i + 4] == 32))
						return (set_func_token(buff + 4));
	}
}

bool generate_token(int fd, char *buff)
{
	int i = 0;
	skip_whitespace(buff, &i);
	if (buff[i] == '\n')
		//move next line
	if (buff[i] == '\0')
	{
		err->err_str = strdup("empty file\n");
		return (false);
	}
	t_token *token = get_token_data(buff, i);
	if (!token)
		return (NULL);
}