#pragma once
#include <stdbool.h>

typedef enum e_var_type {
	TYPE_UNKNOWN,
	TYPE_INT,
	TYPE_CHAR,
	TYPE_VAR
} t_var_type;

typedef enum e_token_type {
	func,
	ifelse,
	loop,
	statement,
	assignment,
	increment,
	decrement,
	func_call,
	ret_statement,
} t_token_type;

typedef struct s_token {
	t_token_type type;
	char *name;
	char *context;
	t_var_type var_type;
	struct s_token *body;
	struct s_token *next;
} t_token;

bool generate_token(int fd, char *buff, t_token **out_token);
void print_token(t_token *token, int depth);
void free_token(t_token *token);