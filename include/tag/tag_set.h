/**
 * @file tag_set.h
 * @brief the type for tagsset. 
 **/
#ifndef __TAG_SET_H__
#define __TAG_SET_H__
#include <tag/tag_def.h>
#include <dalvik/dalvik.h>

/**
 * @brief the data structure for a tag set
 **/
typedef struct _tag_set_t tag_set_t;

/**
 * @brief the callback function that converts a reason code to human readable description
 * @param tagid the tag id
 * @param why the reason code
 * @return the description string, NULL if error
 **/
typedef const char* (*tag_set_strreason_callback_t)(uint32_t tagid, int why);

/**
 * @brief the call backfunction that converts a tag to human readable string
 * @param tagid the tag id
 * @return the description string
 **/
typedef const char* (*tag_set_to_string_callback_t)(uint32_t tagid);


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
 * @param to_string the to_string callback
 * @param strreason the callback fuction
 * @return nothing
 **/
void tag_set_register_handler(uint32_t* tagid, tag_set_to_string_callback_t to_string, tag_set_strreason_callback_t strreason);

/**
 * @brief report this function invokation is malicious
 * @param why the reasoncode
 * @param class the class path
 * @param method the method name
 * @param sig the signature
 * @param rtype the return type
 * @return a reason code for why this is malicious(the reason code actually (TAGID << 16)|(TAG_REASON_CODE) ), < 0 when error happens
 **/
int tag_set_report_malicious(
		const uint32_t why,
		const char* class, 
		const char* method,
		const dalvik_type_t* const * sig,
		const dalvik_type_t* rtype);

/**
 * @brief convert the reason code to human readable string
 * @param why the reason code
 * @return the human readable code
 **/
const char* tag_set_reason_code_to_string(uint32_t why);

/**
 * @brief get the size of the tag set, for debugging only
 * @param set the tag set
 * @return the size of the tag set
 **/
size_t tag_set_size(const tag_set_t* set);

/**
 * @brief get the tag-id of the k-th tag in the tag set. for debugging only
 * @param set the tag set
 * @param k which one do you want
 * @return the tag id of the element in tag set
 **/
uint32_t tag_set_get_tagid(const tag_set_t* set, uint32_t k);

/**
 * @brief get the resolution value of the k-th tag in the tag set, for debuggin only
 * @param set the tag set
 * @param k which one do you want
 * @return the tag resolution value of the element in tag set
 **/
uint32_t tag_set_get_resol(const tag_set_t* set, uint32_t k);

/**
 * @brief compute the hashcode, this is used to verify the hashcode function works properly
 * @param set the tag set
 * @return the result hashcode
 **/
hashval_t tag_set_compute_hashcode(const tag_set_t* set);
/**
 * @param convert the tag set to a human readable string
 * @pparam ts the tag set
 * @param buf the output buffer
 * @param sz the size
 * @return the result string
 **/
const char* tag_set_to_string(const tag_set_t* ts, char* buf, size_t sz);
int tag_set_contains(tag_set_t* set, uint32_t what);
#endif
