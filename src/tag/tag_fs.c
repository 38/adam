/**
 * @file tag_fs.c
 * @brief Filesystem Related Tags
 **/
#include <tag/tag_def.h>
#include <tag/tag_set.h>
int tag_fs_filename_check(
		uint32_t id, 
		uint32_t res, 
		const char* class, 
		const char* method, 
		const dalvik_type_t * const * signature, 
		const dalvik_type_t* rtype)
{
	return 0;
}
const char* tag_fs_strreason(uint32_t id, int why)
{
	return "not malicious";
}
void tag_fs_init()
{
	tag_set_register_handler(TAG_FILENAME, tag_fs_filename_check, tag_fs_strreason);
}

