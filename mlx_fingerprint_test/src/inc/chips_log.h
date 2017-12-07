#ifndef _CHIPS_LOG_H
#define _CHIPS_LOG_H
#include <stdio.h>

#define ALOGE(fmt,...) printf("\033[1;31m[Error]  %s(%d)"fmt"\033[0m\r\n",__FILE__,__LINE__,__VA_ARGS__)
#define ALOGD(fmt,...) printf("\033[1m[Debug]\033[0m  %s(%d)"fmt"\r\n",__FILE__,__LINE__,__VA_ARGS__)
#define ALOGI(fmt,...) printf("\033[1m[Info]\033[0m   %s(%d)"fmt"\r\n",__FILE__,__LINE__,__VA_ARGS__)
#define ALOGW(fmt,...) printf("\033[1m[Warn]\033[0m   %s(%d)"fmt"\r\n",__FILE__,__LINE__,__VA_ARGS__)

#endif  /*_CHIPS_LOG_H*/
