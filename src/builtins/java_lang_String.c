#include <const_assertion.h>

#include <bci/bci_interface.h>

#define _UNDETERM ((const char*)1)
extern bci_class_t java_lang_String_metadata;

static const char* java_lang_String_empty = NULL;
static const char* java_lang_String_length = NULL;
int java_lang_String_onload()
{
	if(java_lang_String_metadata.size != sizeof(const char*))
	{
		LOG_ERROR("bad class def: java.lang.String");
		return -1;
	}
	if((java_lang_String_empty = stringpool_query("")) == NULL)
	{
		LOG_ERROR("Failed to initialize java.lang.String");
		return -1;
	}
	if((java_lang_String_length = stringpool_query("length")) == NULL)
	{
		LOG_ERROR("Failed to initialize java.lang.String");
		return -1;
	}
	return 0;
}
int java_lang_String_init(void* this, const void* param)
{
	if(*(const char**)this == NULL)
	{
		if(NULL != param)
			*(const char**)this = (const char*)param;
		else 
			*(const char**)this = java_lang_String_empty;
	}
	else if(*(const char**)this != _UNDETERM)
	{
		if(param != *(const char**)this) 
			*(const char**)this = _UNDETERM;
	}
	return 0;
}
int java_lang_String_duplicate(const void* this, void* that)
{
	*(const char**) that = *(const char**) this;
	return 0;
}
cesk_set_t* java_lang_String_get_field(const void* this, const char* fieldname)
{
	cesk_set_t* ret = cesk_set_empty_set();
	if(NULL == ret)
	{
		LOG_ERROR("Can not create the result set");
		goto ERR;
	}
	if(java_lang_String_length == fieldname)
	{
		if(java_lang_String_empty == *(const char**)this)
		{
			cesk_set_push(ret,CESK_STORE_ADDR_ZERO);
		}
		else if(_UNDETERM == *(const char**)this)
		{
			cesk_set_push(ret,CESK_STORE_ADDR_ZERO);
			cesk_set_push(ret,CESK_STORE_ADDR_POS);
		}
		else if(NULL != *(const char**)this)
		{
			cesk_set_push(ret, CESK_STORE_ADDR_POS);
		}
		else
		{
			LOG_WARNING("uninitialized value");
		}
	}
	else
	{
		LOG_ERROR("Unsupported field");
		goto ERR;
	}
	return ret;
ERR:
	if(NULL != ret) cesk_set_free(ret);
	return NULL;
}
const char* java_lang_String_to_string(const void* this, char* buf, size_t size)
{
	if(NULL == *(const char**)this) return "(uninitialized)";
	else if(_UNDETERM == *(const char**)this) return "(undetermistic)";
	else return *(const char**) this;
}
int java_lang_String_merge(void* this, const void* that)
{
	if(*(const char**)this == *(const char**)that)
		*(const char**) this = _UNDETERM;
	return 0;
}
hashval_t java_lang_String_hash(const void* this)
{
	return (*(uintptr_t*) this) * MH_MULTIPLY;
}
int java_lang_String_equal(const void* this, const void* that)
{
	return *(const char**)this == *(const char**) that;
}
bci_class_t java_lang_String_metadata = {
	.size = sizeof(const char*),
	.onload = java_lang_String_onload,
	.initialization = java_lang_String_init,
	.duplicate = java_lang_String_duplicate,
	.to_string = java_lang_String_to_string,
	.merge = java_lang_String_merge,
	.hash = java_lang_String_hash,
	.equal = java_lang_String_equal,
	.get_field = java_lang_String_get_field,
	.super = "java/lang/Object",
	.provides = {
		"java/lang/String", 
		NULL
	}
};

