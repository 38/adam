#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_field.h>

#include <string.h>
#include <stdlib.h>
#include <debug.h>

#ifdef PARSER_COUNT
int dalvik_field_count = 0;
#endif

dalvik_field_t* dalvik_field_from_sexp(const sexpression_t* sexp, const char* class_path, const char* file_name)
{
#ifdef PARSER_COUNT
	dalvik_field_count ++;
#endif

	dalvik_field_t* ret = NULL;
	if(SEXP_NIL == sexp) goto ERR;
	if(NULL == class_path) class_path = "(undefined)";
	if(NULL== file_name)   file_name = "(undefined)";
	const char* name;
	sexpression_t *attr_list , *type_sexp;
	if(!sexp_match(sexp, "(L=C?L?_?A", DALVIK_TOKEN_FIELD, &attr_list, &name, &type_sexp, &sexp))
	{
		LOG_ERROR("bad field definition");
		goto ERR;
	}
	
	ret = (dalvik_field_t*)malloc(sizeof(dalvik_field_t));
	
	ret->name = name;
	ret->path = class_path;
	ret->file = file_name;

	if(NULL == (ret->type = dalvik_type_from_sexp(type_sexp))) 
	{
		LOG_ERROR("can't parse type");
		LOG_DEBUG("type is %s", sexp_to_string(type_sexp,NULL, 0));
		goto ERR;
	}

	if(-1 == (ret->attrs = dalvik_attrs_from_sexp(attr_list))) 
	{
		LOG_ERROR("can't parse attribute");
		goto ERR;
	}

	if(SEXP_NIL != sexp)
	{
		/* it has a defualt value */
		if(!sexp_match(sexp, "(L?A", &ret->defualt_value, &sexp)) 
		{
			LOG_ERROR("can't parse default value");
			goto ERR;
		}
		//LOG_NOTICE("fixme: parse default value of a field");
	}
	else
		ret->defualt_value = NULL;
	return ret;
ERR:
	dalvik_field_free(ret);
	return NULL;
}

void dalvik_field_free(dalvik_field_t* mem)
{
	if(NULL != mem) 
	{
		if(NULL != mem->type) dalvik_type_free(mem->type);
		free(mem);
	}
}
