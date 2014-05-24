#include <dalvik/dalvik_class.h>
#include <log.h>
#include <sexp.h>
#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_field.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_memberdict.h>
#include <string.h>
#include <debug.h>

#ifdef PARSER_COUNT
int dalvik_class_count = 0;
#endif

dalvik_class_t* dalvik_class_from_sexp(const sexpression_t* sexp)
{
#ifdef PARSER_COUNT
	dalvik_class_count ++;
#endif

	dalvik_class_t* class = NULL;
	int length;
	int is_interface;
	const char* class_path;
	int attrs;
	int field_count = 0;
	if(SEXP_NIL == sexp) 
	{
		LOG_ERROR("bad S-Expression");
		goto ERR;
	}
	const char* firstlit;
	sexpression_t* attr_list;
	if(!sexp_match(sexp, "(L?C?A", &firstlit, &attr_list, &sexp))
	{
		LOG_ERROR("can't peek the first literial");
		goto ERR;
	}
	if(DALVIK_TOKEN_CLASS == firstlit)
		is_interface = 0;
	else if(DALVIK_TOKEN_INTERFACE == firstlit)
		is_interface = 1;
	else 
	{
		LOG_ERROR("first literal of a class should not be %s", firstlit);
		goto ERR;
	}
	
	if((attrs = dalvik_attrs_from_sexp(attr_list)) < 0)
	{
		LOG_ERROR("invalid attribute %s", sexp_to_string(attr_list,NULL,0));
		goto ERR;
	}
	
	if(NULL == (class_path = sexp_get_object_path(sexp, &sexp)))
	{
		LOG_ERROR("invalid attribute");
		goto ERR;
	}

	if((length = sexp_length(sexp)) < 0)
	{
		LOG_ERROR("unexcepted length of S-Expression");
		goto ERR;
	}

	/* We allocate length + 1 because we need a NULL at the end of member list */
	class = (dalvik_class_t*)malloc(sizeof(dalvik_class_t) + sizeof(const char*) * (length + 1));  

	if(class == NULL) 
	{
		LOG_ERROR("can not allocate memory for class");
		goto ERR;
	}

	class->path = class_path;
	class->attrs = attrs;
	class->is_interface = is_interface;
	memset(class->members, 0, sizeof(const char*) * length);

	const char* source = "(undefined)";
	class->implements[0] = NULL;
	int num_implements = 0;
	/* Ok, let's go */
	for(;sexp != SEXP_NIL;)
	{
		sexpression_t* this_def, *tail;
		if(!sexp_match(sexp, "(C?A", &this_def, &sexp))
		{
			LOG_DEBUG("failed to fetch next definition in the class, aborting");
			goto ERR;
		}
		if(sexp_match(this_def, "(L=S?", DALVIK_TOKEN_SOURCE, &source))
		{
			LOG_DEBUG("the source file for this class is %s", source);
			/* Do Nothing*/
		}
		else if(sexp_match(this_def, "(L=A", DALVIK_TOKEN_SUPER, &tail))
		{
			if(NULL == (class->super = sexp_get_object_path(tail,NULL)))
			{
				LOG_ERROR("can not get the classpath for the super class");
				goto ERR;
			}
		}
		else if(sexp_match(this_def, "(L=A", DALVIK_TOKEN_IMPLEMENTS, &tail))
		{
			if(num_implements >= 128)
			{
				LOG_WARNING("class %s implements more than 128 interfaces, is this a mistake?", class_path);
				continue;
			}
			if(NULL == (class->implements[num_implements] = sexp_get_object_path(tail, NULL)))
			{
				LOG_ERROR("can not get the classpath for the interface the class implements");
				goto ERR;
			}
			else
			{
				class->implements[++num_implements] = NULL;
			}
		}
		else if(sexp_match(this_def, "(L=A", DALVIK_TOKEN_ANNOTATION, &tail))
		{
			LOG_NOTICE("fixme: ingored psuedo-instruction (annotation)");
			/* just ingore */
		}
		else
		{
			const char* firstlit;
			if(!sexp_match(this_def, "(L?A", &firstlit, &tail))
			{
				LOG_ERROR("failed to peek the first literal");
				goto ERR;
			}
			if(firstlit == DALVIK_TOKEN_METHOD)
			{
				//LOG_DEBUG("we are resuloving method %s", sexp_to_string(this_def, buf));
				dalvik_method_t* method;
				if(NULL == (method = dalvik_method_from_sexp(this_def, class->path, source)))
				{
					LOG_ERROR("can not resolve method %s", sexp_to_string(this_def, NULL,0));
					goto ERR;
				}
				/* Register it */
				if(dalvik_memberdict_register_method(class->path, method) < 0)
				{
					LOG_ERROR("can not register new method %s.%s", class->path, method->name);
					goto ERR;
				}
				LOG_DEBUG("new class method %s.%s", class->path, method->name);
				/* because the method in fact is static object, we can retrive the method thru member dict
				 * So we do not add them to the memberlist */
			}
			else if(firstlit == DALVIK_TOKEN_FIELD)
			{
				dalvik_field_t* field;
				if(NULL == (field = dalvik_field_from_sexp(this_def, class->path, source)))
				{
					LOG_ERROR("can not resolve field %s", sexp_to_string(this_def, NULL,0));
					goto ERR;
				}
				/* Not static? it's the real member */
				if((field->attrs & DALVIK_ATTRS_STATIC) == 0)
				{
					field->offset = field_count;
					class->members[field_count++] = field->name;
				}
				else
				{
					//TODO register static member here
				}
				if(dalvik_memberdict_register_field(class->path, field) < 0)
				{
					LOG_ERROR("can not register new method %s.%s", class->path, field->name);
					goto ERR;
				}
				LOG_DEBUG("new class method %s.%s", class->path, field->name);
			}
			else 
			{
				LOG_WARNING("we are ingoring a field, because we dont know what does '%s' mean", firstlit);
			}
		}
	}
	if(dalvik_memberdict_register_class(class->path, class) < 0) goto ERR;
	LOG_TRACE("Class %s Loaded", class->path);
	return class;
ERR:
	if(NULL != class) free(class);
	return NULL;
}
