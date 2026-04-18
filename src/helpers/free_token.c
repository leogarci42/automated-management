#include "../../includes/header.h"

void free_token(t_token *token)
{
	if (!token) return;
	
	if (token->name)
		free(token->name);
	if (token->context)
		free(token->context);
	if (token->body)
		free_token(token->body);
	if (token->lhs)
		free_token(token->lhs);
	if (token->rhs)
		free_token(token->rhs);
	if (token->next)
		free_token(token->next);
		
	free(token);
}
