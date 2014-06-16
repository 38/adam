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
	int (*initialization)(void* data);  /*!< how to initialize the data used by this instance, this is NOT CONSTRUCTOR */
	int (*finalization)(void* data);    /*!< how to clean up this instance, NOT DESTRUCTOR */
	int (*onload)();                    /*!< actions after this class is loaded from the package */
	int (*ondelete)();                  /*!< actions before this class remove from the name table */
} bci_class_t;

#endif
