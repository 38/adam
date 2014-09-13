#include <readline/readline.h>
#include <readline/history.h>
#include <stdarg.h>
#include <adam.h>
#include <sexp.h>
#ifdef WITH_GRAPHVIZ
#include <graphviz/gvc.h>
#endif
#include "cli_command.h"

cesk_frame_t* input_frame = NULL;
cesk_frame_t* current_frame = NULL;
const void* break_context = NULL;
const void* current_context = NULL;

uint32_t breakpoints[1024];
uint32_t n_breakpoints = 0;
int step = 0;  // 0 means do not break, 1 means step, 2 means next
#ifdef WITH_GRAPHVIZ
GVC_t* gvc = NULL;
#endif

#define PROMPT "(adb) "
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
void cli_code_to_dot(const dalvik_block_t* block, uint32_t iid, FILE* fout)
{
	const dalvik_block_t* graph = dalvik_block_from_method(block->info->class, block->info->method, block->info->signature, block->info->return_type);
	const dalvik_block_t* block_list[DALVIK_BLOCK_MAX_KEYS] = {};
	dfs_block_list(graph, block_list);
	int i,j;
	for(i = 0; i < DALVIK_BLOCK_MAX_KEYS; i ++)
	{
		if(NULL == block_list[i]) continue;
		fprintf(fout, "	CB%d[shape=record,", i);
		if(block_list[i] == block) fprintf(fout, "style=filled,");
		fprintf(fout, "label=\"{Code Block %d",i);
		for(j = block_list[i]->begin; j < block_list[i]->end; j ++)
		{
			static char buf[1024];
			dalvik_instruction_to_string(dalvik_instruction_get(j), buf, sizeof(buf));
			int k;
			for(k = 0; buf[k]; k ++)
				if(buf[k] == '<') buf[k] = '[';
				else if(buf[k] == '>') buf[k] = ']';
			if(j == iid)
				fprintf(fout, "|*%x:%s", j, buf);
			else
				fprintf(fout, "|%x:%s", j, buf);
		}
		fprintf(fout, "}\"];\n");
		for(j = 0; j < block_list[i]->nbranches; j ++)
			if(!block_list[i]->branches[j].disabled)
			{
				if(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(block_list[i]->branches[j]))
					fprintf(fout, "CB%d->R;\n", i);
				else
					fprintf(fout, "CB%d->CB%d;\n", i, block_list[i]->branches[j].block->index);

			}
	}
}
void cli_frame_to_dot(const cesk_frame_t* output, FILE* fout)
{
	fprintf(fout,
		"	offffff01[label = \"-\", shape = circle];\n"
		"	offffff04[label = \"+\", shape = circle];\n"
		"	offffff02[label = \"0\", shape = circle];\n"
		"	offffff00[label = \"\", shape = circle];\n"
	);
	/* node for registers */
	int i;
	fprintf(fout, "	v0[shape = box, label=\"vR\"];\n");
	fprintf(fout, "	v1[shape = box, label = \"vE\"];\n");
	for(i = 2; i < output->size; i ++)
		fprintf(fout, "	v%d[shape = box, label = \"v%d\"];\n", i, i - 2);
	for(i = 0; i < output->size; i ++)
	{
		uint32_t addr;
		cesk_set_iter_t it;
		cesk_set_iter(output->regs[i], &it);
		uint32_t caddr = 0;
		while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&it)))
		{
			if(CESK_STORE_ADDR_IS_CONST(addr))
				caddr |= addr;
			else
				fprintf(fout, "	v%d->o%x;\n", i, addr);
		}
		if(CESK_STORE_ADDR_CONST_CONTAIN(caddr, NEG)) fprintf(fout, "	v%d->offffff01;\n", i);
		if(CESK_STORE_ADDR_CONST_CONTAIN(caddr, ZERO)) fprintf(fout, "	v%d->offffff02;\n", i);
		if(CESK_STORE_ADDR_CONST_CONTAIN(caddr, POS)) fprintf(fout, "	v%d->offffff04;\n", i);
		if(caddr == CESK_STORE_ADDR_EMPTY) fprintf(fout, "	v%d->offffff00;\n", i);
	}

	/* node for Static Fileds */
	cesk_static_table_iter_t iter;
	cesk_static_table_iter(output->statics, &iter);
	uint32_t addr;
	const cesk_set_t* set;
	while(NULL != (set = cesk_static_table_iter_next(&iter, &addr)))
	{
		uint32_t idx = CESK_FRAME_REG_STATIC_IDX(addr);
		uint32_t addr;
		uint32_t caddr = 0;
		cesk_set_iter_t it;
		cesk_set_iter(set, &it);
		fprintf(fout, "	F%d[shape = parallelogram];\n", idx);
		while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&it)))
		{
			if(CESK_STORE_ADDR_IS_CONST(addr))
				caddr |= addr;
			else 
				fprintf(fout, "	F%d->o%x;\n", idx, addr);
		}
		if(CESK_STORE_ADDR_CONST_CONTAIN(caddr, NEG)) fprintf(fout, "	F%d->offffff01;\n", idx);
		if(CESK_STORE_ADDR_CONST_CONTAIN(caddr, ZERO)) fprintf(fout, "	F%d->offffff02;\n", idx);
		if(CESK_STORE_ADDR_CONST_CONTAIN(caddr, POS)) fprintf(fout, "	F%d->offffff04;\n", idx);
		if(caddr == CESK_STORE_ADDR_EMPTY) fprintf(fout, "	F%d->offffff00;\n", idx);
	}

	/* render all objects */
	uint32_t blkidx;
	for(blkidx = 0; blkidx < output->store->nblocks; blkidx ++)
	{
		const cesk_store_block_t* blk = output->store->blocks[blkidx];
		uint32_t ofs;
		for(ofs = 0; ofs < CESK_STORE_BLOCK_NSLOTS; ofs ++)
		{
			if(blk->slots[ofs].value == NULL || blk->slots[ofs].value->type != CESK_TYPE_OBJECT) continue;
			uint32_t addr = blkidx * CESK_STORE_BLOCK_NSLOTS + ofs;
			uint32_t reloc_addr = cesk_alloctab_query(output->store->alloc_tab, output->store, addr);
			if(reloc_addr != CESK_STORE_ADDR_NULL) addr = reloc_addr;
			fprintf(fout, "	o%x[shape = record, label=\"{%x", addr, addr);
			cesk_value_const_t* value = cesk_store_get_ro(output->store, addr);
			const cesk_object_t* obj = value->pointer.object;
			const cesk_object_struct_t* this = obj->members;
			uint32_t i;
			for(i = 0; i < obj->depth; i ++)
			{
				uint32_t j;
				fprintf(fout, "|%s|", this->class.path->value);
				if(this->built_in)
					fprintf(fout, "<B%x>builtin class", i);
				else
				{
					fprintf(fout, "{");
					for(j = 0; j < this->num_members; j ++)
					{
						fprintf(fout, "<O%xF%x>%s", i, j , this->class.udef->members[j]);
						fprintf(fout, (j == this->num_members - 1)?"}":"|");
					}
				}
				CESK_OBJECT_STRUCT_ADVANCE(this);
			}
			fprintf(fout, "}\"];\n");
			obj = value->pointer.object;
			this = obj->members;
			for(i = 0; i < obj->depth; i ++)
			{
				uint32_t j;
				if(this->built_in)
				{
					uint32_t buf[128];
					uint32_t offset = 0;
					for(;;)
					{
						int rc = bci_class_get_addr_list(this->bcidata, offset, buf, sizeof(buf)/sizeof(buf[0]), this->class.bci->class);
						if(rc <= 0) break;
						offset += rc;
						int k;
						for(k = 0; k < rc; k ++) //TODO this also can be a set....
							fprintf(fout, "\to%x:B%x->o%x;\n", addr, i, buf[k]);
					}
				}
				else
				{
					for(j = 0; j < this->num_members; j ++)
					{
						uint32_t taddr;
						uint32_t caddr = 0;
						cesk_set_iter_t it;
						cesk_value_const_t* value = cesk_store_get_ro(output->store, this->addrtab[j]);
						if(NULL == value) continue;
						const cesk_set_t* set = value->pointer.set;
						cesk_set_iter(set, &it);
						while(CESK_STORE_ADDR_NULL != (taddr = cesk_set_iter_next(&it)))
						{
							if(CESK_STORE_ADDR_IS_CONST(taddr))
								caddr |= taddr;
							else 
								fprintf(fout, "\to%x:O%xF%x->o%x;\n", addr, i, j, taddr);
						}
						if(CESK_STORE_ADDR_CONST_CONTAIN(caddr, NEG)) 
							fprintf(fout, "\to%x:O%xF%x->offffff01;\n", addr, i, j);
						if(CESK_STORE_ADDR_CONST_CONTAIN(caddr, ZERO)) 
							fprintf(fout, "\to%x:O%xF%x->offffff02;\n", addr, i, j);
						if(CESK_STORE_ADDR_CONST_CONTAIN(caddr, POS)) 
							fprintf(fout, "\to%x:O%xF%x->offffff04;\n", addr, i, j);
						if(caddr == CESK_STORE_ADDR_EMPTY) 
							fprintf(fout, "\to%x:O%xF%x->offffff00;\n", addr, i, j);
					}
				}

				CESK_OBJECT_STRUCT_ADVANCE(this);
			}
		}
	}

}
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
	static int last_rc = CLI_COMMAND_DONE;
	if(CLI_COMMAND_EXIT == last_rc) return CLI_COMMAND_EXIT;
	const char* cmdline = cli_readline(PROMPT);
	static char last_command[1024] = {};
	if(cmdline && strcmp(cmdline , "") == 0) cmdline = last_command;
	else if(cmdline) strcpy(last_command, cmdline);
	int rc = do_command(cmdline);
	if(last_rc == CLI_COMMAND_EXIT) return CLI_COMMAND_EXIT;
	return last_rc = rc;
}

int main(int argc, char** argv)
{
	int i;
	adam_init();
	cli_command_init();
#ifdef WITH_GRAPHVIZ
	gvc = gvContext();
#endif

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
#ifdef WITH_GRAPHVIZ
	gvFreeContext(gvc);
#endif
	adam_finalize();
	return 0;
}
static inline void _cli_render_frame(const cesk_frame_t* frame, uint32_t inst, const void* context)
#ifdef WITH_GRAPHVIZ
{
	graph_t* g;
	FILE* fdot = fopen("/tmp/frameinfo.dot", "w");
	if(NULL != fdot)
	{
		fprintf(fdot, "digraph FrameInfo{\n");
		cli_frame_to_dot(frame, fdot);
		fprintf(fdot, "}\n");
		fclose(fdot);
		fdot = fopen("/tmp/frameinfo.dot", "r");
		if(NULL != fdot)
		{
			FILE* fps  = fopen("/tmp/frameinfo.ps", "w");
			g = agread(fdot);
			gvLayout(gvc, g, "dot");
			gvRender(gvc, g, "ps", fps);
			agclose(g);
			fclose(fps);
		}
	}
	fdot = fopen("/tmp/code.dot", "w");
	if(NULL != fdot)
	{
		fprintf(fdot, "digraph CodeInfo{\n");
		cli_code_to_dot(cesk_method_context_get_current_block(context), inst, fdot);
		fprintf(fdot, "}\n");
		fclose(fdot);
		fdot = fopen("/tmp/code.dot", "r");
		if(NULL != fdot)
		{
			FILE *fps = fopen("/tmp/code.ps", "w");
			g = agread(fdot);
			gvLayout(gvc, g, "dot");
			gvRender(gvc, g, "ps", fps);
			agclose(g);
			fclose(fps);
		}
	}
}
#else
{}
#endif
int debugger_callback(const dalvik_instruction_t* inst, cesk_frame_t* frame, const void* context)
{
	uint32_t inst_addr = dalvik_instruction_get_index(inst);
	int i, rc;
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
			_cli_render_frame(frame, inst_addr, context);
			//while(cli() == 1);
			for(rc = CLI_COMMAND_DONE; rc == CLI_COMMAND_DONE || rc == CLI_COMMAND_ERROR; rc = cli());
		}
	}
	else if(step == 1)
	{
		step = 0;
		cli_error("0x%x\t%s", inst_addr, dalvik_instruction_to_string(inst, NULL, 0));
		_cli_render_frame(frame, inst_addr, context);
		for(rc = CLI_COMMAND_DONE; rc == CLI_COMMAND_DONE || rc == CLI_COMMAND_ERROR; rc = cli());
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
		printf("%d %d\n", break_depth, current_depth);
		if(current_depth <= break_depth)
		{
			step = 0;
			cli_error("0x%x\t%s", inst_addr, dalvik_instruction_to_string(inst, NULL, 0));
			_cli_render_frame(frame, inst_addr, context);
			for(rc = CLI_COMMAND_DONE; rc == CLI_COMMAND_DONE || rc == CLI_COMMAND_ERROR; rc = cli());
		}
	}
	return 0;
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
int do_frame_dot(cli_command_t* cmd)
{
	const cesk_frame_t* frame;
	if(NULL != current_frame) frame = current_frame;
	else frame = input_frame;
	if(NULL == frame)
	{
		return CLI_COMMAND_ERROR;
	}
	printf("digraph FrameInfo{");
	cli_frame_to_dot(frame, stdout);
	printf("}");
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
	if(NULL == input_frame)
	{
		cli_error("can not invoke a function without an input frame");
		return CLI_COMMAND_ERROR;
	}
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
	else cli_error("%s", cesk_diff_to_string(ret, NULL, 0));
	cesk_frame_t* output = cesk_frame_fork(input_frame);
	cesk_alloctab_t* atab = cesk_alloctab_new(NULL);
	
	if(NULL == output || cesk_frame_set_alloctab(output, atab) < 0 ||cesk_frame_apply_diff(output, ret, rtab, NULL, NULL) < 0)
		cli_error("can not apply the return value of the function to stack frame");

	printf("digraph ResultFrame{\n");
	cli_frame_to_dot(output, stdout);
	printf("}");

	cesk_frame_free(output);

	return CLI_COMMAND_DONE;

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

	Command(23)
		{"frame", "dot", NULL}
		Desc("Vistualize current frame")
		Method(do_frame_dot)
	EndCommand
EndCommands

