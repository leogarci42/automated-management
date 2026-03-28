#include "header.h"

static bool check_file_extensions(char *filename)
{
	for (size_t i = 0; filename[i]; i++)
	{
		if (filename[i] == '.' && filename[i + 1] == 'c' && filename[i + 2] == 'u' && filename[i + 3] == 'c' && filename[i + 4] == 'p' && filename[i + 5] == 'p' && !filename[i + 6])
			return (true);
	}
	err->err_str = strdup("invalid filename");
	return (false);
}

void dummy_parse(char *filename)
{
	err->valid = check_file_extensions(filename);
	error_printer(err);
	// err->valid = parse_file(av[1]); //TODO
	// error_printer(err);
}