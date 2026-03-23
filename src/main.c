#include "header.h"
#include "parser.h"

t_error *err = NULL;

int main(int ac, char **av)
{
	err = (t_error *)malloc(sizeof(t_error));
	if (ac == 1)
		return (write(2, "add: --help to get help on how to use it\n", 41), 0);
	if (ac == 2)
	{
		dummy_parse(av[1]);
		free(err);
	}
}