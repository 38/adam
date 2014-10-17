#include <bci/bci_interface.h>
typedef struct{
	uint8_t init_flag;
	tag_set_t* tags;
} this_t;
int unknown_onload(){return 0;}
int unknown_init(void* this_ptr, const char* classpath, const void* init_param, tag_set_t** p_tags){return 0;}
int unknown_delete(void* this_ptr){return 0;}
int unknown_dup(const void* this_ptr, void* that_ptr){return 0;}
int unknown_get_addr_list(const void* this_ptr, uint32_t offset, uint32_t* buf, size_t size){return 0;}
int unknown_modify(void* this_ptr, uint32_t offset, uint32_t* buf, size_t N){return 0;}
hashval_t unknown_hashcode(const void* data){return 0x7c5fb37ul;}
int unknown_equal(const void* this_ptr, const void* that_ptr){return 1;}
const char* unknown_to_string(const void* this_ptr, char* buf, size_t size){return "dummy class";}
int unknown_merge(void* this_ptr, const void* that_ptr){return 0;}
int unknown_get_reloc_flag(const void* this_ptr){return 0;}
int unknown_instance_of(const void* this_ptr, const dalvik_type_t* type){return BCI_BOOLEAN_UNKNOWN;}
int unknown_get_method(const void* this_ptr, const char* classpath, const char* method, const dalvik_type_t* const * signature, const dalvik_type_t* rettype)
{
	if(method == stringpool_query("appendFilenameExtension")) return 1;
	if(method == stringpool_query("openFileOutput")) return 2;
	return 0;
}
int unknown_invoke(int method_id, bci_method_env_t* env)
{
	if(method_id == 1)
	{
		return bci_interface_return_set(env, bci_interface_read_arg(env, 1,2));
	}
	if(method_id == 2)
	{
		const tag_set_t* tags = cesk_set_get_tags(bci_interface_read_arg(env, 1, 3));
		printf("hit function openFileOutput with tag %s\n", tag_set_to_string(tags, NULL, 0));

				uint32_t* inst[1000];
				void* stack[1000];
				int N = tag_tracker_get_path(0x2222, tag_set_id(tags), inst, 1000, stack);
				int i, j;
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
				}
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
		"<__any__>",
		NULL
	}
};
