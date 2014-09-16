/**
 * @brief this built class emulates the input class that actually does nothing but keep some reference of other address
 * @file addressContainer.c
 **/
#include <bci/bci_interface.h>
#include <stringpool.h>
typedef struct {
	uint8_t init_state;
	size_t N;
	cesk_set_t** args;
	const char* classpath;
	uint32_t current_set;
	uint32_t previous_set;
	uint32_t previous_offset;
	uint32_t current_offset;
	cesk_set_iter_t   current_iter;
	cesk_set_iter_t   previous_iter;
} this_t;
static const char* kw_init;
int addressContainer_onload()
{
	kw_init = stringpool_query("<init>"); 
	return 0;
}
int addressContainer_init(void* this_ptr, const char* classpath, const void* init_param, tag_set_t** p_tags)
{
	this_t* this = (this_t*)this_ptr;
	if(this->init_state == 0) this->init_state = 1;
	else if(this->init_state == 1)
	{
		this->current_offset = 0xffffffffu;
		this->init_state = 2;
		return 0;
	}
	this->current_offset = 0xffffffffu;
	this->N = 0;
	this->args = NULL;
	this->classpath = classpath;
	return 0;
}
int addressContainer_delete(void* this_ptr)
{
	this_t* this = (this_t*)this_ptr;
	if(NULL == this->args) return 0;
	size_t i;
	for(i = 0; i < this->N; i ++)
		cesk_set_free(this->args[i]);
	free(this->args);
	return 0;
}
int addressContainer_dup(const void* this_ptr, void* that_ptr)
{
	size_t i = 0, j;
	const this_t* this = (const this_t*)this_ptr;
	this_t* that = (this_t*) that_ptr;
	that->init_state = this->init_state;
	that->N = this->N;
	if(this->N > 0)
	{
		that->args = (cesk_set_t**)malloc(sizeof(cesk_set_t *) * this->N);
		if(NULL == that->args)
		{
			LOG_ERROR("can not allocate memory for the address container");
			return -1;
		}
		for(i = 0; i < that->N; i ++)
		{
			that->args[i] = cesk_set_fork(this->args[i]);
			if(NULL == that->args[i]) 
			{
				LOG_ERROR("can not duplicate the address set #%zu", i);
				goto ERR;
			}
		}
	}
	else that->args = NULL;
	that->current_set = that->previous_set = 0;
	that->previous_offset = that->current_offset = 0xfffffffful;
	return 0;
ERR:
	for(j = 0; j < i; j ++)
		cesk_set_free(that->args[j]);
	return -1;
}
int addressContainer_get_addr_list(const void* this_ptr, uint32_t offset, uint32_t* buf, size_t size)
{
	this_t* this = (this_t*) this_ptr;
	if(offset != this->current_offset)
	{
		uint32_t count = 0, i, current_size = 0;
		for(i = 0; count <= offset && i < this->N; count += (current_size = cesk_set_size(this->args[i++])));
		if(offset >= count) return 0;
		if(NULL == cesk_set_iter(this->args[i - 1], &this->current_iter))
		{
			LOG_ERROR("failed to get a iterator for address set %d content", i - 1);
			return -1;
		}
		uint32_t num_skip = offset + current_size - count;
		for(;num_skip > 0; num_skip --)
			cesk_set_iter_next(&this->current_iter);
		this->current_offset = offset;
		this->current_set = i - 1;
	}
	this->previous_offset = this->current_offset;
	this->previous_iter = this->current_iter;
	uint32_t addr;
	int ret = 0;
	for(;this->current_set < this->N && size > 0;)
	{
		for(; size > 0 && CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&this->current_iter)); size --)
			buf[ret ++] = addr;
		if(CESK_STORE_ADDR_NULL == addr && size > 0)
		{
			this->current_set ++;
			if(this->current_set < this->N && NULL == cesk_set_iter(this->args[this->current_set], &this->current_iter))
			{
				LOG_ERROR("can not get a iterator to read set %d", this->current_set);
				return -1;
			}
		}
	}
	this->current_offset += ret;
	return ret;
}
int addressContainer_modify(void* this_ptr, uint32_t offset, uint32_t* buf, size_t N)
{
	this_t* this = (this_t*) this_ptr;
	if(this->previous_offset != offset)
	{
		LOG_ERROR("you should first read the data chunck and then write it back");
		return -1;
	}
	size_t i;
	for(i = 0; i < N;)
	{
		uint32_t addr;
		while(i < N && CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&this->previous_iter)))
		{
			if(cesk_set_modify(this->args[this->previous_set], addr, buf[i++]))
			{
				LOG_ERROR("can not modify the vlaue");
				return -1;
			}
		}
		if(i < N)
		{
			this->previous_set ++;
			if(this->previous_set >= this->N || NULL == cesk_set_iter(this->args[this->previous_set], &this->previous_iter))
			{
				LOG_ERROR("can not get a iterator to read set %d", this->current_set);
				return -1;
			}
		}
	}
	this->previous_offset += N;
	return 0;
}
hashval_t addressContainer_hashcode(const void* data)
{
	const this_t* this = (const this_t*)data;
	int i;
	hashval_t ret = 0;
	for(i = 0; i < this->N; i ++)
		ret = ((ret << 16) ^ (ret * ret)) + cesk_set_hashcode(this->args[i]);
	return ret;
}
int addressContainer_equal(const void* this_ptr, const void* that_ptr)
{
	const this_t* this = (this_t*) this_ptr;
	const this_t* that = (this_t*) that_ptr;
	if(this->N != that->N) return 0;
	int i;
	for(i = 0; i < this->N; i ++)
		if(!cesk_set_equal(this->args[i], that->args[i])) return 0;
	return 1;
}
const char* addressContainer_to_string(const void* this_ptr, char* buf, size_t size)
{
	const this_t* this = (const this_t*)this_ptr;
	int i;
	const char* ret = buf;
	for(i = 0; i < this->N; i ++)
	{
		if(buf != cesk_set_to_string(this->args[i], buf, size))
			return NULL;
		uint32_t len = strlen(buf);
		buf += len;
		size -= len;
	}
	return ret;
}
int addressContainer_merge(void* this_ptr, const void* that_ptr)
{
	this_t* this = (this_t*)this_ptr;
	const this_t* that = (const this_t*)that_ptr;
	if(this->N != that->N)
	{
		LOG_ERROR("can not merge address containers in different size");
		return -1;
	}
	int ret = 0;
	int i;
	for(i = 0; i < this->N && ret >= 0; i ++)
		ret = cesk_set_merge(this->args[i], that->args[i]);
	return ret;
}
int addressContainer_get_reloc_flag(const void* this_ptr)
{
	const this_t* this = (const this_t*)this_ptr;
	int ret = 0;
	int i;
	for(i = 0; i < this->N && !ret; i ++)
		ret = cesk_set_get_reloc(this->args[i]);
	return ret;
}
int addressContainer_instance_of(const void* this_ptr, const dalvik_type_t* type)
{
	/* we just simplify this part */
	return BCI_BOOLEAN_TRUE;
}
static const char* _last_class_path = NULL;
int addressContainer_get_method(const void* this_ptr, const char* classpath, const char* method, const dalvik_type_t* const * signature, const dalvik_type_t* rettype)
{
	/* we just simplify this, only supported function call is <init>, and the method id is actually the number of args */
	if(method == kw_init)
	{
		int N = 0;
		for(;signature[N] != NULL; N ++);
		_last_class_path = classpath;
		return N;
	}
	return -1;
}
int addressContainer_invoke(int method_id, bci_method_env_t* env)
{
	const cesk_set_t* this_set = bci_interface_read_arg(env, 0, method_id + 1);
	cesk_set_iter_t this_iter;
	if(NULL == cesk_set_iter(this_set, &this_iter))
	{
		LOG_ERROR("can not get iterator to traverse the this pointer");
		return -1;
	}
	uint32_t this_addr;
	while(CESK_STORE_ADDR_NULL != (this_addr = cesk_set_iter_next(&this_iter)))
	{
		this_t* this = (this_t*)bci_interface_get_rw(env, this_addr, _last_class_path);
		if(NULL == this)
		{
			LOG_ERROR("can not read store at address "PRSAddr, this_addr);
			return -1;
		}
		if(this->init_state == 2 && this->N != method_id)
		{
			LOG_ERROR("invalid number of attributes");
			return -1;
		}
		else
		{
			this->N = method_id;
			this->args = (cesk_set_t**)malloc(sizeof(cesk_set_t*) * this->N);
			if(NULL == this->args)
			{
				LOG_ERROR("can not allocate array for the address holder");
				return -1;
			}
			int i;
			for(i = 0; i < this->N; i ++)
			{
				this->args[i] = cesk_set_empty_set();
				if(NULL == this->args[i])
				{
					LOG_ERROR("can not create new empty set for value %d", i);
					return -1;
				}
			}
		}
		int i;
		for(i = 0; i < this->N; i ++)
		{
			const cesk_set_t* arg = bci_interface_read_arg(env, i + 1, method_id + 1);
			if(NULL == arg) 
			{
				LOG_ERROR("can not read argument %d", i);
				return -1;
			}
			cesk_set_merge(this->args[i], arg);
		}
		bci_interface_release_rw(env, this_addr);
		bci_interface_return_object(env, this_addr);
	}
	return 0;
}
bci_class_t addressContainer_metadata = {
	.size = sizeof(this_t),
	.onload = addressContainer_onload,
	.initialization = addressContainer_init,
	.finalization = addressContainer_delete,
	.duplicate = addressContainer_dup,
	.get_addr_list = addressContainer_get_addr_list,
	.modify = addressContainer_modify,
	.hash = addressContainer_hashcode,
	.equal = addressContainer_equal,
	.to_string = addressContainer_to_string,
	.merge = addressContainer_merge,
	.get_relocation_flag = addressContainer_get_reloc_flag,
	.instance_of = addressContainer_instance_of,
	.get_method = addressContainer_get_method,
	.invoke = addressContainer_invoke,
	.super = "java/lang/Object",
	.provides = {
		"java/io/InputStream",
		"java/io/OutputStream",
		"java/io/FileInputStream",
		"java/io/FileOutputStream",
		"java/io/Reader",
		"java/io/InputStreamReader",
		"java/io/Writer",
		"java/io/OutputStreamWriter",
		"java/io/File",
		NULL
	}
};
