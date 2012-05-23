#include <punani/console.h>
#include <punani/cmd.h>
#include <punani/cvar.h>
#include <string.h>
#include <ctype.h>

#define CMD_MAX_PARAMS 20

typedef struct _cmd_listener {
	const char *cmd_name;
	cmd_parse_callback_t *cb;
} cmd_listener_t;

static cmd_listener_t cmds[] = {
	{
		.cmd_name = "cvar_list",
		.cb = cmd_cvar_list,
	},
	{
		.cmd_name = "set",
		.cb = cmd_set,
	},
};


/* Easy string tokeniser */
static int easy_explode(char *str, char split,
			char **toks, int max_toks)
{
	char *tmp;
	int tok;
	int state;

	for(tmp=str,state=tok=0; *tmp && tok <= max_toks; tmp++) {
		if ( state == 0 ) {
			if ( *tmp == split && (tok < max_toks)) {
				toks[tok++] = NULL;
			}else if ( !isspace(*tmp) ) {
				state = 1;
				toks[tok++] = tmp;
			}
		}else if ( state == 1 ) {
			if ( tok < max_toks ) {
				if ( *tmp == split || isspace(*tmp) ) {
					*tmp = '\0';
					state = 0;
				}
			}else if ( *tmp == '\n' )
				*tmp = '\0';
		}
	}

	return tok;
}

void cmd_parse(const char *raw, size_t max_len)
{
	char input[max_len];

	strncpy(input, raw, max_len);

	char *tok[2];
	char *param[CMD_MAX_PARAMS];
	char **paramv;
	int ntok;
	size_t i;
	int handled;

	ntok = easy_explode(input, ' ', tok, 2);

	switch(ntok) {
	case 1:
		ntok = 0;
		paramv = NULL;
		break;
	case 2:
		ntok = easy_explode(tok[1], ' ', param, CMD_MAX_PARAMS);
		paramv = param;
		break;
	default:
		return;
	}

	handled = 0;

	for(i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
		if (strcmp(tok[0], cmds[i].cmd_name) == 0) {
			(*cmds[i].cb)(ntok, paramv);
			handled = 1;
			break;
		}
	}

	if (!handled) {
		/* fallback to try and just set a variable anyway. */
		strncpy(input, raw, max_len);
		ntok = easy_explode(input, ' ', tok, 2);
		cmd_set(ntok, tok);
	}
}
