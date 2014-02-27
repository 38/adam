#ifndef __CESK_OBJECT_H__
#define __CESK_OBJECT_H__
/**@file cesk_object.h 
 * @brief defination of an abstract object 
 */
#include <constants.h>
typedef struct _cesk_object_t cesk_object_t;
#include <cesk/cesk_value.h>
#include <cesk/cesk_reloc.h>
#include <dalvik/dalvik_class.h>
/**
 * @brief an abstract object value
 *
 * @details A object struct contains all field defined in the class file.
 * 		   But the class may have super class and we have to allocate memory
 *         for the superclass, a signle abstruct object value can not 
 *         reperecent required for the object. Therefore we need a 
 *         list of abstruct object struct(cesk_object_t).
 */
typedef struct {
    //const char*              classpath;   /* the class path of this object */
	const dalvik_class_t*	 class;		/*!<the class of this object struct*/
    size_t                   num_members; /*!<the number of members */
    uint32_t                 valuelist[0];  /*!<the value of the member */
} cesk_object_struct_t;  
/**
 * @brief An abstruct object
 *
 * @details The object is actually a list of object struct which contains all
 * 		   memory that allocate for the superclasses of the object
 */
struct _cesk_object_t {
    uint16_t         depth;      /*!<the depth in inherence tree */
    cesk_object_struct_t  members[0]; /*!<the length of the tree */
};


/**
 * @brief Create a new instance object of class in classpath 
 * @param classpath the class path of the object
 * @return the object created from the object, NULL indicates error
 */
cesk_object_t*   cesk_object_new(const char* classpath);
/**
 * @brief Get address of a dynamic field in an object, return a reference that contains 
 *        the store address of the value set. You can use this address to change the 
 *        set
 * @param object the object
 * @param classpath the class path
 * @param field the field name
 * @return the pointer constains set address
 */
uint32_t* cesk_object_get(cesk_object_t* object, const char* classpath, const char* field);

/**
 * @brief free the object 
 * @param object the object to release
 * @return nothing
 */
void cesk_object_free(cesk_object_t* object);

/**
 * @brief make a copy of the object 
 *
 * @details this fork function is an actual copy function which
 * 		   copy the function immedately.
 * @param object the source object
 * @return the forked object
 */
cesk_object_t* cesk_object_fork(const cesk_object_t* object);
/**
 * @brief get the hash code of an object 
 * @param object
 * @return the hash code
 */
hashval_t cesk_object_hashcode(const cesk_object_t* object); 

/**
 * @brief return wether or not two object are equal
 * @param first
 * @param second
 * @return 1 for first == second, 0 for first != second
 */
int cesk_object_equal(const cesk_object_t* first, const cesk_object_t* second);

/**
 * @brief the classpath of the object 
 * @param object
 * @return the class path of the object
 */
static inline const char* cesk_object_classpath(const cesk_object_t* object)
{
    return object->members[0].class->path;
}
/**
 * @brief print out the data of the object 
 * @param object
 * @param buf the output buffer, if buf is NULL, use default buffer
 * @param sz size of buffer
 * @return the result string 
 */
const char* cesk_object_to_string(const cesk_object_t* object, char* buf, size_t sz);

/**
 * @brief check if the object is a instantce of the class 
 * @param object the object
 * @param classpath the class path of the class
 * @return the result of the check
 */
int cesk_object_instance_of(const cesk_object_t* object, const char* classpath);
/** 
 * @brief compute a non-incremental style hashcode, only for debugging 
 * @param object
 * @return hash code
 */
hashval_t cesk_object_compute_hashcode(const cesk_object_t* object);
/**
 * @brief get set address of the field, this function like 
 *        cesk_object_get, but instead of returning the pointer
 *        to the store address, it returns the address directly
 * @param object the object
 * @param classpath the class path
 * @param field_name the field name of the class
 * @param buf the address buf 
 * @return <0 error otherwise success
 */
static inline int cesk_object_get_addr(const cesk_object_t* object, const char* classpath, const char* field_name, uint32_t* buf)
{
	uint32_t* ret = cesk_object_get((cesk_object_t*)object, classpath, field_name);
	if(NULL == ret) return -1;
	*buf = *ret;
	return 0;
}
/**
 * @brief merge the source object to the destination object,
 *        and look for the relocate table, if there's some address 
 *        which should be relocate, reloate it first
 * @param dest destination object
 * @param sour source object
 * @param reloc_table the relocation table apply during the merge
 * @return indicates if the operation success >= 0 means no error
 * @todo  Q:is this useful?
 **/
int cesk_object_merge_reloc(cesk_object_t* dest, const cesk_object_t* sour, cesk_reloc_table_t* reloc_table);
#endif
