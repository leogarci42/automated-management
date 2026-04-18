#include <fcntl.h>
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
                err->valid = generate_token(buff, token);
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

typedef struct s_symbol {
    char *name;
    t_var_type type;
    struct s_symbol *next;
} t_symbol;

static void free_symbol_table(t_symbol *table) {
    while (table) {
        t_symbol *next = table->next;
        free(table->name);
        free(table);
        table = next;
    }
}

static t_symbol *find_symbol(t_symbol *table, char *name) {
    while (table) {
        if (strcmp(table->name, name) == 0) return table;
        table = table->next;
    }
    return NULL;
}

static void add_symbol(t_symbol **table, char *name, t_var_type type) {
    t_symbol *sym = malloc(sizeof(t_symbol));
    sym->name = strdup(name);
    sym->type = type;
    sym->next = *table;
    *table = sym;
}

static bool validate_types(t_token *token, t_symbol **table) {
    if (!token) return true;
    
    if (token->type == assignment) {
        t_symbol *existing = find_symbol(*table, token->name);
        if (existing) {
            if (existing->type != token->var_type && token->var_type != TYPE_VAR && existing->type != TYPE_VAR) {
                if (err->err_str) free(err->err_str);
                char buf[256];
                snprintf(buf, sizeof(buf), "Error: Cannot reassign variable '%s' to a different type", token->name);
                err->err_str = strdup(buf);
                err->valid = false;
                return false;
            }
            if (existing->type == TYPE_VAR && token->var_type != TYPE_VAR)
                existing->type = token->var_type;
        } else {
            add_symbol(table, token->name, token->var_type);
        }
    }
    
    if (token->body && !validate_types(token->body, table)) return false;
    if (token->next && !validate_types(token->next, table)) return false;
    return true;
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
        t_symbol *table = NULL;
        if (!validate_types(token, &table)) {
            error_printer(err);
        } else {
            printf("\n=== AST Output ===\n");
            print_token(token, 0);
            printf("==================\n\n");
        }
        free_symbol_table(table);
    }
	free_token(token);
}

