#pragma once

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "fd_tracker.h"
#include "error.h"
#include "token.h"

extern char current_arg[256];

void generate_llvm_ir_cpu(t_token *ast, const char *outfile);
void generate_llvm_ir_gpu(t_token *ast, const char *outfile, const char *target_triple);

void generate_llvm_ifelse(t_token *node, FILE *f, int *if_count);
void generate_llvm_compute_call(t_token *node, FILE *f, int *reg_count);
void generate_llvm_statement(t_token *node, FILE *f, int *reg_count);
void generate_node(FILE *f, t_token *node);
void generate_llvm_ret_statement(t_token *node, FILE *f, int *reg_count);
void generate_llvm_loop(t_token *node, FILE *f, int *reg_count);

void reset_var_versions(void);
void set_var_version(const char *name, const char *ssa_name);
const char *get_var_version(const char *name);
void to_llvm_val(const char *var, char *out);
char *trim_space(char *str);
