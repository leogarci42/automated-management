#include "header.h"
#include "parser.h"

t_error *err = NULL;

static void print_help() {
    printf("Usage: ./automated-management <source_file.cucpp> [options]\n");
    printf("Options:\n");
    printf("  --help        Display this help message and exit\n");
    printf("  --emit-llvm   Preserve the generated 'output.ll' LLVM IR file\n");
    printf("  -o <file>     Write output binary to <file> (default: a.out)\n");
}

int main(int ac, char **av)
{
    err = (t_error *)malloc(sizeof(t_error));
    if (!err)
        return (1);
    err->valid = true;
    err->err_str = NULL;
    
    if (ac == 1) {
        printf("Error: Missing input file. Run with --help for usage.\n");
        free(err);
        return 1;
    }

    bool emit_llvm = false;
    char *filename = NULL;
    char *out_bin = "a.out";

    for (int i = 1; i < ac; i++) {
        if (strcmp(av[i], "--help") == 0) {
            print_help();
            free(err);
            return 0;
        } else if (strcmp(av[i], "--emit-llvm") == 0) {
            emit_llvm = true;
        } else if (strcmp(av[i], "-o") == 0) {
            if (i + 1 < ac) {
                out_bin = av[++i];
            } else {
                printf("Error: -o option requires an output filename.\n");
                free(err);
                return 1;
            }
        } else {
            filename = av[i];
        }
    }

    if (filename)
    {
        dummy_parse(filename, emit_llvm, out_bin);

        if (err->err_str)
            free(err->err_str);
    } else {
        printf("Error: No source file specified.\n");
    }
    
    free(err);
    return 0;
}
