#pragma once

typedef enum e_token_type {
	func,
	ifelse,
	loop,
} t_token_type;

typedef struct s_token {
	t_token_type type;
	char *name;
	char *context;
	struct s_token *body;
	struct s_token *next;
} t_token;

bool generate_token(int fd, char *buff, t_token **out_token);
void print_token(t_token *token, int depth);