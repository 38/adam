#ifndef __BCI_CLASS_H__
#define __BCI_CLASS_H__
#include <constants.h>
#include <const_assertion.h>

#include <log.h>


/**
 * @brief the buildin method definition
 **/
typedef struct {
	uint32_t size;    /*!< the size of data section */
	int (*initialization)(const char* class, void* data);  /*!< how to initialize the data used by this instance, this is NOT CONSTRUCTOR */
	int (*finalization)(const char* class, void* data);    /*!< how to clean up this instance, NOT DESTRUCTOR */
	int (*onload)(const char* class);                    /*!< actions after this class is loaded from the package */
	int (*ondelete)(const char* class);                  /*!< actions before this class remove from the name table */
	const char* provides[BCI_CLASS_MAX_PROVIDES];             /*!< the class that this build-in class provides, end with a NULL */
} bci_class_t;

/**
 * @brief initialize a built-in instance
 * @param mem the memory for this instance
 * @param class the class def
 * @param classpath the class path
 * @return result of initialization, < 0 indicates an error
 **/
int bci_class_initialize(void* mem, const bci_class_t* class, const char* classpath);
#endif
