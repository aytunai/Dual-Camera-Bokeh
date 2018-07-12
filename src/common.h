#ifndef _COMMON_H
#define _COMMON_H

#define PC_TEST	//PC���Եĺ�

// Error type code
typedef enum {
	NO_ERROR = 0,
	FILE_READ_ERROR,
	FILE_WRITE_ERROR,
	MEMORY_ERROR,
	OTHER_ERROR,
} ErrorType;

#define CPU_LOG
//�Ƿ����VPU��LOG��Ϣ
#ifdef	CPU_LOG
#define xa_printf(...)	printf(__VA_ARGS__)
#else
#define xa_printf(...)
#endif

//ģ���ֻ�����ʵ�Ŀ�������
typedef struct {
	//YUVͼ��ķֱ���
	int width;
	int height;
	int stride;
	//(��ʱֻ֧��blurYLevel = 1,blurUVLevel1)
	int blurYLevel;		//�黯����(1-30)
	int blurUVLevel;	//�黯����(1-3)

	//����������PC��Ч
	//ͬ����־λ,���ݽ�����ʼ��Ӧ������
	int core0Status;
	int core1Status;


	//���ݽ����Ľ�����Bufferͳһ����
	uint8_t *pYUV;			//YUV��ͼ������	--	�ⲿ���ݽ�����Buffer
	uint8_t *pDepthMap;		//GPU��������ͼ	--  �ⲿ���ݽ�����Buffer
	uint8_t *pOut;			//������黯����	--	�ⲿ���ݽ�����Buffer
	uint8_t *pMemoryPool;	//ͳһ�ڴ����
	uint8_t *pMemoryPool0;	//�㷨�м�����Ҫ���ڴ��(ͳһ�ڴ����,�����malloc memory������),Core0��Global Memory
	uint8_t *pMemoryPool1;	//�㷨�м�����Ҫ���ڴ��(ͳһ�ڴ����,�����malloc memory������),Core1��Global Memory
	uint8_t *pMessageBuff;	//VPU���Ե�buffer��Ϣ(CPU�˽�����������)
}CPU2VPU_Para;

#endif