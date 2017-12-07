#ifndef __CHIPS_FINGERPRINT_H__
#define __CHIPS_FINGERPRINT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define bool int
#define false 0
#define true  1

enum RET {
	RET_ACTIVE_FINGER_LEAVE_IRQ = -10,
	RET_INVALID_SEM = -9,
	RET_MISTOUCH = -8,
	RET_ACTIVE_IDLE_IRQ = -7,
    RET_SAME_SPACE = -6,
    RET_FINGER_EXIST = -5,
	RET_KEY1 = -4,
	RET_KEY2 = -3,
	RET_RESET_IRQ = -2,
    RET_ERROR = -1,
    RET_OK = 0,	
};

#ifdef __cplusplus
}
#endif

#endif


