#include "header.h"
#include "parser.h"

t_error *err = NULL;

int main(int ac, char **av)
{
	err = (t_error *)malloc(sizeof(t_error));
    if (!err)
        return (1);
    err->valid = true;
    err->err_str = NULL;
    
	if (ac == 1)
		return (write(2, "add: --help to get help on how to use it\n", 41), 0);
	if (ac == 2)
	{
		dummy_parse(av[1]);
		
        if (err->err_str)
            free(err->err_str);
        free(err);
	}
    return 0;
}
