#include <bci/bci_interface.h>
#include <dalvik/dalvik.h>
#include <cesk/cesk_set.h>
#include <dalvik/dalvik.h>
#define HASH_SIZE 1023
/**
 * @brief the internal data of an array type
 **/
typedef struct {
	uint8_t init_cnt;     /*!< this is actually a built-in function creation convention, in which only the first byte of the interal space will be
							initialized to zero, based on this, we can distinguish a newly created instance and a reused instance */
	cesk_set_t* set;      /*!< the possible value of the set, in the future, we could replace this with more precise approximation */
	cesk_set_iter_t iter;  /*!< the current iterator for the stream interface (actually we can assume that each stream read and stream write are paired and the address for read has to be continous*/
	cesk_set_iter_t prev_iter; /*!< the previous status of the current iterator, this field is used to check wether or not the read and write are paired */
	int32_t next_offset;   /*!< the next offset in the stream */
	int32_t prev_offset;  /*!< the previous offset in the stream */
	const dalvik_type_t* element_type;  /*!< the type of the element. The built-in class system is designed to handle the reflection, this class is a example for `runtime generic class' */
} array_data_t;
/**
 * @brief the method list
 */
typedef struct _method_t {
	enum {
		NEW_ARRAY,
		NEW_ARRAY_FILLED,
		FILL_ARRAY,
		ARRAY_GET,
		ARRAY_SET
	} type;   /*!< type of this method call */
	const dalvik_type_t *const* signature; /*!< the function signature */
	uint32_t method_id;   /*!< the method id assigned to this method */
	struct _method_t* next;  /*!< the next pointer in the hash table */
} _method_t;
/**
 * @brief how many method in the method table
 **/
static uint32_t _method_count;
/**
 * @brief the method list, _method_list[method_id] ==> pointer to method structure
 **/
static _method_t* _method_list[HASH_SIZE];
/**
 * @brief the method hash table, used to check wether or not this function are previously defined 
 **/
static _method_t* _method_hash[HASH_SIZE];
/**
 * @brief pooled strings for the function name
 **/
static const char *_new_array, *_new_array_filled, *_fill_array, *_array_get, *_array_put;
/**
 * @brief the hashcode of a method 
 * @param typecode the type of this method
 * @param signature the method signature of this function
 * @return the result hashcode
 **/
static inline hashval_t _method_hashcode(uint32_t typecode, const dalvik_type_t* const * signature)
{
	hashval_t sighash = dalvik_type_list_hashcode(signature);
	return sighash ^ (typecode * typecode * MH_MULTIPLY + typecode);
}
/**
 * @brief query a method by typecode(which can be NEW_ARRAY, NEW_ARRAY_FILLED, ...) and the function signature
 * @param typecode the type code of this function
 * @param signature the function signature
 * @return the pointer to the method found in the table, if the method not found, create a method structure for this method and intert to the hash table
 *         NULL when can not create a new node
 **/
static inline const _method_t* _method_query(uint32_t typecode, const dalvik_type_t *const* signature)
{
	uint32_t slot_id =  _method_hashcode(typecode, signature) % HASH_SIZE;
	_method_t* ptr;
	/* find it in the hash table first */
	for(ptr = _method_hash[slot_id]; NULL != ptr; ptr = ptr->next)
	{
		if(ptr->type == typecode && dalvik_type_list_equal(ptr->signature, signature))
			return ptr;
	}
	/* otherwise inster a new one */
	ptr = (_method_t*)malloc(sizeof(_method_t));
	if(NULL == ptr) return NULL;
	ptr->type = typecode;
	ptr->signature = signature;
	ptr->method_id = _method_count ++;
	ptr->next = _method_hash[slot_id];
	_method_hash[slot_id] = ptr;
	_method_list[ptr->method_id] = ptr;
	return ptr;
}

int java_lang_reflect_Array_onload()
{
	_new_array = stringpool_query("<new_array>");
	_new_array_filled = stringpool_query("<new_array_filled>");
	_fill_array = stringpool_query("<fill_array>");
	_array_get = stringpool_query("<array_get>");
	_array_put = stringpool_query("<array_put>");
	return 0;
}
int java_lang_reflect_Array_ondelete()
{
	int i;
	for(i = 0; i < HASH_SIZE; i ++)
	{
		_method_t* ptr;
		for(ptr = _method_hash[i]; NULL != ptr;)
		{
			_method_t* cur = ptr;
			ptr = ptr->next;
			free(cur);
		}
	}
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
	if(operand_type->typecode != element_type->typecode) 
		return BCI_BOOLEAN_FALSE;
	if(operand_type->typecode != DALVIK_TYPECODE_OBJECT) 
		return BCI_BOOLEAN_FALSE;
	if(operand_type->typecode == DALVIK_TYPECODE_ARRAY) 
		return _java_lang_reflect_Array_check_type(element_type->data.array.elem_type, operand_type->data.array.elem_type);
	/* otherwise we should check if element_type can be converted to the operand_type */
	const dalvik_class_t* class = NULL;  /* why we need to intialize? because GCC is soooooo stupid, clang is much better */
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
int java_lang_reflect_Array_get_method(const void* this_ptr, const char* method, const dalvik_type_t* const * typelist, const dalvik_type_t* rtype)
{
	/* we have to handle the functions to emulate the array instructions: <new_array>, <new_array_filled>, <fill_array>, <array_get>, <array_set> */
	uint32_t typecode = 0;
	if(_new_array == method)
		typecode = NEW_ARRAY;
	else if(_new_array_filled == method)
		typecode = NEW_ARRAY_FILLED;
	else if(_fill_array == method)
		typecode = FILL_ARRAY;
	else if(_array_get == method)
		typecode = ARRAY_GET;
	else if(_array_put == method)
		typecode = ARRAY_SET;
	else
	{
		/* TODO other functions */
		return -1;
	}
	const _method_t* item = _method_query(typecode, typelist);
	if(NULL == item)
	{
		LOG_ERROR("can't resolve the method %s(%s)", method, dalvik_type_list_to_string(typelist, NULL, 0));
		return -1;
	}
	return item->method_id;
}
static inline int _new_array_handler(bci_method_env_t* env, const dalvik_type_t* elem_type)
{
	LOG_ERROR("%s", dalvik_type_to_string(elem_type, NULL, 0));
	//TODO create a new array
	return 0;
}
int java_lang_reflect_Array_invoke(int method_id, bci_method_env_t* env)
{
	if(method_id >= _method_count)
	{
		LOG_ERROR("no such method");
		return -1;
	}
	const _method_t* method = _method_list[method_id];
	switch(method->type)
	{
		case NEW_ARRAY:
			return _new_array_handler(env, method->signature[1]);
		default:
			LOG_ERROR("unsupported method");
			return -1;
	}
	return -1;
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
