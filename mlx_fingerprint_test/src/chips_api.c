/**
 * Copyright (C) ShenZhen ChipSailing Technology Co., Ltd. All rights reserved.
 * 
 * Filename: chips_api.c
 * 
 * Author: zwp    ID: 58    Version: 2.0   Date: 2016/10/16
 * 
 * Description: 指纹录入和匹配的相关接口
 * 
 * Others: 
 * 
 * History: 
 *     1. Date:           Author:          ID: 
 *        Modification:  
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include "./inc/CSAlgDll.h"
#include "./inc/chips_sensor.h"
#include "./inc/chips_api.h"
#include "./inc/chips_fingerprint.h"
#include "./inc/chips_log.h"


#if defined SAMPLE_DEBUG
static int samplenum = 0;
#endif


#if defined(CS1073)
#define IMAGE_W 112
#define IMAGE_H 88
#elif defined(CS1175)
#define IMAGE_W 56
#define IMAGE_H 180
#elif defined(CS3105)
#define IMAGE_W 68
#define IMAGE_H 118
#elif (defined(CS336) || defined(CS358))
#define IMAGE_W 64
#define IMAGE_H 80
#elif defined(CS3511)
#define IMAGE_W 96
#define IMAGE_H 96
#endif

#if defined(USE_8_BITS)
#define bufsiz IMAGE_W*IMAGE_H
#elif (defined(USE_16_BITS) || defined(USE_12_BITS))
#define bufsiz IMAGE_W*IMAGE_H*2
#endif

#define FINGER_COUNT 5
#define FINGER_COUNT_SUPPORT 2
#define PERSON_COUNT 5
#if (defined(CS336) || defined(CS358))
#define ENROLL_COUNT 12
#else
#define ENROLL_COUNT 8
#endif	

#define FEATURE_SIZE 28000
#define ENROLL_SAME_SPACE_SCORE 75

#define PATH_SIZE 255
#define FILENAME  "/fingerprint.bin"
static  char chips_fingerprint_path[PATH_SIZE];
static const char store_path[]="/mnt";

#define DEFAULT_DETECT_GAIN_VALUE 0x04F8             
#define DEFAULT_DETECT_BASE_VALUE 0x024B        
#define DEFAULT_DETECT_GRAY_VALUE 0x0200
#define DEFAULT_DETECT_GAIN_BASE_VALUE 0x200c  

#if defined(CALIBRATION)
#define DEFAULT_SAMPLE_LOW_GAIN_VALUE 0x00a3                              
#define DEFAULT_SAMPLE_LOW_BASE_VALUE 0x0296                       

#define DEFAULT_SAMPLE_HIGH_GAIN_VALUE 0x04F4                              
#define DEFAULT_SAMPLE_HIGH_BASE_VALUE 0x0245                      
#endif

#if defined(COATING)
#define DEFAULT_SAMPLE_GAIN_VALUE 0x04F8                              
#define DEFAULT_SAMPLE_BASE_VALUE 0x0260                       
#define DEFAULT_SAMPLE_GAIN_BASE_VALUE 0x200a  
#elif defined(GLASS)
#define DEFAULT_SAMPLE_GAIN_VALUE 0x048A                               
#define DEFAULT_SAMPLE_BASE_VALUE 0x02A8                  
#elif defined(CERAMIC)
#define DEFAULT_SAMPLE_GAIN_VALUE 0x04FB                               
#define DEFAULT_SAMPLE_BASE_VALUE 0x0380                        
#endif


static int g_sample_gain_value = DEFAULT_SAMPLE_GAIN_VALUE;
static int g_sample_base_value = DEFAULT_SAMPLE_BASE_VALUE;

static int g_total_template_num=0;
static int g_fingernum = 0;
static int g_fingerid[FINGER_COUNT_SUPPORT*PERSON_COUNT] = {0};
static int g_width[FINGER_COUNT][ENROLL_COUNT] = {{0}};
static int g_height[FINGER_COUNT][ENROLL_COUNT] = {{0}};
static unsigned char g_template[FINGER_COUNT_SUPPORT*PERSON_COUNT][ENROLL_COUNT][FEATURE_SIZE] = {{{0}}};

static int g_cur_enroll_idx = 0;
static int g_cur_matched_enroll_idx = 0;
static unsigned char g_match_template[FEATURE_SIZE] = {0};


static short g_adj_val[bufsiz] = {0};

#if defined(SAMPLE_DEBUG)
int chips_fs_write( const char* fs_path,unsigned char *buf,int nbytes){
	int fd;
	int ret; 
	
	fd = open( fs_path, O_CREAT|O_RDWR,0666);
	if(fd < 0)
	{
		ALOGE("[chipsailing:%s] open %s failed,ret = %d", __func__,fs_path, fd);
		return -1;
	}
	
	ret=write(fd , buf, nbytes);
	if(ret < 0)
	{
		ALOGE("[chipsailing:%s] write %s failed,ret = %d",__func__, fs_path, ret);
		return -1;
	}

	close(fd);

	return 0;
}

void WriteBmpFile(char *fs_path,unsigned char *buf,short img_w,short img_h){
		int len = img_w*img_h;
		unsigned char temp_head[54] = { 0x42, 0x4d, // file type
				0x0, 0x0, 0x0, 0x00, // file size
				0x00, 0x00, // reserved
				0x00, 0x00, // reserved
				0x36, 0x4, 0x00, 0x00, // head byte
				// infoheader
				0x28, 0x00, 0x00, 0x00, // struct size
				// 0x00,0x01,0x00,0x00, //map width
				0x00, 0x00, 0x00, 0x00, // map width
				// 0x68,0x01,0x00,0x00, //map height
				0x00, 0x00, 0x00, 0x00, // map height
				0x01, 0x00, // must be 1
				0x08, 0x00, // color count
				0x00, 0x00, 0x00, 0x00, // compression
				// 0x00,0x68,0x01,0x00, //data size
				0x00, 0x00, 0x00, 0x00, // data size
				0x00, 0x00, 0x00, 0x00, // dpix
				0x00, 0x00, 0x00, 0x00, // /dpiy
				0x00, 0x00, 0x00, 0x00, // color used
				0x00, 0x00, 0x00, 0x00, // color important
		};
        unsigned char head[1078] = { 0 };
        unsigned char *newbmp = malloc(1078 + len);
		memset(newbmp,0,1078 + len);
                memcpy(head,temp_head,sizeof(temp_head));
		int i, j;
		long num;
		num = img_w;
		head[18] = (unsigned char) (num & 0xFF);
		num = num >> 8;
		head[19] = (unsigned char) (num & 0xFF);
		num = num >> 8;
		head[20] = (unsigned char) (num & 0xFF);
		num = num >> 8;
		head[21] = (unsigned char) (num & 0xFF);

		num = img_h;
		head[22] = (unsigned char) (num & 0xFF);
		num = num >> 8;
		head[23] = (unsigned char) (num & 0xFF);
		num = num >> 8;
		head[24] = (unsigned char) (num & 0xFF);
		num = num >> 8;
		head[25] = (unsigned char) (num & 0xFF);

		j = 0;
		for (i = 54; i < 1078; i = i + 4) 
		{
			head[i] = head[i + 1] = head[i + 2] = (char) j;
			head[i + 3] = 0;
			j++;
		}
        memcpy(newbmp,head,sizeof(head));		
        memcpy(newbmp+1078,buf,len);	
		chips_fs_write(fs_path,newbmp,1078 + len );
		free(newbmp);
}
 
void chips_save_fpsample(unsigned char *image_buf_8,short img_w,short img_h)
{ 
	unsigned int i;
	int j;
	int buflen;
	unsigned char *dst_image_buf_8;
	char Currentname[80] = "/mnt/";
	char samplename[5][7] = {"1.bmp","2.bmp","3.bmp","4.bmp","5.bmp"};
	char *file_path;
	file_path = strcat(Currentname,samplename[samplenum]);
	ALOGI("<%s> Fingerprint File_Path = %s ",__FUNCTION__,file_path);
	buflen = (img_w*img_h);
	dst_image_buf_8 = (unsigned char*)malloc(buflen);
	if(NULL == dst_image_buf_8)
	{
		ALOGE("<%s> Failed to malloc mem for image buffer",__FUNCTION__);
		goto out;
	}
	memset(dst_image_buf_8,0,buflen);	
	#if (defined(USE_16_BITS)||defined(USE_12_BITS))
		
		#if (defined(CS1175) || defined(CS1073))
		for(j = 0;j < buflen;j++)
		{
			*(dst_image_buf_8 + j) = (*(image_buf_8 + j*2) << 4) + (*(image_buf_8 + j*2 + 1) >> 4);
			//ALOGI("<%s> j = %d *(dst_image_buf_8 + j) = 0x%x  0x%x  0x%x ",__FUNCTION__,j,*(dst_image_buf_8 + j),*(image_buf_8 + j*2),*(image_buf_8 + j*2 + 1));
		}
		#else
		for(j = 0;j < buflen;j++)
		{
			*(dst_image_buf_8 + j) = *(image_buf_8 + j*2);
		}
		#endif			
	#else
		memcpy(dst_image_buf_8,image_buf_8,buflen);
	#endif	
	WriteBmpFile(file_path,dst_image_buf_8,img_w,img_h);	
	samplenum++;
	if(samplenum >=5 || samplenum <= -1)
	{
		samplenum = 0;
	}
out:	
	if(dst_image_buf_8 != NULL)
	{
		free(dst_image_buf_8);
		dst_image_buf_8 = NULL;
	}
}
#endif





#if defined(CS3105)
/** 
 *	@brief chips_convert_image_raw_data convert the raw image data of cs_3105 tonormal state * 
 *	@param [in] dst    the pointer pointed to the converted image data
 *	@param [in] src    the pointer pointed to the raw image data 
 *	@param [in] width the width of the image  
 *	@param [in] height the height of the image *   
 *	@return 0 if success 
 *		 or a negative number in case of error	* 
 *	@detail this function is compatible with both 8_bits image data and 16_bits image data.
 *	 please make sure that the size of dst buffer is big enough. 
 */ 
 
 static void chips_convert_image_raw_data(unsigned char *dst,unsigned char *src,unsigned int width,unsigned int height)
 {	 
	 unsigned int row;
	 unsigned int col;	 
	 unsigned char *p_src_row_data;  
	 unsigned char *p_dst_row_data;
	 if(dst == NULL || src == NULL || width < 1 || height < 1 )
	 {		 
		 ALOGE("<%s> invalid arguments",__FUNCTION__);		 
		 return;  
	 }	 
	 for(row = 0; row < height; row++)	 
	 {	 
#if defined(USE_8_BITS)		
		 p_src_row_data = src + (row * width);	 
		 p_dst_row_data = dst + (row * width); 
		 for(col = 0; col < width/2; col++)  
		 {			 
			 *(p_dst_row_data + 2*col) = *(p_src_row_data + col);	 
			 *(p_dst_row_data + 2*col + 1) = *(p_src_row_data + (width + 1)/2 + col);
		 }		 
		 if((width%2) != 0)
		 {	 
			 *(p_dst_row_data + width - 1) = *(p_src_row_data + width/2);		 
		 }
#elif defined(USE_16_BITS)		
		 p_src_row_data = src + (row * width * 2);	 
		 p_dst_row_data = dst + (row * width * 2) ; 		 
		 for(col = 0; col < width/2; col++) 	 
		 {			 
			 *(p_dst_row_data + 4*col) = *(p_src_row_data + 2*col);  
			 *(p_dst_row_data + 4*col + 1) = *(p_src_row_data + 2*col + 1);  
			 *(p_dst_row_data + 4*col + 2) = *(p_src_row_data + width + width%2 + 2*col);	 
			 *(p_dst_row_data + 4*col + 3) = *(p_src_row_data + width + width%2 + 2*col + 1);	 
		 }		 
		 if((width%2) != 0)
		 {	 
			 *(p_dst_row_data + 2*width - 2) = *(p_src_row_data + width - 1);
			 *(p_dst_row_data + 2*width - 1) = *(p_src_row_data + width);	 
		}		 
#endif	
	 }		 
 }
 
#elif (defined(CS336) || defined(CS358)||defined(CS3511))
/**
 * chips_get_next_xo_data - 图像数据解密
 * curData ：图像数据
 * @return: 解密后的图像数据
 */
unsigned char chips_get_next_xo_data(unsigned char curData)
{
	unsigned char nextData = 0;
	unsigned char feedback = (curData&0x80)>>7;

	nextData = curData<<1;

	nextData |= feedback;
	nextData ^= (feedback<<2);
	nextData ^= (feedback<<3);
	nextData ^= (feedback<<4);
	
	return nextData;
}

void chips_deciphering_image(unsigned char *src_image_buf_8,int buflen)
{	
		int i;
		int retval = -1;
		uint8_t regValue = 0;
		retval = read_SFR(0x6e,&regValue);
		if(retval < 0)
		{
			ALOGE("<%s> Failed to read reg_0x6e",__FUNCTION__);
			return;
		}
		if(regValue != 0x00)
		{
			for(i = 0; i < buflen; i++)
			{
				regValue = chips_get_next_xo_data(regValue); 
				*(src_image_buf_8 + i) = (*(src_image_buf_8 + i))^regValue;
			}
		}
}


/** 
 *	@brief chips_convert_image_raw_data convert the raw image data of cs_3105 tonormal state * 
 *	@param [in] dst    the pointer pointed to the converted image data
 *	@param [in] src    the pointer pointed to the raw image data 
 *	@param [in] width the width of the image  
 *	@param [in] height the height of the image *   
 *	@return 0 if success 
 *		 or a negative number in case of error	* 
 *	@detail this function is compatible with both 8_bits image data and 16_bits image data.
 *	 please make sure that the size of dst buffer is big enough. 
 */ 
static void chips_convert_image_raw_data(unsigned char *dst,unsigned char *src,unsigned int width,unsigned int height)
{
	unsigned int row;
	unsigned int col;
	uint8_t *p_src_row_data;
	uint8_t *p_dst_row_data;
	uint16_t *p_dst_row_data_16;
	uint16_t *p_src_row_data_16;
	uint8_t regValue = 0;
	int i,retval;
	
	
	if(dst == NULL || src == NULL || width < 1 || height < 1 )
	 {		 
		 ALOGE("<%s> invalid arguments",__FUNCTION__);		 
		 return;  
	 }

#if defined(USE_8_BITS)

	ALOGI("<%s> use_8_bits width=%d height=%d,regValue= 0x%x",__FUNCTION__,width,height,regValue);
	for(row = 0; row < height; row++)
	{
		p_src_row_data = src + (row * width);	 
		p_dst_row_data = dst + (row * width); 
		int offsetBytes = (width/4);
		for(col = 0; col < (width/4); col++)
		{
			*(p_dst_row_data + col*4) = *(p_src_row_data + col) ;
			*(p_dst_row_data + col*4 + 1) = *(p_src_row_data + col + offsetBytes);
			*(p_dst_row_data + col*4 + 2) = *(p_src_row_data + col + offsetBytes*2);
			*(p_dst_row_data + col*4 + 3) = *(p_src_row_data + col + offsetBytes*3);
		}
	}
	
	
#elif defined(USE_16_BITS)
	ALOGI("<%s> use_16_bits width=%d height=%d,regValue= 0x%x",__FUNCTION__,width,height,regValue);
	for(row = 0; row < height; row++)
	{
		p_src_row_data = src + (row * width * 2);
		p_dst_row_data = dst + (row * width * 2); 
		int offsetBytes = (width/4)*2;
		for(col = 0; col < (width/4); col++)
		{	
			*(p_dst_row_data + col*8) = *(p_src_row_data + col*2);
			*(p_dst_row_data + col*8 + 1) = *(p_src_row_data + col*2 + 1);
			*(p_dst_row_data + col*8 + 2) = *(p_src_row_data + col*2 + offsetBytes);
			*(p_dst_row_data + col*8 + 3) = *(p_src_row_data + col*2 + offsetBytes + 1);
			*(p_dst_row_data + col*8 + 4) = *(p_src_row_data + col*2 + offsetBytes*2);
			*(p_dst_row_data + col*8 + 5) =	*(p_src_row_data + col*2 + offsetBytes*2 + 1);
			*(p_dst_row_data + col*8 + 6) = *(p_src_row_data + col*2 + offsetBytes*3);
			*(p_dst_row_data + col*8 + 7) = *(p_src_row_data + col*2 + offsetBytes*3 + 1);
		}
	}

#elif defined(USE_12_BITS)
	ALOGI("<%s> use_12_bits",__FUNCTION__);
	int lineWidth = width*3/2;
	for(row = 0; row < height; row++)
	{
		
		p_src_row_data = src + (row*lineWidth);
		p_dst_row_data = dst + (row * width *2);
		int offsetBytes = (width/4)*3/2;
		for(col = 0; col < (width/4); col++)
		{
			if(col%2 == 0) //0, 2, 4, ....
			{
				int nOffset = col*3/2;			
				*(p_dst_row_data + col*8) = *(p_src_row_data + nOffset);
				*(p_dst_row_data + col*8 + 1) = (*(p_src_row_data + nOffset + 2) & 0xf0);
				*(p_dst_row_data + col*8 + 2) = *(p_src_row_data + nOffset + offsetBytes);
				*(p_dst_row_data + col*8 + 3) = (*(p_src_row_data + nOffset + offsetBytes + 2) & 0xf0);
				*(p_dst_row_data + col*8 + 4) = *(p_src_row_data + nOffset + offsetBytes*2);
				*(p_dst_row_data + col*8 + 5) = (*(p_src_row_data + nOffset + offsetBytes*2 + 2) & 0xf0);
				*(p_dst_row_data + col*8 + 6) = *(p_src_row_data + nOffset + offsetBytes*3);
				*(p_dst_row_data + col*8 + 7) = (*(p_src_row_data + nOffset + offsetBytes*3 + 2) & 0xf0); 	
				
			}
			else
			{
				int nOffset = (col-1)*3/2;
				*(p_dst_row_data + col*8) = *(p_src_row_data + nOffset + 1);
				*(p_dst_row_data + col*8 + 1) = (*(p_src_row_data + nOffset + 2) & 0x0f);
				*(p_dst_row_data + col*8 + 2) = *(p_src_row_data + nOffset + 1 + offsetBytes);
				*(p_dst_row_data + col*8 + 3) = (*(p_src_row_data + nOffset + offsetBytes + 2) & 0x0f);
				*(p_dst_row_data + col*8 + 4) = *(p_src_row_data + nOffset + 1 + offsetBytes*2);
				*(p_dst_row_data + col*8 + 5) = (*(p_src_row_data + nOffset + offsetBytes*2 + 2) & 0x0f);
				*(p_dst_row_data + col*8 + 6) = *(p_src_row_data + nOffset + 1 + offsetBytes*3);
				*(p_dst_row_data + col*8 + 7) = (*(p_src_row_data + nOffset + offsetBytes*3 + 2) & 0x0f); 
				
			}
		}
	}
#endif

	
}
#endif

/**
 * chips_get_average_value - 算全图均值
 * src ：图像数据指针
 * @return: 平均值
 */
static unsigned char chips_get_average_value(unsigned char *src)
{	
	int i;
	
	unsigned long sum = 0;
	unsigned char avg_value;
	for(i = 0; i < bufsiz; i++)
	{
		sum = sum + *(src + i);
	}
	avg_value = sum/(bufsiz);
	ALOGI("<%s> avg_value = %d ",__FUNCTION__,avg_value);
	return avg_value;
}

/**
 * chips_get_true_image - 图像减校正值
 * src ：图像数据指针
 * @return: 无返回值
 */
static void chips_get_true_image(unsigned char *src)
{
	ALOGI("<%s> ",__FUNCTION__);
	int i;
	int temp = 0;
	for(i = 0; i < bufsiz; i++)
	{
		temp = *(src + i) - *(g_adj_val + i);
		
		if(temp < 0)
		{
			*(src + i) = 0;
		}
		else if(temp > 255)
		{
			*(src + i) = 255;
		}
		else
		{
			*(src + i) = (unsigned char)temp;
		}		
	}	
}
#if defined(CS358)
/**
 * chips_get_adjusted_value_from_mtp - 从mtp中获得图像的平头数据，并获得校正值
 *  
 * @return: 成功：1 失败：0
 */
static int chips_get_adjusted_value_from_mtp(void)
{
	int i;
	unsigned char avg_value;
	uint8_t addr[2] = {0};
	uint8_t *src_image_buf_8;
	uint8_t	*dst_image_buf_8;
	int status;
	src_image_buf_8 = (unsigned char*)malloc(bufsiz + 2);
	if(NULL == src_image_buf_8)
	{
		ALOGE("<%s> Failed to malloc mem for src_image buffer",__FUNCTION__);
		status =  RET_ERROR;
		goto out;
	}	
	
	dst_image_buf_8 = (unsigned char*)malloc(bufsiz);
	if(NULL == dst_image_buf_8)
	{
		ALOGE("<%s> Failed to malloc mem for src_image buffer",__FUNCTION__);
		status =  RET_ERROR;
		goto out;
	}
	
	memset(src_image_buf_8,0,bufsiz + 2);
	memset(dst_image_buf_8,0,bufsiz);
	
	addr[0] = 0x00;
	addr[1] = 0x64;
	
	status  = chips_get_mtp_data(addr,bufsiz + 2,src_image_buf_8);
	if(status < 0)
	{
		ALOGE("<%s> Failed to read mtp data",__FUNCTION__);
		status =  RET_ERROR;
		goto out;	
	}
	chips_convert_image_raw_data(dst_image_buf_8,src_image_buf_8 + 1,IMAGE_W,IMAGE_H);

	//chips_save_fpsample(dst_image_buf_8,IMAGE_W,IMAGE_H);

	avg_value = chips_get_average_value(dst_image_buf_8);
	memset(g_adj_val,0,bufsiz);
	for(i = 0; i < bufsiz; i++)
	{	
		*(g_adj_val + i) = *(dst_image_buf_8 + i) - avg_value;	
	}
	status = RET_OK;
	
out:
	if(src_image_buf_8 != NULL)
	{
		free(src_image_buf_8);
		src_image_buf_8 = NULL;
	}
	if(dst_image_buf_8 != NULL)
	{
		free(dst_image_buf_8);
		dst_image_buf_8 = NULL;
	}
	
	return status;
}
/**
 * chips_get_data_from_mtp - 从mtp读取相关数据
 *  
 * @return: 无返回值
 */
void chips_get_data_from_mtp(void)
{
	
	chips_get_adjusted_value_from_mtp();
	chips_get_vcm_from_mtp();
	chips_get_fc3e_from_mtp();
	chips_get_threshold_value_from_mtp();
}
#endif

/**
 * chips_init_lib - 初始化算法库
 *  
 * @return: 无返回值
 */
void chips_init_lib(void)
{
	char alg_version[100] = {0};
	ChipSailing_Init(read_register,write_register,1);
	ChipSailing_GetAlgVersion(alg_version);
	ALOGD("<%s> AlgVersion  is  %s ",__FUNCTION__,alg_version);
}




 /**
 * chips_get_fid - 获取新录入指纹的ID
 * @fid: 成功时存放新录入指纹的ID
 *  
 * @return: 成功返回0，失败返回负数
 */
int chips_get_fid(int *fid)
{
    int mfingerid = 0;
	int finger_idx = 0;
	bool isExist = false;
	
#if 1	
	for(finger_idx = 0; finger_idx < FINGER_COUNT; finger_idx++)
	{
        ALOGI("<%s> g_fingerid[%d] = %d",__FUNCTION__,finger_idx,g_fingerid[finger_idx]);
	}
#endif 
	
	for(mfingerid = 1; mfingerid <= FINGER_COUNT; mfingerid++)
	{
	    isExist = false;
	    for(finger_idx = 0; finger_idx < FINGER_COUNT; finger_idx++)
		{
		    if(g_fingerid[finger_idx] == mfingerid)
			{
			    isExist = true;
				break;
			}		
		}
		
		if(!isExist)
		    break;	
	}
	
	if(isExist)
	{
		ALOGE("<%s> Fingerprint ID table is full",__FUNCTION__);
		return RET_ERROR;
	}
	else
	{
		ALOGI("<%s> Fingerprint to be enrolled,fid = %d",__FUNCTION__, mfingerid);
		*fid = mfingerid;		
	}
	return RET_OK;
}



 /**
 * chips_get_finger_idx - 获取新录入指纹ID下标
 * @finger_idx: 成功时存放新录入指纹ID下标
 *  
 * @return: 成功返回0，失败返回负数
 */
static int chips_get_finger_idx(int *finger_idx)
{
    int idx = 0;
	
#if 1	
	for(idx = 0; idx < FINGER_COUNT; idx++)
	{
        ALOGI("<%s> g_fingerid[%d] = %d",__FUNCTION__,idx,g_fingerid[idx]);
	}
#endif	
	
	for(idx = 0; idx < FINGER_COUNT; idx++)
	{
	    if(g_fingerid[idx] == 0)
		    break;
	}
	
	if(idx >= FINGER_COUNT)
	{
	    ALOGE("<%s> Fingerprint ID table is full,idx = %d",__FUNCTION__, idx);
		return RET_ERROR;
	}
	else
	{
	    *finger_idx = idx;
		ALOGI("<%s> Fingerprint newly enrolld is going to be stored where finger_idx = %d",__FUNCTION__,*finger_idx);
	}
	
	return RET_OK;
}




 /**
 * sort - 冒泡排序法对指纹模板按大小进行排序
 * @finger_idx: 要进行排序的指纹ID下标
 *  
 * @return: 成功返回0，失败返回负数
 */
static void sort(int finger_idx)
{
    ALOGI("<%s> bubble sort,finger_idx = %d",__FUNCTION__, finger_idx);
	
    int i = 0;
	int j = 0;
	bool exchange = false;
	unsigned temp_width = 0;
	unsigned temp_height = 0;
	unsigned char *temp_feature = NULL;
	
	temp_feature = (unsigned char*)malloc(FEATURE_SIZE);
	if(NULL == temp_feature)
	{
	    ALOGE("<%s> Failed to allocate mem for temp_feature",__FUNCTION__);
		goto out;
	}
	//memset(temp_feature,0,FEATURE_SIZE);
	
	for(i = 0; i < ENROLL_COUNT-1; i++)
	{
	    exchange = false;
		for(j = 0; j < ENROLL_COUNT-1-i; j++)
		{
			temp_width = 0;
			temp_height = 0;
			memset(temp_feature,0,FEATURE_SIZE);
		    if(g_width[finger_idx][j]*g_height[finger_idx][j] < g_width[finger_idx][j+1]*g_height[finger_idx][j+1])
			{
			    temp_width = g_width[finger_idx][j];
				g_width[finger_idx][j] = g_width[finger_idx][j+1];
				g_width[finger_idx][j+1] = temp_width;
				
				temp_height = g_height[finger_idx][j];
				g_height[finger_idx][j] = g_height[finger_idx][j+1];
				g_height[finger_idx][j+1] = temp_height;
				
				memcpy(temp_feature,g_template[finger_idx][j],FEATURE_SIZE);
				memcpy(g_template[finger_idx][j],g_template[finger_idx][j+1],FEATURE_SIZE);
				memcpy(g_template[finger_idx][j+1],temp_feature,FEATURE_SIZE);
			
			    exchange = true;
			}
		}
		
		if(!exchange)
		    break;
	}
out:
    if(temp_feature != NULL)
	{
		free(temp_feature);
		temp_feature = NULL;
	}	
	
	return;
}

 static void chips_load_fingerprints_from_file(char * filename,int file_idx)
 {	 
		 
	 ALOGI("<%s> load filename= %s,file_idx=%x",__FUNCTION__,filename,file_idx);
	 memcpy(chips_fingerprint_path,store_path,PATH_SIZE);
	 strcat(chips_fingerprint_path,"/");
	 strcat(chips_fingerprint_path,filename);
 
	 FILE *fp = fopen(chips_fingerprint_path, "rb+");  //O_RDWR | O_CREATE | O_APPEND
	 if(NULL == fp)
	 {
		 ALOGE("<%s> Failed to fopen storage:%s, for %s",__FUNCTION__, chips_fingerprint_path, strerror(errno));
		 return;
	 }
	 
	 //将文件流设置到文件的起始位置
	 rewind(fp);
	int nn = fread(&g_fingernum,sizeof(int),1,fp);
	if(fseek(fp,sizeof(int),SEEK_SET) < 0)
	{
		ALOGE("<%s> Failed while seeking fingerprints from storage:%s,for %s",__FUNCTION__, chips_fingerprint_path, strerror(errno));
		fclose(fp);
		return;	
	} 	
	if(g_fingernum>FINGER_COUNT_SUPPORT)
	{
		ALOGE("<%s> g_fingernum=%d",__FUNCTION__, g_fingernum);
		g_fingernum=FINGER_COUNT_SUPPORT;
	}
	
	int ni = fread(&g_fingerid[file_idx*FINGER_COUNT_SUPPORT],FINGER_COUNT_SUPPORT*sizeof(int),1,fp);	
	if(fseek(fp,sizeof(int)+FINGER_COUNT*sizeof(int)+FINGER_COUNT*ENROLL_COUNT*sizeof(int)*2,SEEK_SET) < 0)
	{
		ALOGE("<%s> Failed while seeking fingerprints from storage:%s,for %s",__FUNCTION__, chips_fingerprint_path, strerror(errno));
		fclose(fp);
		return;	
	} 	  
	 int nt = fread(g_template[file_idx*FINGER_COUNT_SUPPORT],FINGER_COUNT_SUPPORT*ENROLL_COUNT*FEATURE_SIZE,1,fp);
	 ALOGI("<%s> g_fingernum=%d",chips_fingerprint_path,g_fingernum);
	 g_total_template_num+=g_fingernum;
	 
	 if( nn!=1||ni != 1 || nt != 1)
	 {
		 ALOGW("<%s> corrupt fingerprints storage:%s,ni = %d, nt = %d",__FUNCTION__,\
		 chips_fingerprint_path, ni, nt);
	 }
  
	 fclose(fp);
	 return;
 }

 /**
 * chips_load_fingerprints - 从文件中加载指纹数据到内存中
 *  
 * @return: 无返回值
 */
int filter_file(const struct dirent *dir)
{
	if(strlen(dir->d_name)>=5)
		return 1;
	else
		return 0;
}
 void chips_load_fingerprints(void)
 {
	int file_num=0;
	struct dirent **namelist;

	file_num=scandir(store_path,&namelist,filter_file,alphasort);
	if(file_num<=0||file_num>5)
	{
		ALOGI("<%s>read dir fail, filenum=%x",__FUNCTION__,file_num);
		return;
	}
	g_total_template_num=0;
	while(file_num--)
	{
		chips_load_fingerprints_from_file(namelist[file_num]->d_name,file_num);
	}

	ALOGI("<%s>fingernum=%x,fingerid %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",__FUNCTION__,g_total_template_num,g_fingerid[0],g_fingerid[1],g_fingerid[2],g_fingerid[3],g_fingerid[4]
			,g_fingerid[5],g_fingerid[6],g_fingerid[7],g_fingerid[8],g_fingerid[9]);
 }

 /**
 * chips_save_fingerprints - 将指纹数据从内存回写到文件中
 * @finger_idx: 要保存的的指纹ID下标
 *  
 * @return: 无返回值
 */
static void chips_save_fingerprints(int finger_idx)
{
	memcpy(chips_fingerprint_path,store_path,PATH_SIZE);
 	strcat(chips_fingerprint_path,FILENAME);

	FILE *fp=NULL;	
	if(0!=access(chips_fingerprint_path,0))
	{
		fp = fopen(chips_fingerprint_path, "wb+");     //不存在则创建
	}
	else
	{
		fp = fopen(chips_fingerprint_path, "rb+");  
	}
	if(NULL == fp)
	{
	    ALOGE("<%s> Failed to access fingerprints[%d] from storage:%s, for %s",__FUNCTION__, finger_idx, chips_fingerprint_path, strerror(errno));
		return;
	}
	 
	//将文件偏移量设置到起始位置
	rewind(fp);

	int nn = fwrite(&g_fingernum,sizeof(int),1,fp);
	int fingernum_offset = sizeof(int);
		
	if(fseek(fp,fingernum_offset+finger_idx*sizeof(int),SEEK_SET) < 0)
	{
		ALOGE("<%s> Failed while seeking fingerprints[%d] from storage:%s,for %s",__FUNCTION__, finger_idx, chips_fingerprint_path, strerror(errno));
		fclose(fp);
		return;	
	}

	int ni = fwrite(&g_fingerid[finger_idx],sizeof(int),1,fp);
	int fingerid_offset = fingernum_offset + FINGER_COUNT*sizeof(int);
	
	if(fseek(fp,fingerid_offset+finger_idx*ENROLL_COUNT*sizeof(int),SEEK_SET) < 0)
	{
		ALOGE("<%s> Failed while seeking fingerprints[%d] from storage:%s,for %s",__FUNCTION__, finger_idx, chips_fingerprint_path, strerror(errno));
		fclose(fp);
		return;		
	}

	int nw = fwrite(g_width[finger_idx],ENROLL_COUNT*sizeof(int),1,fp);
	int width_offset = fingerid_offset + FINGER_COUNT*ENROLL_COUNT*sizeof(int);
	
	
	if(fseek(fp,width_offset+finger_idx*ENROLL_COUNT*sizeof(int),SEEK_SET) < 0)
	{
		ALOGE("<%s> Failed while seeking fingerprints[%d] from storage:%s,for %s",__FUNCTION__, finger_idx, chips_fingerprint_path, strerror(errno));
		fclose(fp);
		return;		
	}

	int nh = fwrite(g_height[finger_idx],ENROLL_COUNT*sizeof(int),1,fp);
	int height_offset = width_offset + FINGER_COUNT*ENROLL_COUNT*sizeof(int);
	
	
	if(fseek(fp,height_offset+finger_idx*ENROLL_COUNT*FEATURE_SIZE*sizeof(unsigned char),SEEK_SET) < 0)
	{
		ALOGE("<%s> Failed while seeking fingerprints[%d] from storage:%s,for %s",__FUNCTION__, finger_idx, chips_fingerprint_path, strerror(errno));
		fclose(fp);
		return;		
	}
	int nt = fwrite(g_template[finger_idx],ENROLL_COUNT*FEATURE_SIZE*sizeof(unsigned char),1,fp);
	
	if(nn != 1 || ni != 1 || nw != 1 || nh !=1 || nt != 1)
	{
		ALOGW("<%s> corrupt fingerprints storage:%s,nn = %d,ni = %d,nw = %d, nh = %d, nt = %d",__FUNCTION__,chips_fingerprint_path,nn,ni,nw,nh,nt);
	}
	else
	{
		ALOGI("<%s> Save Fingerprint template success",__FUNCTION__);
	}
	
	fflush(fp);
	fclose(fp);
	return;
}



 /**
 * char_to_short - 将char型的图像数据转化为short型的图像数据
 * @ipdata: char型的图像数据
 * @opdata: short型的图像数据
 * @size: 图像数据的长度
 *  
 * @return: 无返回值
 */
static void char_to_short(unsigned char* ipdata,unsigned short* opdata,int size)
{
	int readSize = 0 ;
	int i = 0;
	int j = 0; 
	for (i = 0,j = 0 ; i < size-1 ; i+= 2,j++ ) 
	{
		opdata[j] =  (((int)ipdata[i]<<8)|ipdata[i+1]);   //寄存器配置fc08=4 
		readSize ++ ; 		
	}
}




static void chips_write_sample_params(void)
{
#if defined(CS358)
	write_SRAM(0xFC3e,0x6808);
	write_SRAM(0xfc34,0x0408);
#elif defined (CS3105)
	#if !defined (GLASS)
	write_SRAM(0xFC3E,0x2000);
	#endif
#elif (defined(CS1175)||defined (CS1073))		
	write_SRAM(0xFC2E,g_sample_gain_value);
	write_SRAM(0xFC30,g_sample_base_value);
#elif(defined(CS3511))	
	write_SRAM(0xFC3e,DEFAULT_SAMPLE_GAIN_BASE_VALUE);
#endif

}

static void chips_write_detect_params(void)
{
#if defined(CS358)
	write_SRAM(0xFC3e,0x680a);
	write_SRAM(0xfc34,0x0108);
#elif defined (CS3105)
	write_SRAM(0xFC3E,0x200a);
#elif (defined(CS1175)||defined (CS1073))		
	write_SRAM(0xFC2E,DEFAULT_DETECT_GAIN_VALUE);
	write_SRAM(0xFC30,DEFAULT_DETECT_BASE_VALUE);
#elif(defined(CS3511))
	write_SRAM(0xFC3e,DEFAULT_DETECT_GAIN_BASE_VALUE);
#endif
}

#if defined(CALIBRATION)
 /**
 * chips_get_sub_block_average_value - 获得sub_block的均值
 * 
 * @return: 成功返回sub_block的均值，失败返回负数
 */
static int chips_get_sub_block_average_value(void)
{
	
	int i;
	int j;
	uint16_t  vcm_val = 0;
	unsigned long long sum = 0;
	int avg_value;
	unsigned char	*src_image_buf_8;
	unsigned char	*dst_image_buf_8;
	int blocksize = 0;
	int retval = 0;
	int status;
#if defined(CS1073)
	write_SRAM(0xFC7C,0x083b);   //Area scan end
	write_SRAM(0xFC7E,0x051c);   //Area scan start    32*32
	blocksize = 32*32;
#elif defined(CS1175)
	write_SRAM(0xFC7C,0x0577);   //Area scan end
	write_SRAM(0xFC7E,0x0340);   //Area scan start    24*56
	blocksize = 24*56;
#endif	

	g_sample_gain_value = DEFAULT_SAMPLE_HIGH_GAIN_VALUE;
	g_sample_base_value = DEFAULT_SAMPLE_HIGH_BASE_VALUE;
	chips_write_sample_params();
	
	src_image_buf_8 = (unsigned char*)malloc(blocksize);
	
	if(NULL == src_image_buf_8)
	{
		ALOGE("<%s> Failed to malloc mem for src_image buffer",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}	
	
	dst_image_buf_8 = (unsigned char*)malloc(blocksize*2);
	
	if(NULL == dst_image_buf_8)
	{
		ALOGE("<%s> Failed to malloc mem for src_image buffer",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}	
	memset(src_image_buf_8,0,blocksize);
	memset(dst_image_buf_8,0,blocksize*2);
	retval = chips_scan_one_image(dst_image_buf_8,blocksize*2);
	if(retval < 0)
	{
		ALOGE("<%s> Failed to get image data",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}
	
	for(j = 0;j < blocksize;j++)
	{
		*(src_image_buf_8 + j) = (*(dst_image_buf_8 + j*2) << 4) + (*(dst_image_buf_8 + j*2 + 1) >> 4);
		//ALOGI("<%s> j = %d *(src_image_buf_8 + j) = 0x%x  0x%x  0x%x ",__FUNCTION__,j,*(src_image_buf_8 + j),*(dst_image_buf_8 + j*2),*(dst_image_buf_8 + j*2 + 1));
	}
	
#if defined(CS1073)
	write_SRAM(0xFC7C,0x0D57);   //Area scan end
	write_SRAM(0xFC7E,0x0000);	 //Area scan start	 88*112
#elif defined(CS1175)
	write_SRAM(0xFC7C,0x06b3);   //Area scan end
	write_SRAM(0xFC7E,0x0000);   //Area scan start   56*180
#endif	

	chips_write_detect_params();
	
	for(i = 0; i < (blocksize); i++)
	{
		sum = sum + *(src_image_buf_8 + i);
	}
	avg_value = sum/(blocksize);
	ALOGI("<%s> avg_value = %d sum =%ld ",__FUNCTION__,avg_value,sum);
	status = avg_value;
out:	
	if(dst_image_buf_8 != NULL)
	{
		free(dst_image_buf_8);
		dst_image_buf_8 = NULL;
	}
	if(src_image_buf_8 != NULL)
	{
		free(src_image_buf_8);
		src_image_buf_8 = NULL;
	}
	

	return status;
}

 /**
 * chips_calibration_sample - 根据均值调整gain、base
 *
 * @avg_val: sub_block均值
 *  
 * @return: 空
 */
void chips_calibration_sample(int avg_val)
{	
	ALOGI("<%s> g_sample_gain_value = 0x%x g_sample_base_value =0x%x",__FUNCTION__,g_sample_gain_value,g_sample_base_value);
	if(avg_val <= 40 && g_sample_gain_value == DEFAULT_SAMPLE_HIGH_GAIN_VALUE)
	{
		g_sample_gain_value = DEFAULT_SAMPLE_LOW_GAIN_VALUE;
		g_sample_base_value = DEFAULT_SAMPLE_LOW_BASE_VALUE;
			
	}
	ALOGI("<%s> g_sample_gain_value = 0x%x g_sample_base_value =0x%x",__FUNCTION__,g_sample_gain_value,g_sample_base_value);
}


 /**
 * chips_auto_calibration - 多组参数
 *
 * @return: 空
 */
void chips_auto_calibration(void)
{
	int avg_val = 0;
	
	avg_val = chips_get_sub_block_average_value();
	if(avg_val < 0)
	{
		ALOGI("<%s> chips_get_sub_block_average_value err",__FUNCTION__);
	}
	else
	{
		chips_calibration_sample(avg_val);
	}
}


#endif
 /**
 * chips_create_feature - 获取指纹特征数据
 * @p_feature: 指纹特征数据
 * @size: 指纹特征数据长度
 *  
 * @return: 成功返回0，失败返回负数
 */
static int chips_create_feature(unsigned char *p_feature,unsigned size)
{
    int i = 0;
	int retval = -1;
	int status = -1;

	int opnCharNum[10] = {0};
	unsigned char  *temp_feature = NULL;
	unsigned char  *image_buf_8= NULL;
	unsigned char  *src_image_buf_8= NULL;
	unsigned short *image_buf_16 = NULL;	

	temp_feature = (unsigned char*)malloc(size);
	if(NULL == temp_feature)
	{
	    ALOGE("<%s> Failed to malloc mem for feature buffer",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}
		
	image_buf_8 = (unsigned char*)malloc(bufsiz);
	if(NULL == image_buf_8)
	{
		ALOGE("<%s> Failed to malloc mem for image buffer",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}
	
	src_image_buf_8 = (unsigned char*)malloc(bufsiz);
	if(NULL == src_image_buf_8)
	{
		ALOGE("<%s> Failed to malloc mem for src_image buffer",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}	
	

	chips_write_sample_params();	

	memset(image_buf_8,0,bufsiz);
	memset(src_image_buf_8,0,bufsiz);
	
	
#if defined(USE_12_BITS)	
	retval = chips_scan_one_image(src_image_buf_8,bufsiz*3/4);
#else
	retval = chips_scan_one_image(src_image_buf_8,bufsiz);
#endif
	chips_write_detect_params();
	if(retval < 0)
	{
		ALOGE("<%s> Failed to get image data",__FUNCTION__);		
		status = RET_ERROR;
		goto out;
	}
	else
	{
		ALOGI("<%s> get image data success",__FUNCTION__);
	
#if (defined (CS3105) || defined(CS336) || defined(CS358)|| defined(CS3511))
	#if !defined(CS3105)
		#if defined(USE_12_BITS)
			chips_deciphering_image(src_image_buf_8,bufsiz*3/4);
		#else
			chips_deciphering_image(src_image_buf_8,bufsiz);
		#endif
	#endif
		chips_convert_image_raw_data(image_buf_8,src_image_buf_8,IMAGE_W,IMAGE_H);
	#if defined(CS358)
		chips_get_true_image(image_buf_8);
	#endif
#else
		memcpy(image_buf_8,src_image_buf_8,bufsiz);
#endif
	 
#if defined(SAMPLE_DEBUG)	
		chips_save_fpsample(image_buf_8,IMAGE_W,IMAGE_H);	
#endif	
  	
	}

#if (defined(USE_16_BITS)||defined(USE_12_BITS))
	image_buf_16 = (unsigned short*)malloc(bufsiz);
	if(NULL == image_buf_16)
	{
	    ALOGE("<%s> Failed to malloc mem for image buffer",__FUNCTION__);
		status = RET_ERROR;	
		goto out;
	}

    memset(image_buf_16,0,bufsiz);	
	char_to_short(image_buf_8,image_buf_16,bufsiz);

	memset(temp_feature,0,size);
printf("createtemplate ...\n");
	retval = ChipSailing_CreateTemplate16(image_buf_16, image_buf_8, (short)IMAGE_W, (short)IMAGE_H, temp_feature, opnCharNum);
#elif defined(USE_8_BITS)
    memset(temp_feature,0,size);
    retval = ChipSailing_CreateTemplate(image_buf_8,(short)IMAGE_W, (short)IMAGE_H, temp_feature, opnCharNum);
#endif

    if(retval == 1)
	{
	    ALOGI("<%s> create template success,%d,%d,%d,%d", __FUNCTION__,opnCharNum[0],opnCharNum[1],opnCharNum[2],opnCharNum[3]);	
		memcpy(p_feature,temp_feature,size);

		status = RET_OK;
	}
	else if(retval == -31)
	{
		 ALOGE("<%s> create template error for bad image,retval = %d", __FUNCTION__,retval);
		 //mlx_proto_gui_init(GUI_WINDOW_BUTTON_USER_INPUT);
	         //sleep(10);
		 status = RET_ERROR;
	 
	}
	else
	{
	    ALOGE("<%s> create template error,retval = %d", __FUNCTION__,retval);
		//mlx_proto_change_rolling_text(3, " ");
		//sleep(1);
		//mlx_proto_change_rolling_text(2, " ");
		//sleep(1);
		//mlx_proto_change_rolling_text(3, "Template Error");
		//sleep(2);
		//mlx_proto_change_rolling_text(3, " ");
		//sleep(1);
		
		status = RET_ERROR;		
	}

out:
	if(image_buf_16 != NULL)
	{
	    free(image_buf_16);
	    image_buf_16 = NULL;
	}

	if(image_buf_8 != NULL)
	{
		free(image_buf_8);
		image_buf_8 = NULL;
	}

	if(temp_feature != NULL)
	{
	    free(temp_feature);
		temp_feature = NULL;
	}

	if(src_image_buf_8 != NULL)
	{
		free(src_image_buf_8);
		src_image_buf_8 = NULL;
	}

    return status;	
}



 /**
 * chips_prepare_enroll - 初始化指纹录入时的存储区域
 *  
 * @return: 成功返回0，失败返回负数
 * @detail: 进入指纹录入界面时调用，仅需调用一次
 */
int chips_prepare_enroll(void)
{
    int finger_idx = 0;
	int retval = -1;
	
	ALOGI("<%s> IMAGE_W = %d,IMAGE_H = %d",__FUNCTION__,IMAGE_W,IMAGE_H);	
	
	retval = chips_get_finger_idx(&finger_idx);
	if(retval < 0)
	{
	    ALOGE("<%s> Failed to get Fingerprint finger_idx",__FUNCTION__);
	    return RET_ERROR;
	}
	
	//initialize
	g_cur_enroll_idx = 0;
	memset(g_width[finger_idx],0,ENROLL_COUNT*sizeof(int));
	memset(g_height[finger_idx],0,ENROLL_COUNT*sizeof(int));
	memset(g_template[finger_idx],0,ENROLL_COUNT*FEATURE_SIZE*sizeof(unsigned char));
	
	return RET_OK;
}




 /**
 * chips_enroll_finger - 指纹录入
 * @remaining_times: 成功时存放一枚指纹录入的剩余次数
 *  
 * @return: 成功返回0，失败返回负数
 * @detail: 每次指纹录入时调用
 */
int chips_enroll_finger(int *remaining_times)
{
    unsigned char *temp_feature = NULL;
	unsigned char *merge_feature = NULL;
	int retval = -1; 
	int status = -1;
	int i = 0;
	int j = 0;
	int finger_idx = 0;
	short match_score = 0;
	
	retval = chips_get_finger_idx(&finger_idx);
	if(retval < 0)
	{
		ALOGE("<%s> Failed to get Fingerprint finger_idx",__FUNCTION__);
	    return RET_ERROR;
	}	

	temp_feature = (unsigned char*)malloc(FEATURE_SIZE);
	if(NULL == temp_feature)
	{
	    ALOGE("<%s> Failed to allocate mem for feature buffer",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}
	
	merge_feature = (unsigned char*)malloc(FEATURE_SIZE);
	if(NULL == merge_feature)
	{
	    ALOGE("<%s> Failed to allocate mem for feature buffer",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}	
	
	memset(temp_feature,0,FEATURE_SIZE);
	retval = chips_create_feature(temp_feature,FEATURE_SIZE);
	if(retval < 0)
	{
	    ALOGE("<%s> Failed to get fingerprint Feature data,retval = %d",__FUNCTION__, retval);
		status = RET_ERROR;	
		goto out;
	}	

	
#if defined(NO_REPEAT_ENROLL)
	//判断手指是否重复录入
	for(i = 0; i < ENROLL_COUNT; i++)
	{
	    for(j = 0; j < FINGER_COUNT; j++)
		{
		    if(g_fingerid[j] == 0)
			    continue;
			
			ALOGI("<%s> matchscore...", __FUNCTION__);	
			match_score = ChipSailing_MatchScore(g_template[j][i],temp_feature);
			
			ALOGI("<%s> matchscore end.", __FUNCTION__);	
			if(match_score >= 45)
			{
				ALOGE("<%s> Fingerprint is exist,fid = %d",__FUNCTION__, g_fingerid[j]);				
				status = RET_FINGER_EXIST;
				goto out;
			}	
		}
	}
#endif

	
	
	unsigned short nPnt = 0;
	unsigned short nNewWidth = 0;
	unsigned short nNewHeight = 0; 
    int mergret = 0;  	
	
	ALOGI("<%s> g_cur_enroll_idx = %d",__FUNCTION__,g_cur_enroll_idx);
	if(g_cur_enroll_idx == 0)
	{
        g_width[finger_idx][g_cur_enroll_idx] = IMAGE_W;
	    g_height[finger_idx][g_cur_enroll_idx] = IMAGE_H;
	    memcpy(g_template[finger_idx][g_cur_enroll_idx],temp_feature,FEATURE_SIZE);
		
		g_cur_enroll_idx++;
		*remaining_times = ENROLL_COUNT - g_cur_enroll_idx;
		status = RET_OK;
		goto out;
	}
	else if((g_cur_enroll_idx > 0) && (g_cur_enroll_idx < ENROLL_COUNT))
	{
		//1、判断新录入模板对现存模板的影响：是否扩大现存模板？
	    for(i = 0; i < g_cur_enroll_idx; i++)
		{
			nPnt = 0;
			nNewWidth = 0;
			nNewHeight = 0;
			memset(merge_feature,0,FEATURE_SIZE);
		    if(0 == ChipSailing_MergeFeature(g_template[finger_idx][i], temp_feature, merge_feature, &nPnt, &nNewWidth,&nNewHeight))
			{
				ALOGI("<%s> ChipSailing_MergeFeature succes, merge_enroll_idx = %d,nNewWidth = %d,nNewHeight = %d,similarity = %d",__FUNCTION__,i,nNewWidth,nNewHeight,nPnt);
			    memset(g_template[finger_idx][i],0,FEATURE_SIZE);
			    memcpy(g_template[finger_idx][i],merge_feature,FEATURE_SIZE);
				g_width[finger_idx][i] = nNewWidth;
			    g_height[finger_idx][i] = nNewHeight;
#if defined(NO_SAME_SPACE)	
				if(nPnt > ENROLL_SAME_SPACE_SCORE)
				{
			        mergret++;
			    }     
#endif			    
			}
			else
			{
				ALOGI("<%s> ChipSailing_MergeFeature error, merge_enroll_idx = %d,nNewWidth = %d,nNewHeight = %d,similarity = %d",__FUNCTION__,i,nNewWidth,nNewHeight,nPnt);
			}
		}
		
		//判断新录入模板对本身的影响：是否得以保存自己？
		//如果手指是按压重复区域，不保存模板；否则保存模板
		if(mergret > 0)
		{
		    ALOGE("<%s> Touched the same space,mergret = %d",__FUNCTION__, mergret);
			status = RET_SAME_SPACE;
			goto out;
		}
		else
		{
			g_width[finger_idx][g_cur_enroll_idx] = IMAGE_W;
	        g_height[finger_idx][g_cur_enroll_idx] = IMAGE_H;
		    memcpy(g_template[finger_idx][g_cur_enroll_idx],temp_feature,FEATURE_SIZE);
			
			g_cur_enroll_idx++;
		    *remaining_times = ENROLL_COUNT - g_cur_enroll_idx;
			//完整录完一枚手指
			if(*remaining_times == 0)
			{
			    g_cur_enroll_idx = 0;
			}
			
			status = RET_OK;
			goto out;
		}
	}
	else
	{
	    ALOGE("<%s> enroll times is out of range,g_cur_enroll_idx = %d",__FUNCTION__, g_cur_enroll_idx);	
		status = RET_ERROR;
		goto out;
	}
	
out:
    if(temp_feature != NULL)
	{
		free(temp_feature);
		temp_feature = NULL;
	}
	
	if(merge_feature != NULL)
	{
		free(merge_feature);
		merge_feature = NULL;
	}
	
	return status;
}




 /**
 * chips_update_template - 保存指纹数据到文件中
 * @fid: 要保存的指纹ID
 *  
 * @return: 成功返回0，失败返回负数
 * @detail: 完整录完一枚指纹时调用
 */
int chips_update_template(int fid)
{
	ALOGI("<%s> Save Fingerprint template, fid = %d",__FUNCTION__,fid);
	
    int finger_idx = 0;
	int retval = -1;
	
	retval = chips_get_finger_idx(&finger_idx);
	if(retval < 0)
	{
		ALOGE("<%s> Failed to get Fingerprint finger_idx",__FUNCTION__);
	    return RET_ERROR;
	}
		
	g_fingerid[finger_idx] = fid;
	g_fingernum++;
	sort(finger_idx);
	chips_save_fingerprints(finger_idx);
	
	return RET_OK;		
}




 /**
 * chips_verify - 指纹匹配
 * @matched_fid: 成功时返回匹配的指纹ID
 *  
 * @return: 成功返回0，失败返回负数
 * @detail: 匹配指纹时调用
 */
int chips_verify(int *matched_fid)
{
	int retval = -1;
	int i = 0;
	int j = 0;
	int loop = 0;
	short match_score = 0;
	bool isMatched = false;	
	
	for(loop = 0; loop < 3; loop++)
	{
		memset(g_match_template,0,FEATURE_SIZE);
		retval = chips_create_feature(g_match_template,FEATURE_SIZE);
		if(retval < 0)
		{
			ALOGE("<%s> Failed to get fingerprint feature data,retval = %d",__FUNCTION__, retval);
			//mlx_proto_change_rolling_text(2, " ");
			//sleep(1);
                	//mlx_proto_change_rolling_text(3, " ");
			//sleep(1);
			//mlx_proto_change_rolling_text(2, "FP Feature Data Failed");
			//sleep(2);
			//mlx_proto_change_rolling_text(2, " ");
			//sleep(1);

			continue;	
		}
		for(j = 0; j < FINGER_COUNT_SUPPORT*PERSON_COUNT; j++)
		{
			if(g_fingerid[j] == 0)
			{
				continue;
			}	
			for(i = 0; i < ENROLL_COUNT; i++)
			{							
				match_score = ChipSailing_MatchScore(g_template[j][i],g_match_template);
				ALOGI("<%s> ChipSailing_MatchScore ok, match_score = %d",__FUNCTION__,match_score);
				if(match_score >= 45)
				{	
					ALOGI("<%s> finger_idx = %d,enroll_idx = %d",__FUNCTION__,j,i);
					g_cur_matched_enroll_idx = i;
					*matched_fid = g_fingerid[j];
					isMatched = true;
					break;		
				}	
			}
			
			if(isMatched)
				break;
		}
		
		if(isMatched){
			break;
		}	
	}
		
	if(isMatched)
	{
		ALOGI("<%s> verify ok,matched_fid = %d",__FUNCTION__,*matched_fid);
		return RET_OK;
	}
	else
	{
		ALOGE("<%s> verify error",__FUNCTION__);
		//mlx_proto_change_status_icon(GUI_STATUS_ICON_WRONG);
		//mlx_proto_change_rolling_text(2, " ");
		//sleep(1);
                //mlx_proto_change_rolling_text(3, " ");
		//sleep(1);
				
		return RET_ERROR;

		
	}
}




 /**
 * chips_renew_feature - 更新指纹模板
 * @fid: 要更新模板的指纹ID
 *  
 * @return: 成功返回0，失败返回负数
 * @detail: 指纹匹配成功后调用！
 */
int chips_renew_feature(int fid)
{
	int finger_idx = 0;
	int enroll_idx = 0;
	int retval = -1;
	int status = -1;
	unsigned short nPnt = 0;
	unsigned short nNewWidth = 0;
	unsigned short nNewHeight = 0;
	unsigned char *renew_feature = NULL;
	unsigned char *merge_feature = NULL;
	
	ALOGI("<%s> g_cur_matched_enroll_idx = %d",__FUNCTION__,g_cur_matched_enroll_idx);
	for(finger_idx = 0; finger_idx < FINGER_COUNT; finger_idx++)
	{
	    if(g_fingerid[finger_idx] == fid)
		    break;
	}
	
	if(finger_idx >= FINGER_COUNT)
	{
	    ALOGE("<%s> Fingerprint ID %d not found",__FUNCTION__,fid);
		status = RET_ERROR;
		goto out;
	}
	
	renew_feature = (unsigned char*)malloc(FEATURE_SIZE);
	if(NULL == renew_feature)
	{
	    ALOGE("<%s> Failed to allocate mem for feature buffer",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}
	
	merge_feature = (unsigned char*)malloc(FEATURE_SIZE);
	if(NULL == merge_feature)
	{
	    ALOGE("<%s> Failed to allocate mem for feature buffer",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}	
	
	memset(renew_feature,0,FEATURE_SIZE);
	retval = ChipSailing_RenewFeature(renew_feature, &nPnt, &nNewWidth, &nNewHeight);
	if(retval == 1)
	{
		ALOGE("<%s> Failed to renew feature",__FUNCTION__);
		status = RET_ERROR;
		goto out;
	}
	memset(g_template[finger_idx][g_cur_matched_enroll_idx],0,FEATURE_SIZE);
	memcpy(g_template[finger_idx][g_cur_matched_enroll_idx],renew_feature,FEATURE_SIZE);
	g_width[finger_idx][g_cur_matched_enroll_idx] = nNewWidth;
	g_height[finger_idx][g_cur_matched_enroll_idx] = nNewHeight;	
	
	//bool isRenewed = false;
	for(enroll_idx = g_cur_matched_enroll_idx+1; enroll_idx < ENROLL_COUNT; enroll_idx++)
	{
		nPnt = 0;
		nNewWidth = 0;
		nNewHeight = 0;
		memset(merge_feature,0,FEATURE_SIZE);
		if(0 == ChipSailing_MergeFeature(g_template[finger_idx][enroll_idx],g_match_template,merge_feature, &nPnt, &nNewWidth,&nNewHeight))
		{
			ALOGI("<%s> ChipSailing_MergeFeature succes, merge_enroll_idx = %d,nNewWidth = %d,nNewHeight = %d,similarity = %d",__FUNCTION__,enroll_idx,nNewWidth,nNewHeight,nPnt);
			memset(g_template[finger_idx][enroll_idx],0,FEATURE_SIZE);
			memcpy(g_template[finger_idx][enroll_idx],merge_feature,FEATURE_SIZE);
			g_width[finger_idx][enroll_idx] = nNewWidth;
			g_height[finger_idx][enroll_idx] = nNewHeight;		
			//isRenewed = true;				
		}
		else
		{
			ALOGI("<%s> ChipSailing_MergeFeature error, merge_enroll_idx = %d,nNewWidth = %d,nNewHeight = %d,similarity = %d",__FUNCTION__,enroll_idx,nNewWidth,nNewHeight,nPnt);
		}
	}	
	
	status = RET_OK;
	
out:
	if(renew_feature != NULL)
	{
		free(renew_feature);
		renew_feature = NULL;
	}
	
	if(merge_feature != NULL)
	{
		free(merge_feature);
		merge_feature = NULL;
	}
	
	if(status == RET_OK)
	{
		sort(finger_idx);
		chips_save_fingerprints(finger_idx);
	}
	
	return status;
}



 /**
 * chips_delete_finger - 删除指纹数据
 * @fid: 要删除的指纹ID
 *  
 * @return: 成功返回0，失败返回负数
 * @detail: 删除指纹时调用
 */
int chips_delete_finger(int fid)
{
    int finger_idx = 0;
	
#if 1	
	for(finger_idx = 0; finger_idx < FINGER_COUNT; finger_idx++)
	{
        ALOGI("<%s> g_fingerid[%d] = %d",__FUNCTION__,finger_idx,g_fingerid[finger_idx]);
	}
#endif 

	for(finger_idx = 0; finger_idx < FINGER_COUNT; finger_idx++)
	{
	    if(g_fingerid[finger_idx] == fid)
		    break;
	}
	
	if(finger_idx >= FINGER_COUNT)
	{
	    ALOGE("<%s> FingerprintID not found,fid = %d",__FUNCTION__,fid);
		return RET_ERROR;
	}
	
	g_fingernum--;
	g_fingerid[finger_idx] = 0;
	memset(g_width[finger_idx],0,ENROLL_COUNT*sizeof(int));
	memset(g_height[finger_idx],0,ENROLL_COUNT*sizeof(int));
	memset(g_template[finger_idx],0,ENROLL_COUNT*FEATURE_SIZE*sizeof(unsigned char));
	chips_save_fingerprints(finger_idx);
	
#if 1	
	for(finger_idx = 0; finger_idx < FINGER_COUNT; finger_idx++)
	{
        ALOGI("<%s> g_fingerid[%d] = %d",__FUNCTION__,finger_idx,g_fingerid[finger_idx]);
	}
#endif 

	return RET_OK;
}



 /**
 * chips_get_fingernum - 获取当前的指纹列表中指纹个数
 *  
 * @return: 当前的指纹列表中指纹个数
 */
int chips_get_fingernum(void)
{
	return g_fingernum;
}

