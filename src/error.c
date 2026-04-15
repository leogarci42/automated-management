#include "error.h"

void	error_printer(t_error *err)
{
	if (err->valid)
		return ;
	if (err->err_str)
		printf("%s\n", err->err_str);
	else
		printf("error\n");
	free(err->err_str);
	free(err);
	exit(1);
}