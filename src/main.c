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
	
	bool emit_llvm = false;
	char *filename = NULL;
	
	for (int i = 1; i < ac; i++) {
	    if (strcmp(av[i], "--emit-llvm") == 0) {
	        emit_llvm = true;
	    } else {
	        filename = av[i];
	    }
	}
	
	if (filename)
	{
		dummy_parse(filename, emit_llvm);
		
        if (err->err_str)
            free(err->err_str);
        free(err);
	}
    return 0;
}
