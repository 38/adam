#include "cli_command.h"
extern cli_command_t cli_commands[];
static int ncommands = 0;
int cli_command_init()
{
	int i;
	for(i = 0; cli_commands[i].index != 0xffffffff; i ++)
	{
		int j;
		for(j = 0; cli_commands[i].pattern[j]; j ++)
			if(cli_commands[i].pattern[j] >= (const char*)0x10)
					cli_commands[i].pattern[j] = stringpool_query(cli_commands[i].pattern[j]);
		ncommands ++;
	}
	return 0;
}
static inline int _match_func(const sexpression_t* sexp, const char** class, const char** method, const dalvik_type_t** signature, const dalvik_type_t** rtype)
{
	signature[0] = NULL;
	if(sexp_get_method_address(sexp, &sexp, class, method) < 0)
		return 0;
	sexpression_t* sexp_signature, *this_type;
	if(!sexp_match(sexp, "(C?A", &sexp_signature, &sexp))
		return 0;
	int i;
	for(i = 0; SEXP_NIL != sexp_signature && sexp_match(sexp_signature, "(_?A", &this_type, &sexp_signature); i ++)
		if(NULL == (signature[i] = dalvik_type_from_sexp(this_type)))
			return 0;
	signature[i] = NULL;
	sexpression_t* sexp_rtype;
	if(!sexp_match(sexp, "(_?A", &sexp_rtype, &sexp))
		return 0;
	*rtype = dalvik_type_from_sexp(sexp_rtype);
	return 1;
}
int cli_command_match(sexpression_t* sexp)
{
	int i, j, rc = -1;
	for(i = 0; i < ncommands; i ++)
	{
		sexpression_t* current = sexp;
		sexpression_t* this;
		for(j = 0; cli_commands[i].pattern[j]; j ++)
		{
			int match = 1;
			const char* lit;
			uint32_t n = 0;
			switch((uintptr_t)cli_commands[i].pattern[j])
			{
				case (uintptr_t)STRING:
				case (uintptr_t)FILENAME:
					match = sexp_match(current, "(S?A", &cli_commands[i].args[j].string, &current);
					break;
				case (uintptr_t)NUMBER:
					match = sexp_match(current, "(L?A", &lit, &current);
					if(match)
					{
						if(lit[0] == '0' && lit[1] == 'x') cli_commands[i].args[j].numeral = strtoul(lit + 2, NULL, 16);
						else if(lit[0] == '-') cli_commands[i].args[j].numeral = 0xffffff01;
						else if(lit[0] == 'Z') cli_commands[i].args[j].numeral = 0xffffff02;
						else if(lit[0] == '+') cli_commands[i].args[j].numeral = 0xffffff04;
						else cli_commands[i].args[j].numeral = strtoul(lit, NULL, 10);
					}
					break;
				case (uintptr_t)VALUELIST:
					while(match && SEXP_NIL != current)
					{
						match = sexp_match(current, "(L?A", &lit, &current);
						if(match)
						{
							if(lit[0] == '0' && lit[1] == 'x') cli_commands[i].args[j].list.values[n] = strtoul(lit + 2, NULL, 16);
							else if(lit[0] == '-') cli_commands[i].args[j].list.values[n] = 0xffffff01;
							else if(lit[0] == 'Z') cli_commands[i].args[j].list.values[n] = 0xffffff02;
							else if(lit[0] == '+') cli_commands[i].args[j].list.values[n] = 0xffffff04;
							else cli_commands[i].args[j].list.values[n] = strtoul(lit, NULL, 10);
						}
						n ++;
					}
					cli_commands[i].args[j].list.size = n;
					break;
				case (uintptr_t)CLASSPATH:
					cli_commands[i].args[j].class = sexp_get_object_path(current, (const sexpression_t**)&current);
					match = (cli_commands[i].args[j].class != NULL);
					break;
				case (uintptr_t)SEXPRESSION:
					cli_commands[i].args[j].sexp = current;
					current = SEXP_NIL;
					match = 1;
					break;
				case (uintptr_t)FUNCTION:
					match = sexp_match(current, "(C?A", &this, &current);
					if(match)
					{
						match = _match_func(
									this,
									&cli_commands[i].args[j].function.class,
									&cli_commands[i].args[j].function.method,
									cli_commands[i].args[j].function.signature,
									&cli_commands[i].args[j].function.return_type);
					}
					break;
				case (uintptr_t)REGNAME:
					match = sexp_match(current, "(L?A", &lit, &current);
					if(match)
					{
						if(0 == strcmp(lit, "vE")) cli_commands[i].args[j].numeral = 0;
						else if(0 == strcmp(lit, "vR")) cli_commands[i].args[j].numeral = 1;
						else cli_commands[i].args[j].numeral = atoi(lit + 1) + 2;
					}
					break;
				default:
					match = sexp_match(current, "(L=A", cli_commands[i].pattern[j] ,&current);
					if(match) cli_commands[i].args[j].literal = cli_commands[i].pattern[j];
			}
			if(match == 0) break;
		}
		int k;
		if(cli_commands[i].pattern[j] == NULL && SEXP_NIL == current) 
		{
			rc = cli_commands[i].action(cli_commands + i);
			for(k = 0; k < j; k ++)
			{
				if(cli_commands[i].pattern[k] == FUNCTION)
				{
					dalvik_type_free((dalvik_type_t*)cli_commands[i].args[k].function.return_type);
					int i;
					for(i = 0; NULL != cli_commands[i].args[k].function.signature[i]; i ++)
						dalvik_type_free((dalvik_type_t*)cli_commands[i].args[k].function.signature[i]);
				}
			}
			break;
		}
		for(k = 0; k < j; k ++)
		{
			if(cli_commands[i].pattern[k] == FUNCTION)
			{
				dalvik_type_free((dalvik_type_t*)cli_commands[i].args[k].function.return_type);
				int i;
				for(i = 0; NULL != cli_commands[i].args[k].function.signature[i]; i ++)
					dalvik_type_free((dalvik_type_t*)cli_commands[i].args[k].function.signature[i]);
			}
		}
	}
	return rc;
}
int cli_command_get_help_text(sexpression_t* what, void* buf, uint32_t nlines, uint32_t nchar)
{
	char* charbuf = (char*)buf;
	int i, j, rc = 0;
	for(i = 0; i < ncommands; i ++)
	{
		sexpression_t* current = what;
		int match = 1;
		for(j = 0; match && SEXP_NIL != current && cli_commands[i].pattern[j]; j ++)
		{
			if(cli_commands[i].pattern[j] < (const char*) 0x10) match = 0;
			else match = sexp_match(current, "(L=A", cli_commands[i].pattern[j], &current);
		}
		if(match) 
		{
			size_t free = nchar;
			char* ptr = charbuf;
			*(ptr++) = '(';
			for(j = 0; cli_commands[i].pattern[j]; j ++)
			{
				switch((uintptr_t)cli_commands[i].pattern[j])
				{
					case (uintptr_t)STRING:
						snprintf(ptr, free, "\"<string>\" ");
						break;
					case (uintptr_t)FILENAME:
						snprintf(ptr, free, "\"<file name>\" ");
						break;
					case (uintptr_t)CLASSPATH:
						snprintf(ptr, free, "<class path> ");
						break;
					case (uintptr_t)FUNCTION:
						snprintf(ptr, free, "[<class path>/<method name> (<type1> <type2> ... <typeN>) <type>] ");
						break;
					case (uintptr_t)SEXPRESSION:
						snprintf(ptr, free, "<what you want> ");
						break;
					case (uintptr_t)NUMBER:
						snprintf(ptr, free, "<numeral value> ");
						break;
					case (uintptr_t)VALUELIST:
						snprintf(ptr, free, "<value list>");
						break;
					case (uintptr_t)REGNAME:
						snprintf(ptr, free, "<register name>");
						break;
					default:
						if((uintptr_t)cli_commands[i].pattern[j] < 0x10) 
							snprintf(ptr, free, "%s", "<!@#$%^&*()>");
						else 
							snprintf(ptr, free, "%s ", cli_commands[i].pattern[j]);
				}
				size_t used = strlen(ptr);
				free -= used;
				ptr += used;
			}
			if(j > 0) ptr --, free ++;
			ptr[0] = ')';
			ptr[1] = 0;
			ptr ++, free --;
			snprintf(ptr, free, " %s", cli_commands[i].description);
			charbuf += nchar;
			rc ++;
		}
	}
	qsort(buf, rc, nchar, (int (*)(const void*, const void*))strcmp); 
	return rc;
}
#ifdef WITH_READLINE
static inline char* _strcat(const char* str1,const char* str2)
{
	if(NULL == str1 || NULL == str2) return NULL;
	size_t sz = strlen(str1) + strlen(str2) + 1;
	char* ret = (char*)malloc(sz);
	if(NULL == ret) return ret;
	strcpy(ret, str1);
	strcpy(ret + strlen(str1), str2);
	return ret;
}
static inline const char* _cli_command_completor(const char* text, const char* type, int state)
{
	static char buf[1024];
	char input[1024];
	char* res;
	int len;
	extern char* filename_completion_function(const char*, int);
	static int idx;
	static int rc;
	extern cesk_frame_t* input_frame;
	
	static const char* classpath[16384];
	static const char* methodname[16384];
	static const dalvik_type_t* const* signature[16384];
	static const dalvik_type_t* return_type[16384];
	static char mbuf[16384][16384];

	int i;
	while(*text == ' ' || *text == '\t') text ++;
	if((uintptr_t)type < 0x10)
	{
		switch((uintptr_t)type)
		{
			case (uintptr_t)STRING:
				if(*text == 0 && !state) return "\"";
				return NULL;
			case (uintptr_t)FILENAME:
				snprintf(input, sizeof(input), "%s", text==NULL?"":text + 1);
				len = strlen(input);
				if(len > 1 && input[len-1] == '\"')
				{
					if(input[len-2] == '/') 
						input[len - 1] = 0;
					else
						input[len - 2] = '"';
				}
				static int flag = 0;
				if(state == 0) flag = 0;
				if(flag == 0)
					res = filename_completion_function(input, state);
				else 
					res = NULL;
				if(res || flag)
				{
					if(flag == 0)
					{
						snprintf(buf, sizeof(buf), "\"%s\"", res);
						free(res);
						flag = 1;
					}
					else
					{
						int l = strlen(buf);
						buf[l - 1] = '/';
						//buf[l] = '\"';
						//buf[l + 1] = 0;
						flag = 0;
					}
					return buf + (text?strlen(text):0);
				}
				flag = 0;
				return NULL;
			case (uintptr_t)CLASSPATH:
				if(!state) 
				{
					idx = 0;
					if((rc = dalvik_memberdict_find_class_by_prefix(text, classpath, sizeof(classpath)/sizeof(classpath[0]))) < 0) return NULL;
				}
				if(idx < rc) return classpath[idx ++] + (text?strlen(text):0);
				return NULL;
			case (uintptr_t)REGNAME:
				if(NULL == input_frame) return NULL;
				if(!state) idx = 0;
				if(idx < input_frame->size)
				{
					if(idx == 0) snprintf(buf, sizeof(buf), "vE");
					else if(idx == 1) snprintf(buf, sizeof(buf), "vR");
					else snprintf(buf, sizeof(buf), "v%d", idx - 2);
					idx ++;
					return buf + (text?strlen(text):0);
				}
				return NULL;
			case (uintptr_t) FUNCTION:
				if(!state)
				{
					int cnt;
					rc = 0;
					if((cnt = dalvik_memberdict_find_member_by_prefix(
									"", "", 
									classpath, methodname, 
									signature, return_type, 
									sizeof(classpath)/sizeof(classpath[0]))) < 0)
						return NULL;
					for(i = 0; i < cnt; i ++)
						if(signature[i])
								snprintf(mbuf[rc++], 1024, "[%s/%s%s%s]", 
									classpath[i], methodname[i], 
									dalvik_type_list_to_string(signature[i], NULL, 0), 
									dalvik_type_to_string(return_type[i], NULL, 0));
					idx = 0;
				}
				while(idx < rc)
				{
					type = mbuf[idx++];
					const char* ptr = text;
					for(;*ptr && *type && *type == *ptr; type ++, ptr ++);
					//return (*text == 0)?type:NULL;
					if(*ptr == 0) return type;
				}
				return NULL;
		}
		return NULL;
	}
	else 
	{
		if(!state)
		{
			for(;*text && *type && *type == *text; type ++, text ++);
			return (*text == 0)?type:NULL;
		}
		else return NULL;
	}
}
static inline int _cli_command_match(const cli_command_t* cmd, const char* begin, const char* end, int k)
{
	int j;
	static char buf[10240];
	const char* ptr;
	for(ptr = begin, j = 0; ptr < end; buf[1 + j++] = *(ptr++));
	buf[j + 1] = 0;
	buf[0] = '(';
	buf[j + 1] = ')';
	sexpression_t* sexp;
	if(NULL == sexp_parse(buf, &sexp)) return 0;
	sexpression_t* current = sexp, *this;
	int i;
	for(j = 0; cmd->pattern[j] && j < k; j ++)
	{
		int match = 1;
		const char* lit;
		const char* class, *method;
		dalvik_type_t* type[128];
		dalvik_type_t* return_type;
		switch((uintptr_t)cmd->pattern[j])
		{
			case (uintptr_t)STRING:
			case (uintptr_t)FILENAME:
				match = sexp_match(current, "(S?A", &lit, &current);
				break;
			case (uintptr_t)NUMBER:
				match = sexp_match(current, "(L?A", &lit, &current);
				break;
			case (uintptr_t)VALUELIST:
				while(match && SEXP_NIL != current)
					match = sexp_match(current, "(L?A", &lit, &current);
				break;
			case (uintptr_t)CLASSPATH:
				match = (NULL != sexp_get_object_path(current, (const sexpression_t**)&current));
				break;
			case (uintptr_t)SEXPRESSION:
				current = SEXP_NIL;
				match = 1;
				break;
			case (uintptr_t)FUNCTION:
				match = sexp_match(current, "(C?A", &this, &current);
				if(match)
				{
					return_type = NULL;   /* FOR THE STUPID GCC!!!! */
					match = _match_func(this, 
							             &class, &method, 
										 (const dalvik_type_t**)type, 
										 (const dalvik_type_t**) &return_type);
					if(NULL != return_type) dalvik_type_free(return_type);
					for(i = 0; type[i]; i ++) dalvik_type_free(type[i]);
				}
				break;
			case (uintptr_t)REGNAME:
				match = sexp_match(current, "(L?A", &lit, &current);
				break;
			default:
				match = sexp_match(current, "(L=A", cmd->pattern[j] ,&current);
		}
		if(match == 0) 
		{
			if(SEXP_NIL != sexp) sexp_free(sexp);
			return 0;
		}
	}
	if(SEXP_NIL != sexp) sexp_free(sexp);
	return 1;
}
char* cli_command_completion_function(const char* text, int state)
{
	while(*text == ' ' || *text == '\t') text ++;
	if(0 == *text)
	{
		static int last_index;
		if(!state) 
			last_index = 0;
		int next_index = last_index + 1;
		if(next_index < ncommands)
		{
			const char* word = _cli_command_completor("", cli_commands[next_index].pattern[0], 0);
			last_index = next_index;
			return _strcat("(", word);
		}
		else return NULL;
	}
	else if(*text == '(') 
	{
		static int last_state = 0;
		sexpression_t* sexp;
		if(NULL != sexp_parse(text, &sexp))
		{
			sexp_free(sexp);
			return NULL;
		}
		static int next_index;
		const char* ptr = text + 1;
		const char* last = NULL;
		int k = 0;
		for(;;)
		{
			sexpression_t* sexp;
			const char* rc = sexp_parse(ptr, &sexp);
			if(NULL != rc) 
			{
				last = ptr;
				ptr = rc;
				sexp_free(sexp);
				k ++;
			}
			else break;
		}
		if(*ptr == 0 && k > 0) k --;
		else last = ptr;
		if(!state) next_index = 0, last_state = 0;
		for(;next_index < ncommands;)
		{
			if(!_cli_command_match(cli_commands + next_index, text + 1, last, k)) 
			{
				last_state = 0;
				next_index ++;
				continue;
			}
			if(cli_commands[next_index].pattern[k] == NULL)
			{
				last_state = 0;
				next_index ++;
				return _strcat(text, ")");
			}
			const char* rc = _cli_command_completor(last, cli_commands[next_index].pattern[k], last_state);
			if(last_state == 0 && NULL != rc) last_state = 1;
			if(NULL == rc) last_state = 0, next_index ++;
			else return _strcat(text,rc);
		}
		return NULL;
	}
	return NULL;
}
#endif
