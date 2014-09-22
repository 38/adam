#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_label.h>
#include <dalvik/dalvik_exception.h>
#include <debug.h>

#ifdef PARSER_COUNT
int dalvik_method_count = 0;
#endif

dalvik_method_t* dalvik_method_from_sexp(const sexpression_t* sexp, const char* class_path,const char* file)
{

#ifdef PARSER_COUNT
	dalvik_method_count ++;
#endif

	dalvik_method_t* method = NULL;

	if(SEXP_NIL == sexp) return NULL;
	
	if(NULL == class_path) class_path = "(undefined)";
	if(NULL == file) file = "(undefined)";

	const char* name;
	sexpression_t *attrs, *arglist, *ret, *body;
	/* matches (method (attribute-list) method-name (arg-list) return-type body) */
	if(!sexp_match(sexp, "(L=C?L?C?_?A", DALVIK_TOKEN_METHOD, &attrs, &name, &arglist, &ret, &body))
	{
		LOG_ERROR("bad method defination"); 
		return NULL;
	}

	/* get attributes */
	int attrnum;
	if((attrnum = dalvik_attrs_from_sexp(attrs)) < 0)
	{
		LOG_ERROR("can not parse attributes");
		return NULL;
	}

	/* get number of arguments */
	int num_args;
	num_args = sexp_length(arglist);

	/* Now we know the size we have to allocate for this method */
	method = (dalvik_method_t*)malloc(sizeof(dalvik_method_t) + sizeof(dalvik_type_t*) * (num_args + 1));
	if(NULL == method) 
	{
		LOG_ERROR("can not allocate memory for method argument list");
		return NULL;
	}
	memset(method->args_type, 0, sizeof(dalvik_type_t*) * (num_args + 1));

	method->num_args = num_args;
	method->path = class_path;
	method->file = file;
	method->name = name;

	/* Setup the type of argument list */
	int i;
	for(i = 0;arglist != SEXP_NIL && i < num_args; i ++)
	{
		sexpression_t *this_arg;
		if(!sexp_match(arglist, "(_?A", &this_arg, &arglist))
		{
			LOG_ERROR("invalid argument list");
			goto ERR;
		}
		if(NULL == (method->args_type[i] = dalvik_type_from_sexp(this_arg)))
		{
			LOG_ERROR("invalid argument type @ #%d", i);
			goto ERR;
		}
	}

	/* Setup the return type */
	if(NULL == (method->return_type = dalvik_type_from_sexp(ret)))
	{
		LOG_ERROR("invalid return type");
		goto ERR;
	}

	/* Now fetch the body */
	
	int current_line_number = 0;    /* Current Line Number */
	uint32_t last = DALVIK_INSTRUCTION_INVALID;
	int label_stack[DALVIK_METHOD_LABEL_STACK_SIZE];  /* how many label can one isntruction assign to */
	int label_sp;                              /* label stack pointer */
	int from_label[DALVIK_MAX_CATCH_BLOCK];    /* NOTICE: the maximum number of catch block is limited to this constant */
	int to_label  [DALVIK_MAX_CATCH_BLOCK];
	int label_st  [DALVIK_MAX_CATCH_BLOCK];    /* 0: haven't seen any label related to the label. 
												* 1: seen from label before
												* 2: seen from and to label
												*/
	label_sp  = 0;
	dalvik_exception_handler_t* excepthandler[DALVIK_MAX_CATCH_BLOCK] = {};
	dalvik_exception_handler_set_t* current_ehset = NULL;
	int number_of_exception_handler = 0;
	for(;body != SEXP_NIL;)
	{
		sexpression_t *this_smt;
		if(!sexp_match(body, "(C?A", &this_smt, &body))
		{
			LOG_ERROR("invalid method body");
			goto ERR;
		}
		/* First check if the statement is a psuedo-instruction */
		const char* arg;
#if LOG_LEVEL >= 6
		char buf[4096];
		static int counter = 0;
#endif
		LOG_DEBUG("#%d current instruction : %s",(++counter) ,sexp_to_string(this_smt, buf, sizeof(buf)) );
		if(sexp_match(this_smt, "(L=L=L?", DALVIK_TOKEN_LIMIT, DALVIK_TOKEN_REGISTERS, &arg))
		{
			/* (limit-registers k) */
			method->num_regs = atoi(arg);
			LOG_DEBUG("uses %d registers", method->num_regs);
		}
		else if(sexp_match(this_smt, "(L=L?", DALVIK_TOKEN_LINE, &arg))
		{
			/* (line arg) */
			current_line_number = atoi(arg);
		}
		else if(sexp_match(this_smt, "(L=L?", DALVIK_TOKEN_LABEL, &arg))
		{
			/* (label arg) */
			int lid = -1, i;
			if(dalvik_label_exists(arg))
			{
				lid = dalvik_label_get_label_id(arg);
				if(0xfffffffful != dalvik_label_jump_table[lid]) 
				{
					LOG_ERROR("redefine an existed label %s", arg);
					goto ERR;
				}
			}
			else 
				lid = dalvik_label_get_label_id(arg);
			if(lid < -1) 
			{
				LOG_ERROR("can not create label for %s", arg);
				goto ERR;
			}
			/* since there might be more than one labels at this pointer, so that we 
			 * need a label stack to record all label id which is pointing to current
			 * instruction */
			if(label_sp < DALVIK_METHOD_LABEL_STACK_SIZE)
				label_stack[label_sp++] = lid;
			else
				LOG_WARNING("label stack overflow, might loss some label here");
			int enbaled_count = 0;
			dalvik_exception_handler_t* exceptionset[DALVIK_MAX_CATCH_BLOCK];
			for(i = 0; i < number_of_exception_handler; i ++)
			{
				if(lid == from_label[i] && label_st[i] == 0)
					label_st[i] = 1;
				else if(lid == to_label[i] && label_st[i] == 1)
					label_st[i] = 2;
				else if(lid == from_label[i] && label_st[i] != 0)
					LOG_WARNING("meet from label twice, it might be a mistake");
				else if(lid == to_label[i] && label_st[i] != 1)
					LOG_WARNING("to label is before from label, it might be a mistake");
				
				if(label_st[i] == 1)
				{
					if(enbaled_count >= DALVIK_MAX_CATCH_BLOCK)
					{
						LOG_ERROR("too many catch blocks, try to adjust constant DALVIK_MAX_CATCH_BLOCK");
						goto ERR;
					}
					exceptionset[enbaled_count++] = excepthandler[i];
					LOG_DEBUG("exception %s is catched by handler at label #%d", 
					           excepthandler[i]->exception,
							   excepthandler[i]->handler_label);
				}
			}
			current_ehset = dalvik_exception_new_handler_set(enbaled_count, exceptionset);
		}
		else if(sexp_match(this_smt, "(L=A", DALVIK_TOKEN_ANNOTATION, &arg))
		{
			/* Simplely ignore */
			LOG_INFO("fixme: ignored psuedo-insturction (annotation)");
		}
		else if(sexp_match(this_smt, "(L=A", DALVIK_TOKEN_CATCH, &arg) || 
				sexp_match(this_smt, "(L=A", DALVIK_TOKEN_CATCHALL, &arg))
		{
			excepthandler[number_of_exception_handler] = 
				dalvik_exception_handler_from_sexp(
						this_smt, 
						from_label + number_of_exception_handler, 
						to_label + number_of_exception_handler);
			if(excepthandler[number_of_exception_handler] == NULL)
			{
				LOG_WARNING("invalid exception handler %s", sexp_to_string(this_smt, NULL, 0));
				continue;
			}
			LOG_DEBUG("exception %s is handlered in label #%d", 
					  excepthandler[number_of_exception_handler]->exception, 
					  excepthandler[number_of_exception_handler]->handler_label);
			label_st[number_of_exception_handler] = 0;
			if(DALVIK_MAX_CATCH_BLOCK < ++ number_of_exception_handler)
			{
				LOG_ERROR("too many catch blocks in a signle method, try to adjust DALVIK_MAX_CATCH_BLOCK (currently %d", DALVIK_MAX_CATCH_BLOCK);
				goto ERR;
			}
		}
		else if(sexp_match(this_smt, "(L=A", DALVIK_TOKEN_FILL, &arg))
		{
			//TODO: fill-array-data psuedo-instruction
			LOG_INFO("fixme: (fill-array-data) is to be implemented");
		}
		else
		{
			dalvik_instruction_t* inst = dalvik_instruction_new();
			inst->method = method;
			if(NULL == inst) 
			{
				LOG_ERROR("can not create new instruction");
				goto ERR;
			}
			if(dalvik_instruction_from_sexp(this_smt, inst, current_line_number) < 0)
			{
				LOG_ERROR("can not recognize instuction %s", sexp_to_string(this_smt, NULL, 0));
				goto ERR;
			}
			if(DALVIK_INSTRUCTION_INVALID == last) 
				method->entry = dalvik_instruction_get_index(inst);
			else
				dalvik_instruction_set_next(last, inst);
			last = dalvik_instruction_get_index(inst);
			inst->handler_set = current_ehset;
			if(label_sp > 0)
			{
				int i;
				for(i = 0; i < label_sp; i++)
				{
					LOG_DEBUG("assigned instruction %p to label #%d", inst, label_stack[i]);
					dalvik_label_jump_table[label_stack[i]] = dalvik_instruction_get_index(inst);
				}
				label_sp = 0;
			}
		}
	}
	return method;
ERR:
	dalvik_method_free(method);
	return NULL;
}

void dalvik_method_free(dalvik_method_t* method)
{
	if(NULL == method) return;
	if(NULL != method->return_type) dalvik_type_free(method->return_type);
	int i;
	for(i = 0; i < method->num_args; i ++)
		if(NULL != method->args_type[i])
			dalvik_type_free((dalvik_type_t*)method->args_type[i]);
	free(method);
}
