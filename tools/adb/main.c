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
const char* kw_break;
const char* kw_func;
const char* kw_block;
const char* kw_inst;
const char* kw_clear;
const char* kw_frame;
const char* kw_get;
const char* kw_set;
const char* kw_register;
const char* kw_run;
const char* kw_info;
const char* kw_new;
const char* kw_continue;
const char* kw_step;
const char* kw_next;
const char* kw_backtrace;

cesk_frame_t* input_frame = NULL;
cesk_frame_t* current_frame = NULL;
const void* break_context = NULL;
const void* current_context = NULL;

uint32_t breakpoints[1024];
uint32_t n_breakpoints = 0;
int step = 0;  // 0 means do not break, 1 means step, 2 means next

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
		cli_error("Possible commands: help, quit, load, list, break, frame");
		cli_error("Use `(help command-name)' for the detials of each command");
	}
	else if(sexp_match(sexp, "(L=A", kw_quit, &sexp))
	{
		cli_error("Leave ADB.");
	}
	else if(sexp_match(sexp, "(L=A", kw_load, &sexp))
	{
		cli_error("Load the instruction file from a directory");
		cli_error("(load/dir \"<dir-name>\")");
	}
	else if(sexp_match(sexp, "(L=A", kw_help, &sexp))
	{
		cli_error("This help command");
		cli_error("(help what-you-want)");
	}
	else if(sexp_match(sexp, "(L=A", kw_list, &sexp))
	{
		cli_error("List the code of some function");
		cli_error("(list/blocks class name typelist)\tPrint the block graph");
		cli_error("(list/code class name typelist)\tPrint all code in this function");
		cli_error("(list/code class name typelist blockid)\tPrint this block");
	}
	else if(sexp_match(sexp, "(L=A", kw_break, &sexp))
	{
		cli_error("Add a breakpoint");
		cli_error("(break/func class name typelist)\tAdd a breakpoint at a given function");
		cli_error("(break/block class name typelist block_id)\tAdd a breakpoint at a given block");
		cli_error("(break/inst inst_id)\t\t\tAdd a breakpoint at address");
		cli_error("(break/list)\t\t\t\tList all breakpoints");
	}
	else if(sexp_match(sexp, "(L=A", kw_frame, &sexp))
	{
		cli_error("Input stack frame operations");
		cli_error("(frame/set register-id value1 value2 value3 .... )\tSet the value of the register");
		cli_error("(frame/info)\t Print the frame info");
		cli_error("(frame/new class name typelist)\tCreate a new frame for a function");
	}
	else if(sexp_match(sexp, "(L=A", kw_run, &sexp))
	{
		cli_error("Run a function with the Input frame");
		cli_error("(run class name typelist)");
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
		const dalvik_type_t *R = dalvik_type_from_sexp(sexp);
		const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
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
	else if(kw_code == type && sexp == 0)
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
	else
		cli_error("invalid command");
}
static inline void cli_do_break(sexpression_t* sexp)
{
	const char* name;
	const char* class;
	sexpression_t* tl;
	const char* blockid = "0";
	const char* iid;
	const dalvik_block_t* block_list[DALVIK_BLOCK_MAX_KEYS] = {};
	if(sexp_match(sexp, "(L=L?L?C?_?", kw_func, &class, &name, &tl, &sexp) || 
	   sexp_match(sexp, "(L=L?L?C?L?_?", kw_block, &class, &name, &tl, &sexp, &blockid))
	{
		sexpression_t* this;
		const dalvik_type_t *T[128];
		int i;
		for(i = 0;SEXP_NIL != tl && sexp_match(tl, "(_?A", &this, &tl);i ++)
			T[i] = dalvik_type_from_sexp(this);
		T[i] = NULL;
		const dalvik_type_t *R = dalvik_type_from_sexp(sexp);
		const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
		if(NULL == graph)
		{
			cli_error("Can not find the function");
			return;
		}
		dfs_block_list(graph, block_list);

		uint32_t id = atoi(blockid);

		if(id < DALVIK_BLOCK_MAX_KEYS && NULL != block_list[id])
		{
			breakpoints[n_breakpoints++] = block_list[id]->begin;
			cli_error("Breakpoint %d at instruction 0x%x", n_breakpoints - 1, block_list[id]->begin);
		}
	}
	else if(sexp_match(sexp, "(L=L?", kw_inst, &iid))
	{
		uint32_t id = atoi(iid);
		breakpoints[n_breakpoints++] = id;
		cli_error("Breakpoint %d at instruction 0x%x", n_breakpoints - 1, breakpoints[n_breakpoints - 1]);
	}
	else if(sexp_match(sexp, "(L=A", kw_list, &sexp))
	{
		int i;
		for(i = 0; i < n_breakpoints; i ++)
		{
			if(breakpoints[i] == CESK_STORE_ADDR_NULL) continue;
			cli_error("#%d\t0x%x", i, breakpoints[i]); 
		}
	}
	else
		cli_error("invalid command");
}
static inline void cli_do_frame(sexpression_t* sexp)
{
	const char* reg;
	const char* name;
	const char* class;
	sexpression_t* tl;
	if(sexp_match(sexp, "(L=A", kw_info, &sexp))
	{
		const cesk_frame_t* frame;
		if(NULL != current_frame) frame = current_frame;
		else frame = input_frame;
		if(NULL == frame)
		{
			cli_error("No current frame");
			return;
		}
		uint32_t blkidx, i;
		printf("Registers\n");
		for(i = 0; i < frame->size; i ++)
		{
			if(i > 1)
				printf("\tv%d\t%s\n", i - 2, cesk_set_to_string(frame->regs[i], NULL, 0));
			else if(i == 0)
				printf("\tvR\t%s\n",  cesk_set_to_string(frame->regs[i], NULL, 0));
			else if(i == 1)
				printf("\tvE\t%s\n",  cesk_set_to_string(frame->regs[i], NULL, 0));
		}
		const cesk_store_t* store = frame->store;
		printf("Store\n");
		for(blkidx = 0; blkidx < store->nblocks; blkidx ++)
		{
			const cesk_store_block_t* blk = store->blocks[blkidx];
			uint32_t ofs;
			for(ofs = 0; ofs < CESK_STORE_BLOCK_NSLOTS; ofs ++)
			{
				if(blk->slots[ofs].value == NULL) continue;
				uint32_t addr = blkidx * CESK_STORE_BLOCK_NSLOTS + ofs;
				uint32_t reloc_addr = cesk_alloctab_query(store->alloc_tab, store, addr);
				if(reloc_addr != CESK_STORE_ADDR_NULL)
					printf("\t"PRSAddr"("PRSAddr")\t%s\n", reloc_addr, addr,cesk_value_to_string(blk->slots[ofs].value, NULL, 0));
				else
					printf("\t"PRSAddr"\t%s\n", addr, cesk_value_to_string(blk->slots[ofs].value, NULL, 0));
			}
		}
	}
	else if(sexp_match(sexp, "(L=L?L?C?_?", kw_new, &class, &name, &tl, &sexp))
	{
		sexpression_t* this;
		const dalvik_type_t *T[128];
		int i;
		for(i = 0;SEXP_NIL != tl && sexp_match(tl, "(_?A", &this, &tl);i ++)
			T[i] = dalvik_type_from_sexp(this);
		T[i] = NULL;
		const dalvik_type_t *R = dalvik_type_from_sexp(sexp);
		const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
		if(NULL == graph)
		{
			cli_error("can not find method %s.%s", class, name);
			return; 
		}
		input_frame = cesk_frame_new(graph->nregs);
	}
	else if(sexp_match(sexp, "(L=L?A", kw_set, &reg, &sexp))
	{
		if(NULL == input_frame)
		{
			cli_error("No input frame");
			return;
		}
		uint32_t regnum;
		if(strcmp(reg, "vR") == 0)
			regnum = 0;
		else if(strcmp(reg, "vE") == 0)
			regnum = 1;
		else
			regnum = atoi(reg + 1) + 2;
		const char *this;
		cesk_frame_register_clear(input_frame, regnum, NULL, NULL);
		for(;sexp_match(sexp, "(L?A", &this, &sexp);)
		{
            if(SEXP_NIL == this) break;
			uint32_t addr;
			if(strcmp(this, "-") == 0) addr = CESK_STORE_ADDR_NEG;
			else if(strcmp(this, "Z") == 0) addr = CESK_STORE_ADDR_ZERO;
			else if(strcmp(this, "+") == 0) addr = CESK_STORE_ADDR_POS;
			else addr = atoi(this);
			cesk_frame_register_push(input_frame, regnum, addr, 1, NULL, NULL);
		}
	}
	else
		cli_error("invalid comamnd");
}
static inline void cli_do_run(sexpression_t* sexp)
{
	const char* name;
	const char* class;
	sexpression_t* tl;
	if(sexp_match(sexp, "(L?L?C?A", &class, &name, &tl, &sexp) && NULL != input_frame) 
	{
		sexpression_t* this;
		const dalvik_type_t *T[128];
		int i;
		for(i = 0;SEXP_NIL != tl && sexp_match(tl, "(_?A", &this, &tl);i ++)
			T[i] = dalvik_type_from_sexp(this);
		T[i] = NULL;
		const dalvik_type_t *R = dalvik_type_from_sexp(sexp);
		const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
		if(NULL == graph) return;
		cesk_reloc_table_t* rtab;
		cesk_diff_t* ret = cesk_method_analyze(graph, input_frame, NULL, &rtab);
		if(NULL == ret) cli_error("function returns with an error");
		else cli_error("function returns with diff \n%s", cesk_diff_to_string(ret, NULL, 0));
		cesk_diff_free(ret);
		current_frame = NULL;
	}
	else cli_error("invalid command");
}
static int do_command(const char* cmdline)
{
	if(NULL == cmdline) return 0;
	sexpression_t* sexp, *start;
	for(;;)
	{
		if((cmdline = sexp_parse(cmdline, &sexp)) == NULL)
		{
			if(SEXP_EOF != sexp) 
			{
				cli_error("can not parse the input");
				if(NULL != sexp) sexp_free(sexp);
			}
			break;
		}
		start = sexp;
		const char* verb;
		if(sexp_match(sexp ,"(L?A", &verb, &sexp)) 
		{
			if(verb == kw_quit) 
			{
				sexp_free(start);
				return 0;
			}
			else if(verb == kw_load)
				cli_do_load(sexp);
			else if(verb == kw_help) 
				cli_do_help(sexp);
			else if(verb == kw_list)
				cli_do_list(sexp);
			else if(verb == kw_break)
				cli_do_break(sexp);
			else if(verb == kw_frame)
				cli_do_frame(sexp);
			else if(verb == kw_run)
				cli_do_run(sexp);
			else if(verb == kw_continue)
			{
				sexp_free(start);
				return 2;
			}
			else if(verb == kw_step)
			{
				sexp_free(start);
				step = 1;
				return 2;
			}
			else if(verb == kw_next)
			{
				sexp_free(start);
				step = 2;
				break_context = current_context;
				return 2;
			}
			else if(verb == kw_backtrace)
			{
				const void* p;
				int i = 0;
				for(p = current_context; NULL != p; p = cesk_method_context_get_caller_context(p))
				{
					const dalvik_block_t* block = cesk_method_context_get_current_block(p);
					printf("#%d\t%s.%s Block #%d\n", i++, block->info->class, block->info->method, block->index);
				}
			}
			else
				cli_error("unknown action %s", verb);
		}
		else
		{
			cli_error("invalid command `%s'", sexp_to_string(sexp, NULL, 0));
			sexp_free(start);
			return 1;
		}
		sexp_free(start);
	}
	return 1;
}
static int cli()
{
	const char* cmdline = cli_readline(PROMPT);
	static char last_command[1024] = {};
	if(cmdline && strcmp(cmdline , "") == 0) cmdline = last_command;
	else if(cmdline) strcpy(last_command, cmdline);
	return do_command(cmdline);
}
int main(int argc, char** argv)
{
	adam_init();
	
	kw_load = stringpool_query("load");
	kw_quit = stringpool_query("quit");
	kw_help = stringpool_query("help");
	kw_dir = stringpool_query("dir");
	kw_blocks = stringpool_query("blocks");
	kw_code = stringpool_query("code");
	kw_list = stringpool_query("list");
	kw_break = stringpool_query("break");
	kw_func = stringpool_query("func");
	kw_block = stringpool_query("block");
	kw_inst = stringpool_query("inst");
	kw_clear = stringpool_query("clear");
	kw_frame = stringpool_query("frame");
	kw_get = stringpool_query("get");
	kw_set = stringpool_query("set");
	kw_register = stringpool_query("register");
	kw_run = stringpool_query("run");
	kw_info = stringpool_query("info");
	kw_new = stringpool_query("new");
	kw_continue = stringpool_query("continue");
	kw_step = stringpool_query("step");
	kw_next = stringpool_query("next");
	kw_backtrace = stringpool_query("backtrace");

	int i;
	for(i = 1; i <argc; i ++)
	{
		FILE* fp = fopen(argv[i], "r");
		if(NULL == fp) continue;
		char buf[1024];
		int cli_ret = 1;
		for(;NULL != fgets(buf, sizeof(buf), fp) && cli_ret;)
			cli_ret = do_command(buf);
		fclose(fp);
		if(cli_ret == 0) 
		{
			if(NULL != input_frame) cesk_frame_free(input_frame);
			adam_finalize();
			return 0;
		}
	}
	cli_error("ADB - the ADAM Debugger");
	cli_error("type `(help)' for more infomation");
	while(cli());
	if(NULL != input_frame) cesk_frame_free(input_frame);
	adam_finalize();
	return 0;
}
int debugger_callback(const dalvik_instruction_t* inst, cesk_frame_t* frame, const void* context)
{
	uint32_t inst_addr = dalvik_instruction_get_index(inst);
	int i;
	current_context = context;
	current_frame = frame;
	for(i = 0; i < n_breakpoints; i ++)
		if(breakpoints[i] == inst_addr) break;
	if(i != n_breakpoints)
	{
		const dalvik_block_t* block = cesk_method_context_get_current_block(context);
		if(NULL == block)
			cli_error("Can not get current block");
		else
		{
			cli_error("Breakpoint %d, function %s.%s, Block #%d", i, block->info->class, block->info->method, block->index);
			cli_error("0x%x\t%s", inst_addr, dalvik_instruction_to_string(inst, NULL, 0));
			while(cli() == 1);
		}
	}
	else if(step == 1)
	{
		step = 0;
		cli_error("0x%x\t%s", inst_addr, dalvik_instruction_to_string(inst, NULL, 0));
		while(cli() == 1);
	}
	else if(step == 2)
	{
		int break_depth = 0;
		const void* ptr;
		for(ptr = break_context; NULL != ptr; ptr = cesk_method_context_get_caller_context(ptr))
			break_depth ++;
		int current_depth = 0;
		for(ptr = current_context; NULL != ptr; ptr = cesk_method_context_get_caller_context(ptr))
			current_depth ++;
		if(current_depth <= break_depth)
		{
			step = 0;
			cli_error("0x%x\t%s", inst_addr, dalvik_instruction_to_string(inst, NULL, 0));
			while(cli() == 1);
		}
	}

	return 0;
}
