#include "header.h"
#include <fcntl.h>

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

static bool parse_line(int fd)
{
	char buff[1024];
	ssize_t r = read(fd, buff, 1024);
	if (r < 0)
	{
		err->err_str = strdup("read error");
		return (false);
	}
	else
		err->valid = generate_token(fd, buff);
	return (err->valid);
}

static bool parse_file(char *filename)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		err->err_str = strdup("Failed to open file");
		return (false);
	}
	else
		err->valid = parse_line(fd);
}

void dummy_parse(char *filename)
{
	err->valid = check_file_extensions(filename);
	error_printer(err);
	err->valid = parse_file(filename); //TODO
	// error_printer(err);
}