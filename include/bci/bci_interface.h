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
 * @brief get the k-th argument
 * @param env the environ
 * @param k the k-th argument
 * @param N the total number of arguments
 * @return result set, NULL for error
 **/
const cesk_set_t* bci_interface_read_arg(const bci_method_env_t* env, uint32_t k, uint32_t N);

/**
 * @brief read an object field from the frame store
 * @param env the environ
 * @param set the reference set 
 * @param classpath the class path
 * @param field the fieldname
 * @return the result set, NULL if error occurs
 **/
const cesk_set_t* bci_interface_read_object(const bci_method_env_t* env, cesk_set_t* set, const char* classpath, const char* field);

/**
 * @brief create an object in the invocation environ
 * @param env the environ
 * @param path the class path
 * @param init_param the initialization parameter
 * @return the address of the newly create isntance
 **/
uint32_t bci_interface_new_object(bci_method_env_t* env, const char* path, const void* init_param);

/**
 * @brief return a single value from the bulit-in function
 * @param env the envrion
 * @param addr the return address 
 * @return < 0 if there's an error
 **/
int bci_interface_return_single_address(bci_method_env_t* env, uint32_t addr);

/**
 * @brief return a value set from the bulit-in function
 * @param env the envrion
 * @param addr the return address 
 * @return < 0 if there's an error
 **/
int bci_interface_return_set(bci_method_env_t* env, const cesk_set_t* set);

/**
 * @brief get a writable pointer to BCI data in the address
 * @param env the envrion
 * @param addr the address 
 * @param classpath the class path of the BCI class
 * @return the pointer to BCI data section, NULL indicates errors
 **/
void* bci_interface_get_rw(bci_method_env_t* env, uint32_t addr, const char* classpath);

/**
 * @brief get a readonly pointer to BCI data in the address
 * @param env the envrion
 * @param addr the address 
 * @param classpath the class path of the BCI class
 * @return the pointer to BCI data section, NULL indicates errors
 **/
const void* bci_interface_get_ro(bci_method_env_t* env, uint32_t addr, const char* classpath);

/**
 * @brief make the object in the address as a part of return value 
 * @param env the envrion
 * @param addr the address
 * @return < 0 indicates an error
 **/
int bci_interface_return_object(bci_method_env_t* env, uint32_t addr);
#endif
