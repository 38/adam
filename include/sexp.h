#ifndef __SEXP_H__
#define __SEXP_H__
#include <stdint.h>
#include <stdlib.h>
/**
 * @file sexp.h
 * @brief Utils for maintanance of S-Expression.
 * 
 * @details 
 * This file provides functions to parse, process a S-Expression.
 *
 * Syntax of S-Expression:
 *
 *      sexp := atom | list
 *
 *      list := (sexp*)
 *
 *      atom := stirng|literal
 * The parser parse a list to a cons:
 *
 *      "(1 2 3 4)" will be parseed as 
 *      (cons 1 (cons 2 (cons 3 (cons 4 NIL))))
 *
 * function sexp_parse is used for parse a S-Expression:
 *
 *      sexp_parse("(invoke-direct java/lang/string.length,1)");
 *
 * Notice that all string include literal and string will be converted 
 * to a pooled string after parsed. So we can compare the string
 * by comparing the memory address.
 *
 * function sexp_match can perform a pattern match like want we do in
 * functional programming languages:
 *
 *      sexp_match(sexp, "(L=L?C", TOKEN1, lit2, tail);
 *
 * This function returns 1 if the S-Expression matchs pattern
 *
 * TOKEN1 :: lit2 :: tail
 *
 * The reason why we don't need a type for empty S-Expression NIL 
 * is it's actully a singleton, we can use NULL pointer to repr it.
 */
enum{
    SEXP_TYPE_LIT,    /*!<A literal */
    SEXP_TYPE_STR,    /*!<A string */
    SEXP_TYPE_CONS    /*!<A pair */
};

/** @brief header structure of a S-Expression */
typedef struct {
	/** Type of the S-Expression */
    int type;
	/** Actuall data */
    char data[0];
} sexpression_t;

/** @brief data type for a string S-Expression. It's actually a pooled string, for more detail about pooled string, see documentation for stringpool.h */
typedef const char* sexp_str_t;

/** @brief data type for a literal S-Expression, pooled string */
typedef const char* sexp_lit_t;

/** @brief data structure for a cons */ 
typedef struct{
    char seperator;  		/*!<The sperator between first and second, this is useful when parsing instruction */
    sexpression_t * first;  /*!<The first element of this cons */
    sexpression_t * second; /*!<The scond element of this cons */
} sexp_cons_t;

/**@brief Parse a string into sexpression. 
 * @param str String to parse
 * @param buf the output buffer 
 * @return The remaining string after current S-Expression has been parsed
 *               NULL indicates an error
 */
const char* sexp_parse(const char* str, sexpression_t** buf);

/**@brief free memory for a S-Expression recursively */
void sexp_free(sexpression_t* buf);

/** 
 * @brief Check S-Expression matches a pattern 
 * @details Like printf function, pattern only describe the property of following function
 * The pattern can be defined as
 *
 * PATTERN := ({TYPE DESC}* | TYPE DESC
 *
 * TYPE    :=  C|L|S|A
 *
 * DESC    :=  ?|=
 *
 * If the pattern starts with a (, that means the pattern is going to match into a list
 * The type name is either C, L, S, A, which means Cons, Literal and String and Anything
 * The desc field indicates how to use this variable. If the desc is =, that means this
 *     part of the list must have THE SAME TARGET ADDRESS and same type.
 *     Notice that type C can not have '='
 *     '?' means we what to store the value in a variable and also, verify the type
 *     The type A is only used at the end of list, which return the remaining list
 *
 * @param sexpr input S-Expression
 * @param pattern the pattern string
 * @return 1 means the S-Expression matches the pattern, otherwise means doesn't match
 */
int sexp_match(const sexpression_t* sexpr, const char* pattern, ...);

/** @brief strip one expected elements (either string or literal) in front of the sexpr if there's some,
 * This is useful because, instructions like, 
 * move, move/16, move/from16 which are actually same.ALL STRINGS ARE ASSUMED TO BE POOLED
 * @param sexpr  the S-Expression to be stripped. 
 * @param ... the expected value, all values are const char*. the arg list ends with a NULL pointer
 */
 /*
 * for example sexpr = (from16 v123,456)
 * sexp_strip(sexpr, "from16", "16", NULL) = (v123,456)
 * sexp_strip(sexpr, "abc", "def", NULL) = (from16 v123,456)
 * NOTICE: All string assumed to be a pooled string, that means address equal <==> value equal */
const sexpression_t* sexp_strip(const sexpression_t* sexpr, ...);

/**@brief Get object path (a/b/c d) ==> 'a/b/c' && (d)
 * @param sexpr Input S-Expression
 * @param remaining The remaining element after object path is parsed 
 * @return a pooled string of object path, NULL indicates error
 */
const char* sexp_get_object_path(const sexpression_t* sexpr, const sexpression_t ** remaining);

/** @brief Get objet path (a/b/c d) ==> 'a/b' 'c' && (d) .
 *  
 *  This function is actually do the same thing like sexp_strip,
 *  the difference is this function return 2 pooled strings which are 
 *  class path and method name seperately, rather than a single object path
 *
 *  @param sexp S-Expression
 *  @param remaining Remaining S-Expression after parse
 *  @param path	buf for path
 *  @param name buf for name
 *  @return >=0 means success, <0 means error
 */
int sexp_get_method_address(const sexpression_t* sexp, const sexpression_t ** remaining, const char** path, const char** name);

/** @brief return the length of the S-Expression sexp_length(s) <==> (length s)
 *  @param sexp S-Expression
 *  @return length of the S-Expression, -1 indicate an error */
int sexp_length(const sexpression_t* sexp);

/** @brief convert s-expression to string 
 *  @param sexp	s-expression to be convert
 *  @param buf   the buffer for output, if buf is NULL, use default buf
 *  @return the result string, NULL means error
 */
char* sexp_to_string(const sexpression_t* sexp, char* buf);

/** @brief empty S-Expression */
#define SEXP_NIL NULL

#endif /* __SEXP_H__ */
