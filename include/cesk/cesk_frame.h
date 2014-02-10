#ifndef __CESK_FRAME_H__
#define __CESK_FRAME_H__
#include <cesk/cesk_set.h>

typedef struct {
    uint16      size;    /* the number of registers in this frame */
    cesk_set_t* regs[0]; /* register values */
} cesk_frame_t;

/* duplicate the frame */
cesk_frame_t* cesk_frame_fork(cesk_frame_t* frame);

/* merge two frame, dest <- dest + sour */
int cesk_frame_merge(cesk_frame_t* dest, const cessk_frame_t* sour);


#endif /* __CESK_FRAME_H__ */
