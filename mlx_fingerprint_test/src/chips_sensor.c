/*********************************************************************************
 * Copyright (C) ShenZhen ChipSailing Technology Co., Ltd. All rights reserved.
 * 
 * Filename: chips_sensor.c
 * 
 * Author: zwp    ID: 58    Version: 2.0   Date: 2016/10/16
 * 
 * Description: HAL层与驱动交互的相关接口函数
 * 
 * Others: 
 * 
 * History: 
 *     1. Date:           Author:          ID: 
 *        Modification:  
 * 
 *  
 * 
 *********************************************************************************/

#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h> //esd

#include "./inc/chips_sensor.h"
#include "./inc/chips_api.h"
#include "./inc/chips_fingerprint.h"
#include "./inc/chips_log.h"

#define CHIPS_DEV_NAME "/dev/cs_spi"

#if defined(CS358)
static unsigned short vcm_value = 0;
static unsigned short fc3e_value = 0;
static unsigned char thr_value = 0;
#endif

static sem_t g_sem;
static int g_fd = -1;

struct chips_ioc_transfer {
	unsigned char cmd;
	unsigned short addr;
	unsigned long buf;
	unsigned short actual_len;
};

struct param {
	unsigned char cmd;
	unsigned short addr;
	unsigned short data;
};

struct config {
	unsigned long p_param;
	int num;
};

struct chips_key_event {
	int code;
	int value;
};

#define MODE_NORMAL 0x51
#define MODE_SLEEP  0x72
#define MODE_DSLEEP 0x76
#define MODE_IDLE   0x70

#define CHIPS_IOC_MAGIC			'k'
#define CHIPS_IOC_INPUT_KEY_EVENT	         _IOW(CHIPS_IOC_MAGIC, 7, struct chips_key_event)
#define CHIPS_IOC_SET_RESET_GPIO       	   _IOW(CHIPS_IOC_MAGIC,8,unsigned char) 
#define CHIPS_IOC_SPI_MESSAGE	             _IOWR(CHIPS_IOC_MAGIC, 9,struct chips_ioc_transfer)
#define CHIPS_IOC_SPI_SEND_CMD             _IOW(CHIPS_IOC_MAGIC,10,unsigned char)
#define CHIPS_IOC_ENABLE_IRQ               _IO(CHIPS_IOC_MAGIC,11)
#define CHIPS_IOC_DISABLE_IRQ              _IO(CHIPS_IOC_MAGIC,12)
#define CHIPS_IOC_SENSOR_CONFIG            _IOW(CHIPS_IOC_MAGIC,13,unsigned long)

#define CHIPS_W_SRAM 0xAA
#define CHIPS_R_SRAM 0xBB
#define CHIPS_W_SFR  0xCC
#define CHIPS_R_SFR  0xDD

#define REG_42_VAULE 0x77

unsigned char thr_val = 0;

/************************
 IC配置参数
 *************************/
struct param param[] = {

#if defined(CS3105)
		/*
		 {CHIPS_W_SFR,0x4d,0x04},    // clear reset irq
		 {CHIPS_W_SFR,0x42,0x9C},
		 {CHIPS_W_SFR,0x89,0x00},
		 {CHIPS_W_SFR,0x60,0x09},
		 {CHIPS_W_SFR,0x55,0x00},
		 {CHIPS_W_SFR,0x63,0x60},
		 {CHIPS_W_SFR,0x46,0x70},
		 {CHIPS_W_SFR,0x47,0x00},
		 //---------adc change --------------
		 {CHIPS_W_SRAM,0xfc80,0x000f},   //adc_bias ==============
		 //------------------------------------
		 {CHIPS_W_SRAM,0xfc3e,0x6007},
		 {CHIPS_W_SRAM,0xfc02,0x1d56},   // pwr_en af==============
		 {CHIPS_W_SRAM,0xfc3c,0xb001},   //comp_en,dac_en,adc_ref_en
		 //----------for base ----------------------
		 {CHIPS_W_SRAM,0xfc2a,0x0000},   //base_en for passive default is 5 ,close the base
		 //--------base_end------------------------------

		 */

		{	CHIPS_W_SFR,0x42,0x9c},

		{	CHIPS_W_SFR,0x05,0x00},
		{	CHIPS_W_SFR,0x07,0x80},
		{	CHIPS_W_SFR,0x08,0x00},

		{	CHIPS_W_SFR,0x89,0x00},
		{	CHIPS_W_SFR,0x4d,0x04},
		{	CHIPS_W_SFR,0x55,0x00},

		{	CHIPS_W_SRAM,0xfc80,0x000f},   //adc_bias ==============

		{	CHIPS_W_SRAM,0xfc3e,0x200c},
		{	CHIPS_W_SRAM,0xfc02,0x1d66},   // pwr_en af==============

		{	CHIPS_W_SRAM,0xfc3c,0xb001},   //comp_en,dac_en,adc_ref_en
		{	CHIPS_W_SRAM,0xfc34,0x0410},

		{	CHIPS_W_SRAM,0xfc32,0x1e8b},   //for high vol sel
		{	CHIPS_W_SRAM,0xfc38,0x0003},
		{	CHIPS_W_SRAM,0xfc3a,0xaaaa},   // pwr_en af==============

		{	CHIPS_W_SRAM,0xfc2a,0x0000},
		{	CHIPS_W_SRAM,0xfc84,0x4501},
		{	CHIPS_W_SRAM,0xfc82,0x083e},

		{	CHIPS_W_SFR,0x47,0x00},
		{	CHIPS_W_SFR,0x63,0x60},
		{	CHIPS_W_SFR,0x46,0x70},
#if defined(USE_16_BITS)
		{	CHIPS_W_SFR,0x60,0x09},
#elif defined(USE_8_BITS)
		{	CHIPS_W_SFR,0x60,0x00},
#endif

		/***********************************************
		 新12点布局，根据布局图配置地址寄存器
		 [   8  9   ]
		 [2  0  1  3]
		 [6  4  5  7]
		 [   A  B   ]
		 ***********************************************/
		{	CHIPS_W_SRAM,0xFC40, 0x0328},   // Area 0 L
		{	CHIPS_W_SRAM,0xFC42, 0x012F},   // Area 0 H

		{	CHIPS_W_SRAM,0xFC44, 0x0340},   // Area 1 L
		{	CHIPS_W_SRAM,0xFC46, 0x0147},   // Area 1 H

		{	CHIPS_W_SRAM,0xFC48, 0x0308},   // Area 2 L
		{	CHIPS_W_SRAM,0xFC4A, 0x010f},   // Area 2 H

		{	CHIPS_W_SRAM,0xFC4C, 0x0364},   // Area 3 L
		{	CHIPS_W_SRAM,0xFC4E, 0x016B},   // Area 3 H

		{	CHIPS_W_SRAM,0xFC50, 0x0528},   // Area 4 L
		{	CHIPS_W_SRAM,0xFC52, 0x012F},   // Area 4 H

		{	CHIPS_W_SRAM,0xFC54, 0x0540},   // Area 5 L
		{	CHIPS_W_SRAM,0xFC56, 0x0147},   // Area 5 H

		{	CHIPS_W_SRAM,0xFC58, 0x0508},   // Area 6 L
		{	CHIPS_W_SRAM,0xFC5A, 0x010f},   // Area 6 H

		{	CHIPS_W_SRAM,0xFC5C, 0x0564},   // Area 7 L
		{	CHIPS_W_SRAM,0xFC5E, 0x016B},   // Area 7 H

		{	CHIPS_W_SRAM,0xFC60, 0x0128},   // Area 8 L
		{	CHIPS_W_SRAM,0xFC62, 0x012F},   // Area 8 H

		{	CHIPS_W_SRAM,0xFC64, 0x0140},   // Area 9 L
		{	CHIPS_W_SRAM,0xFC66, 0x0147},   // Area 9 H

		{	CHIPS_W_SRAM,0xFC68, 0x0728},   // Area A L
		{	CHIPS_W_SRAM,0xFC6A, 0x012F},   // Area A H

		{	CHIPS_W_SRAM,0xFC6C, 0x0740},   // Area B L
		{	CHIPS_W_SRAM,0xFC6E, 0x0147},   // Area B H
#elif defined(CS336)
		/*
		 {CHIPS_W_SFR,0x05,0x00},     //close GPIO_PUEN_CFG0
		 {CHIPS_W_SFR,0x07,0x80},     //close GPIO_PDEN_CFG0
		 {CHIPS_W_SFR,0x08,0x00},    //close GPIO_PDEN_CFG1
		 {CHIPS_W_SFR,0x42,0x9c},
		 {CHIPS_W_SFR,0x89,0x00},
		 #if defined(USE_16_BITS)
		 {CHIPS_W_SFR,0x60,0x09},
		 #elif defined(USE_8_BITS)
		 {CHIPS_W_SFR,0x60,0x08},
		 #elif defined(USE_12_BITS)
		 {CHIPS_W_SFR,0x60,0x0B},
		 #endif
		 {CHIPS_W_SFR,0x4d,0x04},
		 {CHIPS_W_SFR,0x55,0x00}, //nomal int=3  key_en=0
		 {CHIPS_W_SRAM,0xfc80,0x000f},    //adc_bias ==============
		 {CHIPS_W_SRAM,0xfc3a,0xbaaa},   // ref press
		 {CHIPS_W_SRAM,0xfc3e,0x2009}, // adc_ref dac_af====================== for glass
		 {CHIPS_W_SRAM,0xfc02,0x1d67},  // pwr_en af==============
		 {CHIPS_W_SRAM,0xfc32,0x788b}, //for high vol sel for 2061 + rotation
		 {CHIPS_W_SRAM,0xfc38,0x0003},    //end=00, start=11 for 2061
		 {CHIPS_W_SRAM,0xfc3c,0xb001}, //comp_en,dac_en,adc_ref_en
		 {CHIPS_W_SRAM,0xfc2a,0x0000},         //close the min cap
		 {CHIPS_W_SFR,0x63,0x38},
		 //{CHIPS_W_SFR,0x6e,0x00},
		 {CHIPS_W_SFR,0x46,0x70},
		 */
		{	CHIPS_W_SFR,0x42,0x9c},
		{	CHIPS_W_SFR,0x46,0x70},
		{	CHIPS_W_SFR,0x4d,0x04},
		{	CHIPS_W_SFR,0x55,0x30},
#if defined(USE_16_BITS)
		{	CHIPS_W_SFR,0x60,0x09},
#elif defined(USE_8_BITS)
		{	CHIPS_W_SFR,0x60,0x08},
#elif defined(USE_12_BITS)
		{	CHIPS_W_SFR,0x60,0x0B},
#endif
		{	CHIPS_W_SFR,0x63,0x38},
		{	CHIPS_W_SFR,0x89,0x00},
		{	CHIPS_W_SRAM,0xfc02,0x057f},
		{	CHIPS_W_SRAM,0xfc04,0x0001},
		{	CHIPS_W_SRAM,0xfc28,0x1825},
		{	CHIPS_W_SRAM,0xfc2a,0x0002},
		{	CHIPS_W_SRAM,0xfc32,0x608b},
		{	CHIPS_W_SRAM,0xfc34,0x0408},
		{	CHIPS_W_SRAM,0xfc3a,0xaaaa},
		{	CHIPS_W_SRAM,0xfc3c,0xb001},
		{	CHIPS_W_SRAM,0xfc3e,0x280c},
		{	CHIPS_W_SRAM,0xfc80,0x000f},
		{	CHIPS_W_SRAM,0xfc82,0x0000},
		{	CHIPS_W_SRAM,0xfc84,0x8f00},

		{	CHIPS_W_SRAM,0xfc06,0x0050},
		{	CHIPS_W_SRAM,0xfc08,0x2e00},
		{	CHIPS_W_SRAM,0xfc0a,0x000f},
		{	CHIPS_W_SRAM,0xfc1a,0x0025},
		{	CHIPS_W_SRAM,0xfc1e,0x0025},
		{	CHIPS_W_SRAM,0xfc1c,0x002a},
		{	CHIPS_W_SRAM,0xfc2c,0x004f},
		{	CHIPS_W_SRAM,0xfc2e,0x0000},
		{	CHIPS_W_SRAM,0xfc16,0x042e},
		{	CHIPS_W_SRAM,0xfc20,0x0048},
		{	CHIPS_W_SRAM,0xfc18,0x004f},
		{	CHIPS_W_SRAM,0xfc30,0x002c},
		{	CHIPS_W_SRAM,0xfc22,0x0049},
		{	CHIPS_W_SRAM,0xfc24,0x004e},

#elif defined(CS3511)

		{	CHIPS_W_SFR,0x05,0x00},     //close GPIO_PUEN_CFG0
		{	CHIPS_W_SFR,0x07,0x80},     //close GPIO_PDEN_CFG0
		{	CHIPS_W_SFR,0x08,0x00},    //close GPIO_PDEN_CFG1
		{	CHIPS_W_SFR,0x42,0xa5},     //0x42,0x9c
		{	CHIPS_W_SFR,0x89,0x00},    //#pmu_vref_gt0/
		{	CHIPS_W_SFR,0x44,0x08},
#if defined(USE_16_BITS)
		{	CHIPS_W_SFR,0x60,0x09},
#elif defined(USE_8_BITS)
		{	CHIPS_W_SFR,0x60,0x08},
#elif defined(USE_12_BITS)
		{	CHIPS_W_SFR,0x60,0x0B},
#endif
		{	CHIPS_W_SFR,0x4d,0x04},
		{	CHIPS_W_SFR,0x55,0x00}, //nomal int=3  key_en=0
		{	CHIPS_W_SRAM,0xfc80,0x000f},    //adc_bias ==============
		{	CHIPS_W_SRAM,0xfc34,0x0104}, //0208  // lp_dummy_no,scan_num   // for slp
		{	CHIPS_W_SRAM,0xfc36,0x38c2},    // lp_dummy_no   // for slp
		//{CHIPS_W_SRAM,0xfc36,0x28c2},    // lp_dummy_no   // for slp
		//{CHIPS_W_SRAM,0xfc3a,0xafeb},   // ref press
		{	CHIPS_W_SRAM,0xfc3e,0x2007}, // adc_ref dac_af====================== for glass  0x200a
		{	CHIPS_W_SRAM,0xfc02,0x1d69},  // pwr_en af==============
		{	CHIPS_W_SRAM,0xfc32,0x788b}, //for high vol sel for 2061 + rotation
		{	CHIPS_W_SRAM,0xfc38,0x0003},    //end=00, start=11 for 2061
		{	CHIPS_W_SRAM,0xfc3c,0xb001}, //comp_en,dac_en,adc_ref_en
		//======== for base ========
		{	CHIPS_W_SRAM,0xfc2a,0x0002},

		{	CHIPS_W_SRAM,0xfc82,0x080f},
		{	CHIPS_W_SRAM,0xfc84,0x4f00},
		//sensor timing
		{	CHIPS_W_SRAM,0xfc08,0x2100},		// ph1 & sensor rst rising edge
		{	CHIPS_W_SRAM,0xfc0a,0x0014},	// sensor rst falling edge
		{	CHIPS_W_SRAM,0xfc16,0x0421},	// vtx rising edge
		{	CHIPS_W_SRAM,0xfc18,0x0050},	// vtx falling edge
		{	CHIPS_W_SRAM,0xfc2c,0x0050},	// ph1 falling edge
		{	CHIPS_W_SRAM,0xfc2e,0x0000},	// ph2 rising edge
		{	CHIPS_W_SRAM,0xfc30,0x0020},	// ph2 falling edge
		{	CHIPS_W_SRAM,0xfc1a,0x001a},	// cds1 rising edge
		{	CHIPS_W_SRAM,0xfc1c,0x001f},	// cds1 falling edge
		{	CHIPS_W_SRAM,0xfc1e,0x001a},	// cds2 rising edge
		{	CHIPS_W_SRAM,0xfc20,0x004e},	// cds2 falling edge
		{	CHIPS_W_SRAM,0xfc22,0x004f},	// cks rising edge
		{	CHIPS_W_SRAM,0xfc24,0x2450},	// cks falling edge

		{	CHIPS_W_SFR,0x63,0x38},
		{	CHIPS_W_SFR,0x46,0x30},

		{	CHIPS_W_SRAM,0xFC40, 0x0420},	// Area 0 L
		{	CHIPS_W_SRAM,0xFC42, 0x0127},	// Area 0 H 1

		{	CHIPS_W_SRAM,0xFC44, 0x0438},	// Area 1 L
		{	CHIPS_W_SRAM,0xFC46, 0x013F},	// Area 1 H 1

		{	CHIPS_W_SRAM,0xFC48, 0x0408},	// Area 2 L
		{	CHIPS_W_SRAM,0xFC4A, 0x010F},	// Area 2 H 1

		{	CHIPS_W_SRAM,0xFC4C, 0x0450},	// Area 3 L
		{	CHIPS_W_SRAM,0xFC4E, 0x0157},	// Area 3 H 1

		{	CHIPS_W_SRAM,0xFC50, 0x0720},	// Area 4 L
		{	CHIPS_W_SRAM,0xFC52, 0x0127},	// Area 4 H 1

		{	CHIPS_W_SRAM,0xFC54, 0x0738},	// Area 5 L
		{	CHIPS_W_SRAM,0xFC56, 0x013F},	// Area 5 H 1

		{	CHIPS_W_SRAM,0xFC58, 0x0708},	// Area 6 L
		{	CHIPS_W_SRAM,0xFC5A, 0x010F},	// Area 6 H 1

		{	CHIPS_W_SRAM,0xFC5C, 0x0750},	// Area 7 L
		{	CHIPS_W_SRAM,0xFC5E, 0x0157},	// Area 7 H 1

		{	CHIPS_W_SRAM,0xFC60, 0x0120},	// Area 8 L
		{	CHIPS_W_SRAM,0xFC62, 0x0127},	// Area 8 H 1

		{	CHIPS_W_SRAM,0xFC64, 0x0138},	// Area 9 L
		{	CHIPS_W_SRAM,0xFC66, 0x013F},	// Area 9 H 1

		{	CHIPS_W_SRAM,0xFC68, 0x0A20},	// Area A L
		{	CHIPS_W_SRAM,0xFC6A, 0x0127},	// Area A H 1

		{	CHIPS_W_SRAM,0xFC6C, 0x0A38},	// Area B L
		{	CHIPS_W_SRAM,0xFC6E, 0x013F},	// Area B H 1
#elif defined(CS358)	
		{	CHIPS_W_SFR,0x46,0x70},
		{	CHIPS_W_SFR,0x4d,0x04},
		{	CHIPS_W_SFR,0x55,0x00},
#if defined(USE_16_BITS)
		{	CHIPS_W_SFR,0x60,0x09},
#elif defined(USE_8_BITS)
		{	CHIPS_W_SFR,0x60,0x08},
#elif defined(USE_12_BITS)
		{	CHIPS_W_SFR,0x60,0x0B},
#endif
		{	CHIPS_W_SFR,0x63,0x38},
		{	CHIPS_W_SFR,0x89,0x00},
		{	CHIPS_W_SFR,0x47,0x00},
		{	CHIPS_W_SRAM,0xfc02,0x057f},
		{	CHIPS_W_SRAM,0xfc04,0x0002},
		{	CHIPS_W_SRAM,0xfc28,0x1825},
		{	CHIPS_W_SRAM,0xfc2a,0x0002},
		{	CHIPS_W_SRAM,0xfc32,0x608b},
		{	CHIPS_W_SRAM,0xfc34,0x0408},
		{	CHIPS_W_SRAM,0xfc3a,0xbaa2},
		{	CHIPS_W_SRAM,0xfc3c,0xf001},
		//{CHIPS_W_SRAM,0xfc3e,0x6808},
		{	CHIPS_W_SRAM,0xfc80,0x000F},
		{	CHIPS_W_SRAM,0xfc82,0x002f},
		//{CHIPS_W_SRAM,0xfc84,0xE785},
		{	CHIPS_W_SRAM,0xfc06,0x0050},
		{	CHIPS_W_SRAM,0xfc08,0x0000},
		{	CHIPS_W_SRAM,0xfc0a,0x000f},
		{	CHIPS_W_SRAM,0xfc1a,0x0025},
		{	CHIPS_W_SRAM,0xfc1e,0x0025},
		{	CHIPS_W_SRAM,0xfc1c,0x002a},
		{	CHIPS_W_SRAM,0xfc2c,0x002c},
		{	CHIPS_W_SRAM,0xfc2e,0x002e},
		{	CHIPS_W_SRAM,0xfc16,0x042e},
		{	CHIPS_W_SRAM,0xfc20,0x0048},
		{	CHIPS_W_SRAM,0xfc18,0x004f},
		{	CHIPS_W_SRAM,0xfc30,0x004f},
		{	CHIPS_W_SRAM,0xfc22,0x0049},
		{	CHIPS_W_SRAM,0xfc24,0x004e},
#elif defined(CS1073)	
		{ CHIPS_W_SFR, 0x0F, 0x01 }, { CHIPS_W_SFR, 0x1C, 0x1D }, { CHIPS_W_SFR,
				0x1F, 0x0A }, { CHIPS_W_SFR, 0x13, 0x31 },
		{ CHIPS_W_SFR, 0x42, 0xc8 },  //{CHIPS_W_SFR,0x42,0xAA},
		{ CHIPS_W_SFR, 0x63, 0x60 }, { CHIPS_W_SFR, 0x22, 0x07 },
		{ CHIPS_W_SFR, 0x50, 0x01 },   // clear reset irq
		{ CHIPS_W_SFR, 0x47, 0x00 },   // close pump,低功耗
		{ CHIPS_W_SRAM, 0xFC8C, 0x0001 }, { CHIPS_W_SRAM, 0xFC90, 0x0001 },
		{ CHIPS_W_SRAM, 0xFC92, 0x0002 }, //2016/12/5
		{ CHIPS_W_SRAM, 0xFC02, 0x0420 }, { CHIPS_W_SRAM, 0xFC1A, 0x0C30 },
		{ CHIPS_W_SRAM, 0xFC22, 0x086C }, //打开Normal下12个敏感区域
		{ CHIPS_W_SRAM, 0xFC2E, 0x04f8 }, //增益参数
		{ CHIPS_W_SRAM, 0xFC30, 0x024b }, //base参数
		{ CHIPS_W_SRAM, 0xFC06, 0x0039 }, { CHIPS_W_SRAM, 0xFC08, 0x0004 }, {
				CHIPS_W_SRAM, 0xFC0A, 0x0016 },
		{ CHIPS_W_SRAM, 0xFC0C, 0x0022 }, { CHIPS_W_SRAM, 0xFC12, 0x002A }, {
				CHIPS_W_SRAM, 0xFC14, 0x0035 },
		{ CHIPS_W_SRAM, 0xFC16, 0x002B }, { CHIPS_W_SRAM, 0xFC18, 0x0039 }, {
				CHIPS_W_SRAM, 0xFC28, 0x002E },
		{ CHIPS_W_SRAM, 0xFC2A, 0x0018 }, { CHIPS_W_SRAM, 0xFC26, 0x282D }, {
				CHIPS_W_SRAM, 0xFC82, 0x01FF },
		{ CHIPS_W_SRAM, 0xFC84, 0x0007 }, { CHIPS_W_SRAM, 0xFC86, 0x0001 }, {
				CHIPS_W_SRAM, 0xFC80, 0x0718 }, //chips_write_sram_bit(0xFC80, 12,0);
		{ CHIPS_W_SRAM, 0xFC88, 0x380B }, //chips_write_sram_bit(0xFC88, 9, 0);
		{ CHIPS_W_SRAM, 0xFC8A, 0x7C8B }, //chips_write_sram_bit(0xFC8A, 2, 0);
		{ CHIPS_W_SRAM, 0xFC8E, 0x3354 },
#if defined(USE_16_BITS)	
		{ CHIPS_W_SFR, 0x60, 0x09 },
#elif defined(USE_8_BITS)
		{	CHIPS_W_SFR,0x60,0x08},
#endif
		/*
		 [   8  9   ]
		 [2  0  1  3]
		 [6  4  5  7]
		 [   A  B   ]
		 ***********************************************/
		{ CHIPS_W_SRAM, 0xFC40, 0x0528 }, // Area 0 H
		{ CHIPS_W_SRAM, 0xFC42, 0x0521 }, // Area 0 L

		{ CHIPS_W_SRAM, 0xFC44, 0x0828 }, // Area 1 H
		{ CHIPS_W_SRAM, 0xFC46, 0x0821 }, // Area 1 L

		{ CHIPS_W_SRAM, 0xFC48, 0x0124 }, // Area 2 H
		{ CHIPS_W_SRAM, 0xFC4A, 0x011D }, // Area 2 L

		{ CHIPS_W_SRAM, 0xFC4C, 0x0C24 }, // Area 3 H
		{ CHIPS_W_SRAM, 0xFC4E, 0x0C1D }, // Area 3 L

		{ CHIPS_W_SRAM, 0xFC50, 0x053C }, // Area 4 H
		{ CHIPS_W_SRAM, 0xFC52, 0x0535 }, // Area 4 L

		{ CHIPS_W_SRAM, 0xFC54, 0x083C }, // Area 5 H
		{ CHIPS_W_SRAM, 0xFC56, 0x0835 }, // Area 5 L

		{ CHIPS_W_SRAM, 0xFC58, 0x013C }, // Area 6 H
		{ CHIPS_W_SRAM, 0xFC5A, 0x0135 }, // Area 6 L

		{ CHIPS_W_SRAM, 0xFC5C, 0x0C3C }, // Area 7 H
		{ CHIPS_W_SRAM, 0xFC5E, 0x0C35 }, // Area 7 L

		{ CHIPS_W_SRAM, 0xFC60, 0x050C }, // Area 8 H
		{ CHIPS_W_SRAM, 0xFC62, 0x0505 }, // Area 8 L

		{ CHIPS_W_SRAM, 0xFC64, 0x080C }, // Area 9 H
		{ CHIPS_W_SRAM, 0xFC66, 0x0805 }, // Area 9 L

		{ CHIPS_W_SRAM, 0xFC68, 0x0554 }, // Area A H
		{ CHIPS_W_SRAM, 0xFC6A, 0x054D }, // Area A L

		{ CHIPS_W_SRAM, 0xFC6C, 0x0854 }, // Area B H
		{ CHIPS_W_SRAM, 0xFC6E, 0x084D }, // Area B L

#elif defined(CS1175)
		{	CHIPS_W_SFR,0x0F,0x01},
		{	CHIPS_W_SFR,0x1C,0x1D},
		//{CHIPS_W_SFR,0x13,0x31},
		{	CHIPS_W_SFR,0x1F,0x0A},
		{	CHIPS_W_SFR,0x42,0xc8},  //{CHIPS_W_SFR,0x42,0xAA},
		{	CHIPS_W_SFR,0x63,0x60},
		{	CHIPS_W_SFR,0x22,0x07},
		{	CHIPS_W_SFR,0x50,0x01},   // clear reset irq
		{	CHIPS_W_SFR,0x47,0x00},   // close pump,低功耗
		{	CHIPS_W_SRAM,0xFC8C, 0x0001},
		{	CHIPS_W_SRAM,0xFC90, 0x0001},
		{	CHIPS_W_SRAM,0xFC92, 0x0002}, //2016/12/5
		{	CHIPS_W_SRAM,0xFC02, 0x0420},
		{	CHIPS_W_SRAM,0xFC1A, 0x0C30},
		{	CHIPS_W_SRAM,0xFC22, 0x086C}, //打开Normal下12个敏感区域
		{	CHIPS_W_SRAM,0xFC2E, 0x04f8}, //增益参数
		{	CHIPS_W_SRAM,0xFC30, 0x024b}, //base参数
		{	CHIPS_W_SRAM,0xFC06, 0x0039},
		{	CHIPS_W_SRAM,0xFC08, 0x0004},
		{	CHIPS_W_SRAM,0xFC0A, 0x0016},
		{	CHIPS_W_SRAM,0xFC0C, 0x0022},
		{	CHIPS_W_SRAM,0xFC12, 0x002A},
		{	CHIPS_W_SRAM,0xFC14, 0x0035},
		{	CHIPS_W_SRAM,0xFC16, 0x002B},
		{	CHIPS_W_SRAM,0xFC18, 0x0039},
		{	CHIPS_W_SRAM,0xFC28, 0x002E},
		{	CHIPS_W_SRAM,0xFC2A, 0x0018},
		{	CHIPS_W_SRAM,0xFC26, 0x282D},
		{	CHIPS_W_SRAM,0xFC82, 0x01FF},
		{	CHIPS_W_SRAM,0xFC84, 0x0007},
		{	CHIPS_W_SRAM,0xFC86, 0x0001},
		{	CHIPS_W_SRAM,0xFC80, 0x0718}, //chips_write_sram_bit(0xFC80, 12,0);
		{	CHIPS_W_SRAM,0xFC88, 0x380B}, //chips_write_sram_bit(0xFC88, 9, 0);
		{	CHIPS_W_SRAM,0xFC8A, 0x7C8B}, //chips_write_sram_bit(0xFC8A, 2, 0);
		{	CHIPS_W_SRAM,0xFC8E, 0x3354},
		{	CHIPS_W_SRAM,0xFC7C, 0x06b3},
#if defined(USE_16_BITS)	
		{	CHIPS_W_SFR,0x60,0x09},
#elif defined(USE_8_BITS)
		{	CHIPS_W_SFR,0x60,0x08},
#endif
		/*
		 [   8  9   ]
		 [2  0  1  3]
		 [6  4  5  7]
		 [   A  B   ]
		 **********************************************/
		{	CHIPS_W_SRAM,0xFC40, 0x0347}, // Area 0 H
		{	CHIPS_W_SRAM,0xFC42, 0x0340}, // Area 0 L

		{	CHIPS_W_SRAM,0xFC44, 0x0377}, // Area 1 H
		{	CHIPS_W_SRAM,0xFC46, 0x0370}, // Area 1 L

		{	CHIPS_W_SRAM,0xFC48, 0x0047}, // Area 2 H
		{	CHIPS_W_SRAM,0xFC4A, 0x0040}, // Area 2 L

		{	CHIPS_W_SRAM,0xFC4C, 0x0647}, // Area 3 H
		{	CHIPS_W_SRAM,0xFC4E, 0x0640}, // Area 3 L

		{	CHIPS_W_SRAM,0xFC50, 0x0017}, // Area 4 H
		{	CHIPS_W_SRAM,0xFC52, 0x0010}, // Area 4 L

		{	CHIPS_W_SRAM,0xFC54, 0x0317}, // Area 5 H
		{	CHIPS_W_SRAM,0xFC56, 0x0310}, // Area 5 L

		{	CHIPS_W_SRAM,0xFC58, 0x0617}, // Area 6 H
		{	CHIPS_W_SRAM,0xFC5A, 0x0610}, // Area 6 L

		{	CHIPS_W_SRAM,0xFC5C, 0x0077}, // Area 7 H
		{	CHIPS_W_SRAM,0xFC5E, 0x0070}, // Area 7 L

		{	CHIPS_W_SRAM,0xFC60, 0x0677}, // Area 8 H
		{	CHIPS_W_SRAM,0xFC62, 0x0670}, // Area 8 L

		{	CHIPS_W_SRAM,0xFC64, 0x00A7}, // Area 9 H
		{	CHIPS_W_SRAM,0xFC66, 0x00A0}, // Area 9 L

		{	CHIPS_W_SRAM,0xFC68, 0x03A7}, // Area A H
		{	CHIPS_W_SRAM,0xFC6A, 0x03A0}, // Area A L

		{	CHIPS_W_SRAM,0xFC6C, 0x06A7}, // Area B H
		{	CHIPS_W_SRAM,0xFC6E, 0x06A0}, // Area B L
#endif
	};

/**
 * getCurrentTime - 获取当前时间
 *  
 * @return: 返回当前时间
 */
long getCurrentTime(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 * chips_clean_sem_count - 信号量清0
 *  
 * @return: 无返回值
 */
void chips_clean_sem_count(void) {
	//while(chips_check_sem_count()>0)
	//{
	//	sem_trywait(&g_sem);
	//}
}
/**
 * chips_check_sem_count - 检测信号量
 *  
 * @return: 1 有信号
 *			0 无信号
 */
int chips_check_sem_count(void) {
	int sval = 0;
	//sem_getvalue(&g_sem,&sval);
	return sval;
}

/**
 * chips_sig_fun - 信号量P操作
 * @signum: 
 *  
 * @return: 无返回值
 */
static void chips_sig_fun(int signum) {
	//sem_post(&g_sem);
	//ALOGD("<%s>    %d     ",__FUNCTION__,chips_check_sem_count());
}

/**
 * chips_sig_post - 自生成信号量P操作
 * @signum: 
 *  
 * @return: 无返回值
 */
void chips_sig_post(void) {
	//sem_post(&g_sem);
}


/**
 * chips_sig_wait_interrupt - 信号量V操作
 *  
 * @return: 成功返回0，失败返回负数
 */
int chips_sig_wait_interrupt(void) {
	uint8_t status = 0;
	uint8_t mode = 0;
	int retval = -1;
	//while((status = sem_wait(&g_sem)) == -1 && errno == EINTR);
	while (1) {
		//printf("read_0x50\n");
		retval = read_SFR(0x50, &status);
		if (retval < 0) {
			continue;
		}

		//printf("a:%x\n",status);
		if (status == 0x08 || status == 0x04 || status == 0x01) {
			retval = read_SFR(0x46, &mode);
			if (retval < 0) {
				continue;
			}

			//printf("b:%x\n",mode);
			if (mode == MODE_SLEEP)
				break;
		}
		usleep(5000);
	}

	return status;
}

/**
 * chips_set_flag - 设置文件属性
 * @fd: 打开的文件描述符
 * @flags: 要设置的文件属性
 *  
 * @return: 无返回值
 */
static void chips_set_flag(int fd, int flags) {
	int val;

	if ((val = fcntl(fd, F_GETFL, 0)) < 0) {
		ALOGE("<%s> fcntl get flag error", __FUNCTION__);
		return;
	}

	val |= flags;

	if (fcntl(fd, F_SETFL, val) < 0) {
		ALOGE("<%s> fcntl set flag error", __FUNCTION__);
		return;
	}
}

/** 
 * chips_set_reset_gpio - 设置复位gpio电平状态
 * @level: ：非0为拉高，0为拉低
 * @return: 成功返回0，失败返回负数
 */
static int chips_set_reset_gpio(int level) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;

		}
	}
	if (ioctl(g_fd, CHIPS_IOC_SET_RESET_GPIO, (unsigned long) &level) < 0) {
		ALOGE("<%s> Failed to set reset_gpio state", __FUNCTION__);

		return -1;
	}
	return 0;
}

/**
 * chips_register_sig_fun - 注册异步信号处理函数
 *  
 * @return: 成功返回0，失败返回负数
 */
int chips_register_sig_fun(void) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	signal(SIGIO, chips_sig_fun);

	if (fcntl(g_fd, F_SETOWN, getpid()) < 0) {
		ALOGE("<%s> fcntl setown error", __FUNCTION__);
		return -1;
	}

	chips_set_flag(g_fd, FASYNC);
	sem_init(&g_sem, 0, 0);

	return 0;
}

/**
 * chips_enable_irq - 中断使能
 *  
 * @return: 成功返回0，失败返回负数
 */
int chips_enable_irq(void) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	if (ioctl(g_fd, CHIPS_IOC_ENABLE_IRQ, NULL) < 0) {
		ALOGE("<%s> Failed to enable irq", __FUNCTION__);
		return -1;
	}

	return 0;
}

/**
 * chips_disable_irq - 关闭中断
 *  
 * @return: 成功返回0，失败返回负数
 */
int chips_disable_irq(void) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	if (ioctl(g_fd, CHIPS_IOC_DISABLE_IRQ, NULL) < 0) {
		ALOGE("<%s> Failed to disable irq", __FUNCTION__);
		return -1;
	}

	return 0;
}

/**
 * chips_send_key_event - 通知驱动上报key事件
 * @key: key事件结构体，包含keycode以及keyvalue
 *  
 * @return: 成功返回0，失败返回负数
 */
int chips_send_key_event(int keycode, int keyvalue) {
	struct chips_key_event key;
	key.code = keycode;
	key.value = keyvalue;

	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	if (ioctl(g_fd, CHIPS_IOC_INPUT_KEY_EVENT, (unsigned long) &key) < 0) {
		ALOGE("<%s> Failed to send key event", __FUNCTION__);
		return -1;
	}

	return 0;
}

/**
 * chips_get_screen_state - 获取屏幕亮灭状态
 * @screen_state: 执行成功时存储屏幕状态，0表示亮；1表示灭
 *  
 * @return: 失败返回负数
 */
int chips_get_screen_state(int *screen_state) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device,for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	if (read(g_fd, screen_state, sizeof(int)) == -1) {
		ALOGE("<%s> Failed to read from %s,for %s", __FUNCTION__,
				CHIPS_DEV_NAME, strerror(errno));
		return -1;
	}

	return 0;
}

/**
 * chips_close_fd - 关闭打开的文件描述符
 *  
 * @return: 无返回值
 */
void chips_close_fd(void) {
	if (g_fd > 0) {
		close(g_fd);
		g_fd = -1;
	}
}

/**
 * chips_sfr_read - 读SFR寄存器
 * @addr: 寄存器起始地址
 * @data: 读到的数据
 * @len: 读取的数据长度
 *  
 * @return: 成功返回0，失败返回负数
 */
static int chips_sfr_read(uint16_t addr, uint8_t *data, uint16_t len) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	struct chips_ioc_transfer ioc = { .cmd = CHIPS_R_SFR, .addr = addr, .buf =
			(unsigned long) data, .actual_len = len, };

	if (ioctl(g_fd, CHIPS_IOC_SPI_MESSAGE, (unsigned long) &ioc) < 0) {
		ALOGE("<%s> Failed to read SFR from addr = 0x%x,len = %d", __FUNCTION__,
				addr, len);
		return -1;
	}
	return 0;
}

/**
 * chips_sfr_write - 写SFR寄存器
 * @addr: 寄存器起始地址
 * @data: 写入的数据
 * @len: 写入的数据长度
 *  
 * @return: 成功返回0，失败返回负数
 */
static int chips_sfr_write(uint16_t addr, uint8_t *data, uint16_t len) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	struct chips_ioc_transfer ioc = { .cmd = CHIPS_W_SFR, .addr = addr, .buf =
			(unsigned long) data, .actual_len = len, };

	if (ioctl(g_fd, CHIPS_IOC_SPI_MESSAGE, (unsigned long) &ioc) < 0) {
		ALOGE("<%s> Failed to write SFR at addr = 0x%x,len = %d", __FUNCTION__,
				addr, len);
		return -1;
	}
	return 0;
}

/**
 * chips_sram_read - 读SRAM寄存器
 * @addr: 寄存器起始地址
 * @data: 读到的数据
 * @len: 读取的数据长度
 *  
 * @return: 成功返回0，失败返回负数
 */
static int chips_sram_read(uint16_t addr, uint8_t *data, uint16_t len) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	struct chips_ioc_transfer ioc = { .cmd = CHIPS_R_SRAM, .addr = addr, .buf =
			(unsigned long) data, .actual_len = len, };

	if (ioctl(g_fd, CHIPS_IOC_SPI_MESSAGE, (unsigned long) &ioc) < 0) {
		ALOGE("<%s> Failed to read SRAM form addr = 0x%x,len = %d",
				__FUNCTION__, addr, len);
		return -1;
	}
	return 0;
}

/**
 * chips_sram_write - 写SRAM寄存器
 * @addr: 寄存器起始地址
 * @data: 写入的数据
 * @len: 写入的数据长度
 *  
 * @return: 成功返回0，失败返回负数
 */
static int chips_sram_write(uint16_t addr, uint8_t *data, uint16_t len) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	struct chips_ioc_transfer ioc = { .cmd = CHIPS_W_SRAM, .addr = addr, .buf =
			(unsigned long) data, .actual_len = len, };

	if (ioctl(g_fd, CHIPS_IOC_SPI_MESSAGE, (unsigned long) &ioc) < 0) {
		ALOGE("<%s> Failed to write SRAM at addr = 0x%x,len = %d", __FUNCTION__,
				addr, len);
		return -1;
	}
	return 0;
}

/**
 * chips_spi_send_cmd - 发送spi命令
 * @cmd: spi命令
 *  
 * @return: 成功返回0，失败返回负数
 */
static int chips_spi_send_cmd(uint8_t cmd) {
	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	if (ioctl(g_fd, CHIPS_IOC_SPI_SEND_CMD, (unsigned long) &cmd) < 0) {
		ALOGE("<%s> Failed to send spi cmd:0x%x", __FUNCTION__, cmd);
		return -1;
	}

	return 0;
}

/**
 * chips_sensor_config - 给IC下载参数
 *  
 * @return: 成功返回0，失败返回负数
 */
int chips_sensor_config(void) {
	struct config config = { .p_param = (unsigned long) param, .num =
			sizeof(param) / sizeof(param[0]), };

	if (g_fd < 0) {
		g_fd = open(CHIPS_DEV_NAME, O_RDWR);
		if (g_fd == -1) {
			ALOGE("<%s> Failed to open device for %s", __FUNCTION__,
					strerror(errno));
			return -1;
		}
	}

	if (ioctl(g_fd, CHIPS_IOC_SENSOR_CONFIG, (unsigned long) &config) < 0) {
		ALOGE("<%s> Failed to write configs to sensor", __FUNCTION__);
		return -1;
	}

	return 0;
}

int read_SFR(uint16_t addr, uint8_t *data) {
	return chips_sfr_read(addr, data, 1);
}

int write_SFR(uint16_t addr, uint8_t data) {
	return chips_sfr_write(addr, &data, 1);
}

int read_SRAM(uint16_t addr, uint16_t *data) {
	uint8_t rx_buf[2] = { 0 };
	int status = -1;

	status = chips_sram_read(addr, rx_buf, 2);
	if (status == 0) {
		*data = ((rx_buf[1] << 8) & 0xFF00) | (rx_buf[0] & 0x00FF);
	}
	return status;
}

int write_SRAM(uint16_t addr, uint16_t data) {
	uint8_t tx_buf[2] = { 0 };
	tx_buf[0] = (uint8_t) (data & 0x00FF);       //低8位
	tx_buf[1] = (uint8_t) ((data & 0xFF00) >> 8);  //高8位

	return chips_sram_write(addr, tx_buf, 2);
}

/**
 * read_register - 读取SFR寄存器，只读取一个字节
 * @addr: 寄存器地址
 *  
 * @return: 成功返回读取的数据，失败返回负数
 * @detail: 用于初始化算法库
 */
unsigned char read_register(unsigned char addr) {
	int status = -1;
	unsigned char data = 0;

	status = chips_sfr_read(addr, &data, 1);

	return status < 0 ? status : data;
}

/**
 * write_register - 写SFR寄存器，只写入一个字节
 * @addr: 寄存器地址
 * @value: 写入的数据
 *  
 * @return: 成功返回0，失败返回负数
 * @detail: 用于初始化算法库
 */
int write_register(unsigned char addr, unsigned char value) {
	return chips_sfr_write(addr, &value, 1);
}

/**
 * chips_scan_one_image - 获取图像数据
 * @buffer: 成功时存储图像数据
 * @len: 读取的图像数据长度
 *  
 * @return: 成功返回0，失败返回负数
 */
int chips_scan_one_image(uint8_t *buffer, uint16_t len) {
	int status = -1;
	uint8_t mode = 0;

	status = write_SFR(0x47, 0x02);
	if (status < 0) {
		ALOGE("<%s> Failed to write 0x03 to 0x47", __FUNCTION__);
		return status;
	}

	status = write_SRAM(0xFC00, 0x0003);
	if (status < 0) {
		ALOGE("<%s> Failed to write 0x0003 to reg_0xFC00", __FUNCTION__);
		return status;
	}

	usleep(1000);

	status = chips_sram_read(0xFFFF, buffer, len);
	if (status < 0) {
		ALOGE("<%s> Failed to read sram from reg_0xFFFF,len = %d", __FUNCTION__,
				len);
	}

	return status;
}

/**
 * chips_spi_wakeup - spi唤醒
 *  
 * @return: 成功返回0，失败返回负数
 */
static int chips_spi_wakeup(void) {
	int status = -1;
	uint8_t wakeup = 0xd5;

	status = chips_spi_send_cmd(wakeup);
	if (status < 0) {
		ALOGE("<%s> Failed to send spi cmd, cmd = 0x%x", __FUNCTION__, wakeup);
	}

	return status;
}
#if defined(CS358)
/**
 * chips_get_mtp_data - mtp数据读取
 *  
 * @return: 1 成功 0 失败
 */
int chips_get_mtp_data(uint8_t *addr,int butlen,uint8_t *buffer)
{
	uint8_t rd_data = 0;
	uint16_t rd_addr = 0;
	int i,j;
	write_SFR(0xb9,0x12);
	for(i = 0; i < butlen/2;i++)
	{
		if(i > 0)
		{
			if(addr[1] != 0xff)
			{
				addr[1] = addr[1] + 1;
			}
			else
			{
				addr[0] = addr[0] + 1;
				addr[1] = 0x00;
			}
		}
		write_SFR(0xb0,addr[1]);  //low addr
		write_SFR(0xb1,addr[0]);// high addr 

		write_SFR(0xBa,0x01);

		read_SFR(0xba,&rd_data);
		if(rd_data == 0x02)
		{
			write_SFR(0xba,0x02);
			write_SFR(0xba,0x00);
		}
		else
		{
			ALOGI("<%s> error ",__FUNCTION__);
			return RET_ERROR;
		}

		read_SFR(0xb6,&rd_data);  //low data
		*(buffer + i*2 ) = rd_data;
		read_SFR(0xb7,&rd_data);//high data
		*(buffer + i*2 +1) = rd_data;
	}
	write_SFR(0xb9,0x10);
	return RET_OK;
}

/**
 * chips_get_vcm_from_mtp - 读取 vcm 值
 *  
 * @return: 无返回值
 */
void chips_get_vcm_from_mtp(void)
{
	int retval = -1;
	uint8_t addr[2] = {0};
	uint8_t mtp_data[2] = {0};
	addr[0] = 0x00;
	addr[1] = 0x30;
	retval = chips_get_mtp_data(addr,2,mtp_data);
	if(retval < 0)
	{
		ALOGE("<%s> Failed to read mtp data",__FUNCTION__);
	}
	else
	{
		vcm_value = mtp_data[1]*256 + mtp_data[0];
		ALOGI("<%s> vcm_value = 0x%x ",__FUNCTION__,vcm_value);
	}
}

/**
 * chips_get_fc3e_from_mtp - 读取 FC3E 的值
 *  
 * @return: 无返回值
 */
void chips_get_fc3e_from_mtp(void)
{
	int retval = -1;
	uint8_t addr[2] = {0};
	uint8_t mtp_data[2] = {0};
	addr[0] = 0x00;
	addr[1] = 0x31;
	retval = chips_get_mtp_data(addr,2,mtp_data);
	if(retval < 0)
	{
		ALOGE("<%s> Failed to read mtp data",__FUNCTION__);
	}
	else
	{
		fc3e_value = mtp_data[1]*256 + mtp_data[0];
		ALOGI("<%s> fc3e_value = 0x%x ",__FUNCTION__,fc3e_value);
	}
}

/**
 * chips_get_threshold_value_from_mtp - 读取中断阈值
 *  
 * @return: 无返回值
 */
void chips_get_threshold_value_from_mtp(void)
{
	int retval = -1;
	uint8_t addr[2] = {0};
	uint8_t mtp_data[2] = {0};
	addr[0] = 0x00;
	addr[1] = 0x32;
	retval = chips_get_mtp_data(addr,2,mtp_data);
	if(retval < 0)
	{
		ALOGE("<%s> Failed to read mtp data",__FUNCTION__);
	}
	else
	{
		thr_value = mtp_data[0] - 30;
		ALOGI("<%s> thr_value = 0x%x ",__FUNCTION__,thr_value);
	}
}

/**
 * chips_write_data_from_mtp - 配置MTP读到的数据
 *  
 * @return: 无返回值
 */
void chips_write_data_from_mtp(void)
{
	write_SRAM(0xFC84,vcm_value);
	write_SRAM(0xFC3E,fc3e_value);
	write_SFR(0x42,thr_value);
}
#endif
/**
 * chips_check_finger_up_state - 检测手指离开
 *  
 * @return: 无返回值
 */
int chips_check_finger_up_state(void) {

	uint8_t mode = 0;
	int status = 0;
#if (defined (CS3105) || defined (CS336) || defined (CS358)|| defined (CS3511))
	status = read_SFR(0x55, &mode);
	if(mode == 0x80)
	{
		//ALOGE("<%s> ",__FUNCTION__);
		return 1;
	}
#else
	status = read_SFR(0x42, &mode);
	if (mode == REG_42_VAULE) {
		//ALOGE("<%s> ",__FUNCTION__);
		return 1;
	}
#endif
	return 0;
}
/**
 * chips_check_finger_up - 检测手指离开
 *  
 * @return: 无返回值
 */
void chips_check_finger_up(void) {

#if defined (CS3105)
	write_SFR(0x55,0x80);

	write_SRAM(0xFC32,0x098B);

	write_SRAM(0xFC40,0x0128);   // Area 0  		0xFC60
	write_SRAM(0xFC42,0x012F);// Area 0    8	0xFC62

	write_SRAM(0xFC44,0x0508);// Area 1 		0xFC58
	write_SRAM(0xFC46,0x010f);// Area 1    6	0xFC5A

	write_SRAM(0xFC48,0x0364);// Area 2 		0xFC4C
	write_SRAM(0xFC4A,0x016B);// Area 2		3	0xFC4E

	write_SRAM(0xFC4C,0x0740);// Area 3 		0xFC6C
	write_SRAM(0xFC4E,0x0147);// Area 3    B	0xFC6E

#elif (defined (CS336) || defined(CS358)|| defined (CS3511))
	write_SFR(0x55,0x80);

#elif defined (CS1073)
	write_SFR(0x42, REG_42_VAULE);

	write_SRAM(0xFC26, 0x682D);

	write_SRAM(0xfc12, 0x0035);
	write_SRAM(0xfc14, 0x0003);

	write_SRAM(0xFC30, 0x026d);
	write_SRAM(0xFC2e, 0x054f);

	write_SRAM(0xFC40, 0x050C);   // Area 0 H 		0xFC60
	write_SRAM(0xFC42, 0x0505);   // Area 0 L   8	0xFC62

	write_SRAM(0xFC44, 0x013C);   // Area 1 H		0xFC58
	write_SRAM(0xFC46, 0x0135);   // Area 1 L   6	0xFC5A

	write_SRAM(0xFC48, 0x0C24);   // Area 2 H		0xFC4C
	write_SRAM(0xFC4A, 0x0C1D);   // Area 2 L	3	0xFC4E

	write_SRAM(0xFC4C, 0x0854);   // Area 3 H		0xFC6C
	write_SRAM(0xFC4E, 0x084D);   // Area 3 L   B	0xFC6E

#elif defined (CS1175)
	write_SFR(0x42,REG_42_VAULE);

	write_SRAM(0xFC26,0x682D);

	write_SRAM(0xfc12,0x0035);
	write_SRAM(0xfc14,0x0003);

	write_SRAM(0xFC30, 0x026d);
	write_SRAM(0xFC2e, 0x054f);

	write_SRAM(0xFC40,0x0677);   // Area 0 H 		0xFC60
	write_SRAM(0xFC42,0x0670);// Area 0 L   8	0xFC62

	write_SRAM(0xFC44,0x0617);// Area 1 H		0xFC58
	write_SRAM(0xFC46,0x0610);// Area 1 L   6	0xFC5A

	write_SRAM(0xFC48,0x0647);// Area 2 H		0xFC4C
	write_SRAM(0xFC4A,0x0640);// Area 2 L	3	0xFC4E

	write_SRAM(0xFC4C,0x06A7);// Area 3 H		0xFC6C
	write_SRAM(0xFC4E,0x06A0);// Area 3 L   B	0xFC6E
#endif

}

/**
 * chips_check_finger_down - 检测手指按下
 *  
 * @return: 无返回值
 */
void chips_check_finger_down(void) {
#if defined (CS3105)
	write_SFR(0x55,0x00);

	write_SFR(0x42,0xaa);

	write_SRAM(0xFC32,0x088B);

	write_SRAM(0xFC40,0x0328);   // Area 0 H
	write_SRAM(0xFC42,0x012F);// Area 0 L   8

	write_SRAM(0xFC44,0x0340);// Area 1 H
	write_SRAM(0xFC46,0x0147);// Area 1 L   6

	write_SRAM(0xFC48,0x0308);// Area 2 H
	write_SRAM(0xFC4A,0x010f);// Area 2 L	3

	write_SRAM(0xFC4C,0x0364);// Area 3 H
	write_SRAM(0xFC4E,0x016B);// Area 3 L   b

#elif (defined (CS336) || defined(CS358)|| defined (CS3511))
	write_SFR(0x55,0x00);

#elif defined (CS1073)
	write_SFR(0x42, 0xdd);

	write_SRAM(0xFC26, 0x082D);

	write_SRAM(0xFC12, 0x002A);
	write_SRAM(0xFC14, 0x0035);

	write_SRAM(0xFC30, 0x024b);
	write_SRAM(0xFC2e, 0x04f8);

	write_SRAM(0xFC40, 0x0528);   // Area 0 H 		0xFC60
	write_SRAM(0xFC42, 0x0521);   // Area 0 L   8	0xFC62

	write_SRAM(0xFC44, 0x0850);   // Area 1 H		0xFC58
	write_SRAM(0xFC46, 0x0821);   // Area 1 L   6	0xFC5A

	write_SRAM(0xFC48, 0x0124);   // Area 2 H		0xFC4C
	write_SRAM(0xFC4A, 0x011D);   // Area 2 L	3	0xFC4E

	write_SRAM(0xFC4C, 0x0C24);   // Area 3 H		0xFC6C
	write_SRAM(0xFC4E, 0x0C1D);   // Area 3 L   B	0xFC6E

#elif defined (CS1175)
	write_SFR(0x42,0xdd);

	write_SRAM(0xFC26,0x082D);

	write_SRAM(0xFC12, 0x002A);
	write_SRAM(0xFC14, 0x0035);

	write_SRAM(0xFC30, 0x024b);
	write_SRAM(0xFC2e, 0x04f8);

	write_SRAM(0xFC40,0x0347);   // Area 0 H
	write_SRAM(0xFC42,0x0340);// Area 0 L   8

	write_SRAM(0xFC44,0x0377);// Area 1 H
	write_SRAM(0xFC46,0x0370);// Area 1 L   6

	write_SRAM(0xFC48,0x0047);// Area 2 H
	write_SRAM(0xFC4A,0x0040);// Area 2 L	

	write_SRAM(0xFC4C,0x0647);// Area 3 H
	write_SRAM(0xFC4E,0x0640);// Area 3 L

#endif
}

/**
 * chips_force_to_idle - 切换IC到IDLE模式
 *  
 * @return: 成功返回0，失败返回负数
 */
static int chips_force_to_idle(void) {
	uint8_t mode = 0;
	uint8_t status = 0;
	int retval = -1;
	int i = 0;

	retval = read_SFR(0x46, &mode);
	if (retval < 0) {
		ALOGE("<%s> Failed to read reg_0x46", __FUNCTION__);
		return -1;
	}

	if (mode == MODE_IDLE) {
		return 0;
	} else if (mode == MODE_NORMAL) {
		;
	} else {
		chips_spi_wakeup();
		//usleep(1000);
	}

	/***************************************************************
	 程序走到这里时，IC可能的模式有SLEEP、NORMAL以及DEEP_SLEEP
	 判断spi_wakeup是否成功,并判断此时IC处于何种模式
	 ***************************************************************/

	retval = write_SFR(0x46, MODE_IDLE); //active_idle
	if (retval < 0) {
		ALOGE("<%s> Failed to write 0x70 to reg_0x46", __FUNCTION__);
		return -1;
	}

	for (i = 0; i < 20; i++) {
		usleep(10000);
		retval = read_SFR(0x50, &status);
		if (retval < 0) {
			ALOGE("<%s> Failed to read reg_0x50+2", __FUNCTION__);
			continue;;
		}

		if (status == 0x01) {          //sleep、normal模式下active_idle成功
			break;
		}
	}
	/*
	 if(status == 0){
	 return 0;                   //deep_sleep模式下active_idle成功，退出
	 }
	 */
	if (i >= 20) {
		ALOGE("<%s> Failed to active idle,reg_0x50 = 0x%x", __FUNCTION__,
				status);
		return -1;
	}

	retval = write_SFR(0x50, 0x01);        //清除idle_irq
	if (retval < 0) {
		ALOGE("<%s> Failed to write 0x01 to reg_0x50", __FUNCTION__);
		return -1;
	}

	for (i = 0; i < 5; i++) {
		retval = read_SFR(0x50, &status);
		if (retval < 0) {
			ALOGE("<%s> Failed to read reg_0x50+3", __FUNCTION__);
			continue;;
		}

		if (status == 0x00)               //清除idle_irq成功
			break;

	}

	if (i >= 5) {
		ALOGE("<%s> Failed to clear active idle irq,reg_0x50 = 0x%x",
				__FUNCTION__, status);
		return -1;
	}

	return 0;
}

/**
 * chips_set_sensor_mode - 设置IC工作模式
 * @mode: 要设置的IC工作模式
 *  
 * @return: 成功返回0，失败返回负数
 */
int chips_set_sensor_mode(int mode) {
	int status = -1;
	status = chips_force_to_idle();
	if (status < 0) {
		ALOGE("<%s> Failed to set idle mode", __FUNCTION__);
		return status;
	}

	switch (mode) {
	case IDLE:        //idle
		break;

	case NORMAL:      //normal

		status = write_SFR(0x46, MODE_NORMAL);
		//usleep(1000);
		if (status < 0) {
			ALOGE("<%s> Failed to write 0x71 to reg_0x46", __FUNCTION__);
		}
		break;

	case SLEEP:       //sleep

		status = write_SFR(0x46, MODE_SLEEP);
		//usleep(1000);
		if (status < 0) {
			ALOGE("<%s> Failed to write 0x72 to reg_0x46", __FUNCTION__);
		}
		break;

	case DEEP_SLEEP:  //deep sleep
		status = write_SFR(0x46, MODE_DSLEEP);
		//usleep(1000);
		if (status < 0) {
			ALOGE("<%s> Failed to write 0x76 to reg_0x46", __FUNCTION__);
		}
		break;

	default:
		ALOGE("<%s> unrecognized mode,mode = 0x%x", __FUNCTION__, mode);
		status = -1;
		break;
	}
	return status;
}

/**
 * chips_detect_finger_down - 妫€娴嬩腑鏂槸鍚︿负鎵嬫寚鎸変笅瑙﹀彂鐨勪腑鏂? * @reg_49_48_val: 鐢ㄤ簬瀛樺偍瀵艰埅鏁版嵁
 *  
 * @return: 鎴愬姛杩斿洖0锛屽け璐ヨ繑鍥炶礋鏁? * @detail: 妫€娴嬫墜鎸囦腑鏂蛋SLEEP->NROMAL妯″紡
 */
#if 0
int chips_detect_finger_down(uint16_t *reg_49_48_val)
{
	uint8_t status = 0;
	uint8_t status2 = 0;

	uint8_t mode = 0;
	int retval = -1;
	int i = 0;
	uint8_t buffer[2] = {0};

	retval = read_SFR(0x46,&mode);
	if(retval < 0)
	{
		ALOGE("<%s> Failed to read reg_0x46+",__FUNCTION__);
		return RET_ERROR;
	}

#if (defined(CS3105) || defined(CS336) || defined(CS358)|| defined (CS3511))
	retval = read_SFR(0x4D,&status2);
	if(retval < 0)
	{
		ALOGE("<%s> Failed to read reg_0x4D",__FUNCTION__);
		return RET_ERROR;
	}
	if(status2 == 0x04)
	{
		ALOGI("<%s> Finger reset irq , mode = 0x%x,status = 0x%x status2 = 0x%x",__FUNCTION__,mode,status,status2);
		return RET_RESET_IRQ;
	}
#endif		

	for(i = 0; i < 20; i++)
	{
		retval = read_SFR(0x50,&status);
		if(retval < 0)
		{
			ALOGE("<%s> Failed to read reg_0x50+",__FUNCTION__);
			continue;
		}
		if(status == 0x08 || status == 0x04 || status == 0x01)
		break;
	}

	switch(status)
	{
		case 0x08:
		//ALOGI("<%s> Finger detected in sleep mode",__FUNCTION__);
		if(chips_check_finger_up_state())
		{
			return RET_ACTIVE_FINGER_LEAVE_IRQ;
		}
		return RET_OK;
		case 0x04:
		//ALOGI("<%s> Finger detected in normal mode",__FUNCTION__);
		if(chips_check_finger_up_state())
		{
			return RET_ACTIVE_FINGER_LEAVE_IRQ;
		}
		retval = chips_sfr_read(0x48,buffer,2);
		if(retval == 0)
		{
			*reg_49_48_val = ((((buffer[1] << 8)&0xFF00)|(buffer[0]&0x00FF))&0x0FFF);
			//ALOGI("<%s> reg_49_48_val = 0x%x",__FUNCTION__,(unsigned)*reg_49_48_val);
		}
		return RET_OK;

#if (defined(CS1175)||defined (CS1073))
		case 0x01:
		retval = read_SFR(0x0F,&status2);
		if(retval < 0)
		{
			ALOGE("<%s> Failed to read reg_0x0F",__FUNCTION__);
			return RET_ERROR;
		}
		if(status2 == 0x2B)
		{
			return RET_RESET_IRQ;
		}
		else
		{
			ALOGE("<%s> Active_idle irq detected, mode = 0x%x,status = 0x%x status2 = 0x%x",__FUNCTION__,mode,status,status2);
			return RET_ACTIVE_IDLE_IRQ;
		}
#endif

		default:
		ALOGE("<%s> invalid sem detected, mode = 0x%x,status = 0x%x status2 = 0x%x",__FUNCTION__,mode,status,status2);
		return RET_INVALID_SEM;

	}
}
#else

/**
 * chips_detect_finger_down - 妫€娴嬩腑鏂槸鍚︿负鎵嬫寚鎸変笅瑙﹀彂鐨勪腑鏂? * @reg_49_48_val: 鐢ㄤ簬瀛樺偍瀵艰埅鏁版嵁
 *  
 * @return: 鎴愬姛杩斿洖0锛屽け璐ヨ繑鍥炶礋鏁? * @detail: 妫€娴嬫墜鎸囦腑鏂蛋SLEEP->NROMAL妯″紡
 */
int chips_detect_finger_down(void) {
	uint8_t status = 0;
	uint8_t status2 = 0;

	uint8_t mode = 0;
	int retval = -1;
	int i = 0;

	retval = read_SFR(0x46, &mode);
	if (retval < 0) {
		ALOGE("<%s> Failed to read reg_0x46+", __FUNCTION__);
		return RET_ERROR;
	}

	retval = read_SFR(0x4D, &status2);
	if (retval < 0) {
		ALOGE("<%s> Failed to read reg_0x4D", __FUNCTION__);
		return RET_ERROR;
	}
	if (status2 == 0x04) {
		ALOGI(
				"<%s> Finger reset irq , mode = 0x%x,status = 0x%x status2 = 0x%x",
				__FUNCTION__, mode, status, status2);
		return RET_RESET_IRQ;
	}

	for (i = 0; i < 20; i++) {
		retval = read_SFR(0x50, &status);
		if (retval < 0) {
			ALOGE("<%s> Failed to read reg_0x50+", __FUNCTION__);
			continue;
			//return RET_ERROR;
		}
		if (status == 0x08 || status == 0x04 || status == 0x01)
			break;
	}

	switch (status) {
	case 0x08:
		//ALOGI("<%s> Finger detected in sleep mode",__FUNCTION__);
		if (mode == 0x72) {
			if (chips_check_finger_up_state()) {
				return RET_ACTIVE_FINGER_LEAVE_IRQ;
			}
			return RET_OK;
	}
	case 0x04:
		//ALOGI("<%s> Finger detected in normal mode",__FUNCTION__);
		if (mode == 0x71) {
			if (chips_check_finger_up_state()) {
				return RET_ACTIVE_FINGER_LEAVE_IRQ;
			}
			return RET_OK;
	}
	default:
		ALOGE(
				"<%s> invalid sem detected, mode = 0x%x,status = 0x%x status2 = 0x%x",
				__FUNCTION__, mode, status, status2);
		return RET_INVALID_SEM;

	}
}

#endif

/**
 * chips_reset_sensor - 纭欢澶嶄綅IC
 *
 * @return: 鎴愬姛杩斿洖0锛屽け璐ヨ繑鍥炶礋鏁?*/
int chips_reset_sensor(void) {
int ret = -1;

ret = chips_set_reset_gpio(0);
if (ret < 0) {
ALOGE("Failed to set reset_gpio low,ret = %d", ret);
return ret;
}
/*
usleep(5000);

ret = chips_set_reset_gpio(1);
if (ret < 0) {
ALOGE("Failed to set reset_gpio high,ret = %d", ret);
return ret;
}

usleep(5000);
*/
return 0;
}

static int chips_16clk_write(void) {
unsigned char byte = '#';
int ret = -1;

ret = chips_spi_send_cmd(byte);
if (ret < 0) {
ALOGE("Failed to write chips_16clk_write+1", __FUNCTION__);
return ret;
}

ret = chips_spi_send_cmd(byte);
if (ret < 0) {
ALOGE("Failed to write chips_16clk_write+2", __FUNCTION__);
return ret;
}

return 0;
}

/**
 * chips_esd_reset - 纭欢澶嶄綅IC
 *
 * @return: 鎴愬姛杩斿洖0锛屽け璐ヨ繑鍥炶礋鏁?*/

int chips_esd_reset(void) {
int ret = -1;
ALOGI("<%s> ^_^", __FUNCTION__);
ret = chips_set_reset_gpio(0);
if (ret < 0) {
ALOGE("Failed to set reset_gpio low,ret = %d", ret);
return ret;
}

chips_16clk_write();

usleep(12000);

ret = chips_set_reset_gpio(1);
if (ret < 0) {
ALOGE("Failed to set reset_gpio high,ret = %d", ret);
return ret;
}

usleep(5000);

ret = chips_set_reset_gpio(0);
if (ret < 0) {
ALOGE("Failed to set reset_gpio low,ret = %d", ret);
return ret;
}

usleep(5000);

ret = chips_set_reset_gpio(1);
if (ret < 0) {
ALOGE("Failed to set reset_gpio high,ret = %d", ret);
return ret;
}

usleep(5000);

ret = chips_set_reset_gpio(0);
if (ret < 0) {
ALOGE("Failed to set reset_gpio low,ret = %d", ret);
return ret;
}

usleep(5000);

ret = chips_set_reset_gpio(1);
if (ret < 0) {
ALOGE("Failed to set reset_gpio high,ret = %d", ret);
return ret;
}

usleep(5000);

return 0;
}

