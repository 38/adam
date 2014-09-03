#include <bci/bci_class.h>
#include <bci/bci_interface.h>
int bci_class_initialize(void* mem, const void* init_param, tag_set_t** p_tags, const bci_class_t* class)
{
	if(NULL == mem || NULL == class)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(NULL != class->initialization)
	{
		if(class->initialization(mem, init_param, p_tags) < 0)
		{
			LOG_ERROR("failed initlaize the new built-in class");
			return -1;
		}
	}
	return 0;
}

cesk_set_t* bci_class_get_field(const void* this, const char* fieldname, const bci_class_t* class)
{
	if(NULL == this || NULL == fieldname || NULL == class) return NULL;
	return (class->get_field == NULL)?NULL:class->get_field(this, fieldname);
}
int bci_class_put_field(void* this, const char* fieldname, const cesk_set_t* set, cesk_store_t* store, int keep, const bci_class_t *class)
{
	if(NULL == this || NULL == fieldname || NULL == set || NULL == store || NULL == class) return -1;
	return (class->put_field == NULL)?0:class->put_field(this, fieldname, set, store, keep);
}
int bci_class_get_addr_list(const void* this, uint32_t offset, uint32_t* buf, size_t sz, const bci_class_t* class)
{
	if(NULL == this || NULL == buf || NULL == class) return -1;
	return (class->get_addr_list == NULL)?0:class->get_addr_list(this, offset, buf, sz);
}
hashval_t bci_class_hashcode(const void* this, const bci_class_t* class)
{
	if(NULL == this || NULL == class) 
		return 0;
	return (NULL == class->hash)?0:class->hash(this);
}
int bci_class_duplicate(const void* this, void* that, const bci_class_t* class)
{
	if(NULL == this || NULL == that || NULL == class)
		return -1;
	return (NULL == class->duplicate)?0:class->duplicate(this, that);
}
int bci_class_equal(const void* this, const void* that, const bci_class_t* class)
{
	if(NULL == this || NULL == that || NULL == class)
		return -1;
	return (NULL == class->equal)?-1:class->equal(this, that);
}
const char* bci_class_to_string(const void* this, char* buf, size_t size, const bci_class_t* class)
{
	if(NULL == this) return NULL;
	static char _buf[1024];
	if(NULL == buf)
	{
		buf = _buf;
		size = sizeof(_buf);
	}
	return (NULL == class->to_string)?"":class->to_string(this, buf, size);
}
int bci_class_get_relocation_flag(const void* this, const bci_class_t* class)
{
	if(NULL == this || NULL == class) return -1;
	return (NULL == class->get_relocation_flag)?0:class->get_relocation_flag(this);
}
int bci_class_merge(void* this, const void* that, const bci_class_t* class)
{
	if(NULL == this || NULL == that || NULL == class) return -1;
	return (NULL == class->merge)?0:class->merge(this, that);
}
int bci_class_modify(void* this, uint32_t offset, uint32_t* new, size_t N, const bci_class_t* class)
{
	if(NULL == this || NULL == new || NULL == class) return -1;
	return (NULL == class->modify)?0:class->modify(this, offset, new, N);
}
int bci_class_instance_of(const void* this, const char* classpath, const bci_class_t* class)
{
	if(NULL == this || NULL == class) return -1;
	return (NULL == class->modify)?0:class->instance_of(this, classpath);
}
int bci_class_get_method(const void* this, const char* methodname, 
                         const dalvik_type_t * const * typelist, 
						 const dalvik_type_t* rtype, const bci_class_t* class)
{
	if(NULL == methodname || NULL == class) return -1;
	return (NULL == class->get_method)?-1:class->get_method(this, methodname, typelist, rtype);
}
int bci_class_invoke(void* this, const void* const_this, int method_id, bci_method_env_t* env, const bci_class_t* class)
{
	if(method_id < 0 || NULL == class) return -1;
	return (NULL == class->invoke)?-1:class->invoke(this, const_this, method_id, env);
}
