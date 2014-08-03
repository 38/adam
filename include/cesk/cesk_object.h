#ifndef __CESK_OBJECT_H__
#define __CESK_OBJECT_H__
/**
 * @file cesk_object.h 
 * @brief defination of an abstract object 
 **/

typedef struct _cesk_object_t cesk_object_t;
#include <log.h>

#include <const_assertion.h>
#include <constants.h>
#include <cesk/cesk_value.h>
#include <cesk/cesk_store.h>
#include <dalvik/dalvik_class.h>
#include <bci/bci_class.h>
/**
 * @brief an abstract object value
 *
 * @details A object struct contains all field defined in the class file.
 * 		   But the class may have superclass and we have to allocate memory
 *         for the it, a signle abstruct object value can not 
 *         reperecent requirement for the object. Therefore we need a 
 *         list of abstruct object struct(cesk_object_t).
 **/
typedef struct {
	union{
		const dalvik_class_t*	 udef;		   /*!< the user defined class*/
		const bci_class_wrap_t*       bci;          /*!< the built-in class interface */
		const struct {
			const char* value;                 /*!< value of class path */
		}*path;                                /*!< the class path */
		void*                    generic;      /*!< the generic pointer */
	} __attribute__((__packed__)) class;       /*!<the class of this object struct*/
	uint32_t                 num_members:31;   /*!<the number of members, if it's a built-in class, this is the size of the data area */
	uint8_t                  built_in:1;       /*!<is this object a built-in class*/
	uint32_t                 addrtab[0];       /*!<the value of the member */
	char                     bcidata[0];       /*!< the storage for built-in class */
} cesk_object_struct_t; 
CONST_ASSERTION_SIZE(cesk_object_struct_t, addrtab, 0);
CONST_ASSERTION_LAST(cesk_object_struct_t, addrtab);
CONST_ASSERTION_SIZE(cesk_object_struct_t, bcidata, 0);
CONST_ASSERTION_LAST(cesk_object_struct_t, bcidata);
/**
 * @brief An abstruct object
 *
 * @details The object is actually a list of object struct which contains all
 * 		   memory that allocate for the superclasses of the object
 **/
struct _cesk_object_t {
	uint16_t         	  depth;      /*!<the depth in inherence tree */
	size_t				  size;       /*!<the size of the object useful when clone an object */
	size_t                nbuiltin;   /*!<the number of builtin classes */
	cesk_object_struct_t* builtin;    /*!< a pointer to the built-in class structure, if this object has one */
	cesk_object_struct_t  members[0]; /*!<the length of the tree */
};
CONST_ASSERTION_LAST(cesk_object_t, members);
CONST_ASSERTION_SIZE(cesk_object_t, members, 0);
/** 
 * @brief this macro return the offset of a given field pointer to the 
 *        beginging of the object, it's used when allocating a new field
 *        value, because one object can have many value set, their address
 *        must not be same
 * @param object 
 * @param p_field the pointer to the field
 * @return the offset of the field 
 **/
#define CESK_OBJECT_FIELD_OFS(object, p_field) ((uint32_t)(((char*)(p_field)) - ((char*)(object))))
/**
 * @brief goto the next object struct 
 * @param cur the current struct
 **/
#define CESK_OBJECT_STRUCT_ADVANCE(cur) do{\
	if(0 == cur->built_in)\
		cur = (typeof(cur))((cur)->addrtab + (cur)->num_members);\
	else\
		cur = (typeof(cur))((cur)->bcidata + (cur)->num_members);\
} while(0)

/**
 * @brief Create a new instance object of class in classpath 
 * @param classpath the class path of the object
 * @return the object created from the object, NULL indicates error
 */
cesk_object_t* cesk_object_new(const char* classpath);
/**
 * @brief Get address of a dynamic field in an object, return a reference that contains 
 *        the store address of the value set. You can use this address to change the 
 *        set
 * @param object the object
 * @param classpath the class path
 * @param field_name the field name
 * @param p_bci_class if this class is a built-in class, NULL will be returned and the bci_class will
 *        return from this variable
 * @param p_bci_data if this class is a built-in class, NULL will  be retuend from the function and
 *        the pointer to instance data will be stored in this variable
 * @return the pointer constains set address in the store
 */
uint32_t* cesk_object_get(
		cesk_object_t* object, 
		const char* classpath, 
		const char* field_name, 
		const bci_class_t** p_bci_class,
		void** p_bci_data);

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
	return object->members[0].class.path->value;
}
/**
 * @brief print out the data of the object 
 * @param object
 * @param buf the output buffer, if buf is NULL, use default buffer
 * @param sz size of buffer
 * @param brief if it's not 0, print the class briefly
 * @return the result string 
 */
const char* cesk_object_to_string(const cesk_object_t* object, char* buf, size_t sz, int brief);

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
 *        to the store address, it returns the store address directly
 * @param object the object
 * @param classpath the class path
 * @param field_name the field name of the class
 * @param p_bci_class if the class is a built-in class, p_bci_class will be used for storing a pointer to the built-in class def (and return -1)
 * @param p_bci_data  if the class is a built-in class, p_bci_data will be used for storing a pointer to the instance of this class (and return -1)
 * @param buf the address buf 
 * @return <0 error otherwise success
 */
static inline int cesk_object_get_addr(
		const cesk_object_t* object, 
		const char* classpath, 
		const char* field_name, 
		const bci_class_t** p_bci_class,
		const void** p_bci_data,
		uint32_t* buf)
{
	uint32_t* ret = cesk_object_get((cesk_object_t*)object, classpath, field_name, p_bci_class, (void**)p_bci_data);
	if(NULL == ret) return -1;
	*buf = *ret;
	return 0;
}
#endif
