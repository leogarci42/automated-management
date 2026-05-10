#pragma once

#include "codegen.h"

typedef struct s_symbol {
	char *name;
	t_var_type type;
	struct s_symbol *next;
} t_symbol;

bool check_file_extensions(char *filename);
bool parse_file(char *filename, t_token **token);

bool has_exec_target(t_token *token, t_exec_target target);
bool validate_types(t_token *token, t_symbol **table);
void free_symbol_table(t_symbol *table);

const char *detect_target_triple(bool prefer_gpu);
const char *detect_gpu_triple(void);
bool llc_has_target(const char *llc_cmd, const char *needle);
const char *pick_linker_cmd(bool *is_ld);
