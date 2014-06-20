#include <readline/readline.h>
#include <readline/history.h>
#include <stdarg.h>
#include <adam.h>

const char* kw_load;
const char* kw_quit;
const char* kw_help;
const char* kw_dir;
const char* kw_list;
const char* kw_blocks;
const char* kw_code;


#define PROMPT "(adb) "
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
		cli_error("usage:\t(load/dir <dirname>)");
	}
}
static inline void cli_do_help(sexpression_t* sexp)
{
	if(SEXP_NIL == sexp)
	{
		cli_error("Possible commands: help, quit, load, list");
		cli_error("Use `(help command-name)' for the detials of each command");
	}
	else if(sexp_match(sexp, "(L=A", kw_quit, sexp))
	{
		cli_error("Leave ADB.");
	}
	else if(sexp_match(sexp, "(L=A", kw_load, sexp))
	{
		cli_error("Load the instruction file from a directory");
		cli_error("(load/dir \"<dir-name>\")");
	}
	else if(sexp_match(sexp, "(L=A", kw_help, sexp))
	{
		cli_error("This help command");
		cli_error("(help what-you-want)");
	}
	else if(sexp_match(sexp, "(L=A", kw_list, sexp))
	{
		cli_error("List the code of some function");
		cli_error("(list/blocks class name typelist)\tPrint the block graph");
		cli_error("(list/code class name typelist)\tPrint all code in this function");
		cli_error("(list/code class name typelist blockid)\tPrint this block");
	}
}
static inline void dfs_block_list(const dalvik_block_t* entry, const dalvik_block_t** bl)
{
	if(NULL == entry) return;
	if(bl[entry->index] != NULL) return;
	bl[entry->index] = entry;
	int i;
	for(i = 0; i < entry->nbranches; i ++)
		if(!entry->branches[i].disabled)
		{
			if(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(entry->branches[i])) continue;
			dfs_block_list(entry->branches[i].block, bl);
		}
}
static inline void cli_do_list(sexpression_t* sexp)
{
	const char* name;
	const char* class;
	const char* type;
	sexpression_t* tl;
	const dalvik_block_t* block_list[DALVIK_BLOCK_MAX_KEYS] = {};
	if(sexp_match(sexp, "(L?L?L?C?A", &type, &class, &name, &tl, &sexp))
	{
		sexpression_t* this;
		const dalvik_type_t *T[128];
		int i;
		for(i = 0;SEXP_NIL != tl && sexp_match(tl, "(_?A", &this, &tl);i ++)
			T[i] = dalvik_type_from_sexp(this);
		T[i] = NULL;
		const dalvik_block_t* graph = dalvik_block_from_method(class, name, T);
		if(NULL == graph)
		{
			cli_error("Can not find the function");
			return;
		}
		dfs_block_list(graph, block_list);
	}
	if(kw_blocks == type)
	{
		int i,j;
		for(i = 0; i < DALVIK_BLOCK_MAX_KEYS; i ++)
		{
			if(NULL == block_list[i]) continue;
			printf("Block #%d --> ", i);
			for(j = 0; j < block_list[i]->nbranches; j ++)
				if(!block_list[i]->branches[j].disabled)
				{
					if(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(block_list[i]->branches[j])) 
						printf("return ");
					else
						printf("#%d ", block_list[i]->branches[j].block->index);
				}
			puts("");
		}
	}
	if(kw_code == type && sexp == 0)
	{
		int i;
		for(i = 0; i < DALVIK_BLOCK_MAX_KEYS; i ++)
		{
			if(block_list[i] == NULL) continue;
			uint32_t j;
			printf("Block #%d:\n", i);
			for(j = block_list[i]->begin; j < block_list[i]->end; j ++)
			{
				const dalvik_instruction_t* inst = dalvik_instruction_get(j);
				printf("\t0x%x\t%s\n", j, dalvik_instruction_to_string(inst, NULL, 0));
			}
		}
	}
}
int main()
{
	adam_init();
	
	kw_load = stringpool_query("load");
	kw_quit = stringpool_query("quit");
	kw_help = stringpool_query("help");
	kw_dir = stringpool_query("dir");
	kw_blocks = stringpool_query("blocks");
	kw_code = stringpool_query("code");
	kw_list = stringpool_query("list");

	cli_error("ADB - the ADAM Debugger");
	cli_error("type `(help)' for more infomation"); 
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
				else if(verb == kw_help) 
					cli_do_help(sexp);
				else if(verb == kw_list)
					cli_do_list(sexp);
			}
			else 
				cli_error("invalid command `%s'", sexp_to_string(sexp, NULL));
		}
	}
QUIT:
	return 0;
}
