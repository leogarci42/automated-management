#include "codegen.h"
#include "token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int loop_count = 0;

void generate_llvm_ifelse(t_token *node, FILE *f, int *if_count, int *reg_count)
{
	char v1[64], op[8], v2[64];
	sscanf(node->context, "%s %s %s", v1, op, v2);
	char ll_v1[64], ll_v2[64];
	to_llvm_val_ex(f, reg_count, v1, ll_v1);
	to_llvm_val_ex(f, reg_count, v2, ll_v2);
	int i = (*if_count)++;
	const char *icmp_op = "sgt";
	if (strcmp(op, "<") == 0)
		icmp_op = "slt";
	else if (strcmp(op, "==") == 0)
		icmp_op = "eq";
	fprintf(f, "  %%cmp%d = icmp %s i32 %s, %s\n", i, icmp_op, ll_v1, ll_v2);
	fprintf(f, "  br i1 %%cmp%d, label %%if.then%d, label %%if.end%d\n", i, i, i);
	fprintf(f, "if.then%d:\n", i);
	generate_node(f, node->body);
	t_token *tail = node->body;
	while (tail && tail->next)
		tail = tail->next;
	if (!tail || tail->type != ret_statement)
		fprintf(f, "  br label %%if.end%d\n", i);
	fprintf(f, "if.end%d:\n", i);
}

void generate_llvm_loop(t_token *node, FILE *f, int *reg_count)
{
	char v1[64], op[8], v2[64];
	int n = sscanf(node->context, "%63s %7s %63s", v1, op, v2);
	if (n < 3)
		return;
	char ll_init[64], ll_v2[64];
	to_llvm_val_ex(f, reg_count, v1, ll_init);
	to_llvm_val_ex(f, reg_count, v2, ll_v2);
	int id = loop_count++;
	const char *icmp_op = "sgt";
	if (strcmp(op, "<") == 0)
		icmp_op = "slt";
	else if (strcmp(op, "==") == 0)
		icmp_op = "eq";

	char phi_name[96];
	snprintf(phi_name, sizeof(phi_name), "%s_phi%d", v1, id);
	set_var_version(v1, phi_name);

	char *body_buf = NULL;
	size_t body_size = 0;
	FILE *body_f = open_memstream(&body_buf, &body_size);
	if (!body_f)
		return;
	generate_node(body_f, node->body);
	fclose(body_f);

	const char *next_ver = get_var_version(v1);
	if (!next_ver)
		next_ver = phi_name;

	char backedge_label[96];
	backedge_label[0] = '\0';
	char last_line[256];
	last_line[0] = '\0';
	if (body_buf && body_size > 0)
	{
		const char *p = body_buf;
		const char *line = p;
		while (*p)
		{
			if (*p == '\n')
			{
				size_t len = (size_t)(p - line);
				if (len > 0)
				{
					char tmp[256];
					if (len >= sizeof(tmp))
						len = sizeof(tmp) - 1;
					memcpy(tmp, line, len);
					tmp[len] = '\0';
					char *trim = tmp;
					while (*trim && isspace((unsigned char)*trim))
						trim++;
					if (*trim)
					{
						strncpy(last_line, trim, sizeof(last_line) - 1);
						last_line[sizeof(last_line) - 1] = '\0';
						char *end = trim + strlen(trim);
						while (end > trim && isspace((unsigned char)end[-1]))
							end--;
						if (end > trim && end[-1] == ':')
						{
							size_t lbl_len = (size_t)(end - trim - 1);
							if (lbl_len >= sizeof(backedge_label))
								lbl_len = sizeof(backedge_label) - 1;
							memcpy(backedge_label, trim, lbl_len);
							backedge_label[lbl_len] = '\0';
						}
					}
				}
				line = p + 1;
			}
			p++;
		}
	}
	if (backedge_label[0] == '\0')
		snprintf(backedge_label, sizeof(backedge_label), "loop.body%d", id);
	bool has_terminator = false;
	if (last_line[0] != '\0')
	{
		char *t = last_line;
		while (*t && isspace((unsigned char)*t))
			t++;
		if (strncmp(t, "br ", 3) == 0 || strncmp(t, "ret ", 4) == 0 || strncmp(t, "switch ", 7) == 0)
			has_terminator = true;
	}
	bool has_backedge = !has_terminator;

	fprintf(f, "  br label %%loop.pre%d\n", id);
	fprintf(f, "loop.pre%d:\n", id);
	fprintf(f, "  br label %%loop.cond%d\n", id);
	fprintf(f, "loop.cond%d:\n", id);
	if (has_backedge)
		fprintf(f, "  %%%s = phi i32 [%s, %%loop.pre%d], [%%%s, %%%s]\n", phi_name, ll_init, id, next_ver, backedge_label);
	else
		fprintf(f, "  %%%s = phi i32 [%s, %%loop.pre%d]\n", phi_name, ll_init, id);
	fprintf(f, "  %%cmp_loop%d = icmp %s i32 %%%s, %s\n", id, icmp_op, phi_name, ll_v2);
	fprintf(f, "  br i1 %%cmp_loop%d, label %%loop.body%d, label %%loop.end%d\n", id, id, id);
	fprintf(f, "loop.body%d:\n", id);
	if (body_buf && body_size > 0)
		fwrite(body_buf, 1, body_size, f);
	if (!has_terminator)
		fprintf(f, "  br label %%loop.cond%d\n", id);
	fprintf(f, "loop.end%d:\n", id);
	set_var_version(v1, phi_name);
	(*reg_count)++;
	free(body_buf);
}
