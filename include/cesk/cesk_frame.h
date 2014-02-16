#ifndef __CESK_FRAME_H__
#define __CESK_FRAME_H__
#include <cesk/cesk_set.h>

typedef struct {
    uint16_t       size;     /* the number of registers in this frame */
    cesk_store_t*  store;    /* the store for this frame */
    cesk_set_t*    regs[0];  /* register values */
} cesk_frame_t;

/* duplicate the frame */
cesk_frame_t* cesk_frame_fork(cesk_frame_t* frame);

/* merge two frame, dest <- dest + sour */
//int cesk_frame_merge(cesk_frame_t* dest, const cessk_frame_t* sour);

/* create an empty stack which is empty */
cesk_frame_t* cesk_frame_new(uint16_t size);

/* despose a cesk frame */
void cesk_frame_free(cesk_frame_t* frame);

/* compare if two stack frame are equal */
int cesk_frame_equal(cesk_frame_t* first, cesk_frame_t* second);

/* garbage collect on a frame */
int cesk_frame_gc(cesk_frame_t* frame);

#endif /* __CESK_FRAME_H__ */
