#pragma once

typedef enum e_token_type {
	func,
	ifelse,
	loop,
} t_token_type;

typedef struct s_token {
	t_token_type type;
	char *name;
	char *condition;
	struct s_token *body;
	struct s_token *next;
} t_token;