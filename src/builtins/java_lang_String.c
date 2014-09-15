#include <const_assertion.h>

#include <bci/bci_interface.h>

#define _UNDETERM ((const char*)1)
typedef struct {
	uint8_t init_status;
	const char* str;
} string_data_t;
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
int java_lang_String_init(void* this, const char* classpath, const void* param, tag_set_t** tags)
{
	string_data_t* self = (string_data_t*) this;
	if(0 == self->init_status)
	{
		self->str = (const char*)param;
		self->init_status = 1;
	}
	else if(self->str != _UNDETERM)
	{
		if(param != self->str) self->str = _UNDETERM;
	}
	return 0;
}
int java_lang_String_duplicate(const void* this, void* that)
{
	*(string_data_t*) that = *(string_data_t*) this;
	return 0;
}
cesk_set_t* java_lang_String_get_field(const void* this, const char* fieldname)
{
	const string_data_t* self = (const string_data_t*) this;
	cesk_set_t* ret = cesk_set_empty_set();
	if(NULL == ret)
	{
		LOG_ERROR("Can not create the result set");
		goto ERR;
	}
	if(java_lang_String_length == fieldname)
	{
		if(java_lang_String_empty == self->str)
		{
			cesk_set_push(ret,CESK_STORE_ADDR_ZERO);
		}
		else if(_UNDETERM == self->str)
		{
			cesk_set_push(ret,CESK_STORE_ADDR_ZERO);
			cesk_set_push(ret,CESK_STORE_ADDR_POS);
		}
		else if(NULL != self->str)
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
	const string_data_t* self = (const string_data_t*) this;
	if(NULL == self->str) return "(uninitialized)";
	else if(_UNDETERM == self->str) return "(undetermistic)";
	else return self->str;
}
int java_lang_String_merge(void* this, const void* that)
{
	string_data_t* left = (string_data_t*) this;
	const string_data_t* right = (const string_data_t*) that;
	if(left->str != right->str)
		left->str = _UNDETERM;
	return 1;
}
hashval_t java_lang_String_hash(const void* this)
{
	const string_data_t* self = (const string_data_t*) this;
	return ((uintptr_t)self->str) * MH_MULTIPLY;
}
int java_lang_String_equal(const void* this, const void* that)
{
	const string_data_t* left = (const string_data_t*) this;
	const string_data_t* right = (const string_data_t*) that;
	return left->str == right->str;
}
int java_lang_String_instance_of(const void* this, const dalvik_type_t* type)
{
	extern bci_class_t java_lang_Object_metadata;
	if(type->typecode != DALVIK_TYPECODE_OBJECT) return BCI_BOOLEAN_FALSE;
	return type->data.object.path ==  java_lang_Object_metadata.provides[0];
}
bci_class_t java_lang_String_metadata = {
	.size = sizeof(string_data_t),
	.onload = java_lang_String_onload,
	.initialization = java_lang_String_init,
	.duplicate = java_lang_String_duplicate,
	.to_string = java_lang_String_to_string,
	.merge = java_lang_String_merge,
	.hash = java_lang_String_hash,
	.equal = java_lang_String_equal,
	.get_field = java_lang_String_get_field,
	.instance_of = java_lang_String_instance_of,
	.super = "java/lang/Object",
	.provides = {
		"java/lang/String", 
		NULL
	}
};

