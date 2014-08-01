/** @file dalvik_type.c
 *  @todo type list pool
 **/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <dalvik/dalvik_type.h>
#include <log.h>
#include <dalvik/dalvik_tokens.h>
#include <debug.h>
const char* dalvik_type_atom_name[DALVIK_TYPECODE_NUM_ATOM] = {
	[DALVIK_TYPECODE_VOID] = "void"  ,
	[DALVIK_TYPECODE_INT]  = "int"   ,
	[DALVIK_TYPECODE_LONG] = "long"  ,
	[DALVIK_TYPECODE_SHORT]= "short" ,
	[DALVIK_TYPECODE_WIDE] = "wide"  ,
	[DALVIK_TYPECODE_FLOAT]= "float" ,
	[DALVIK_TYPECODE_DOUBLE]="double",
	[DALVIK_TYPECODE_CHAR]=  "char",
	[DALVIK_TYPECODE_BYTE]=  "byte",
	[DALVIK_TYPECODE_BOOLEAN]="boolean"
};


dalvik_type_t* dalvik_type_atom[DALVIK_TYPECODE_NUM_ATOM];
static inline dalvik_type_t* _dalvik_type_alloc(int typecode)
{
	dalvik_type_t* ret = (dalvik_type_t*)malloc(sizeof(dalvik_type_t));
	if(NULL == ret) return NULL;
	memset(ret, 0, sizeof(dalvik_type_t));
	ret->typecode = typecode;
	return ret;
}
static inline dalvik_type_t* _dalvik_type_new(int typecode)
{
	if(DALVIK_TYPE_IS_ATOM(typecode)) return dalvik_type_atom[typecode];
	else return _dalvik_type_alloc(typecode);
}
int dalvik_type_init(void)
{
	int i;
	for(i = 0; i < DALVIK_TYPECODE_NUM_ATOM; i ++)
	{
		dalvik_type_atom[i] = _dalvik_type_alloc(i);
		if(NULL == dalvik_type_atom[i])
		{
			LOG_ERROR("Unable to create a new atmoc type");
			return -1;
		}
	}
	return 0;
}

void dalvik_type_finalize(void)
{
	int i;
	for(i = 0; i < DALVIK_TYPECODE_NUM_ATOM; i ++)
		free(dalvik_type_atom[i]);
}

dalvik_type_t* dalvik_type_from_sexp(const sexpression_t* sexp)
{
   const char* curlit;
   LOG_DEBUG("parsing type %s", sexp_to_string(sexp, NULL));
   if(sexp_match(sexp, "L?", &curlit))  /* A single literal ? atom */
   {
	   if(curlit == DALVIK_TOKEN_VOID)
		   return DALVIK_TYPE_VOID;
	   else if(curlit == DALVIK_TOKEN_INT)
		   return DALVIK_TYPE_INT;
	   else if(curlit == DALVIK_TOKEN_WIDE)
		   return DALVIK_TYPE_WIDE;
	   else if(curlit == DALVIK_TOKEN_LONG)
		   return DALVIK_TYPE_LONG;
	   else if(curlit == DALVIK_TOKEN_SHORT)
		   return DALVIK_TYPE_SHORT;
	   else if(curlit == DALVIK_TOKEN_BYTE)
		   return DALVIK_TYPE_BYTE;
	   else if(curlit == DALVIK_TOKEN_CHAR)
		   return DALVIK_TYPE_CHAR;
	   else if(curlit == DALVIK_TOKEN_BOOLEAN)
		   return DALVIK_TYPE_BOOLEAN;
	   else if(curlit == DALVIK_TOKEN_FLOAT)
		   return DALVIK_TYPE_FLOAT;
	   else if(curlit == DALVIK_TOKEN_DOUBLE)
		   return DALVIK_TYPE_DOUBLE;
	   else 
		   return NULL;
   }
   if(sexp_match(sexp, "(L?A", &curlit, &sexp))
   {
	   if(curlit == DALVIK_TOKEN_OBJECT)  /* [object a/b/c] */
	   {
		   dalvik_type_t* ret;
		   ret = _dalvik_type_alloc(DALVIK_TYPECODE_OBJECT);
		   if(NULL == ret) return NULL;
		   if(NULL == (ret->data.object.path = sexp_get_object_path(sexp,NULL)))
		   {
			   dalvik_type_free(ret);
			   return NULL;
		   }
		   return ret;
	   }
	   else if(curlit == DALVIK_TOKEN_ARRAY)  /* [array ...] */
	   {
		   dalvik_type_t* ret;
		   ret = _dalvik_type_alloc(DALVIK_TYPECODE_ARRAY);
		   if(NULL == ret) return NULL;
		   
		   /* We have too unpack the sexp first */

		   if(sexp_match(sexp, "(_?", &sexp) == 0) return NULL;
		   
		   if(NULL == (ret->data.array.elem_type = dalvik_type_from_sexp(sexp)))
		   {
			   dalvik_type_free(ret);
			   return NULL;
		   }
		   return ret;
	   }
   }
   return NULL;
}

void dalvik_type_free(dalvik_type_t* type)
{
	if(NULL == type) return;
	if(DALVIK_TYPE_IS_ATOM(type->typecode)) return;
	switch(type->typecode)
	{
		case DALVIK_TYPECODE_ARRAY:
			dalvik_type_free(type->data.array.elem_type);
		default: 
			free(type);
	}
}

int dalvik_type_equal(const dalvik_type_t* left, const dalvik_type_t* right)
{
	if(NULL == left || NULL == right) 
		return left == right;   /* if left and right are nulls, we just return true */
	if(DALVIK_TYPE_IS_ATOM(left->typecode))
	{
		/* atomic type is singleton */
		return left == right;
	}
	/* if two type are not same */
	if(left->typecode != right->typecode) return 0;
	switch(left->typecode)
	{
		case DALVIK_TYPECODE_ARRAY:
			return dalvik_type_equal(left->data.array.elem_type, 
									 right->data.array.elem_type);
		case DALVIK_TYPECODE_OBJECT:
			return left->data.object.path == right->data.object.path;
		default:
			LOG_ERROR("unknown type");
			return -1;
	}
}
hashval_t dalvik_type_hashcode(const dalvik_type_t* type)
{
	if(NULL == type) return 0xe7c54276ul;   /* if there's no type, just return a magic number */
	if(DALVIK_TYPE_IS_ATOM(type->typecode))
	{
		/* for a signleton, the hashcode is based on the memory address */
		return (((uintptr_t)type)&0xfffffffful) * MH_MULTIPLY;
	}
	switch(type->typecode)
	{
		case DALVIK_TYPECODE_ARRAY:
			return (dalvik_type_hashcode(type->data.array.elem_type) * MH_MULTIPLY) ^ 0x375cf68cul;
		case DALVIK_TYPECODE_OBJECT:
			return ((uintptr_t)type->data.object.path ^ 0x5fc7f961ul) * MH_MULTIPLY;
		default:
			LOG_ERROR("unknown type");
			return 0;
	}
}
hashval_t dalvik_type_list_hashcode(const dalvik_type_t * const * typelist)
{
	int i;
	hashval_t h = 0;
	if(NULL == typelist) return 0x7c5b32f7ul;   /* just a magic number */
	for(i = 0; typelist[i] != NULL; i ++)
	{
		h ^= dalvik_type_hashcode(typelist[i]);
		h *= MH_MULTIPLY;
	}
	return h;
}
int dalvik_type_list_equal(const dalvik_type_t * const * left, const dalvik_type_t * const * right)
{
	if(NULL == left || NULL == right)
		return left == right;
	int i;
	for(i = 0; left[i] != NULL && right[i] != NULL; i ++)
		if(dalvik_type_equal(left[i], right[i]) == 0)
			return 0;
	/* Because the only possiblity of left[i] == right[i] here is both variable is NULL */
	return left[i] == right[i];
}
static inline int _dalvik_type_to_string_imp(const dalvik_type_t* type, char* buf, size_t sz)
{
	if(NULL == type)
		return snprintf(buf, sz, "(null-type)");
	if(DALVIK_TYPE_IS_ATOM(type->typecode))
	{
		int pret = snprintf(buf, sz, "(atom %s)", dalvik_type_atom_name[type->typecode]);
		if(pret > sz) return sz;
		return pret;
	}
	else
	{
		char* p;
		int pret;
		switch(type->typecode)
		{
			case DALVIK_TYPECODE_OBJECT:
				pret = snprintf(buf, sz, "(object %s)", type->data.object.path);
				if(pret > sz) return sz;
				return pret;
			case DALVIK_TYPECODE_ARRAY:
				p = buf;
				p += snprintf(p, sz, "(array ");
				if(p > buf + sz) p = buf + sz;
				p += _dalvik_type_to_string_imp(type->data.array.elem_type, p, buf + sz - p);
				p[0] = ')';
				p[1] = 0;
				p ++;
				return p - buf;
			default:
				pret = snprintf(buf, sz, "(unknown-type)");
				if(pret > sz) return sz;
				return pret;
		}
	}
}
const char* dalvik_type_to_string(const dalvik_type_t* type, char* buf, size_t sz)
{
	static char _buf[1024];
	if(NULL == buf)
	{
		buf = _buf;
		sz = sizeof(_buf);
	}
	_dalvik_type_to_string_imp(type, buf, sz);
	return buf;
}
const char* dalvik_type_list_to_string(const dalvik_type_t * const * list, char* buf, size_t sz)
{
	static char _buf[1024];
	if(NULL == buf)
	{
		buf = _buf;
		sz = sizeof(_buf);
	}
	char* p = buf;
	p += snprintf(p, buf + sz - p, "(type-list");
	if(p > buf + sz) p = buf + sz;
	int i;
	for(i = 0; NULL != list[i]; i ++)
	{
		p += _dalvik_type_to_string_imp(list[i], p, buf + sz - p);
		if(NULL != list[i+1])
			p += snprintf(p, buf + sz - p, ",");   /* not the last one, add seperator */
	}
	snprintf(p, buf + sz - p, ")");
	return buf;
}
dalvik_type_t* dalvik_type_clone(const dalvik_type_t* type)
{
	if(NULL == type) return NULL;
	if(DALVIK_TYPE_IS_ATOM(type->typecode))  
		return (dalvik_type_t*) type;   /* reuse all atmoic type, we need to modify some value */
	dalvik_type_t *ret = NULL;
	switch(type->typecode)
	{
		case DALVIK_TYPECODE_OBJECT:
			ret = _dalvik_type_alloc(DALVIK_TYPECODE_OBJECT);
			if(NULL == ret)
			{
				LOG_ERROR("can not allocate memory for the new copy of type %s", dalvik_type_to_string(type, NULL, 0));
				return NULL;
			}
			ret->data.object.path = type->data.object.path;
			return ret;
		case DALVIK_TYPECODE_ARRAY:
			ret = _dalvik_type_alloc(DALVIK_TYPECODE_ARRAY);
			if(NULL == ret)
			{
				LOG_ERROR("can not allocate memory for the new copy of type %s", dalvik_type_to_string(type, NULL, 0));
				return NULL;
			}
			ret->data.array.elem_type = dalvik_type_clone(type->data.array.elem_type);
			return ret;
		default:
			LOG_ERROR("unknown type, do not clone");
			return NULL;
	}
}

const dalvik_type_t**  dalvik_type_list_clone(const dalvik_type_t * const * type)
{
	if(NULL == type) return NULL;
	int i;
	for(i = 0; type[i] != NULL; i ++);
	size_t sz = sizeof(dalvik_type_t*) * (i + 1);
	const dalvik_type_t** ret = (const dalvik_type_t**)malloc(sizeof(dalvik_type_t*) * (i+1));
	memset(ret, 0, sz);
	if(NULL == ret) 
	{
		LOG_ERROR("can not allocate memory for type array");
		goto ERROR;
	}
	ret[i] = NULL;
	for(i --; i >=0; i --)
	{
		ret[i] = dalvik_type_clone(type[i]);
		if(NULL == ret[i]) 
		{
			LOG_ERROR("can not clone type %s", dalvik_type_to_string(type[i], NULL, 0));
			goto ERROR;
		}
	}
	return ret;
ERROR:
	LOG_ERROR("can not clone the type list %s", dalvik_type_list_to_string(type, NULL, 0));
	if(NULL != ret)
	{
		for(i = 0; NULL != type[i]; i ++)
			if(ret[i] != NULL)
				dalvik_type_free((dalvik_type_t*)ret[i]);
		free(ret);
	}
	return NULL;
}

void dalvik_type_list_free(dalvik_type_t **list)
{
	int i;
	for(i = 0; NULL != list[i]; i ++)
		dalvik_type_free(list[i]);
	free(list);
}
