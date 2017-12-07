#ifndef __CHIPS_COMMON_H__
#define __CHIPS_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CS1073
//#define CS1175
#if (defined(CS1073)||defined(CS1175))
	#define USE_16_BITS  //默认16bit
	#define CALIBRATION
#endif

//#define CS3105
#if defined(CS3105)
	#define USE_16_BITS  //默认16bit
#endif

//#define CS336
#if defined(CS336)
	#define USE_8_BITS
	//#define USE_12_BITS
	//#define USE_16_BITS
#endif

//#define CS358
#if defined(CS358)
#define USE_8_BITS
#endif 

//#define CS3511
#if defined(CS3511)
#define USE_8_BITS
//#define USE_12_BITS
//#define USE_16_BITS
#endif


#define COATING
//#define GLASS
//#define CERAMIC


#define CHECK_FINGER_UP  //检测手指抬起
#define OPEN_ERROR_TAG   //匹配错误时返回错误信息
#define NO_SAME_SPACE    //录入时重复区域判断
#define NO_REPEAT_ENROLL //重复手指录入判断
//#define DISPLAY_SIG      //是否有亮灭屏信号

//#define ANDROID_7
//#define FOR_Ali

//#define SAMPLE_DEBUG

void chips_init_lib(void);
int chips_get_fid(int *fid);
void chips_load_fingerprints();
int chips_prepare_enroll(void);
int chips_enroll_finger(int *remaining_times);
int chips_update_template(int fid);
int chips_verify(int *matched_fid);
int chips_renew_feature(int fid);
int chips_delete_finger(int fid);
int chips_get_fingernum(void);
void chips_get_data_from_mtp(void);

void chips_auto_calibration(void);

#ifdef __cplusplus
}
#endif

#endif