#ifndef __CESK_OBJECT_H__
#define __CESK_OBJECT_H__
#include <constants.h>

#include <cesk/cesk_value.h>
#include <dalvik/dalvik_class.h>
/* a abstract object value */
typedef struct {
    //const char*              classpath;   /* the class path of this object */
	const dalvik_class_t*	 class;
    size_t                   num_members; /* the number of members */
    uint32_t                 valuelist[0];  /* the value of the member */
} cesk_object_struct_t;  

typedef struct {
    uint16_t         depth;      /* the depth in inherence tree */
    cesk_object_struct_t  members[0]; /* the length of the tree */
} cesk_object_t;


/* Create a new instance object of class in <classpath> */
cesk_object_t*   cesk_object_new(const char* classpath);
/* Get address of a dynamic field in an object, 
 * this function is to return a reference to the cell contains the value*/
uint32_t* cesk_object_get(cesk_object_t* object, const char* classpath, const char* field);

/* free the object */
void cesk_object_free(cesk_object_t* object);

/* make a copy of the object */
cesk_object_t* cesk_object_fork(cesk_object_t* object);
/* get the hash code of an object */
hashval_t cesk_object_hashcode(cesk_object_t* object); 

/* compare two object */
int cesk_object_equal(cesk_object_t* first, cesk_object_t* second);

/* the classpath of the object */
static inline const char* cesk_object_classpath(cesk_object_t* object)
{
    return object->members[0].class->path;
}
/* print out the data of the object */
const char* cesk_object_to_string(cesk_object_t* object, char* buf, size_t sz);

/* check if the object is a instantce of the class */
int cesk_object_instance_of(cesk_object_t* object, const char* classpath);
/* compute a non-incremental style hashcode, only for debugging */
hashval_t cesk_object_compute_hashcode(cesk_object_t* object);
#endif
