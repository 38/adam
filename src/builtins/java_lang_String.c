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
static const char* kw_init;
static const char* kw_concat;
int java_lang_String_onload()
{
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
	kw_init = stringpool_query("<init>");
	kw_concat = stringpool_query("concat");
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
int java_lang_String_get_method(const void* this_ptr, const char* classpath, const char* method, const dalvik_type_t* const * signature, const dalvik_type_t* rettype)
{
	if(method == kw_init) return 0;
	if(method == kw_concat) return 1;
	return -1;
}
static inline int _java_lang_String_concat(bci_method_env_t* env)
{
	extern bci_class_t java_lang_String_metadata;
	const cesk_set_t* this = bci_interface_read_arg(env, 0, 2);
	const cesk_set_t* that = bci_interface_read_arg(env, 1, 2);
	if(NULL == this || NULL == that)
	{
		LOG_ERROR("can not read argument");
		return -1;
	}
	tag_set_t* new_tags = tag_set_empty();
	if(NULL == new_tags)
	{
		LOG_ERROR("can not create new tag set");
		return -1;
	}
	uint32_t addr;
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(this, &iter))
	{
		LOG_ERROR("can not read `this' set");
		return -1;
	}
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		tag_set_t* tmp = tag_set_merge(new_tags, bci_interface_read_tag(env, addr));
		tag_set_free(new_tags);
		new_tags = tmp;
	}
	
	if(NULL == cesk_set_iter(that, &iter))
	{
		LOG_ERROR("can not read `that' set");
		return -1;
	}
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		tag_set_t* tmp = tag_set_merge(new_tags, bci_interface_read_tag(env, addr));
		tag_set_free(new_tags);
		new_tags = tmp;
	}

	addr = bci_interface_new_object(env, java_lang_String_metadata.provides[0], _UNDETERM);
	if(CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("can not allocate new object");
		return -1;
	}

	if(bci_interface_append_tag_set(env, addr, new_tags) < 0)
	{
		LOG_ERROR("can not change the tag set");
		return -1;
	}

	bci_interface_return_object(env, addr);
	bci_interface_return_single_address(env, addr);
	return 0;
}
int java_lang_String_invoke(int method_id, bci_method_env_t* env)
{
	if(method_id == 0) return 0;
	else if(method_id == 1) return _java_lang_String_concat(env);
	return -1;
}
bci_class_t java_lang_String_metadata = {
	.size = sizeof(string_data_t),
	.load = java_lang_String_onload,
	.initialize = java_lang_String_init,
	.duplicate = java_lang_String_duplicate,
	.to_string = java_lang_String_to_string,
	.merge = java_lang_String_merge,
	.hash = java_lang_String_hash,
	.equal = java_lang_String_equal,
	.get = java_lang_String_get_field,
	.is_instance = java_lang_String_instance_of,
	.get_method = java_lang_String_get_method,
	.invoke = java_lang_String_invoke,
	.super = "java/lang/Object",
	.provides = {
		"java/lang/String", 
		NULL
	}
};

