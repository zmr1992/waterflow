#ifndef __KEY_H
#define __KEY_H	 
#include "common.h" 

//////////////////////////////////////////////////////////////////////////////////	 

//按键IO端口定义
#define KEY0 		PFin(9)   
#define KEY1 		PFin(8)		
#define KEY2 		PFin(7)		
#define KEY3 	  PFin(6)		
//外部输入计数
#define K_IN		PCin(5)
//5路漏水输入端口
#define W1_IN		PFin(0)
#define W2_IN		PFin(1)
#define W3_IN		PFin(12)
#define W4_IN		PFin(13)
#define W5_IN		PAin(15)
//5路继电器控制输出
#define K1_OUT	PFout(15)
#define K2_OUT	PDout(11)
#define K3_OUT	PDout(12)
#define K4_OUT	PDout(13)
#define K5_OUT	PAout(7)
//按键值定义
#define K1_DATA		1
#define KEY0_DATA	  2
#define KEY1_DATA	  3
#define KEY2_DATA	  4
#define KEY3_DATA   5

//变量声明
extern u8   keydown_data;    //按键按下后就返回的值
extern u8   keyup_data;      //按键抬起返回值
extern u16  key_time;
extern u8   key_tem; 

//函数声明
void KEY_Init(void);	      //IO初始化
void key_scan(u8 mode);  		//按键扫描函数	

#endif
