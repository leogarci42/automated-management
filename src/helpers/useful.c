#include "../../includes/header.h"

void skip_whitespace(char *buff, int *i)
{
	while (buff[*i] == ' ' || buff[*i] == '\t' || buff[*i] == '\n' || buff[*i] == '\r')
		(*i)++;
}

int	isprint(int c)
{
	return ((c >= 32) && (c <= 126));
}
