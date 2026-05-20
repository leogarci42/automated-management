#include "dummy_check_internal.h"
#include <fcntl.h>

bool check_file_extensions(char *filename)
{
	for (size_t i = 0; filename[i]; i++)
	{
		if (filename[i] == '.' && filename[i + 1] == 'c' && filename[i + 2] == 'u' && filename[i + 3] == 'c' && filename[i + 4] == 'p' && filename[i + 5] == 'p' && !filename[i + 6])
			return true;
	}
	err->err_str = strdup("invalid filename");
	return false;
}

static bool parse_line(int fd, t_token **token)
{
	char buff[1025];
	ssize_t r = read(fd, buff, 1024);
	if (r < 0)
	{
		err->err_str = strdup("read error");
		return false;
	}
	buff[r] = '\0';
	err->valid = generate_token(buff, token);
	return err->valid;
}

bool parse_file(char *filename, t_token **token)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		err->err_str = strdup("Failed to open file");
		return false;
	}
	err->valid = parse_line(fd, token);
	close(fd);
	return err->valid;
}
