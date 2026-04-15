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

static bool parse_line(int fd, t_token **token)
{
        char buff[1025];
        ssize_t r = read(fd, buff, 1024);
        if (r < 0)
        {
                err->err_str = strdup("read error");
                return (false);
        }
        else
        {
                buff[r] = '\0';
                err->valid = generate_token(fd, buff, token);
        }
		return (err->valid);
}

static bool parse_file(char *filename, t_token **token)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		err->err_str = strdup("Failed to open file");
		return (false);
	}
	else
		err->valid = parse_line(fd, token);
    close(fd);
	return (err->valid);
}

void dummy_parse(char *filename)
{
	err->valid = check_file_extensions(filename);
	error_printer(err);
    
    t_token *token = NULL;
	err->valid = parse_file(filename, &token);
    if (!err->valid)
	    error_printer(err);
    else if (token)
    {
        printf("\n=== AST Output ===\n");
        print_token(token, 0);
        printf("==================\n\n");
    }
	free_token(token);
}
