#ifndef _CS_ALG_DLL_H_
#define _CS_ALG_DLL_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*==============================================================================
函数:ChipSailing_Init
功能:	初始化，只需在初始化的时候调用一次
参数:
read_register－输入，读寄存器函数指针
write_register - 输入，写寄存器函数指针
nChipIdex - 输入，芯片型号，1：1501/2511    2:1801/2811   3：3711      5:3511

==============================================================================*/
int ChipSailing_Init(unsigned char (*read_register)(unsigned char addr),
		int (*write_register)(unsigned char addr, unsigned char value),int nChipIdex);



/*==============================================================================
函数:ChipSailing_AutoGain   
功能:	自动增益   8位用
参数:
data			－输入，指纹数据
nWidth			- 输入，指纹图像宽
nHeight			－输入，指纹图像高
pGainBase       - 输入，当前增益BASE值   pGainBase[0]:增益；pGainBase[1]:base；pGainBase[2]:gray；
pNewGainBase    －输入，调整后增益BASE值 同pGainBase

返回值:	
        1:需要更新增益BASE值且重新采集图像；
        0:不需要更新增益BASE值且不需要重新采集图像。
==============================================================================*/
int ChipSailing_AutoGain(unsigned char * data, int nWidth, int nHeight, int *pGainBase, int *pNewGainBase );

/*==============================================================================
函数:ChipSailing_AutoGain16  
功能:	自动增益 16位用
参数:
data			－输入，指纹数据
nWidth			- 输入，指纹图像宽
nHeight			－输入，指纹图像高
pGainBase       - 输入，当前增益BASE值    pGainBase[0]:增益；pGainBase[1]:base；pGainBase[2]:gray；
pNewGainBase    －输入，调整后增益BASE值  同pGainBase

返回值:	
        1:需要更新增益BASE值且重新采集图像；
        0:不需要更新增益BASE值且不需要重新采集图像。
==============================================================================*/
int ChipSailing_AutoGain16(unsigned short * data, int nWidth, int nHeight, int *pGainBase, int *pNewGainBase );


/*==============================================================================
函数:ChipSailing_Enhance16to8
功能:	 16位输入处理成8位输出
参数:
ipImage－输入，原始图像数据
opImage－输出，增强后的图像数据
iImgW - 输入，图像宽度
iImgH - 输入，图像高度

返回值:	1:成功，else：失败。
==============================================================================*/
signed short ChipSailing_Enhance16to8(unsigned short *ipImage, unsigned char *opImage,signed short iImgW, signed short iImgH);


/*==============================================================================
函数:ChipSailing_CreateTemplate
功能:	提取特征函数   8位用
参数:
iopImage－输入输出，图像数据
iImgW - 输入，图像宽度
iImgH - 输入，图像高度
opFeature － 输出，特征数据,大小为28000*sizeof(unsigned char)
opnCharNum - 大小为10的数组，
             opnCharNum[0]:当前图像指纹特征个数;
			 opnCharNum[1]:当前图像覆盖像素点个数;
			 opnCharNum[2]:提示下一次录入方向  0：向右；1：向上；2：向左；3：向下;
			 opnCharNum[3]:当前图像图像质量（0-100）;
			 others:预留。

返回值:	1:成功，else：失败。
==============================================================================*/
signed short ChipSailing_CreateTemplate(unsigned char *iopImage, signed short iImgW, signed short iImgH, unsigned char *opFeature, int *opnCharNum);


/*==============================================================================
函数:ChipSailing_CreateTemplate16
功能:	提取特征函数   16位用
参数:
ipImage－输入，原始图像数据
opImage－输出，增强后的图像数据
iImgW - 输入，图像宽度
iImgH - 输入，图像高度
opFeature － 输出，特征数据,大小为28000*sizeof(unsigned char)
opnCharNum - 大小为10的数组，
             opnCharNum[0]:当前图像指纹特征个数;
			 opnCharNum[1]:当前图像覆盖像素点个数;
			 opnCharNum[2]:提示下一次录入方向  0：向右；1：向上；2：向左；3：向下; 具体方向需要FAE根据模组摆放的位置做调整；
			 opnCharNum[3]:当前图像图像质量（0-100）;
			 others:预留。

返回值:	1:成功，else：失败。
==============================================================================*/
signed short ChipSailing_CreateTemplate16(unsigned short *ipImage, unsigned char *opImage,signed short iImgW, signed short iImgH, unsigned char *opFeature, int *opnCharNum);


/*============================================================================
名称：ChipSailing_MatchScore
功能：对两个特征进行精确比对，返回比的结果
参数：
piFeatureA:特征A数据
piFeatureB:特征B数据
返回：比对分数
============================================================================*/
signed short ChipSailing_MatchScore(unsigned char *piFeatureA, unsigned char *piFeatureB);


/*============================================================================
名称：ChipSailing_MergeFeature
功能：合并两个指纹特征
参数：
piFeatureA:特征A数据，既模板数据
piFeatureB:特征B数据，既录入指纹数据
opFeature: 合并后的指纹特征
pnPnt:   输出，指纹区域占整个图像的百分比，范围0-100
pnNewWidth:合并后的指纹宽度
pnNewHeight：合并后的指纹高度
返回：0成功，1失败
============================================================================*/
signed short ChipSailing_MergeFeature(unsigned char *piFeatureA, unsigned char *piFeatureB, unsigned char* opFeature, unsigned short *pnPnt, unsigned short *pnNewWidth, unsigned short* pnNewHeight);


/*============================================================================
名称：ChipSailing_RenewFeature
功能：更新指纹特征
参数：
opFeature: 更新后的指纹特征
pnPnt:   合并后的指纹特征个数，最大60个
pnNewWidth:更新后的指纹宽度
pnNewHeight：更新后的指纹高度
返回：0成功，1失败
============================================================================*/
signed short ChipSailing_RenewFeature(unsigned char* opFeature, unsigned short *pnPnt, unsigned short *pnNewWidth, unsigned short* pnNewHeight);


/*============================================================================
名称：ChipSailing_CalCentroid
功能：计算指纹区域质心
参数：
lpInBuffer: 输入，原始图像数据
iImgW ： 输入，图像宽度
iImgH ： 输入，图像高度
pX    ： 输出，质心水平坐标 
pY    ： 输出，质心垂直坐标 
返回：1:有手指按下，0：没有手指按下
============================================================================*/
int ChipSailing_CalCentroid(unsigned char *lpInBuffer, int nImgW, int nImgH, int *pX, int *pY);


/*==============================================================================
函数:	ChipSailing_DetectFinger
功能:	探测是否有手指
参数:
iopImage - 输入，图像数据
iImgW - 输入，图像宽度
iImgH - 输入，图像高度
nMinMean - 输入，图像最小均值
nMaxMean - 输入，图像最大均值

返回值:	手指面积占整个图像百分比
==============================================================================*/
signed short ChipSailing_DetectFinger(unsigned char *iopImage, unsigned short iImgW, unsigned short iImgH,unsigned char nMinMean,unsigned char nMaxMean );


/*==============================================================================
函数:	ChipSailing_SignalStrength
功能:	计算当前图像信号的强弱，用于8位数据；
参数:
iopImage - 输入，图像数据
iImgW - 输入，图像宽度
iImgH - 输入，图像高度


返回值:	当前图像信号的强弱
==============================================================================*/
int ChipSailing_SignalStrength(unsigned char *iopImage, signed short iImgW, signed short iImgH);


/*==============================================================================
函数:	ChipSailing_SignalStrength16
功能:	计算当前图像信号的强弱，用于16位数据；
参数:
iopImage - 输入，图像数据
iImgW - 输入，图像宽度
iImgH - 输入，图像高度


返回值:	当前图像信号的强弱
==============================================================================*/
int ChipSailing_SignalStrength16(unsigned char *iopImage, signed short iImgW, signed short iImgH);


/*==============================================================================
函数:	ChipSailing_GetAlgVersion
功能:	读取算法版本号
参数:
pstrbuf:算法版本号存放缓冲区，大小为100字节

返回值:	 无
==============================================================================*/
void ChipSailing_GetAlgVersion(char *pstrbuf);



#ifdef __cplusplus
}
#endif


#endif	// _CS_ALG_DLL_H_
