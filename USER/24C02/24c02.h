#ifndef __24C02_H
#define __24C02_H
#include "common.h"  

////////////////////////////////////////////////////////////////////////////////// 	

//IO���庯��	 
#define IIC_SCL      PBout(8) //SCL
#define IIC_SDAOUT   PBout(9) //���SDA	 
#define IIC_SDAIN    GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9) //����SDA 

//IIC��غ���
void IIC_Init(void);          //��ʼ��IIC��IO��				 
void IIC_Start(void);				  //����IIC��ʼ�ź�
void IIC_Stop(void);	  			//����IICֹͣ�ź�
u8 MCU_Wait_Ack(void); 				//IIC�ȴ�ACK�ź�
void MCU_Send_Ack(void);			//IIC����ACK�ź�
void MCU_NOAck(void);				  //IIC������ACK�ź�
void IIC_write_OneByte(u8 Senddata);
u8 IIC_Read_OneByte(u8 ack);
	
//EEPROM24c02��غ���
u8 AT24C02_ReadByte(u8 ReadAddr);							     //ָ����ַ��ȡһ���ֽ�
void AT24C02_WriteByte(u8 WriteAddr,u8 DataToWrite);		//ָ����ַд��һ���ֽ�

void AT24C02_Write(u8 WriteAddr,u8 *pBuffer,u8 WriteNum);	//��ָ����ַ��ʼд��ָ�����ȵ�����
void AT24C02_Read(u8 ReadAddr,u8 *pBuffer,u8 ReadNum);   	//��ָ����ַ��ʼ����ָ�����ȵ�����

u8 AT24C02_Test(void);  //�������
void AT24C02_Init(void); //��ʼ��IIC

#endif
















