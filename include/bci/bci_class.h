/**
 * @brief the built-in class description and class def table
 **/
#ifndef __BCI_CLASS_H__
#define __BCI_CLASS_H__
#include <constants.h>
#include <const_assertion.h>
/** @brief the built-in class */
typedef struct _bci_class_t bci_class_t;
/** @brief use for int class comparasion, returns a constant address */
typedef uint32_t (*bci_class_compare_fn_t)(const cesk_object_t*, const cesk_object_t*);
/** @brief the built-in class */
struct _bci_class_t{
	const char* path;      /*!< the class path of this built-in class */
	size_t      size;      /*!< the additional size of a instance */
	uint32_t    nfields;   /*!< the number of fields */
	bci_class_compare_fn_t cmp; /*!< the compare function */
	const char* fields[0]; /*!< the field names */
};
CONST_ASSERTION_FIRST(bci_class_t, path);

/**
 * @brief init the bci_class.c
 * @return < 0 indicates an error
 **/
int bci_class_init();
/**
 * @brief finalize bci_class
 * @return nothing
 **/
void bci_class_finalize();
/**
 * @brief define a new built-in clas
 * @param class the class definition
 * @param path the class path, a pooled string
 * @return result of operation < 0 indicates an error
 **/
int bci_class_define(bci_class_t* class, const char* path);
/**
 * @brief find a class by name
 * @param path the class path
 * @return the class definition, NULL indicates error
 **/
bci_class_t* bci_class_find(const char* path);
#endif
