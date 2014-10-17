#include <bci/bci_interface.h>
typedef struct{
	uint8_t init_flag;
	const char* classpath;
} this_t;
const char* report;
const char* init;
const char* _tag;
int unknown_onload()
{
	report = stringpool_query("__Report__");
	init = stringpool_query("__init__");
	_tag = stringpool_query("__Tag__");
	return 0;
}
int unknown_init(void* this_ptr, const char* classpath, const void* init_param, tag_set_t** p_tags)
{
	((this_t*)this_ptr)->classpath = classpath;
	return 0;
}
int unknown_delete(void* this_ptr)
{
	return 0;
}
int unknown_dup(const void* this_ptr, void* that_ptr)
{
	((this_t*)that_ptr)->classpath = ((this_t*)this_ptr)->classpath;
	return 0;
}
int unknown_get_addr_list(const void* this_ptr, uint32_t offset, uint32_t* buf, size_t size)
{
	return 0;
}
int unknown_modify(void* this_ptr, uint32_t offset, uint32_t* buf, size_t N)
{
	return 0;
}
hashval_t unknown_hashcode(const void* data)
{
	return 0x7c5fb37ul;
}
int unknown_equal(const void* this_ptr, const void* that_ptr)
{
	return 1;
}
const char* unknown_to_string(const void* this_ptr, char* buf, size_t size)
{
	return ((this_t*)this_ptr)->classpath;
}
int unknown_merge(void* this_ptr, const void* that_ptr)
{
	return 0;
}
int unknown_get_reloc_flag(const void* this_ptr)
{
	return 0;
}
int unknown_instance_of(const void* this_ptr, const dalvik_type_t* type)
{
	return BCI_BOOLEAN_UNKNOWN;
}
int unknown_get_method(const void* this_ptr, const char* classpath, const char* method, const dalvik_type_t* const * signature, const dalvik_type_t* rettype)
{
	if(strcmp(method, "getData") == 0) return 1;
	if(classpath == report) 
		return 0xff00 + atoi(method + 7) + 1;
	return 0;
}
int unknown_invoke(int method_id, bci_method_env_t* env)
{
	if(method_id > 0xff00)
	{
		method_id -= 0xff00;
		int i;
		puts("========================================================");
		for(i = 0; i < method_id; i ++)
		{
			printf("arg #%d\n", i);
			const cesk_set_t* set = bci_interface_read_arg(env, i, method_id);
			cesk_set_iter_t iter;
			cesk_set_iter(set, &iter);
			uint32_t addr;
			while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
			{
				cesk_value_const_t* value = cesk_store_get_ro(env->frame->store, addr);
				if(NULL == value) continue;
				printf("\t%s\n", cesk_value_to_string((const cesk_value_t*)value, NULL, 0));
			}
		}
		puts("---------------------stack------------------------------");

		const void* p;
		for(p = cesk_method_context_get_caller_context(env->context); NULL != p; p = cesk_method_context_get_caller_context(p))
		{
			const dalvik_block_t* block = cesk_method_context_get_current_block(p);
			printf("\t%s.%s@%s:%d\n", block->info->class, block->info->method, dalvik_instruction_get(block->begin)->method->file, dalvik_instruction_get(block->begin)->line);
		}

		puts("========================================================");
		return 0;
	}
	return 0;
}

bci_class_t unknown_metadata = {
	.size = sizeof(this_t),
	.load = unknown_onload,
	.initialize = unknown_init,
	.finalize = unknown_delete,
	.duplicate = unknown_dup,
	.read = unknown_get_addr_list,
	.write = unknown_modify,
	.hash = unknown_hashcode,
	.equal = unknown_equal,
	.to_string = unknown_to_string,
	.merge = unknown_merge,
	.has_reloc_ref = unknown_get_reloc_flag,
	.is_instance = unknown_instance_of,
	.get_method = unknown_get_method,
	.invoke = unknown_invoke,
	.super = "java/lang/Object",
	.provides = {
		"android/widget/Toast",
		"android/app/Activity",
		"java/util/BitSet",
		"__Report__",
		"<__dummy__>",
		NULL
	}
};
