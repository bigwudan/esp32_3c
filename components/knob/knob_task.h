#ifndef _KNOB_TASK_H
#define _KNOB_TASK_H

#include <stdio.h>
#include <knob.h>

#define	PIN_ENC1A			BIT0	 	//编码器A路信号对应到 GPIO 输入结果的 bit位
#define	PIN_ENC1B			BIT1


#ifndef BIT0

#define BIT0	0x01
#define BIT1	0x02
#define BIT2	0x04
#define BIT3	0x08
#define BIT4	0x10
#define BIT5	0x20
#define BIT6	0x40
#define BIT7	0x80

#define BIT8	0x0100
#define BIT9	0x0200
#define BIT10	0x0400
#define BIT11	0x0800
#define BIT12	0x1000
#define BIT13	0x2000
#define BIT14	0x4000
#define BIT15	0x8000

#define BIT16	0x010000
#define BIT17	0x020000
#define BIT18	0x040000
#define BIT19	0x080000
#define BIT20	0x100000
#define BIT21	0x200000
#define BIT22	0x400000
#define BIT23	0x800000

#endif





//====================================================================================
//以下是编码器处理结果的宏定义， 这里定义了3个编码器，结果放在同一个字节里
//在188的产品里，只需要用到 ENC1的定义

#define ENC_1_R			0x01					//代表编码器1 产生了一次右旋动作
#define ENC_1_L			0x02 					//代表编码器1 产生了一次左旋动作
#define ENC_1_MSK		(ENC_1_R|ENC_1_L)

#define ENC_2_R			0x04					//代表编码器2 产生了一次右旋动作
#define ENC_2_L			0x08                    //代表编码器2 产生了一次左旋动作
#define ENC_2_MSK		(ENC_2_R|ENC_2_L)

#define ENC_3_R			0x10					//代表编码器3 产生了一次右旋动作
#define ENC_3_L			0x20                    //代表编码器3 产生了一次左旋动作
#define ENC_3_MSK		(ENC_3_R|ENC_3_L)


//====================================================================================
//以下定义是针对任意一个编码器的通用宏定义

#define ECA     BIT1		//编码器A路GPIO信号对应到数据里的 bit位，只能用bit0 和 bit1
#define ECB     BIT0

//---------------------------------------------------
//顺时针 的4个相位的引脚值
#define ABaB    ((ECA|ECB)<<2 | ECB)         //1101
#define aBab    ((ECB)<<2 | 0)               //0100
#define abAb    ( ECA)                       //0010
#define AbAB    ((ECA)<<2 | (ECA|ECB))       //1011

#define EC_S1   ABaB
#define EC_S2   aBab
#define EC_S3   abAb
#define EC_S4   AbAB

//---------------------------------------------------
//逆时针的4个相位的引脚值
#define ABAb    ((ECA|ECB)<<2 | ECA)        //1110
#define Abab    ((ECA)<<2 | 0)              //1000
#define abaB    ( ECB)                      //0001
#define aBAB    ((ECB)<<2 | (ECA|ECB))      //0111

#define EC_N1   ABAb
#define EC_N2   Abab
#define EC_N3   abaB
#define EC_N4   aBAB

//#define aB  (ECB)            //01
//#define ab  (0)                   //00
//#define Ab  (ECA)             //10
//#define AB  (ECA|ECB)   //10


//extern u8  encodeEvent;


void EncodeScanTimeOutCtrl(void);	// 编码器信号超时调用程序
unsigned char EncoderPinScan1(unsigned char pin);		// 1号编码器的扫描
//void EncoderPinScan2(u8 pin);		// 2号编码器的扫描
//void EncoderPinScan3(u8 pin);		// 2号编码器的扫描

unsigned char EncoderScan(unsigned int gpio_A, unsigned int gpio_B);				// 
void EncoderScanThread(void);

void knob_task_init();

enum knob_state knob_task_get_state();

#endif