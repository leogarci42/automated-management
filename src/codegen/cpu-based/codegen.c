#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int reg_count = 0;
static int if_count = 0;
char current_arg[256];

void generate_node(FILE *f, t_token *node) 
{
	if (!node)
		return;
	if (node->type == statement)
		generate_llvm_statement(node, f, &reg_count);
	else if (node->type == ret_statement)
		generate_llvm_ret_statement(node, f, &reg_count);
	else if (node->type == ifelse)
		generate_llvm_ifelse(node, f, &if_count);
	else if (node->type == compute_call)
		generate_llvm_compute_call(node, f, &reg_count);
	else if (node->type == loop)
		generate_llvm_loop(node, f, &reg_count);

	generate_node(f, node->next);
}

void generate_llvm_ir_cpu(t_token *ast, const char *outfile)
{
	FILE *f = fopen(outfile, "w");
	if (!f) return;

	fprintf(f, "; ModuleID = 'test.cucpp'\n");
	fprintf(f, "source_filename = \"test.cucpp\"\n\n");

	fprintf(f, "@.str = private unnamed_addr constant [4 x i8] c\"%%d\\0A\\00\", align 1\n");
	fprintf(f, "declare i32 @printf(i8*, ...)\n\n");

	t_token *curr = ast;
	while (curr)
	{
		if (curr->type == compute && curr->exec_target == EXEC_CPU)
		{
			reg_count = 0;
			reset_var_versions();
			if (curr->context)
			{
				strcpy(current_arg, curr->context);
				current_arg[strcspn(current_arg, "\r\n ")] = 0;
			}
			else
				current_arg[0] = '\0';
			if (strlen(current_arg) > 0) 
				fprintf(f, "define i32 @%s(i32 %%%s_arg) {\n", curr->name, current_arg);
			else
				fprintf(f, "define i32 @%s() {\n", curr->name);
			fprintf(f, "entry:\n");
			
			generate_node(f, curr->body);
			fprintf(f, "}\n\n");
		}
		curr = curr->next;
	}
	fclose(f);
}
