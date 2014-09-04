/**
 * @brief this is the interface provided by BCI subsystem to 
 *        built-in classes.
 **/
#ifndef __BCI_INTERFACE_H__
#define __BCI_INTERFACE_H__

typedef struct _bci_method_env_t bci_method_env_t;

#include <stringpool.h>
#include <bci/bci.h>
#include <cesk/cesk.h>
#include <tag/tag.h>


#define PackageInit_Begin \
int builtin_library_init()\
{

#define PackageInit_End \
	return 0;\
}\
void builtin_library_finalize()\
{\
}

#define Provides_Class(module_name) do{\
	extern bci_class_t module_name##_metadata;\
	uint32_t i;\
	for(i = 0; NULL != module_name##_metadata.provides[i]; i ++) {\
		module_name##_metadata.provides[i] = stringpool_query(module_name##_metadata.provides[i]);\
		module_name##_metadata.super       = stringpool_query(module_name##_metadata.super);\
		if(bci_nametab_register_class(module_name##_metadata.provides[i], &module_name##_metadata) < 0)\
		{\
			LOG_ERROR("Can not register built-in class %s", module_name##_metadata.provides[i]);\
			return -1;\
		}\
		else\
		{\
			LOG_DEBUG("Registered built-in class %s", module_name##_metadata.provides[i]);\
			if(NULL != module_name##_metadata.onload)\
				module_name##_metadata.onload(module_name##_metadata.provides[i]);\
		}\
	}\
}while(0)


/**
 * @brief read the value of reigster from the envrion
 * @param env the environ
 * @param regid the register id
 * @return the result set, NULL if error occurs
 **/
const cesk_set_t* bci_interface_read_register(const bci_method_env_t* env, uint32_t regid);

/**
 * @brief read a object field from the frame store
 * @param env the environ
 * @param set the reference set 
 * @param classpath the class path
 * @param field the fieldname
 * @return the result set, NULL if error occurs
 **/
const cesk_set_t* bci_interface_read_object(const bci_method_env_t* env, cesk_set_t* set, const char* classpath, const char* field);

#endif
