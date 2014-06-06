#include <readline/readline.h>
#include <readline/history.h>
#include <stdarg.h>
#include <adam.h>
#define PROMPT "> "
void cli_error(const char* fmt, ...)
{
	va_list vp;
	va_start(vp,fmt);
	fputs("\033[33m",stderr);
	vfprintf(stderr,fmt,vp);
	fputs("\033[0m\n", stderr);
	va_end(vp);
}
char* cli_readline(const char* prompt)
{
	static char * buffer = NULL;
	if(buffer) free(buffer);
	buffer = readline(prompt);
	if(buffer) add_history(buffer);
	return buffer;
}
static inline int cli_char_in(char c, const char* chars)
{
	for(;*chars;chars++)
		if(c == *chars) return 1;
	return 0;
}
static inline void cli_do_load(sexpression_t* sexp)
{
	const char* kw_dir = stringpool_query("dir");
	const char* path;
	if(sexp_match(sexp, "(L=S?", kw_dir, &path))
	{
		if(dalvik_loader_from_directory(path) < 0)
		{
			cli_error("can not load path %s", path);
		}
	}
	else
	{
		cli_error("invailid command format");
	}
}
int main()
{
	adam_init();
	const char* kw_load = stringpool_query("load");
	const char* kw_quit = stringpool_query("quit");
	for(;;)
	{
		const char* cmdline = cli_readline(PROMPT);
		sexpression_t* sexp;
		if(NULL == cmdline) break;
		for(;;)
		{
			if((cmdline = sexp_parse(cmdline, &sexp)) == NULL)
			{
				if(SEXP_EOF == sexp) break;
				cli_error("can not parse the input");
				break;
			}
			const char* verb;
			if(sexp_match(sexp ,"(L?A", &verb, &sexp))
			{
				if(verb == kw_quit) goto QUIT;
				else if(verb == kw_load)
					cli_do_load(sexp);
			}
			else 
				cli_error("invalid command `%s'", sexp_to_string(sexp, NULL));
		}
	}
QUIT:
	return 0;
}
