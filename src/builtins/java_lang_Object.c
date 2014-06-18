#include <bci/bci_interface.h>
hashval_t java_lang_Object_hash(const void* data)
{
	return 0x38745c6;
}
int java_lang_Object_get_addr_list(const void* data, uint32_t* buf, size_t sz)
{
	if(NULL == data || NULL == buf) return -1;
	return 0;
}
int java_lang_Object_equal(const void* this, const void* that)
{
	return 1;
}
bci_class_t java_lang_Object_metadata = {
	.size = 0,
	.get_addr_list = java_lang_Object_get_addr_list,
	.hash = java_lang_Object_hash,
	.equal = java_lang_Object_equal,
	.provides = {
		"java/lang/Object",
		NULL
	}
};
