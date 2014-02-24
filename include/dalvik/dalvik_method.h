#ifndef __DALVIK_METHOD_H__
#define __DALVIK_METHOD_H__
/** @file dalvik_method.h
 *  @brief method definations
 */
#include <stdint.h>

#include <log.h>
#include <sexp.h>
#include <dalvik/dalvik_instruction.h>
#include <dalvik/dalvik_type.h>
#include <dalvik/dalvik_attrs.h>
/** @brief the method defination structure */
typedef struct {
    /* DO NOT ADD ANY DEFINITION HERE */
    const char*          name;     /*!<Method Name. THIS MUST BE THE FIRST MEMBER OF THE STRCUT*/
    const char*          path;     /*!<class Path */
    const char*          file;     /*!<File Name  */
    uint32_t             flags;    /*!<Flags */
    dalvik_type_t*       return_type; /*!<return type */
    uint32_t             num_args; /*!<number of arguments */
    uint16_t             num_regs;  /*!<how many register the method uses */
    uint32_t             entry;     /*!<the offset of first instruction */
    const dalvik_type_t*       args_type[0];   /*!<the type list contains a null tail */
    /* DO NOT ADD DEFINITION AFTER args_type */
} dalvik_method_t;

/**@brief create a new method defination from a s-expression
 * @param sexp s-expression
 * @param class_path class path
 * @param file file name
 * @return method defination
 */
dalvik_method_t* dalvik_method_from_sexp(const sexpression_t* sexp, const char* class_path ,const char* file);

/** @brief free a method
 *  @param method method
 *  @return nothing
 */
void dalvik_method_free(dalvik_method_t* method);

#endif
