#include <stdarg.h>

#include <sexp.h>
#include <stringpool.h>
#include <string.h>
#include <debug.h>
sexpression_t _sexp_eof;
/**
 * @brief allocate a new s-expression
 * @param type the type of this S-Expression (literal/stirng/cons)
 * @return the newly created S-Expression
 **/
static inline sexpression_t* _sexp_alloc(int type)
{
	size_t size = sizeof(sexpression_t);
	switch(type)
	{
		case SEXP_TYPE_LIT:
			size += sizeof(sexp_lit_t);
			break;
		case SEXP_TYPE_STR:
			size += sizeof(sexp_str_t);
			break;
		case SEXP_TYPE_CONS:
			size += sizeof(sexp_cons_t);
			break;
		default:
			return NULL;
	}
	sexpression_t* ret = (sexpression_t*) malloc(size);
	if(NULL != ret) 
	{
		memset(ret, 0, size);
		ret->type = type;
	}
	return ret;
}
void sexp_free(sexpression_t* buf)
{
	sexp_cons_t* cons_data;
	if(SEXP_NIL == buf)  return; /* A empty S-Expression */
	if(SEXP_EOF == buf)  return; /* EOF singleton */
	switch(buf->type)
	{
		case SEXP_TYPE_CONS:     /* A Cons S-Expression, free the memory recursively */
			cons_data = (sexp_cons_t*) buf->data;
			sexp_free(cons_data->first);
			sexp_free(cons_data->second);
		default:
			free(buf);
	}
}
/** @brief strip the white space, return value if the function eated a space */
static inline int _sexp_parse_ws(const char** p) 
{
	int ret = 0;
	if(**p == '-') 
	{
		(*p) ++;
		ret = 1;
	}
	for(; **p == ' ' || 
		  **p == '\t' ||
		  **p == '\r' ||
		  **p == '\n'||
		  **p == '/' ||
		  //**p == '-' ||
		  **p == ',' ||
		  **p == '.';
		  (*p) ++) ret = 1;
	return ret;
}
static inline void _sexp_parse_comment(const char** str)
{
	if(**str == ';')
	{
		(*str) ++;
		while(**str && **str != '\n')
			(*str) ++;
		_sexp_parse_ws(str);
		_sexp_parse_comment(str);
	}
}
/**
 * @brief parse the list from str  .... ), the first '(' is already eatten 
 * @note this function is a tail recursion in the original version, now it's a loop
 **/
static inline const char* _sexp_parse_list(const char* str, sexpression_t** buf)
{
	sexpression_t* head = NULL, **input = buf;
	for(;;)
	{
		/* strip the leading white spaces */
		_sexp_parse_ws(&str);
		_sexp_parse_comment(&str);
		/* If it's an empty list, then return NIL */
		if(*str == ')' || *str == ']' || *str == '}') 
		{
			*buf = SEXP_NIL;
			return str + 1;
		}
		else
		{
			/* The list has at least one element */
			*buf = _sexp_alloc(SEXP_TYPE_CONS);
			sexp_cons_t* data = (sexp_cons_t*)((*buf)->data);
			if(NULL == *buf) goto ERR;
			if(*str == '-') str--;
			str = sexp_parse(str, &data->first);
			if(NULL == str) goto ERR;
			data->seperator = *str;
			if(NULL == head) head = *buf;
			buf = &data->second;
		}
	}
ERR:
	sexp_free(head);
	*input = NULL;
	return NULL;
}
/*@todo escape sequences support */
static inline const char* _sexp_parse_string(const char* str, sexpression_t** buf)
{
	int escape = 0;
	stringpool_accumulator_t accumulator;
	stringpool_accumulator_init(&accumulator, str); 
	for(;*str; str ++)
	{
		if(escape == 0)
		{
			if(*str == '"') 
			{
				*buf = _sexp_alloc(SEXP_TYPE_STR);
				if(NULL == *buf) return NULL;
				sexp_str_t* data = (sexp_str_t*)((*buf)->data);
				(*data) = stringpool_accumulator_query(&accumulator);
				return str + 1;
			}
			else if(*str != '\\')
				stringpool_accumulator_next(&accumulator,*str);
			else
				escape = 1;
		}
		else
		{
			escape = 0;
			/* TODO: escape sequences */
			stringpool_accumulator_next(&accumulator, *str);
		}
	}
	*buf = NULL;
	return NULL;
}
/*@todo escape sequences support */
static inline const char* _sexp_parse_char(const char* str, sexpression_t** buf)
{
	char tmp[3] = {};
	*buf = _sexp_alloc(SEXP_TYPE_STR);
	if(NULL == *buf) 
	{
		*buf = NULL;
		return NULL;
	}
	sexp_str_t* data = (sexp_str_t*)((*buf)->data);
	if(str[0] == '\\')
	{
		tmp[0] = str[1];
		if(str[1] == 0) str ++;
		else str += 2;
		/* TODO: escape sequences */
	}
	else 
	{
		tmp[0] = str[0];
		str ++;
	}
	*data = stringpool_query(tmp);
	return str;
}
/* parse a literal */
#define RANGE(l,r,v) (((l) <= (v)) && ((v) <= (r)))
static inline const char* _sexpr_parse_literal(const char* str, sexpression_t** buf)
{
	stringpool_accumulator_t accumulator;
	stringpool_accumulator_init(&accumulator, str);
	if(*str == '-')
		stringpool_accumulator_next(&accumulator, *(str ++));
	for(; *str != '\r' &&
		  *str != '\n' &&
		  *str != ' '  &&
		  *str != '\t' &&
		  *str != '/'  &&
		  *str != '-'  &&
		  *str != '.'  &&
		  *str != ','  &&
		  *str != '('  &&
		  *str != ')'  &&
		  *str != '['  &&
		  *str != ']'  &&
		  *str != '{'  &&
		  *str != '}'  &&
		  *str != 0; str++)
		stringpool_accumulator_next(&accumulator, *str);
	*buf = _sexp_alloc(SEXP_TYPE_LIT);
	if(*buf == NULL) return NULL;
	sexp_lit_t* data;
	data = (sexp_lit_t*)((*buf)->data);
	*data = stringpool_accumulator_query(&accumulator);
	return str;
}
const char* sexp_parse(const char* str, sexpression_t** buf)
{
	if(NULL == str) return NULL;
	_sexp_parse_ws(&str);
	_sexp_parse_comment(&str);
	if(*str == 0)
	{
		(*buf) = SEXP_EOF;
		return NULL;
	}
	else if(*str == ')' || *str == ']' || *str == '}') return NULL;
	else if(*str == '(' || *str == '[' || *str == '{') return _sexp_parse_list(str + 1, buf);
	else if(*str == '"') return _sexp_parse_string(str + 1, buf);
	else if(*str == '#') return _sexp_parse_char(str + 1, buf);
	else return _sexpr_parse_literal(str, buf);
}
static inline int _sexp_match_one(const sexpression_t* sexpr, char tc, char sc, const void** this_arg)
{
	int ret = 1;
	int type;
	if(sexpr == SEXP_NIL) 
	{
		if(tc == 'C' && sc == '?') 
		{
			(*this_arg) = SEXP_NIL;
			return 1;
		}
		return 0;
	}
	/* Determine the type expected */
	switch(tc)
	{
		case 'C':
			type = SEXP_TYPE_CONS;
			break;
		case 'L':
			type = SEXP_TYPE_LIT;
			break;
		case 'S':
			type = SEXP_TYPE_STR;
			break;
		case '_':
			type = sexpr->type;
			break;
		default:
			ret = 0;
			goto DONE;
	}
	if(type != sexpr->type)
	{
		/* The type does not match ? */
		ret = 0;
		goto DONE;
	}
	if(sc == 0)
	{
		/* A type name without a suffix, bad pattern */
		ret = 0;
		goto DONE;
	}
	if(SEXP_TYPE_CONS == type && sc == '=')
	{
		/* Cons can not used as input */
		ret = 0;
		goto DONE;
	}
	if(tc == '_' && sc == '=')
	{
		/* wildcard can not used as input */
		ret = 0;
		goto DONE;
	}
	if(sc == '=')
	{
	/* We are going to verify it*/
		const char* this_chr = (const char*) this_arg;
		if(this_chr == *(const char**)sexpr->data) 
			ret = 1;
		else 
			ret = 0;
		goto DONE;
	}
	else if(sc == '?')
	{
		/* Copy the point to user */
		ret = 1;
		if(type != SEXP_TYPE_CONS && tc != '_')
			(*this_arg) = *(const void**)sexpr->data;
		else
			(*this_arg) = sexpr;
	}
DONE:
	return ret;
}
int sexp_match(const sexpression_t* sexpr, const char* pattern, ...)
{
	int ret = 1;
	va_list va;
	const void** this_arg;
	if(NULL == pattern) return 0;
	va_start(va,pattern);
	if(pattern[0] != '(')
	{
		this_arg = va_arg(va, const void **);
		if(*pattern != 0) 
			ret = _sexp_match_one(sexpr, pattern[0], pattern[1], this_arg);
		else
			ret = 0;
	}
	else
	{
		for(pattern++; *pattern && ret ; pattern += 2)
		{
			this_arg = va_arg(va, const void**);
			if('A' == pattern[0])
			{
				pattern ++;
				(*this_arg) = sexpr;
				sexpr = SEXP_NIL;
				break;
			}
			if(sexpr == SEXP_NIL) break;
			if(sexpr->type != SEXP_TYPE_CONS) 
			{
				ret = 0;
				break;
			}
			sexp_cons_t* cons = (sexp_cons_t*)sexpr->data;
			ret = _sexp_match_one(cons->first, pattern[0], pattern[1], this_arg);
			sexpr = cons->second;
		}
		if(*pattern == 0 && sexpr == SEXP_NIL && ret) ret = 1;
		else ret = 0;
	}
	va_end(va);
	return ret;
}
const sexpression_t* sexp_strip(const sexpression_t* sexpr, ...)
{
	va_list va;
	if(NULL == sexpr) return sexpr;
	if(sexpr->type != SEXP_TYPE_CONS) return sexpr;
	va_start(va, sexpr);
	sexp_cons_t*   cons = (sexp_cons_t*) sexpr->data;
	void* first_addr = *(void**)cons->first->data;
	for(;;)
	{
		const char* this;
		this = va_arg(va, const char*);
		if(this == NULL) break;   /* we are reaching the end of the list */
		if(first_addr == this)
			return cons->second;   /* strip the expected element */
	}
	va_end(va);
	return sexpr;   /* nothing to strip */
}
const char* sexp_get_object_path(const sexpression_t* sexpr, const sexpression_t** remaining)
{
	int len = 0;
	static char buf[4096];   /* Issue: thread safe */
	if(sexpr == NULL) return NULL;
	for(; sexpr != SEXP_NIL;)
	{
		/* Because the property of parser, we are excepting a cons here */
		if(sexpr->type != SEXP_TYPE_CONS) 
			return NULL;   
		sexp_cons_t *cons = (sexp_cons_t*)sexpr->data;
		 /* first should not be a non-literial */
		if(SEXP_NIL == cons->first || cons->first->type != SEXP_TYPE_LIT) 
			return NULL;   
		const char *p;
		for(p = *(const char**)cons->first->data;
			*p;
			 p++) buf[len++] = *p;
		buf[len ++] = '/';
		sexpr = cons->second;
		if(cons->seperator != '/')
			break;
	}
	if(NULL != remaining) (*remaining) = sexpr;
	buf[--len] = 0;
	return stringpool_query(buf);
}
int sexp_get_method_address(const sexpression_t* sexpr, const sexpression_t** remaining, const char** path, const char** name)
{
	int len = 0;
	char buf[4096];
	char* last = NULL;
	if(sexpr == SEXP_NIL) return -1;
	for(; sexpr != SEXP_NIL;)  
	{
		/* this loop will execute at least once, so last must not be a null value */
		/* Because the property of parser, we are excepting a cons here */
		if(sexpr->type != SEXP_TYPE_CONS) 
			return -1;   
		sexp_cons_t *cons = (sexp_cons_t*)sexpr->data;
		 /* first should not be a non-literial */
		if(SEXP_NIL == cons->first || cons->first->type != SEXP_TYPE_LIT) 
			return -1;   
		const char *p;
		last = buf + len - 1;
		for(p = *(const char**)cons->first->data;
			*p;
			 p++) buf[len++] = *p;
		buf[len ++] = '/';
		sexpr = cons->second;
		if(cons->seperator != '/')
			break;
	}
	if(NULL != remaining) (*remaining) = sexpr;
	buf[--len] = 0;
	(*name) = stringpool_query(last + 1);
	if(last >= buf)
	{
		(*last) = 0;
		(*path) = stringpool_query(buf);
	}
	else
		(*path) = stringpool_query("");
	return 0;
}
int sexp_length(const sexpression_t* sexp)
{
	if(SEXP_NIL == sexp) return 0;
	if(sexp->type == SEXP_TYPE_LIT ||
	   sexp->type == SEXP_TYPE_STR) 
		return 1;
	int ret = 0;
	for(;sexp != SEXP_NIL && sexp->type == SEXP_TYPE_CONS; ret ++)
		sexp = ((sexp_cons_t*)sexp->data)->second;
	return ret;
}
char* sexp_to_string(const sexpression_t* sexp, char* buf, size_t sz)
{
	static char defualt_buf[1024];
	char * ret;
	if(buf == NULL) buf = defualt_buf, sz = sizeof(defualt_buf);
	ret = buf;
	if(NULL == buf) return NULL;
	if(sz == 0) return buf;
	if(SEXP_NIL == sexp) 
	{
		snprintf(buf, sz, "nil");
		return buf;
	}
	if(sexp->type == SEXP_TYPE_LIT)
	{
		snprintf(buf, sz, "%s", *(sexp_lit_t*)sexp->data);
		sz -= strlen(buf);
	}
	else if(sexp->type == SEXP_TYPE_STR)
	{
		snprintf(buf, sz, "'%s'", *(sexp_str_t*)sexp->data);
		sz -= strlen(buf);
	}
	else if(sexp->type == SEXP_TYPE_CONS && sz > 2)
	{
		sexp_cons_t *cons = (sexp_cons_t*)sexp->data;
		if(sz > 1) 
		{
			buf[0] = '(';
			buf ++, sz --;
		}
		if(NULL == sexp_to_string(cons->first, buf, sz)) return NULL;
		sz -= strlen(buf);
		buf += strlen(buf);
		if(sz > 1)
		{
			buf[0] = ' ';
			buf ++, sz --;
		}
		if(NULL == sexp_to_string(cons->second, buf, sz)) return NULL;
		sz -= strlen(buf);
		buf += strlen(buf);
		if(sz > 1)
		{
			buf[0] = ')';
			sz--;
			buf[1] = 0;
		}
	}
	return ret;
}
