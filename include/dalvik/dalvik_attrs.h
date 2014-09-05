#ifndef __DALVIK_ATTRS_H__
#define __DALVIK_ATTRS_H__
/**@file dalvik_attrs.h
 * @brief define dalvik object attributes 
 */
/* The general attribute flags should use low 16 bits only */
#define DALVIK_ATTRS_ABSTARCT    0x1
#define DALVIK_ATTRS_ANNOTATION  0x2
#define DALVIK_ATTRS_FINAL       0x4
#define DALVIK_ATTRS_PRIVATE     0x8
#define DALVIK_ATTRS_PROCTED     0x10
#define DALVIK_ATTRS_PUBLIC      0x20
#define DALVIK_ATTRS_STATIC      0x40
#define DALVIK_ATTRS_SYNCRONIZED 0x80
#define DALVIK_ATTRS_TRASIENT    0x100
#define DALVIK_ATTRS_VOLATILE    0x200

/* DO NOT define any attribute lager than 0xffff */

/** @brief parse a sexpression to a attribute 
 *  @return attribute code, otherwise return negative value 
 */
int dalvik_attrs_from_sexp(const sexpression_t* sexp);
#endif
