/**
 * @brief Builtin Classes Name Table
 * @details The bci_nametable is used to find a class or a method def
 * @note The BCI framework DO NOT DISTINGUISH OVERLOADED FUNCTIONS, that means 
 *       The buildin class is responsible to resolve the overloaded function
 **/
#ifndef __BCI_NAMETAB_H__
#define __BCI_NAMETAB_H__
#include <constants.h>
#include <const_assertion.h>

#include <log.h>

#include <bci/bci_class.h>
#include <bci/bci_method.h>

/**
 * @brief initialize the name lookup table 
 * @return result of this operation, < 0 indicates an error
 **/
int bci_nametab_init();

/**
 * @brief finalize the name lookup table
 * @return nothing
 **/
void bci_nametab_finialize();


/**
 * @brief insert a name to table 
 * @param clspath the class path
 * @param def the class definition
 * @return the result of insertion, < 0 indicates an error
 **/
int bci_nametab_register_class(const char* clspath, bci_class_t* def); 

/**
 * @brief look for a class definition
 * @param clspath the class path
 * @return the class object, NULL if not found
 **/
const bci_class_wrap_t* bci_nametab_get_class(const char* clspath);
#endif
