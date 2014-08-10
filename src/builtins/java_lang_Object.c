#include <bci/bci_interface.h>
#include <stringpool.h>
static const char* _hashcode = NULL;
static dalvik_type_t* _hashcode_rtype = NULL;
static dalvik_type_t* _hashcdoe_args[1] = {NULL};
hashval_t java_lang_Object_hash(const void* data)
{
	return 0x38745c6;
}
int java_lang_Object_get_addr_list(const void* data, uint32_t offset, uint32_t* buf, size_t sz)
{
	if(NULL == data || NULL == buf) return -1;
	return 0;
}
int java_lang_Object_equal(const void* this, const void* that)
{
	return 1;
}
int java_lang_Object_duplicate(const void* this, void* that)
{
	return 0;
}
const char* java_lang_Object_to_string(const void* this, char* buf, size_t sz)
{
	if(0 == sz) return NULL;
	buf[0] = 0;
	return buf;
}
int java_lang_Object_merge(void* this, const void* that)
{
	return 0;
}
int java_lang_Object_instance_of(const void* this, const char* classpath)
{
	extern bci_class_t java_lang_Object_metadata;
	return classpath ==  java_lang_Object_metadata.provides[0];
}
int java_lang_Object_onload()
{
	_hashcode = stringpool_query("hashCode");
	_hashcode_rtype = DALVIK_TYPE_INT;
	return 0;
}
int java_lang_Object_get_method(const void* this, const char* methodname, const dalvik_type_t* const* args, const dalvik_type_t* rtype)
{
	if(methodname == _hashcode && dalvik_type_list_equal(args, (const dalvik_type_t* const*) _hashcdoe_args) && dalvik_type_equal(rtype, _hashcode_rtype))
		return BCI_CLASS_METHOD_CONST | 0;
	return -1;
}
int java_lang_Object_invoke(void* this, const void* const_this, int method_id, bci_method_env_t* env)
{
	if((BCI_CLASS_METHOD_CONST | 0) == method_id) 
	{
		//DO SOMETHING
	}
	return 0;
}
bci_class_t java_lang_Object_metadata = {
	.size = 0,
	.onload = java_lang_Object_onload,
	.get_addr_list = java_lang_Object_get_addr_list,
	.hash = java_lang_Object_hash,
	.equal = java_lang_Object_equal,
	.duplicate = java_lang_Object_duplicate,
	.to_string = java_lang_Object_to_string,
	.instance_of = java_lang_Object_instance_of,
	.merge = java_lang_Object_merge,
	.get_method = java_lang_Object_get_method,
	.invoke = java_lang_Object_invoke,
	.super = NULL,
	.provides = {
		"java/lang/Object",
		NULL
	}
};
