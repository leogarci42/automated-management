#pragma once

#include "header.h"

typedef struct s_error {
	bool valid;
	char *err_str;
} t_error;

extern t_error *err;

void	error_printer(t_error *err);
