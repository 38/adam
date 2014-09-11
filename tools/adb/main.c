#include <readline/readline.h>
#include <readline/history.h>
#include <stdarg.h>
#include <adam.h>
#include <sexp.h>
#include "cli_command.h"

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
enum{
	CLI_COMMAND_ERROR = -1,
	CLI_COMMAND_EXIT = 0,
	CLI_COMMAND_DONE = 1,
	CLI_COMMAND_CONT = 2
};
static int do_command(const char* cmdline)
{
	if(NULL == cmdline) return 0;
	sexpression_t* sexp;
	int ret = CLI_COMMAND_DONE;
	for(;;)
	{
		if((cmdline = sexp_parse(cmdline, &sexp)) != NULL)
		{
			int rc = cli_command_match(sexp);
			sexp_free(sexp);
			switch(rc)
			{
				case CLI_COMMAND_ERROR: 
					cli_error("can not execute the command");
					return CLI_COMMAND_ERROR;
				case CLI_COMMAND_EXIT:  return CLI_COMMAND_EXIT;
				case CLI_COMMAND_CONT:  ret = CLI_COMMAND_CONT; break;
			}
		}
		else
		{
			if(SEXP_EOF != sexp) 
			{
				cli_error("can not parse the input");
				if(NULL != sexp) sexp_free(sexp);
				return CLI_COMMAND_ERROR;
			}
			else break;
		}
	}
	return ret;
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
	int i;
	adam_init();
	cli_command_init();
#ifndef __OS_X__
	rl_basic_word_break_characters = "";
	rl_completion_entry_function = cli_command_completion_function;
#endif

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

int do_help(cli_command_t* cmd)
{
	sexpression_t* what = cmd->args[1].sexp;
	char help_text[64][128];
	int lines = cli_command_get_help_text(what, help_text, 64, 128);
	int i;
	for(i = 0; i < lines; i ++)
		cli_error("%s\n", help_text[i]);
	return CLI_COMMAND_DONE;
}
int do_load(cli_command_t* cmd)
{
	const char* path = cmd->args[1].string;
	if(dalvik_loader_from_directory(path) < 0)
		cli_error("can not load path %s", path);
	return CLI_COMMAND_DONE;
}
int do_list_blocks(cli_command_t* cmd)
{
	const char* name = cmd->args[2].function.method;
	const char* class = cmd->args[2].function.class;
	const dalvik_block_t* block_list[DALVIK_BLOCK_MAX_KEYS] = {};
	const dalvik_type_t * const * T = (const dalvik_type_t * const *) cmd->args[2].function.signature;
	const dalvik_type_t* R = (const dalvik_type_t*) cmd->args[2].function.return_type;
	const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
	if(NULL == graph)
	{
		cli_error("can not find function %s %s.%s%s", dalvik_type_to_string(R, NULL, 0), class, name, dalvik_type_list_to_string(T, NULL, 0));
		return CLI_COMMAND_ERROR;
	}
	dfs_block_list(graph, block_list);
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
	return CLI_COMMAND_DONE;
}
int do_list_code(cli_command_t* cmd)
{
	const char* name = cmd->args[2].function.method;
	const char* class = cmd->args[2].function.class;
	const dalvik_block_t* block_list[DALVIK_BLOCK_MAX_KEYS] = {};
	const dalvik_type_t * const * T = (const dalvik_type_t * const *) cmd->args[2].function.signature;
	const dalvik_type_t* R = (const dalvik_type_t*) cmd->args[2].function.return_type;
	const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
	if(NULL == graph)
	{
		cli_error("can not find function %s %s.%s%s", dalvik_type_to_string(R, NULL, 0), class, name, dalvik_type_list_to_string(T, NULL, 0));
		return CLI_COMMAND_ERROR;
	}
	dfs_block_list(graph, block_list);
	if(cmd->index == 3)
	{
		int i;
		for(i = 0; i < DALVIK_BLOCK_MAX_KEYS; i ++)
		{
			if(block_list[i] == NULL) continue;
			uint32_t j;
			printf("Block #%d:\n", i);
			for(j = block_list[i]->begin; j < block_list[i]->end; j ++)
				printf("\t0x%x\t%s\n", j, dalvik_instruction_to_string(dalvik_instruction_get(j), NULL, 0));
		}
	}
	else
	{
		uint32_t i = cmd->args[3].numeral;
		if(block_list[i] != NULL)
		{
			uint32_t j;
			printf("Block #%d:\n", i);
			for(j = block_list[i]->begin; j < block_list[i]->end; j ++)
				printf("\t0x%x\t%s\n", j, dalvik_instruction_to_string(dalvik_instruction_get(j), NULL, 0));
		}
	}
	return CLI_COMMAND_DONE;
}
int do_quit(cli_command_t* cmd)
{
	return CLI_COMMAND_EXIT;
}
int do_list_class(cli_command_t* cmd)
{
	const char* classpath[128];
	const char* prefix = "";
	if(cmd->index == 6) prefix = cmd->args[2].class;
	int rc;
	if((rc = dalvik_memberdict_find_class_by_prefix(prefix, classpath, sizeof(classpath)/sizeof(classpath[0]))) < 0)
	{
		return CLI_COMMAND_EXIT;
	}
	int i;
	for(i = 0; i < rc; i ++)
		printf("%s\n", classpath[i]);
	return CLI_COMMAND_DONE;
}
int do_list_method(cli_command_t* cmd)
{
	const char* object_path = cmd->args[2].class;
	const char* classpath[128];
	const char* methodname[128];
	const dalvik_type_t* const* signature[128];
	const dalvik_type_t* return_type[128];
	int rc;
	if(NULL == object_path) object_path = "";
	if((rc = dalvik_memberdict_find_member_by_prefix(
					object_path, "", 
					classpath, methodname, 
					signature, return_type, 
					sizeof(classpath)/sizeof(classpath[0]))) < 0)
	{
		return CLI_COMMAND_ERROR;
	}
	int i;
	for(i = 0; i < rc; i ++)
	{
		if(signature[i] == NULL) continue;
		cli_error("%s %s.%s%s", dalvik_type_to_string(return_type[i], NULL, 0), classpath[i], methodname[i], dalvik_type_list_to_string(signature[i], NULL, 0));
	}
	return CLI_COMMAND_DONE;
}
int do_break(cli_command_t* cmd)
{
	uint32_t iid = 0xfffffffful;
	if(cmd->index == 11)
	{
		iid = cmd->args[2].numeral;
	}
	else
	{
		const char* name = cmd->args[2].function.method;
		const char* class = cmd->args[2].function.class;
		const dalvik_block_t* block_list[DALVIK_BLOCK_MAX_KEYS] = {};
		const dalvik_type_t * const * T = (const dalvik_type_t * const *) cmd->args[2].function.signature;
		const dalvik_type_t* R = (const dalvik_type_t*) cmd->args[2].function.return_type;
		const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
		if(NULL == graph)
		{
			cli_error("can not find function %s %s.%s%s", dalvik_type_to_string(R, NULL, 0), class, name, dalvik_type_list_to_string(T, NULL, 0));
			return CLI_COMMAND_ERROR;
		}
		dfs_block_list(graph, block_list);
		int id = 0;
		if(cmd->index == 10) id = cmd->args[3].numeral;
		if(id < DALVIK_BLOCK_MAX_KEYS && NULL != block_list[id])  
			iid = block_list[id]->begin;
		else
			return CLI_COMMAND_ERROR;
	}
	breakpoints[n_breakpoints++] = iid;
	cli_error("Breakpoint %d at instruction 0x%x", n_breakpoints - 1, iid);
	return CLI_COMMAND_DONE;
}
int do_break_list(cli_command_t* cmd)
{
	int i;
	for(i = 0; i < n_breakpoints; i ++)
	{
		if(breakpoints[i] == CESK_STORE_ADDR_NULL) continue;
		cli_error("#%d\t0x%x", i, breakpoints[i]);
	}
	return CLI_COMMAND_DONE;
}
int do_frame_info(cli_command_t* cmd)
{
	const cesk_frame_t* frame;
	if(NULL != current_frame) frame = current_frame;
	else frame = input_frame;
	if(NULL == frame)
	{
		return CLI_COMMAND_ERROR;
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
	printf("Static Fields\n");
	cesk_static_table_iter_t sit;
	if(NULL == cesk_static_table_iter(frame->statics, &sit)) return CLI_COMMAND_ERROR;
	uint32_t addr;
	const cesk_set_t* pset;
	while(NULL != (pset = cesk_static_table_iter_next(&sit, &addr)))
	{
		printf("\t"PRSAddr"\t%s\n", addr, cesk_set_to_string(pset, NULL, 0));
	}
	return CLI_COMMAND_DONE;
}
int do_frame_new(cli_command_t* cmd)
{
	const char* name = cmd->args[2].function.method;
	const char* class = cmd->args[2].function.class;
	const dalvik_type_t * const * T = (const dalvik_type_t * const *) cmd->args[2].function.signature;
	const dalvik_type_t* R = (const dalvik_type_t*) cmd->args[2].function.return_type;
	const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
	if(NULL == graph)
	{
		cli_error("can not find function %s %s.%s%s", dalvik_type_to_string(R, NULL, 0), class, name, dalvik_type_list_to_string(T, NULL, 0));
		return CLI_COMMAND_ERROR;
	}
	input_frame = cesk_frame_new(graph->nregs);
	return CLI_COMMAND_DONE;
}
int do_frame_set(cli_command_t* cmd)
{
	if(NULL == input_frame) return CLI_COMMAND_ERROR;
	uint32_t regid = cmd->args[2].numeral;
	uint32_t N = cmd->args[3].list.size;
	uint32_t* data = cmd->args[3].list.values;
	int i;
	for(i = 0; i < N; i ++)
		cesk_frame_register_push(input_frame, regid, data[i], 1, NULL, NULL);
	return CLI_COMMAND_DONE;
}
int do_run(cli_command_t* cmd)
{
	const char* name = cmd->args[1].function.method;
	const char* class = cmd->args[1].function.class;
	const dalvik_type_t * const * T = (const dalvik_type_t * const *) cmd->args[1].function.signature;
	const dalvik_type_t* R = (const dalvik_type_t*) cmd->args[1].function.return_type;
	const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
	if(NULL == graph)
	{
		cli_error("can not find function %s %s.%s%s", dalvik_type_to_string(R, NULL, 0), class, name, dalvik_type_list_to_string(T, NULL, 0));
		return CLI_COMMAND_ERROR;
	}
	cesk_reloc_table_t* rtab;
	cesk_diff_t* ret = cesk_method_analyze(graph, input_frame, NULL, &rtab);
	if(NULL == ret) cli_error("function returns with an error");
	else cli_error("%s", cesk_diff_to_string(ret, NULL, 0));
	cesk_diff_free(ret);
	current_frame = NULL;
	return CLI_COMMAND_DONE;
}
int do_run_dot(cli_command_t* cmd)
{
	const char* name = cmd->args[1].function.method;
	const char* class = cmd->args[1].function.class;
	const dalvik_type_t * const * T = (const dalvik_type_t * const *) cmd->args[1].function.signature;
	const dalvik_type_t* R = (const dalvik_type_t*) cmd->args[1].function.return_type;
	const dalvik_block_t* graph = dalvik_block_from_method(class, name, T, R);
	if(NULL == graph)
	{
		cli_error("can not find function %s %s.%s%s", dalvik_type_to_string(R, NULL, 0), class, name, dalvik_type_list_to_string(T, NULL, 0));
		return CLI_COMMAND_ERROR;
	}
	cesk_reloc_table_t* rtab;
	cesk_diff_t* ret = cesk_method_analyze(graph, input_frame, NULL, &rtab);
	if(NULL == ret) cli_error("function returns with an error");
	cesk_frame_t* output = cesk_frame_fork(input_frame);
	if(NULL == output || cesk_frame_apply_diff(output, ret, rtab, NULL, NULL) < 0)
		cli_error("can not apply the return value of the function to stack frame");
	FILE* fout = stdout;
	fprintf(fout, "digraph {\n"
		"	P[shape = circle];\n"
		"	N[shape = circle];\n"
		"	Z[shape = circle];\n"
		"	Nan[shape = circle];\n"
	);
	/* node for registers */
	int i;
	fprintf(fout, "	v0[shape = box, label=\"vR\"];\n");
	fprintf(fout, "	v1[shape = box], label = \"vE\";\n");
	for(i = 2; i < output->size; i ++)
		fprintf(fout, "	v%d[shape = box, label = \"v%d\"];\n", i, i - 2);

	fprintf(fout, "}\n");
	return 0;

}
int do_backtrace(cli_command_t* cmd)
{
	const void* p;
	int i = 0;
	for(p = current_context; NULL != p; p = cesk_method_context_get_caller_context(p))
	{
		const dalvik_block_t* block = cesk_method_context_get_current_block(p);
		printf("#%d\t%s.%s Block #%d\n", i++, block->info->class, block->info->method, block->index);
	}
	return CLI_COMMAND_DONE;
}
int do_step(cli_command_t* cmd)
{
	step = 1;
	return CLI_COMMAND_CONT;
}
int do_next(cli_command_t* cmd)
{
	step = 2;
	return CLI_COMMAND_CONT;
}
int do_continue(cli_command_t* cmd)
{
	return CLI_COMMAND_CONT;
}
int do_clean_cache(cli_command_t* cmd)
{
	cesk_method_clean_cache();
	return CLI_COMMAND_DONE;
}
Commands
	Command(0)
		{"help", SEXPRESSION, NULL}
		Desc("print help message")
		Method(do_help)
	EndCommand

	Command(1)
		{"load", FILENAME, NULL}
		Desc("load dalvik bytecode from a director")
		Method(do_load)
	EndCommand
	
	Command(2)
		{"list", "blocks", FUNCTION, NULL}
		Desc("List all blocks of a function")
		Method(do_list_blocks)	
	EndCommand

	Command(3)
		{"list", "code", FUNCTION, NULL}
		Desc("List the code of a function")
		Method(do_list_code)
	EndCommand

	Command(4)
		{"list", "code", FUNCTION, NUMBER, NULL}
		Desc("List the code of a block")
		Method(do_list_code)
	EndCommand

	Command(5)
		{"quit"}
		Desc("Leave ADAM")
		Method(do_quit)
	EndCommand

	Command(6)
		{"list", "class", CLASSPATH, NULL}
		Desc("List classes registed with the prefix")
		Method(do_list_class)
	EndCommand
	
	Command(7)
		{"list", "class", NULL}
		Desc("List classes registed")
		Method(do_list_class)
	EndCommand

	Command(8)
		{"list", "method", CLASSPATH, NULL}
		Desc("List member functions of the class registed")
		Method(do_list_method)
	EndCommand

	Command(9)
		{"break", "func", FUNCTION, NULL}
		Desc("Add a breakpoint at the beginning of the function")
		Method(do_break)
	EndCommand

	Command(10)
		{"break", "block", FUNCTION, NUMBER, NULL}
		Desc("Add a breakpoint at the specified block within the function")
		Method(do_break)
	EndCommand

	Command(11)
		{"break", "instruction", NUMBER, NULL}
		Desc("Add a breakpoint at the instruction")
		Method(do_break)
	EndCommand

	Command(12)
		{"break", "list", NULL}
		Desc("List all break points")
		Method(do_break_list)
	EndCommand

	Command(13)
		{"frame", "info", NULL}
		Desc("Show current frame content")
		Method(do_frame_info)
	EndCommand

	Command(14)
		{"frame", "new", FUNCTION, NULL}
		Desc("Create a new frame for the specified function")
		Method(do_frame_new)
	EndCommand

	Command(15)
		{"frame", "set", REGNAME, VALUELIST, NULL}
		Desc("Modify the content of reigster")
		Method(do_frame_set)
	EndCommand

	Command(16)
		{"run", FUNCTION, NULL}
		Desc("Run the specified function")
		Method(do_run)
	EndCommand

	Command(17)
		{"backtrace", NULL}
		Desc("Print stack backtrace")
		Method(do_backtrace)
	EndCommand

	Command(18)
		{"step", NULL}
		Desc("Step into")
		Method(do_step)
	EndCommand 

	Command(19)
		{"next", NULL}
		Desc("step over")
		Method(do_next)
	EndCommand

	Command(20)
		{"continue", NULL}
		Desc("continue")
		Method(do_continue)
	EndCommand

	Command(21)
		{"cache", "clean", NULL}
		Desc("Clean analyzer cache")
		Method(do_clean_cache)
	EndCommand

	Command(22)
		{"rundot", FUNCTION, NULL}
		Desc("Run the function and visualize the output frame")
		Method(do_run_dot)
	EndCommand
EndCommands

