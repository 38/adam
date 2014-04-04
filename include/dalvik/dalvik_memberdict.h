#ifndef __DALVIK_MEMBERDICT_H__
#define __DALVIK_MEMBERDICT_H__
/** @file dalvik_memberdict.h
 *  @brief member dictionary
 *
 *  @details
 *  This file provide a group of function that can be used for member searching,
 *  The search key is (pooled_class_path, pooled_member_name)
 */
#include <constants.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_field.h>
#include <dalvik/dalvik_class.h>

#include <log.h>

/**
 * @brief initialization
 * @return nothing
 */
int dalvik_memberdict_init();
/**
 * @brief finalzation
 * @return nothing
 */
void dalvik_memberdict_finalize();

/** @brief register a method member for some class path 
 *  @param class_path the class path(a pooled string)
 *  @param method the method object
 *  @return result of operation
 */
int dalvik_memberdict_register_method(const char* class_path, dalvik_method_t* method);

/** @brief register a field member for some class path 
 *  @param class_path the class path (pooled)
 *  @param field pooled file name
 *  @return result of operation
 */
int dalvik_memberdict_register_field(const char* class_path, dalvik_field_t* field);
/** @brief register a class 
 *  @param class_path the class path 
 *  @param class the class defination
 *  @return result of operation
 */
int dalvik_memberdict_register_class(const char* class_path, dalvik_class_t* class);
/** @brief retrive a method by class_path and name and the type of args is type 
 *  @param class_path the pooled class path
 *  @param name the pooled method name
 *  @param args the arguement list(because java support overload, so it's impossible to get a object without knowning the argument list)  
 */
dalvik_method_t* dalvik_memberdict_get_method(const char* class_path, const char* name, const dalvik_type_t *const* args);
/** @brief retrive a field by class path and name 
 *  @param class_path the class path (pooled)
 *  @param name the field name 
 *  @return the field defination
 */
dalvik_field_t* dalvik_memberdict_get_field(const char* class_path, const char* name);
/** @brief retrive a class by class path 
 *  @param class_path pooled class path
 *  @return class defination
 */
dalvik_class_t* dalvik_memberdict_get_class(const char* class_path);


#endif /* __DALVIK_MEMBERDICT_H__ */
