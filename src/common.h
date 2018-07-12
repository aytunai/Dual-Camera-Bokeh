#ifndef _COMMON_H
#define _COMMON_H

#define PC_TEST	//PC测试的宏

// Error type code
typedef enum {
	NO_ERROR = 0,
	FILE_READ_ERROR,
	FILE_WRITE_ERROR,
	MEMORY_ERROR,
	OTHER_ERROR,
} ErrorType;

#define CPU_LOG
//是否输出VPU的LOG信息
#ifdef	CPU_LOG
#define xa_printf(...)	printf(__VA_ARGS__)
#else
#define xa_printf(...)
#endif

//模拟手机上真实的开发环境
typedef struct {
	//YUV图像的分辨率
	int width;
	int height;
	int stride;
	//(暂时只支持blurYLevel = 1,blurUVLevel1)
	int blurYLevel;		//虚化力度(1-30)
	int blurUVLevel;	//虚化力度(1-3)

	//后面的针对于PC无效
	//同步标志位,传递进来初始化应该清零
	int core0Status;
	int core1Status;


	//传递进来的解析作Buffer统一管理
	uint8_t *pYUV;			//YUV的图像数据	--	外部传递进来的Buffer
	uint8_t *pDepthMap;		//GPU输出的深度图	--  外部传递进来的Buffer
	uint8_t *pOut;			//输出的虚化数据	--	外部传递进来的Buffer
	uint8_t *pMemoryPool;	//统一内存管理
	uint8_t *pMemoryPool0;	//算法中间所需要的内存池(统一内存管理,解决掉malloc memory的问题),Core0的Global Memory
	uint8_t *pMemoryPool1;	//算法中间所需要的内存池(统一内存管理,解决掉malloc memory的问题),Core1的Global Memory
	uint8_t *pMessageBuff;	//VPU调试的buffer信息(CPU端解析并输出结果)
}CPU2VPU_Para;

#endif