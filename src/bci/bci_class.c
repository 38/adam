#include <bci/bci_class.h>
int bci_class_initialize(void* mem, const bci_class_t* class , const char* classpath)
{
	if(NULL == mem || NULL == class || NULL == classpath)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	
	return 0;
}
