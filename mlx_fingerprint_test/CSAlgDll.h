#ifndef _CS_ALG_DLL_H_
#define _CS_ALG_DLL_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*==============================================================================
����:ChipSailing_Init
����:	��ʼ����ֻ���ڳ�ʼ����ʱ�����һ��
����:
read_register�����룬���Ĵ�������ָ��
write_register - ���룬д�Ĵ�������ָ��
nChipIdex - ���룬оƬ�ͺţ�1��1501/2511    2:1801/2811   3��3711      5:3511

==============================================================================*/
int ChipSailing_Init(unsigned char (*read_register)(unsigned char addr),
		int (*write_register)(unsigned char addr, unsigned char value),int nChipIdex);



/*==============================================================================
����:ChipSailing_AutoGain   
����:	�Զ�����   8λ��
����:
data			�����룬ָ������
nWidth			- ���룬ָ��ͼ���
nHeight			�����룬ָ��ͼ���
pGainBase       - ���룬��ǰ����BASEֵ   pGainBase[0]:���棻pGainBase[1]:base��pGainBase[2]:gray��
pNewGainBase    �����룬����������BASEֵ ͬpGainBase

����ֵ:	
        1:��Ҫ��������BASEֵ�����²ɼ�ͼ��
        0:����Ҫ��������BASEֵ�Ҳ���Ҫ���²ɼ�ͼ��
==============================================================================*/
int ChipSailing_AutoGain(unsigned char * data, int nWidth, int nHeight, int *pGainBase, int *pNewGainBase );

/*==============================================================================
����:ChipSailing_AutoGain16  
����:	�Զ����� 16λ��
����:
data			�����룬ָ������
nWidth			- ���룬ָ��ͼ���
nHeight			�����룬ָ��ͼ���
pGainBase       - ���룬��ǰ����BASEֵ    pGainBase[0]:���棻pGainBase[1]:base��pGainBase[2]:gray��
pNewGainBase    �����룬����������BASEֵ  ͬpGainBase

����ֵ:	
        1:��Ҫ��������BASEֵ�����²ɼ�ͼ��
        0:����Ҫ��������BASEֵ�Ҳ���Ҫ���²ɼ�ͼ��
==============================================================================*/
int ChipSailing_AutoGain16(unsigned short * data, int nWidth, int nHeight, int *pGainBase, int *pNewGainBase );


/*==============================================================================
����:ChipSailing_Enhance16to8
����:	 16λ���봦���8λ���
����:
ipImage�����룬ԭʼͼ������
opImage���������ǿ���ͼ������
iImgW - ���룬ͼ����
iImgH - ���룬ͼ��߶�

����ֵ:	1:�ɹ���else��ʧ�ܡ�
==============================================================================*/
signed short ChipSailing_Enhance16to8(unsigned short *ipImage, unsigned char *opImage,signed short iImgW, signed short iImgH);


/*==============================================================================
����:ChipSailing_CreateTemplate
����:	��ȡ��������   8λ��
����:
iopImage�����������ͼ������
iImgW - ���룬ͼ����
iImgH - ���룬ͼ��߶�
opFeature �� �������������,��СΪ28000*sizeof(unsigned char)
opnCharNum - ��СΪ10�����飬
             opnCharNum[0]:��ǰͼ��ָ����������;
			 opnCharNum[1]:��ǰͼ�񸲸����ص����;
			 opnCharNum[2]:��ʾ��һ��¼�뷽��  0�����ң�1�����ϣ�2������3������;
			 opnCharNum[3]:��ǰͼ��ͼ��������0-100��;
			 others:Ԥ����

����ֵ:	1:�ɹ���else��ʧ�ܡ�
==============================================================================*/
signed short ChipSailing_CreateTemplate(unsigned char *iopImage, signed short iImgW, signed short iImgH, unsigned char *opFeature, int *opnCharNum);


/*==============================================================================
����:ChipSailing_CreateTemplate16
����:	��ȡ��������   16λ��
����:
ipImage�����룬ԭʼͼ������
opImage���������ǿ���ͼ������
iImgW - ���룬ͼ����
iImgH - ���룬ͼ��߶�
opFeature �� �������������,��СΪ28000*sizeof(unsigned char)
opnCharNum - ��СΪ10�����飬
             opnCharNum[0]:��ǰͼ��ָ����������;
			 opnCharNum[1]:��ǰͼ�񸲸����ص����;
			 opnCharNum[2]:��ʾ��һ��¼�뷽��  0�����ң�1�����ϣ�2������3������; ���巽����ҪFAE����ģ��ڷŵ�λ����������
			 opnCharNum[3]:��ǰͼ��ͼ��������0-100��;
			 others:Ԥ����

����ֵ:	1:�ɹ���else��ʧ�ܡ�
==============================================================================*/
signed short ChipSailing_CreateTemplate16(unsigned short *ipImage, unsigned char *opImage,signed short iImgW, signed short iImgH, unsigned char *opFeature, int *opnCharNum);


/*============================================================================
���ƣ�ChipSailing_MatchScore
���ܣ��������������о�ȷ�ȶԣ����رȵĽ��
������
piFeatureA:����A����
piFeatureB:����B����
���أ��ȶԷ���
============================================================================*/
signed short ChipSailing_MatchScore(unsigned char *piFeatureA, unsigned char *piFeatureB);


/*============================================================================
���ƣ�ChipSailing_MergeFeature
���ܣ��ϲ�����ָ������
������
piFeatureA:����A���ݣ���ģ������
piFeatureB:����B���ݣ���¼��ָ������
opFeature: �ϲ����ָ������
pnPnt:   �����ָ������ռ����ͼ��İٷֱȣ���Χ0-100
pnNewWidth:�ϲ����ָ�ƿ��
pnNewHeight���ϲ����ָ�Ƹ߶�
���أ�0�ɹ���1ʧ��
============================================================================*/
signed short ChipSailing_MergeFeature(unsigned char *piFeatureA, unsigned char *piFeatureB, unsigned char* opFeature, unsigned short *pnPnt, unsigned short *pnNewWidth, unsigned short* pnNewHeight);


/*============================================================================
���ƣ�ChipSailing_RenewFeature
���ܣ�����ָ������
������
opFeature: ���º��ָ������
pnPnt:   �ϲ����ָ���������������60��
pnNewWidth:���º��ָ�ƿ��
pnNewHeight�����º��ָ�Ƹ߶�
���أ�0�ɹ���1ʧ��
============================================================================*/
signed short ChipSailing_RenewFeature(unsigned char* opFeature, unsigned short *pnPnt, unsigned short *pnNewWidth, unsigned short* pnNewHeight);


/*============================================================================
���ƣ�ChipSailing_CalCentroid
���ܣ�����ָ����������
������
lpInBuffer: ���룬ԭʼͼ������
iImgW �� ���룬ͼ����
iImgH �� ���룬ͼ��߶�
pX    �� ���������ˮƽ���� 
pY    �� ��������Ĵ�ֱ���� 
���أ�1:����ָ���£�0��û����ָ����
============================================================================*/
int ChipSailing_CalCentroid(unsigned char *lpInBuffer, int nImgW, int nImgH, int *pX, int *pY);


/*==============================================================================
����:	ChipSailing_DetectFinger
����:	̽���Ƿ�����ָ
����:
iopImage - ���룬ͼ������
iImgW - ���룬ͼ����
iImgH - ���룬ͼ��߶�
nMinMean - ���룬ͼ����С��ֵ
nMaxMean - ���룬ͼ������ֵ

����ֵ:	��ָ���ռ����ͼ��ٷֱ�
==============================================================================*/
signed short ChipSailing_DetectFinger(unsigned char *iopImage, unsigned short iImgW, unsigned short iImgH,unsigned char nMinMean,unsigned char nMaxMean );


/*==============================================================================
����:	ChipSailing_SignalStrength
����:	���㵱ǰͼ���źŵ�ǿ��������8λ���ݣ�
����:
iopImage - ���룬ͼ������
iImgW - ���룬ͼ����
iImgH - ���룬ͼ��߶�


����ֵ:	��ǰͼ���źŵ�ǿ��
==============================================================================*/
int ChipSailing_SignalStrength(unsigned char *iopImage, signed short iImgW, signed short iImgH);


/*==============================================================================
����:	ChipSailing_SignalStrength16
����:	���㵱ǰͼ���źŵ�ǿ��������16λ���ݣ�
����:
iopImage - ���룬ͼ������
iImgW - ���룬ͼ����
iImgH - ���룬ͼ��߶�


����ֵ:	��ǰͼ���źŵ�ǿ��
==============================================================================*/
int ChipSailing_SignalStrength16(unsigned char *iopImage, signed short iImgW, signed short iImgH);


/*==============================================================================
����:	ChipSailing_GetAlgVersion
����:	��ȡ�㷨�汾��
����:
pstrbuf:�㷨�汾�Ŵ�Ż���������СΪ100�ֽ�

����ֵ:	 ��
==============================================================================*/
void ChipSailing_GetAlgVersion(char *pstrbuf);



#ifdef __cplusplus
}
#endif


#endif	// _CS_ALG_DLL_H_
