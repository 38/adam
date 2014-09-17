#include <bci/bci_class.h>
#include <bci/bci_interface.h>
int bci_class_initialize(void* mem, const char* classpath, const void* init_param, tag_set_t** p_tags, const bci_class_t* class)
{
	if(NULL == mem || NULL == class)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(NULL != class->initialize)
	{
		if(class->initialize(mem, classpath, init_param, p_tags) < 0)
		{
			LOG_ERROR("failed initlaize the new built-in class");
			return -1;
		}
	}
	return 0;
}
int bci_class_finalize(void* mem, const bci_class_t* class)
{
	if(NULL == mem || NULL == class) return -1;
	return (class->finalize == NULL)?0:class->finalize(mem);
}
cesk_set_t* bci_class_get(const void* this, const char* fieldname, const bci_class_t* class)
{
	if(NULL == this || NULL == fieldname || NULL == class) return NULL;
	return (class->get == NULL)?NULL:class->get(this, fieldname);
}
int bci_class_put(void* this, const char* fieldname, const cesk_set_t* set, cesk_store_t* store, int keep, const bci_class_t *class)
{
	if(NULL == this || NULL == fieldname || NULL == set || NULL == store || NULL == class) return -1;
	return (class->put == NULL)?0:class->put(this, fieldname, set, store, keep);
}
int bci_class_read(const void* this, uint32_t offset, uint32_t* buf, size_t sz, const bci_class_t* class)
{
	if(NULL == this || NULL == buf || NULL == class) return -1;
	return (class->read == NULL)?0:class->read(this, offset, buf, sz);
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
	return (NULL == class->to_string)?"built-in-object":class->to_string(this, buf, size);
}
int bci_class_has_reloc_ref(const void* this, const bci_class_t* class)
{
	if(NULL == this || NULL == class) return -1;
	return (NULL == class->has_reloc_ref)?0:class->has_reloc_ref(this);
}
int bci_class_merge(void* this, const void* that, const bci_class_t* class)
{
	if(NULL == this || NULL == that || NULL == class) return -1;
	return (NULL == class->merge)?0:class->merge(this, that);
}
int bci_class_write(void* this, uint32_t offset, uint32_t* new, size_t N, const bci_class_t* class)
{
	if(NULL == this || NULL == new || NULL == class) return -1;
	return (NULL == class->write)?0:class->write(this, offset, new, N);
}
int bci_class_is_instance(const void* this, const dalvik_type_t* type, const bci_class_t* class)
{
	if(NULL == this || NULL == class) return -1;
	return (NULL == class->is_instance)?BCI_BOOLEAN_UNKNOWN:class->is_instance(this, type);
}
int bci_class_get_method(const void* this, const char* classpath, const char* methodname, 
                         const dalvik_type_t * const * typelist, 
						 const dalvik_type_t* rtype, const bci_class_t* class)
{
	if(NULL == methodname || NULL == class) return -1;
	return (NULL == class->get_method)?-1:class->get_method(this, classpath, methodname, typelist, rtype);
}
int bci_class_invoke(int method_id, bci_method_env_t* env, const bci_class_t* class)
{
	if(method_id < 0 || NULL == class) return -1;
	return (NULL == class->invoke)?-1:class->invoke(method_id, env);
}
