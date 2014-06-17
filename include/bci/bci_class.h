#ifndef __BCI_CLASS_H__
#define __BCI_CLASS_H__
#include <constants.h>
#include <const_assertion.h>

#include <log.h>
typedef struct _bci_class_t bci_class_t;
#include <cesk/cesk_set.h>


/**
 * @brief the buildin method definition
 **/
struct _bci_class_t {
	uint32_t size;    /*!< the size of data section */
	
	int (*onload)();                    /*!< actions after this class is loaded from the package return value < 0 means error */
	int (*ondelete)();                  /*!< actions before this class remove from the name table return value < 0 means error */
	
	int (*initialization)(void* this);  /*!< how to initialize the data used by this instance, this is NOT CONSTRUCTOR return value < 0 means error */
	int (*finalization)(void* this);    /*!< how to clean up this instance, NOT DESTRUCTOR return value < 0 means error*/

	int (*duplicate)(const void* this, void* that); /*!< how to make a duplicate return value < 0 means error*/

	cesk_set_t* (*get_field)(const void* this, const char* fieldname);  /*!< callback that get a pointer to a field return an new set contains the field*/
	int (*put_field)(void* this, const char* fieldname, const cesk_set_t* set, cesk_store_t* store, int keep); /*!< set the field value */

	int (*get_addr_list)(void* this, uint32_t* buf,size_t sz);              /*!< get the reference list address, return the number of address copied to buffer, < 0 when error */
	const char* provides[BCI_CLASS_MAX_PROVIDES];             /*!< the class that this build-in class provides, end with a NULL */
}; 

/**
 * @brief initialize a built-in instance
 * @param mem the memory for this instance
 * @param class the class def
 * @param classpath the class path
 * @return result of initialization, < 0 indicates an error
 **/
int bci_class_initialize(void* mem, const bci_class_t* class);
/**
 * @brief get the value of the field
 * @param this the object memory
 * @param fieldname the name of the field
 * @param class the class def
 * @return the result address set of this operation(newly created object, so need to free by caller)
 **/
cesk_set_t* bci_class_get_field(const void* this, const char* fieldname, const bci_class_t* class);
/**
 * @brief set the value of the field
 * @param this the object memory
 * @param fieldname the fieldname
 * @param set the new address of the value
 * @param store the store contains this object, cuz we need to maintan the refcnt
 * @param keep if keep == 1, means keep the old value no matter this value has been reused
 * @param class the class def
 * @return the result of this operation
 **/
int bci_class_put_field(void* this, const char* fieldname, const cesk_set_t* set, cesk_store_t* store, int keep, const bci_class_t* class);

/**
 * @brief get an address list that used by this object 
 * @param this the object
 * @param buf the buffer
 * @param sz the buffer size
 * @param class the class def
 * @return the value copied to the buffer < 0 means error
 **/
int bci_class_get_addr_list(void* this, uint32_t* buf, size_t sz, const bci_class_t* class);
#endif
