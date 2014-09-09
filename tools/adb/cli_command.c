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
static inline int _match_func(const sexpression_t** sexp, const char** class, const char** method, const dalvik_type_t** signature, const dalvik_type_t** rtype)
{
	if(sexp_get_method_address(*sexp, sexp, class, method) < 0)
		return 0;
	sexpression_t* sexp_signature, *this_type;
	if(!sexp_match(*sexp, "(C?A", &sexp_signature, sexp))
		return 0;
	int i;
	for(i = 0; SEXP_NIL != sexp_signature && sexp_match(sexp_signature, "(_?A", &this_type, &sexp_signature); i ++)
		if(NULL == (signature[i] = dalvik_type_from_sexp(this_type)))
			return 0;
	signature[i] = NULL;
	sexpression_t* sexp_rtype;
	if(!sexp_match(*sexp, "(_?A", &sexp_rtype, sexp))
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
		for(j = 0; cli_commands[i].pattern[j]; j ++)
		{
			int match = 0;
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
					match = (_match_func(
								(const sexpression_t**)&current, 
								&cli_commands[i].args[j].function.class,
								&cli_commands[i].args[j].function.method,
								cli_commands[i].args[j].function.signature,
								&cli_commands[i].args[j].function.return_type) >= 0);
					break;
				default:
					match = sexp_match(current, "(L=A", cli_commands[i].pattern[j] ,&current);
					if(match) cli_commands[i].args[j].literal = cli_commands[i].pattern[j];
			}
			if(match == 0) break;
		}
		int k;
		if(cli_commands[i].pattern[j] == NULL) 
		{
			rc = cli_commands[i].action(cli_commands + i);
			for(k = 0; k < j; k ++)
			{
				if(cli_commands[i].pattern[k] == FUNCTION)
				{
					dalvik_type_free((dalvik_type_t*)cli_commands[i].args[k].function.return_type);
					dalvik_type_list_free((dalvik_type_t**)cli_commands[i].args[k].function.signature);
				}
			}
			break;
		}
		for(k = 0; k < j; k ++)
		{
			if(cli_commands[i].pattern[k] == FUNCTION)
			{
				dalvik_type_free((dalvik_type_t*)cli_commands[i].args[k].function.return_type);
				dalvik_type_list_free((dalvik_type_t**)cli_commands[i].args[k].function.signature);
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
						snprintf(ptr, free, "<class path>/<method name> (<type1> <type2> ... <typeN>) <type> ");
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
