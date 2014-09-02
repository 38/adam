/**
 * @file tag_fs.c
 * @brief Filesystem Related Tags
 **/
#include <tag/tag_def.h>
#include <tag/tag_set.h>
const char* tag_fs_strreason(uint32_t id, int why)
{
	return "not malicious";
}
void tag_fs_init()
{
	tag_set_register_handler(TAG_FILENAME, tag_fs_strreason);
}

