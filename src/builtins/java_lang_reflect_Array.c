#include <bci/bci_interface.h>
#include <dalvik/dalvik.h>
#include <cesk/cesk_set.h>
#include <dalvik/dalvik.h>
typedef struct {
	uint8_t init_cnt;
	cesk_set_t* set;
	cesk_set_iter_t iter;
	cesk_set_iter_t prev_iter;
	int32_t next_offset;
	int32_t prev_offset;
	const dalvik_type_t* element_type;
} array_data_t;
int java_lang_reflect_Array_onload()
{
	return 0;
}
int java_lang_reflect_Array_ondelete()
{
	return 0;
}
int java_lang_reflect_Array_init(void* this_ptr, const void* init_param, tag_set_t** p_tags)
{
	array_data_t* this = (array_data_t*)this_ptr;
	if(this->init_cnt == 0) this->init_cnt = 1;
	else if(this->init_cnt == 1) this->init_cnt = 2;
	this->set = cesk_set_empty_set();
	this->next_offset = -1;
	if(NULL == this->set) 
	{
		LOG_ERROR("can not initialize a empty array");
		return -1;
	}
	this->element_type = (dalvik_type_t*) init_param;
	return 0;
}
int java_lang_reflect_Array_finialize(void* this_ptr)
{
	array_data_t* this = (array_data_t*)this_ptr;
	cesk_set_free(this->set);
	return 0;
}
int java_lang_reflect_Array_get_addr_list(const void* this_ptr, uint32_t offset, uint32_t* buf, size_t sz)
{
	array_data_t* this = (array_data_t*)this_ptr;
	if(-1 == this->next_offset)
	{
		if(NULL == cesk_set_iter(this->set, &this->iter))
		{
			LOG_ERROR("failed to get a iterator for array content");
			return -1;
		}
		int i;
		for(i = 0; i < offset; i ++)
			cesk_set_iter_next(&this->iter);
	}
	this->prev_iter = this->iter;
	this->prev_offset = this->next_offset;
	uint32_t addr;
	int ret = 0;
	for(;sz > 0 && CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&this->iter)); sz--)
		buf[ret ++] = addr;
	return ret;
}
int java_lang_reflect_Array_modify(void* this_ptr, uint32_t offset, uint32_t* new, size_t N)
{
	array_data_t* this = (array_data_t*) this_ptr;
	if(this->prev_offset != offset)
	{
		LOG_ERROR("you should first read the data chunck then write it back");
		return -1;
	}
	size_t i;
	for(i = 0; i < N; i ++)
	{
		uint32_t old_val = cesk_set_iter_next(&this->prev_iter);
		if(CESK_STORE_ADDR_NULL == old_val) 
		{
			LOG_ERROR("can not modify a value that not exists");
			return -1;
		}
		if(cesk_set_modify(this->set, old_val, new[i]) < 0)
		{
			LOG_ERROR("can not update the value");
			return -1;
		}
	}
	return 0;
}
hashval_t java_lang_reflect_Array_hashcode(const void* data)
{
	return cesk_set_hashcode((const cesk_set_t*)data);
}
int java_lang_reflect_Array_equal(const void* this_ptr, const void* that_ptr)
{
	const array_data_t* this = (const array_data_t*)this_ptr;
	const array_data_t* that = (const array_data_t*)that_ptr;
	return cesk_set_equal(this->set, that->set);
}
const char* java_lang_reflect_Array_to_string(const void* this_ptr, char* buf, size_t size)
{
	const array_data_t* this = (const array_data_t*)this_ptr;
	return cesk_set_to_string(this->set, buf, size);
}
int java_lang_reflect_Array_get_reloc_flag(const void* this_ptr)
{
	const array_data_t* this = (const array_data_t*) this_ptr;
	return this->init_cnt == 2;
}
int java_lang_reflect_Array_merge(void* this_ptr, const void* that_ptr)
{
	const array_data_t* that = (const array_data_t*) that_ptr;
	array_data_t* this = (array_data_t*) this_ptr;
	int rc = cesk_set_merge(this->set, that->set);
	if(rc < 0) return rc;
	this->next_offset = -1;
	return 0;
}
static inline int _java_lang_reflect_Array_check_type(const dalvik_type_t* element_type, const dalvik_type_t* operand_type)
{
	if(operand_type->typecode != element_type->typecode) return BCI_BOOLEAN_FALSE;
	if(operand_type->typecode != DALVIK_TYPECODE_OBJECT) return BCI_BOOLEAN_FALSE;
	if(operand_type->typecode == DALVIK_TYPECODE_ARRAY) return _java_lang_reflect_Array_check_type(element_type->data.array.elem_type, operand_type->data.array.elem_type);
	/* otherwise we should check if element_type can be converted to the operand_type */
	const dalvik_class_t* class;
	const char* element_classpath = element_type->data.object.path;
	const char* operand_classpath = operand_type->data.object.path;
	for(;element_classpath != operand_classpath && NULL != (class = dalvik_memberdict_get_class(element_classpath));)
	{
		int i;
		for(i = 0; class->implements[i]; i ++)
		{
			if(class->implements[i] == operand_classpath) return BCI_BOOLEAN_TRUE;
		}
		element_classpath = class->super;
	}
	if(NULL == class) return BCI_BOOLEAN_FALSE;
	return BCI_BOOLEAN_TRUE;
}
int java_lang_reflect_Array_instance_of(const void* this_ptr, const dalvik_type_t* type)
{
	const array_data_t* this = (const array_data_t*) this_ptr;
	if(type->typecode != DALVIK_TYPECODE_ARRAY) return BCI_BOOLEAN_FALSE;
	return _java_lang_reflect_Array_check_type(this->element_type, type->data.array.elem_type); 
}
//static const dalvik_type_t* _signature[1024];
int java_lang_reflect_Array_get_method(const void* this_ptr, const char* method, const dalvik_type_t* const * typelist, const dalvik_type_t* rtype)
{
	//TODO resolve method signatures
	/* we have to handle the functions to emulate the array instructions: <new_array>, <new_array_filled>, <fill_array>, <array_get>, <array_set> */

	return 0;
}
int java_lang_reflect_Array_invoke(int method_id, bci_method_env_t* env)
{
	return 0;
}
bci_class_t java_lang_reflect_Array_metadata = {
	.size = sizeof(array_data_t),
	.onload = java_lang_reflect_Array_onload,
	.ondelete = java_lang_reflect_Array_ondelete,
	.initialization = java_lang_reflect_Array_init,
	.get_addr_list = java_lang_reflect_Array_get_addr_list,
	.hash = java_lang_reflect_Array_hashcode,
	.equal = java_lang_reflect_Array_equal,
	.to_string = java_lang_reflect_Array_to_string,
	.get_relocation_flag = java_lang_reflect_Array_get_reloc_flag,
	.merge = java_lang_reflect_Array_merge,
	.instance_of = java_lang_reflect_Array_instance_of,
	.modify = java_lang_reflect_Array_modify,
	.get_method = java_lang_reflect_Array_get_method,
	.invoke = java_lang_reflect_Array_invoke,
	.super = "java/lang/Object",
	.provides = {
		"java/lang/reflect/Array",
		NULL
	}
};
