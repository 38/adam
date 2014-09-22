#include <bci/bci_interface.h>
#include <stringpool.h>
extern bci_class_t org_apache_http_HttpClient_metadata;
typedef struct {
	uint8_t istat;
	const char* classpath;
} this_t;
int org_apache_http_HttpClient_load()
{
	return 0;
}
int org_apache_http_HttpClient_init(void* self, const char* classpath, const void* init_param, tag_set_t** p_tagset)
{
	this_t* this = (this_t*)self;
	if(this->istat == 0) this->classpath = classpath, this->istat = 1;
	else if(this->istat == 1) this->istat = 2;
	return 0;
}
hashval_t org_apache_http_HttpClient_hashcode(const void* data)
{
	return 0x23fb7c5ul;
}
int org_apache_http_HttpClient_equal(const void* this, const void* that)
{
	return 1;
}
int org_apache_http_HttpClient_isinstance(const void* this_ptr, const dalvik_type_t* type)
{
	const this_t* this = (const this_t*)this_ptr;
	if(type->typecode != DALVIK_TYPECODE_OBJECT) return BCI_BOOLEAN_FALSE;
	if(type->data.object.path == org_apache_http_HttpClient_metadata.provides[0] ||
	   type->data.object.path == this->classpath) return BCI_BOOLEAN_TRUE;
	else return BCI_BOOLEAN_FALSE;
}
const char* org_apache_http_HttpClient_tostring(const void* this_ptr, char* buf, size_t sz)
{
	return "HttpClient";
}
int org_apache_http_HttpClient_get_method(const void* this, const char* classpath, const char* methodname, const dalvik_type_t* const * args, const dalvik_type_t* rtype)
{
	return -1;
}
int org_apache_http_HttpClient_invoke(int method_id, bci_method_env_t* env)
{
	return 0;
}
bci_class_t org_apache_http_HttpClient_metadata = {
	.size = sizeof(this_t),
	.load = org_apache_http_HttpClient_load,
	.initialize = org_apache_http_HttpClient_init,
	.hash = org_apache_http_HttpClient_hashcode,
	.equal = org_apache_http_HttpClient_equal,
	.is_instance = org_apache_http_HttpClient_isinstance,
	.to_string = org_apache_http_HttpClient_tostring,
	.get_method = org_apache_http_HttpClient_get_method,
	.invoke = org_apache_http_HttpClient_invoke,
	.super = "java/lang/Object",
	.provides = {
		"org/apache/http/HttpClient",
		"org/apache/http/impl/client/DefaultHttpClient",
		NULL
	}
};

