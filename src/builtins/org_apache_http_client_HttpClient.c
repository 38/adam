#include <bci/bci_interface.h>
#include <stringpool.h>
extern bci_class_t org_apache_http_HttpClient_metadata;
typedef struct {
	uint8_t istat;
	const char* classpath;
} this_t;
const static char* kw_init = NULL;
const static char* kw_execute = NULL;
int org_apache_http_HttpClient_load()
{
	kw_init = stringpool_query("<init>");
	kw_execute = stringpool_query("execute");
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
	return "";
}
int org_apache_http_HttpClient_get_method(const void* this, const char* classpath, const char* methodname, const dalvik_type_t* const * args, const dalvik_type_t* rtype)
{
	if(kw_init == methodname) return 0;
	else if(kw_execute == methodname) return 1;
	return -1;
}
cesk_set_t* addressContainer_dereference(uint32_t addr, const uint32_t* index, bci_method_env_t* env);
static inline int _execute_handler(bci_method_env_t* env)
{
	const cesk_set_t* url_set = bci_interface_read_arg(env, 1, 3);
	if(NULL == url_set) return -1;
	cesk_set_iter_t iter;
	uint32_t addr;
	if(NULL == cesk_set_iter(url_set, &iter))
	{
		return -1;
	}
	uint32_t idx[] = {0, 0xfffffffful};
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		cesk_set_t* result = addressContainer_dereference(addr, idx, env);
		uint32_t saddr;
		cesk_set_iter_t siter;
		if(NULL == cesk_set_iter(result, &siter))
		{
			return -1;
		}
		while(CESK_STORE_ADDR_NULL != (saddr = cesk_set_iter_next(&siter)))
		{
			const tag_set_t* tags = bci_interface_read_tag(env, saddr);
			if(tag_set_contains(tags, TAG_FILECONTENT))
			{
				uint32_t* inst[1000];
				void* stack[1000];
				int N = tag_tracker_get_path(TAG_FILECONTENT, tag_set_id(tags), inst, 1000, stack);
				int i, j;
				printf("Try to send user's file content to internet\n");
				printf("%s/%s@%s:%d\n\t%s\n", env->instruction->method->path, env->instruction->method->name,
						env->instruction->method->file, env->instruction->line, dalvik_instruction_to_string(env->instruction, NULL, 0));
				for(i = 0; i < N; i ++)
				{
					puts("=======path========");
					for(j = 0; inst[i][j] != 0xfffffffful; j ++)
					{
						const dalvik_instruction_t* ist = dalvik_instruction_get(inst[i][j]);
						if(j == 0 || inst[i][j-1] != inst[i][j]) printf("%s/%s@%s:%d\n\t%s\n", 
								ist->method->path, ist->method->name,
								ist->method->file, ist->line, dalvik_instruction_to_string(ist, NULL, 0));
					}
					puts("-------stack---------");
					for(;stack[i];)
					{
						uint32_t ip = tag_tracker_stack_backtrace(stack + i);
						const dalvik_instruction_t* ins = dalvik_instruction_get(ip);
						if(DALVIK_INSTRUCTION_INVALID != ip) 
							printf("%s/%s@%s:%d\n\t%s\n", 
									ins->method->path, ins->method->name,
									ins->method->file, ins->line, dalvik_instruction_to_string(ins, NULL, 0)); 
					}
					free(inst[i]);
				}
			}
		}
		cesk_set_free(result);
	}
	return 0;
}
int org_apache_http_HttpClient_invoke(int method_id, bci_method_env_t* env)
{
	if(0 == method_id) return 0;
	else if(1 == method_id) return _execute_handler(env);
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
		"org/apache/http/client/HttpClient",
		"org/apache/http/impl/client/DefaultHttpClient",
		NULL
	}
};

