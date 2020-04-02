#ifndef __KEY_H
#define __KEY_H	 
#include "common.h" 

//////////////////////////////////////////////////////////////////////////////////	 

//����IO�˿ڶ���
#define KEY0 		PFin(9)   
#define KEY1 		PFin(8)		
#define KEY2 		PFin(7)		
#define KEY3 	  PFin(6)		
//�ⲿ�������
#define K_IN		PCin(5)
//5·©ˮ����˿�
#define W1_IN		PFin(0)
#define W2_IN		PFin(1)
#define W3_IN		PFin(12)
#define W4_IN		PFin(13)
#define W5_IN		PAin(15)
//5·�̵����������
#define K1_OUT	PFout(15)
#define K2_OUT	PDout(11)
#define K3_OUT	PDout(12)
#define K4_OUT	PDout(13)
#define K5_OUT	PAout(7)
//����ֵ����
#define K1_DATA		1
#define KEY0_DATA	  2
#define KEY1_DATA	  3
#define KEY2_DATA	  4
#define KEY3_DATA   5

//��������
extern u8   keydown_data;    //�������º�ͷ��ص�ֵ
extern u8   keyup_data;      //����̧�𷵻�ֵ
extern u16  key_time;
extern u8   key_tem; 

//��������
void KEY_Init(void);	      //IO��ʼ��
void key_scan(u8 mode);  		//����ɨ�躯��	

#endif
