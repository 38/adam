#include <bci/bci_interface.h>
#include <stringpool.h>
#include <tag/tag.h>
typedef struct{
	uint8_t init_state;
	cesk_set_t* isr;
	uint32_t current_offset;
	uint32_t previous_offset;
	cesk_set_iter_t current_iter;
	cesk_set_iter_t previous_iter;
} this_t;
static const char* kw_init;
static const char* kw_readLine;
static const char* kw_read;
static cesk_set_t* read_result_set;
int java_io_BufferedReader_onload()
{
	kw_init = stringpool_query("<init>");
	kw_readLine = stringpool_query("readLine");
	kw_read = stringpool_query("read");
	read_result_set = cesk_set_empty_set();
	if(NULL == read_result_set)
	{
		LOG_ERROR("can not allocate new set for the BufferedReader Object");
		return -1;
	}
	cesk_set_push(read_result_set, CESK_STORE_ADDR_ZERO | CESK_STORE_ADDR_NEG | CESK_STORE_ADDR_POS);
	uint32_t tid[] = {TAG_FILECONTENT};
	uint32_t res[] = {TAG_RES_EXACT};
	tag_set_t* tags = tag_set_from_array(tid, res, 1);
	if(NULL == tags)
	{
		LOG_ERROR("can not allocate tag set");
		return -1;
	}
	if(cesk_set_assign_tags(read_result_set, tags) < 0)
	{
		LOG_ERROR("can not assign the tag to the set");
		return -1;
	}
	return 0;
}
int java_io_BufferedReader_unload()
{
	//if(NULL != read_result_set) cesk_set_free(read_result_set);
	/* the reason why we do not perform set free is the this function is called in finalize function */
	if(NULL != read_result_set) free(read_result_set); 
	return 0;
}
int java_io_BufferedReader_init(void* this_ptr, const char* class, const void* param, tag_set_t** p_tags)
{
	this_t* this = (this_t*)this_ptr;
	if(this->init_state == 0)
		this->init_state = 1;
	else if(this->init_state == 1)
		this->init_state = 2;
	this->current_offset = 0xfffffffful;
	if(this->init_state == 1)
	{
		this->isr = cesk_set_empty_set();
		if(NULL == this->isr) return -1;
	}
	return 0;
}
int java_io_BufferedReader_finalize(void* this_ptr)
{
	this_t* this = (this_t*)this_ptr;
	if(0 == this->init_state) return 0;
	cesk_set_free(this->isr);
	return 0;
}
int java_io_BufferedReader_duplicate(const void* sour_ptr, void* dest_ptr)
{
	this_t* dest = (this_t*)dest_ptr;
	const this_t* sour = (const this_t*)sour_ptr;
	dest->init_state = sour->init_state;
	dest->isr = cesk_set_fork(sour->isr);
	dest->current_offset = 0xffffffff;
	return 0;
}
int java_io_BufferedReader_get_addr_list(const void* this_ptr, uint32_t offset, uint32_t* buf, size_t size)
{
	this_t* this = (this_t*) this_ptr;
	if(offset != this->current_offset)
	{
		if(NULL == cesk_set_iter(this->isr, &this->current_iter))
		{
			LOG_ERROR("can not read the address set of the Stream Readers");
			return -1;
		}
		uint32_t i;
		for(i = 0; i < offset; i ++)
			cesk_set_iter_next(&this->current_iter);
		this->current_offset = offset;
	}
	this->previous_offset = this->current_offset;
	this->previous_iter = this->current_iter;
	uint32_t addr;
	int ret = 0;
	for(;size > 0 && CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&this->current_iter));)
		buf[ret ++] = addr;
	this->current_offset += ret;
	return ret;
}
int java_io_BufferedReader_modify(void* this_ptr, uint32_t offset, uint32_t* buf, size_t count)
{
	this_t* this = (this_t*) this_ptr;
	if(offset != this->previous_offset)
	{
		LOG_ERROR("write function should not be called before the read function");
		return -1;
	}
	int i;
	for(i = 0; i < count && i < this->current_offset; i ++)
	{
		uint32_t before = cesk_set_iter_next(&this->previous_iter);
		if(CESK_STORE_ADDR_NULL == before) 
		{
			LOG_ERROR("invalid value offset");
			return -1;
		}
		uint32_t after = buf[i];
		if(cesk_set_modify(this->isr, before, after) < 0)
		{
			LOG_ERROR("can not change the ISR address");
			return -1;
		}
	}
	this->previous_offset += count;
	return 0;
}
hashval_t java_io_BufferedReader_hashcode(const void* this_ptr)
{
	this_t* this = (this_t*) this_ptr;
	if(0 == this->init_state) 
		return 0;
	else
		return cesk_set_hashcode(this->isr);
	return 0;
}
int java_io_BufferedReader_equal(const void* this_ptr, const void* that_ptr)
{
	const this_t* this = (const this_t*)this_ptr;
	const this_t* that = (const this_t*)that_ptr;
	if(0 == this->init_state || 0 == that->init_state)
	{
		LOG_ERROR("invalid comparasion on uninitialized objects");
		return -1;
	}
	return cesk_set_equal(this->isr, that->isr);
}
const char* java_io_BufferedReader_to_string(const void* this_ptr, char* buf, size_t sz)
{
	const this_t* this = (const this_t*)this_ptr;
	snprintf(buf, sz, "BufferedReader%s", cesk_set_to_string(this->isr, NULL, 0));
	return buf;
}
int java_io_BufferedReader_get_rf(const void* this_ptr)
{
	const this_t* this = (const this_t*)this_ptr;
	if(0 == this->init_state)
	{
		LOG_ERROR("can not read the relocation flag of an unintialized object");
		return -1;
	}
	return cesk_set_get_reloc(this->isr);
}
int java_io_BufferedReader_merge(void* dest, const void* sour)
{
	const this_t* that = (const this_t*)sour;
	this_t* this = (this_t*)dest;
	if(this->init_state == 0 || that->init_state == 0)
	{
		LOG_ERROR("can not merge unintialized objects");
		return 0;
	}
	return cesk_set_merge(this->isr, that->isr);
}
int java_io_BufferedReader_instance_of(const void* this_ptr, const dalvik_type_t* type)
{
	extern bci_class_t java_io_BufferedReader_metadata;
	if(DALVIK_TYPECODE_OBJECT == type->typecode)
	{
		if(type->data.object.path == java_io_BufferedReader_metadata.provides[0])
			return BCI_BOOLEAN_TRUE;
	}
	return BCI_BOOLEAN_FALSE;
}
int java_io_BufferedReader_get_method(const void* this, const char* class, const char* method, const dalvik_type_t * const * signature, const dalvik_type_t* rtype)
{
	int nargs;
	for(nargs = 0; signature[nargs] != NULL; nargs ++);
	if(method == kw_init) return nargs;
	else if(method == kw_readLine) return 0xffff;
	else if(method == kw_read) return 0xfffe;
	return -1;
}
static inline int _java_io_BufferedReader_init(uint32_t id, bci_method_env_t* env)
{
	extern bci_class_t java_io_BufferedReader_metadata;
	const cesk_set_t* self_set = bci_interface_read_arg(env, 0, id + 1);
	const cesk_set_t* reader_set = bci_interface_read_arg(env, 1, id + 1);
	if(NULL == self_set || NULL == reader_set)
	{
		LOG_ERROR("can not read method args");
		return -1;
	}
	cesk_set_iter_t self_iter;
	if(NULL == cesk_set_iter(self_set, &self_iter))
	{
		LOG_ERROR("can not read the this pointer set");
		return -1;
	}
	uint32_t this_addr;
	while(CESK_STORE_ADDR_NULL != (this_addr = cesk_set_iter_next(&self_iter)))
	{
		this_t* this = (this_t*)bci_interface_get_rw(env, this_addr, java_io_BufferedReader_metadata.provides[0]);
		if(NULL == this)
		{
			LOG_ERROR("can not read this pointer");
			return -1;
		}
		if(this->init_state == 0)
		{
			LOG_ERROR("invalid initallization status");
			return -1;
		}
		if(cesk_set_merge(this->isr, reader_set) < 0)
		{
			LOG_ERROR("failed to modify the object");
			return -1;
		}
		bci_interface_release_rw(env, this_addr);
		bci_interface_return_object(env, this_addr);
	}
	return 0;
}
static inline int _java_io_BufferedReader_read(bci_method_env_t* env)
{
	extern bci_class_t java_io_BufferedReader_metadata;
	bci_interface_return_set(env, read_result_set);
	return 0;
}
int java_io_BufferedReader_invoke(int id, bci_method_env_t* env)
{
	switch(id)
	{
		case 1:
		case 2:
			return _java_io_BufferedReader_init(id, env);
		case 0xfffe:
			return _java_io_BufferedReader_read(env);
		default:
			LOG_ERROR("unsupported method");
			return -1;
	}
}
bci_class_t java_io_BufferedReader_metadata = {
	.size = sizeof(this_t),
	.load = java_io_BufferedReader_onload,
	.unload = java_io_BufferedReader_unload,
	.initialize = java_io_BufferedReader_init,
	.finalize = java_io_BufferedReader_finalize,
	.duplicate = java_io_BufferedReader_duplicate,
	.read = java_io_BufferedReader_get_addr_list,
	.write = java_io_BufferedReader_modify,
	.hash = java_io_BufferedReader_hashcode,
	.equal = java_io_BufferedReader_equal,
	.to_string = java_io_BufferedReader_to_string,
	.has_reloc_ref = java_io_BufferedReader_get_rf,
	.merge = java_io_BufferedReader_merge,
	.is_instance = java_io_BufferedReader_instance_of,
	.get_method = java_io_BufferedReader_get_method,
	.invoke = java_io_BufferedReader_invoke,
	.super = "java/lang/Object",
	.provides = {
		"java/io/BufferedReader",
		NULL
	}
};
