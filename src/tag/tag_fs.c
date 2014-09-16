/**
 * @file tag_fs.c
 * @brief Filesystem Related Tags
 **/
#include <tag/tag_def.h>
#include <tag/tag_set.h>
uint32_t TAG_FILENAME;
uint32_t TAG_FILECONTENT;
const char* tag_fs_strreason(uint32_t id, int why)
{
	return "not malicious";
}
const char* tag_fs_tostring(uint32_t id)
{
	if(id == TAG_FILENAME)
		return "file-name";
	else if(id == TAG_FILECONTENT)
		return "file-content";
	return "invalid-tag";

}

void tag_fs_init()
{
	tag_set_register_handler(&TAG_FILENAME, tag_fs_tostring, tag_fs_strreason);
	tag_set_register_handler(&TAG_FILECONTENT, tag_fs_tostring, tag_fs_strreason);
}

