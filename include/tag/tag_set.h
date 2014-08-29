/**
 * @file tag_set.h
 * @brief the type for tagsset. 
 **/
#ifndef __TAG_SET_H__
#define __TAG_SET_H__
#include <tag/tag_def.h>
#include <dalvik/dalvik.h>

/**
 * @breif the data structure for a tag set
 **/
typedef struct _tag_set_t tag_set_t;

/**
 * @brief the callback function used to check wether or not specified tag is malicous with current closure
 * @param tagid the tag id
 * @param resolution the resolution
 * @param class the class name
 * @param method the method name
 * @param signature the signature of the function
 * @param the return type of the function
 * @param buf_why a optional buffer which carries the human readable information about why this is malicious
 * @return > 0 if it's possibly malicious, the result is actually a reason code; = 0 if not; < 0 if error
 **/
typedef int (*tag_set_checker_callback_t)(
		uint32_t tagid, 
		uint32_t resolution,
		const char* class, 
		const char* method, 
		const dalvik_type_t* const * signature,
		const dalvik_type_t* return_type);
/**
 * @brief the callback function that converts a reason code to human readable description
 * @param tagid the tag id
 * @param why the reason code
 * @return the description string, NULL if error
 **/
typedef const char* (*tag_set_strreason_callback_t)(uint32_t tagid, int why);


/**
 * @brief initialize this file
 * @return < 0 indicates error
 **/
int tag_set_init();

/**
 * @brief finalization
 * @return nothing
 **/
void tag_set_finalize();

/**
 * @brief make a new empty tag set
 * @return the newly created empty set
 **/
tag_set_t* tag_set_empty();

/**
 * @brief make a new tag set from array
 * @param tags the tag id array
 * @param resols the resolution array
 * @param N the number of members
 * @note NOTICE that the tags array must be sorted
 * @return the result tag set
 **/
tag_set_t* tag_set_from_array(const uint32_t* tags, const uint32_t* resols, size_t N);

/**
 * @brief fork this tag set
 * @param set the tag set
 * @return the newly forked tag set, NULL indicates error
 **/
tag_set_t* tag_set_fork(const tag_set_t* set);

/**
 * @brief free this tag set
 * @param set the tag set to free
 * @return nothing
 **/
void tag_set_free(tag_set_t* set);

/**
 * @brief hash code of this tag set
 * @param set the input tag set
 * @return hashcode of this tag set
 **/
hashval_t tag_set_hashcode(const tag_set_t* set);

/**
 * @brief check wether or not two tag set are equal
 * @param first the first tag set
 * @param second the second tag set
 * @return 1 if they are equal, 0 if not equal, < 0 if error happens
 **/
int tag_set_equal(const tag_set_t* first, const tag_set_t* second);

/**
 * @brief merge two tag set
 * @param first the first tag set
 * @param second the second tag set
 * @return the newly created result tag set
 **/
tag_set_t* tag_set_merge(const tag_set_t* first, const tag_set_t* second);

/**
 * @brief set resolution of this tag
 * @param set the target tag set
 * @param tagid which tag to be blurred
 * @param value the new resolution value
 * @return the pointer to the tag set after modification, NULL indicates error
 **/
tag_set_t* tag_set_change_resolution(tag_set_t* set, uint32_t tagid, uint32_t value);

/**
 * @brief register a checker for specify tag
 * @param tagid the tagid
 * @param callback the callback fuction
 **/
void tag_set_register_tag_handler(uint32_t tagid, tag_set_checker_callback_t checker, tag_set_strreason_callback_t strreason);

/**
 * @brief check wether or not this function call is malicious
 * @param set the tag set
 * @param class the class path
 * @param method the method name
 * @param sig the signature
 * @param rtype the return type
 * @return a reason code for why this is malicious(the reason code actually (TAGID << 16)|(TAG_REASON_CODE) ), < 0 when error happens
 **/
int tag_set_check_malicious(
		const tag_set_t* set, 
		const char* class, 
		const char* method,
		const dalvik_type_t* const * sig,
		const dalvik_type_t* rtype);

/**
 * @brief convert the reason code to human readable string
 * @param why the reason code
 * @return the human readable code
 **/
const char* tag_set_reason_code_to_string(int why);


#endif
