/**
 * @brief the built-in class interface for adam
 **/
#ifndef __BCI_H__
#include <bci/bci_package.h>
#include <bci/bci_nametab.h>
#include <bci/bci_class.h>
#include <bci/bci_method.h>

/**
 * @brief init the built-in class interface
 * @return result of init < 0 indicates error
 **/
int bci_init();
/**
 * @brief finalize the built-in class interface
 * @return nothing
 **/
void bci_finalize();
#endif  
