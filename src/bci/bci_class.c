#include <bci/bci_class.h>
int bci_class_initialize(void* mem, const bci_class_t* class)
{
	if(NULL == mem || NULL == class)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	if(NULL != class->initialization)
	{
		if(class->initialization(mem) < 0)
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
	return (class->get_field == NULL)?
		   NULL:
		   class->get_field(this, fieldname);
}
int bci_class_put_field(void* this, const char* fieldname, const cesk_set_t* set, cesk_store_t* store, int keep, const bci_class_t *class)
{
	if(NULL == this || NULL == fieldname || NULL == set || NULL == store || NULL == class) return -1;
	return (class->put_field == NULL)?
		   -1:
		   class->put_field(this, fieldname, set, store, keep);
}
int bci_class_get_addr_list(const void* this, uint32_t* buf, size_t sz, const bci_class_t* class)
{
	if(NULL == this || NULL == buf || NULL == class) return -1;
	return (class->get_addr_list == NULL)?
		   -1:
		   class->get_addr_list(this, buf, sz);
}
hashval_t bci_class_hashcode(const void* this, const bci_class_t* class)
{
	if(NULL == this || NULL == class) 
		return 0;
	return (NULL == class->hash)?
	       0:
		   class->hash(this);
}
