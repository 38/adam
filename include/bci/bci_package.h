#ifndef __BCI_PACKAGE_H__
#define __BCI_PACKAGE_H__
#include <constant.h>
#include <const_assertion.h>

#include <log.h>

#include <bci/bci_nametab.h>
/**
 * @brief the type for a buildin class package 
 * @note in the design, different pacakge implmements different version of APIs
 **/
typedef struct {
	struct {
		uint32_t maj;      /*!< the major version number */
		uint32_t min;      /*!< the minor version number */
	} api_version;         /*!< the api version this package provides */
	bci_nametab_t* ntab;   /*!< the name lookup table */
} bci_package_t;
#endif /*__BCI_PACKAGE_H__*/
