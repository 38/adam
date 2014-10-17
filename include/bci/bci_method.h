/**
 * @file bci_method.h
 * @brief the adaptor tanslate the BCI calls to CESK method calls
 **/

#include <constants.h>
#include <const_assertion.h>

#include <log.h>

#include <cesk/cesk.h>
#include <dalvik/dalvik_instruction.h>

#include <bci/bci_interface.h>

#ifndef __BCI_METHOD_H__
#define __BCI_METHOD_H__

/**
 * @brief an environment
 **/
struct _bci_method_env_t {
	cesk_reloc_table_t*       rtable;   /*!< relocation table */
	cesk_frame_t*             frame;    /*!< envrion store */
	const dalvik_instruction_t* instruction; /*!< the instruction that calls this  function */
	cesk_diff_buffer_t*       D;        /*!< the diff buffer */
	const void* context;
} __attribute__((__packed__));
#endif
