/**
 * @file tag_def
 * @brief the defination of tag indexes
 **/
#ifndef __TAG_DEF_H__
#define __TAG_DEF_H__
/**
 * @brief indexes of tags supported
 **/
enum {
	TAG_FILENAME,     /*!< set if this value can be a filename */
	TAG_FILECONTENT,  /*!< set if this value can be a filecontent */
	/* Add new tags here */
	TAG_NUM_OF_TAGS   /*!< how many tags supported */
};
/**
 * @brief resolution level of a tag
 **/
enum {
	TAG_RES_NOTHING = 0x0,  /*!< the vlaue carries no information relocated to this tag */
	TAG_RES_PARTIAL = 0x1,  /*!< the value only carries partial information to rebuild the data */
	TAG_RES_FULL    = 0x2,     /*!< the value carries all information to rebuild the data the tag concerns */
	TAG_RES_EXACT   = 0x4,    /*!< the value is exactly what this tag descriibe */
};
#endif
