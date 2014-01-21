#ifndef __DALVIK_ATTRS_H__
#define __DALVIK_ATTRS_H__

#define DALVIK_ATTRS_ABSTARCT    0x1
#define DALVIK_ATTRS_ANNOTATION  0x2
#define DALVIK_ATTRS_FINAL       0x4
#define DALVIK_ATTRS_PRIVATE     0x8
#define DALVIK_ATTRS_PROCTED     0x10
#define DALVIK_ATTRS_PUBLIC      0x20
#define DALVIK_ATTRS_STATIC      0x40
#define DALVIK_ATTRS_SYNCRONIZED 0x80
#define DALVIK_ATTRS_TRASIENT    0x100


int dalvik_attrs_from_sexp(sexpression_t* sexp);
#endif
