#pragma once

#include "codegen.h"

extern t_exec_target pending_target;

char *get_name(char *buff, int *i);
char *get_context(char *buff, int *i);
bool parse_exec_annotation(char *buff, int *i);
t_token *parse_body(char *buff, int *i);
t_token *set_compute_token(char *to_tokenize, int *global_i);
t_token *set_generic_token(char *to_tokenize, int *global_i, t_token_type type);
t_token *get_statement_token(char *buff, int *i);
t_token *try_get_assignment_token(char *buff, int *i);
t_token *get_token_data(char *buff, int *i);
