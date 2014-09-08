#include <bci/bci_interface.h>
#include <dalvik/dalvik.h>
#include <cesk/cesk_set.h>
#include <dalvik/dalvik.h>
#define HASH_SIZE 1023
extern bci_class_t java_lang_reflect_Array_metadata; 
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
static const char *_new_array, *_new_array_filled, *_fill_array, *_array_get, *_array_put, *_classpath;
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
/**
 * @brief this function will be invoked when the built-in system get intialized 
 * @return < 0 for intialization failure
 **/
int java_lang_reflect_Array_onload()
{
	_new_array = stringpool_query("<new_array>");
	_new_array_filled = stringpool_query("<new_array_filled>");
	_fill_array = stringpool_query("<fill_array_data>");
	_array_get = stringpool_query("<array_get>");
	_array_put = stringpool_query("<array_put>");
	_classpath = stringpool_query("java/lang/reflect/Array");
	return 0;
}
/**
 * @brief this function will be invoked before the built-in system get finalized
 * @return < 0 for finalization failure
 **/
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
/**
 * @brief this function will be called after the new instance initialized (with the first byte in this_ptr initialized to 0) 
 *        or the existing instance is reused (first byte won't be initialized)
 * @param this_ptr the pointer to the internal data of this intance
 * @param init_param the initialize parameter
 * @param p_tags the pointer to the reference to the intial tag
 * @return < 0 if there's an error
 **/
int java_lang_reflect_Array_init(void* this_ptr, const void* init_param, tag_set_t** p_tags)
{
	array_data_t* this = (array_data_t*)this_ptr;
	if(this->init_cnt == 0) this->init_cnt = 1;
	else if(this->init_cnt == 1) 
	{
		this->init_cnt = 2;
		return 0;
	}
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
/**
 * @brief this function will be called before the deletion of the instance in the host memory 
 * @param this_ptr the this pointer to be deleted
 * @return < 0 if there's an error
 **/
int java_lang_reflect_Array_finalize(void* this_ptr)
{
	array_data_t* this = (array_data_t*)this_ptr;
	cesk_set_free(this->set);
	return 0;
}
/**
 * @brief duplicate an array object
 * @param this_ptr data of old object
 * @param that_ptr data of new object
 * @return < 0 indicates an error
 **/
int java_lang_reflect_Array_dup(const void* this_ptr, void* that_ptr)
{
	const array_data_t* this = (const array_data_t*)this_ptr;
	array_data_t* that = (array_data_t*) that_ptr;
	that->init_cnt = this->init_cnt;
	that->set = cesk_set_fork(this->set);
	that->next_offset = -1;
	that->element_type = this->element_type;
	return 0;
}

/**
 * @brief this function will be called when the analyzer wants to read all address using a file-like interface
 * @note but in this case we can always assume that the analyzer would first read a bounch of address
 *       and them try to write the same address. In fact this is a very useful property to make the offset stable
 * @param this_ptr the this pointer
 * @param offset like a file offset
 * @param buf the buffer that use to return the value
 * @param sz the size of the buffer
 * @return the number of address that has been read, < 0 indicates an error
 **/
int java_lang_reflect_Array_get_addr_list(const void* this_ptr, uint32_t offset, uint32_t* buf, size_t sz)
{
	array_data_t* this = (array_data_t*)this_ptr;
	if(offset != this->next_offset)
	{
		if(NULL == cesk_set_iter(this->set, &this->iter))
		{
			LOG_ERROR("failed to get a iterator for array content");
			return -1;
		}
		int i;
		for(i = 0; i < offset; i ++)
			cesk_set_iter_next(&this->iter);
		this->next_offset = 0;
	}
	this->prev_iter = this->iter;
	this->prev_offset = this->next_offset;
	uint32_t addr;
	int ret = 0;
	for(;sz > 0 && CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&this->iter)); sz--)
		buf[ret ++] = addr;
	this->next_offset += ret;
	return ret;
}
/**
 * @brief this function will be called when the analyzer wants to write address using a file-like interface
 * @note see the note of java_lang_reflect_Array_get_addr_list 
 * @param this_ptr the this pointer
 * @param offset file offset
 * @param new the array that carries the new value
 * @param N how many value are there in the new vlaue array
 * @return < 0 indicates an error 
 **/
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
/**
 * @brief this function will be called when the analyzer needs the hash code of this object
 * @note this hashcode is different from the hash code in java. This hash code is the hash code
 *       of the analyzer object, rather than a java object. To use hash code in java code,
 *       the analyzer will invoke a function call to the hashCode method
 * @param data the this pointer
 * @return the hash code of this instance object
 **/
hashval_t java_lang_reflect_Array_hashcode(const void* data)
{
	const array_data_t* this = (const array_data_t*)data;
	return cesk_set_hashcode(this->set);
}
/**
 * @brief this function is used to check wether or not two analyzer object are the same
 * @param this_ptr the this pointer
 * @param that_ptr the other object
 * @return 1 for equal, 0 for not equal, < 0 means errore
 **/
int java_lang_reflect_Array_equal(const void* this_ptr, const void* that_ptr)
{
	const array_data_t* this = (const array_data_t*)this_ptr;
	const array_data_t* that = (const array_data_t*)that_ptr;
	return cesk_set_equal(this->set, that->set);
}
/**
 * @biref this function is used to convert this built-class to a human-readable string
 * @param this_ptr the this pointer
 * @param buf the string buffer
 * @param size the size of string
 * @return the result string, NULL when error occurs
 **/
const char* java_lang_reflect_Array_to_string(const void* this_ptr, char* buf, size_t size)
{
	const array_data_t* this = (const array_data_t*)this_ptr;
	const char* ret = cesk_set_to_string(this->set, buf, size);
	buf += strlen(ret);
	dalvik_type_to_string(this->element_type, buf, size - strlen(ret));
	return ret;
}
/**
 * @brief this function is used to check wether or not this class contains a relocated address
 * @param this_ptr the this pointer
 * @return 1 if it contains, 0 if it do not contains
 **/
int java_lang_reflect_Array_get_reloc_flag(const void* this_ptr)
{
	const array_data_t* this = (const array_data_t*) this_ptr;
	return cesk_set_get_reloc(this->set);
}
/**
 * @brief this function is used to merge two instance
 * @param this_ptr the pointer to the destination object
 * @param that_ptr the pointer to the source object
 * @return result of merge, < 0 if failed to merge
 **/
int java_lang_reflect_Array_merge(void* this_ptr, const void* that_ptr)
{
	const array_data_t* that = (const array_data_t*) that_ptr;
	array_data_t* this = (array_data_t*) this_ptr;
	int rc = cesk_set_merge(this->set, that->set);
	if(rc < 0) return rc;
	this->next_offset = -1;
	return 0;
}
/**
 * @brief check wether or not the type of the array is a sub-type of the operand_type 
 * @param element_type the type of each element
 * @param operand_type the type of the type you want to check
 * @return BCI_BOOLEAN_FALSE if not a sub-type/ BCI_BOOLEAN_TRUE if it is
 **/
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
/**
 * @brief called when analyzer wants to know wether or not this object is a instance of given type
 * @param this_ptr the this pointer
 * @param type the type descriptor
 * @return result, use BCI_BOOLEAN
 **/
int java_lang_reflect_Array_instance_of(const void* this_ptr, const dalvik_type_t* type)
{
	const array_data_t* this = (const array_data_t*) this_ptr;
	if(type->typecode != DALVIK_TYPECODE_ARRAY) return BCI_BOOLEAN_FALSE;
	return _java_lang_reflect_Array_check_type(this->element_type, type->data.array.elem_type); 
}
/**
 * @brief called when analyzer is preparing for a function invocation, return a method id if this object supports this function call
 * @param this_ptr the this pointer
 * @param method the method name
 * @param typelist the function signature
 * @param rtype the return type
 * @return the method id, < 0 indicates error/method not found
 **/
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
/**
 * @brief handler for the ufction call <new_array> which creates a new array object
 * @param env the invocation environ
 * @param elem_type the type of each element
 * @return < 0 indicates errors
 **/
static inline int _new_array_handler(bci_method_env_t* env, const dalvik_type_t* elem_type)
{
	uint32_t addr = bci_interface_new_object(env, _classpath, elem_type);
	return bci_interface_return_single_address(env, addr);
}
/**
 * @brief handler for the function call <new_array_filled> which creates a new array object with data filled in the array
 * @param env the invocation environ
 * @param elem_type the type of each element
 * @param nargs the number of arguments
 * @return < 0 indicates errors
 **/
static inline int _new_array_filled_handler(bci_method_env_t* env, const dalvik_type_t* elem_type, uint32_t nargs)
{
	uint32_t addr = bci_interface_new_object(env, _classpath, elem_type);
	uint32_t i;
	array_data_t* this = (array_data_t*)bci_interface_get_rw(env, addr, java_lang_reflect_Array_metadata.provides[0]);
	if(NULL == this)
	{
		LOG_ERROR("can not write new array instance at "PRSAddr, addr);
	}
	for(i = 0; i < nargs; i ++)
	{
		const cesk_set_t* arg = bci_interface_read_arg(env, i, nargs);
		cesk_set_merge(this->set, arg);
	}
	bci_interface_release_rw(env, addr);
	bci_interface_return_object(env, addr);
	return bci_interface_return_single_address(env, addr);
}
/**
 * @biref handler for <array_put>
 * @param env the environ
 * @return the result of invocation
 **/
static inline int _array_put_handler(bci_method_env_t* env)
{
	const cesk_set_t* value = bci_interface_read_arg(env, 0, 3);
	const cesk_set_t* self = bci_interface_read_arg(env, 1, 3);
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(self, &iter))
	{
		LOG_ERROR("can not acquire a iterator to enumerate this pointer");
		return -1;
	}
	uint32_t addr;
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		array_data_t* this = (array_data_t*)bci_interface_get_rw(env, addr, java_lang_reflect_Array_metadata.provides[0]);
		if(NULL == this)
		{
			LOG_ERROR("can not write the object at "PRSAddr, addr);
			return -1;
		}
		cesk_set_merge(this->set, value);
		bci_interface_release_rw(env, addr);
		/* no need to fix the refcnt ? */
		bci_interface_return_object(env, addr);
	}
	return 0;
}
/**
 * @brief handler for <array_get>
 * @param env the envrion
 * @return the result of invocation
 **/
static inline int _array_get_handler(bci_method_env_t* env)
{
	const cesk_set_t* array = bci_interface_read_arg(env, 0, 2);
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(array, &iter))
	{
		LOG_ERROR("can not acquire an iterator to enumerate the possible array object");
		return -1;
	}
	uint32_t addr;
	cesk_set_t* ret = cesk_set_empty_set();
	if(NULL == ret)
	{
		LOG_ERROR("can not create the result set");
		return -1;
	}
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		const array_data_t* this = (const array_data_t*)bci_interface_get_ro(env, addr, java_lang_reflect_Array_metadata.provides[0]);
		if(cesk_set_merge(ret, this->set) < 0)
		{
			LOG_ERROR("can not merge value");
			return -1;
		}
	}
	int rc = bci_interface_return_set(env, ret);
	cesk_set_free(ret);
	return rc;
}
/**
 * @brief handler for <fill_array_data>
 * @brief env the envrion
 * @param label_type the type that carries a label
 * @return the result of invocation
 **/
static inline int _array_fill_handler(bci_method_env_t* env, const dalvik_type_t* label_type)
{
	const cesk_set_t* self = bci_interface_read_arg(env, 0, 1);
	const char* label_name = stringpool_query(label_type->data.object.path + 13);
	uint32_t lid = dalvik_label_get_label_id(label_name);
	uint32_t idx = dalvik_label_jump_table[lid];
	const dalvik_instruction_t* data = dalvik_instruction_get(idx);
	if(NULL == data)
	{
		LOG_ERROR("can not fetch the data instrution");
		return -1;
	}
	if(DVM_ARRAY != data->opcode || DVM_FLAG_ARRAY_DATA != data->flags)
	{
		LOG_ERROR("invalid data instruction at %x", idx);
		return -1;
	}
	const vector_t* data_vec = data->operands[0].payload.data;
	uint32_t value_addr = CESK_STORE_ADDR_EMPTY;
	size_t size = vector_size(data_vec);
	int i;
	for(i = 0; i < size; i ++)
	{
		int32_t *val;
		if(NULL == (val = vector_get(data_vec, i)))
		{
			LOG_ERROR("can not read the data vector");
			return -1;
		}
		if(*val < 0) value_addr |= CESK_STORE_ADDR_NEG;
		else if(*val == 0) value_addr |= CESK_STORE_ADDR_ZERO;
		else if(*val > 0) value_addr |= CESK_STORE_ADDR_POS;
	}
	
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(self, &iter))
	{
		LOG_ERROR("can not acquire a iterator to enumerate the target array objects");
		return -1;
	}
	uint32_t this_addr;
	while(CESK_STORE_ADDR_NULL != (this_addr = cesk_set_iter_next(&iter)))
	{
		array_data_t* this = (array_data_t*)bci_interface_get_rw(env, this_addr, java_lang_reflect_Array_metadata.provides[0]);
		if(NULL == this)
		{
			LOG_WARNING("can not get this pointer at address "PRSAddr, this_addr);
			continue;
		}
		cesk_set_push(this->set, value_addr);
		bci_interface_release_rw(env, this_addr);
		bci_interface_return_object(env, this_addr);
	}
	return 0;
}
/**
 * @brief this function will be called if the analyzer needs to invoke a member function in this class
 * @param method_id the method id
 * @param env the environ
 * @return the result
 **/
int java_lang_reflect_Array_invoke(int method_id, bci_method_env_t* env)
{
	if(method_id >= _method_count)
	{
		LOG_ERROR("no such method");
		return -1;
	}
	const _method_t* method = _method_list[method_id];
	uint32_t nargs = 0;
	switch(method->type)
	{
		case NEW_ARRAY:
			return _new_array_handler(env, method->signature[1]);
		case NEW_ARRAY_FILLED:
			for(;method->signature[nargs ++];);
			return _new_array_filled_handler(env, method->signature[0], nargs);
		case ARRAY_SET:
			return _array_put_handler(env);
		case ARRAY_GET:
			return _array_get_handler(env);
		case FILL_ARRAY:
			return _array_fill_handler(env, method->signature[0]);
		//TODO: newInstance, because it depends on java.lang.Class and refelction
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
	.duplicate = java_lang_reflect_Array_dup,
	.finalization = java_lang_reflect_Array_finalize,
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
