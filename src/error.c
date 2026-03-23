#include "error.h"

void	error_printer(t_error *err)
{
	if (err->valid)
		return ;
	printf("%s\n", err->err_str);
	free(err->err_str);
	free(err);
	exit(1);
}