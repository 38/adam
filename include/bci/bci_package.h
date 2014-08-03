/**
 * @brief Bulitin Class Package
 * @details Each API is a BCI Package
 */
#ifndef __BCI_PACKAGE_H__
#define __BCI_PACKAGE_H__
#include <constants.h>
#include <const_assertion.h>

#include <log.h>

#include <bci/bci_nametab.h>

/**
 * @brief initalize the API package
 * @return < 0 indicates an error 
 **/
int bci_package_init();

/**
 * @brief finalize the API package
 * @return nothing
 **/
void bci_package_finalize();
#endif /*__BCI_PACKAGE_H__*/
