#include <stdio.h>
#include <ctype.h>
#include "led.h"
#include "lcd.h"
#include "24c02.h"
#include "key.h"
#include "cjSON.h"
#include "usart1.h"
/*********************************************************************************
*********************�������� STM32F407Ӧ�ÿ�����(�����)*************************
**********************************************************************************
* �ļ�����: ˮ����������ƽ̨                                                       *
* �ļ�������                                                         						 *
* �������ڣ�2019.04.29                                                           *
* ��    ����V1.0                                                                 *
* ��    �ߣ�zmr                                                            				*
* ˵    ����ͨ��8266ģ�����ӱ�������ƽ̨ʵ��Զ�̼�ز���״̬		
*	���߷�ʽ��
*						PC5										----	�̵��������źţ���̬Ϊ�ߵ�ƽ���룬�����غ󴥷�																														*
*           PF0/1/12/13/PA15				----	��·©ˮ��ƽ���룬©ˮʱΪ�͵�ƽ   
*						PF15/PD11/12/13/PA7		----	��·�̵���������͵�ƽ�Ͽ�����
*						PG0/1/3/4/5						----	��·�������ⲿ�ж�
*                                           
**********************************************************************************
*********************************************************************************/
volatile u8 aRxBuffer[1024]={0x00};
volatile u8 RxCounter=0;
volatile u8 ReceiveState=0;
//��������
u8 ch1_close = 0;
u8 ch2_close = 0;
u8 ch3_close = 0;
u8 ch4_close = 0;
u8 ch5_close = 0;
//��������
u16 ch1=0;
u16 ch2=0;
u16 ch3=0;
u16 ch4=0;
u16 ch5=0;
u16 ch1_val=0;
u16 ch2_val=0;
u16 ch3_val=0;
u16 ch4_val=0;
u16 ch5_val=0;
u16 ch1_count=0;
u16 ch2_count=0;
u16 ch3_count=0;
u16 ch4_count=0;
u16 ch5_count=0;
//���������Сֵ
u16 ch1_max=0;
u16 ch2_max=0;
u16 ch3_max=0;
u16 ch4_max=0;
u16 ch5_max=0;
u16 ch1_min=254;
u16 ch2_min=254;
u16 ch3_min=254;
u16 ch4_min=254;
u16 ch5_min=254;
u16 alarm_value = 0;
//ѭ������
uint32_t ch1_fre=0;
uint32_t ch2_fre=0;
uint32_t ch3_fre=0;
uint32_t ch4_fre=0;
uint32_t ch5_fre=0;
//ʱ��ֵ
uint32_t lastCheckInTime = 0; 
uint32_t lastCheckStatusTime = 0; 
uint32_t lastUploadTime = 0;
uint32_t lastcheckTime = 0;
uint32_t lastSayTime = 0; 
uint32_t uploadTime = 60000*5;
uint32_t checkTime = 60000;
uint32_t postingInterval = 4000; 
uint32_t statusInterval = 10000; 
//����������Կ
char *DEVICEID = "10362";
char *APIKEY = "d3851f09b";
//5����������ID
char *sm1 = "9200";
char *sm2 = "9203";
char *sm3 = "9291";
char *sm4 = "9294";
char *sm5 = "9295";
u8 change = 0;
u8 datatemp[6];	
/*���͵�¼��Ϣ*/
void checkin(void)
{
    printf("{\"M\":\"checkin\",\"ID\":\"%s\",\"K\":\"%s\"}\n", DEVICEID, APIKEY);
}
/*�˳���¼*/
void checkout(void)
{
    printf("{\"M\":\"checkout\",\"ID\":\"%s\",\"K\":\"%s\"}\n", DEVICEID, APIKEY);
}
/*��鵱ǰ��¼״̬*/
void check_status(void)
{
    printf("{\"M\":\"status\"}\n");
}
/*����ָ�Ŀ�굥λ*/
void say(char *toID, char *content)
{
    printf("{\"M\":\"say\",\"ID\":\"%s\",\"C\":\"%s\"}\n", toID, content);
}
/*�ϴ�һ���ӿڵ�ʵʱ����*/
void update1(char *did, char *inputid, float value,char *inputid1, float value1,char *inputid2, float value2,char *inputid3, float value3,char *inputid4, float value4) {
    printf("{\"M\":\"update\",\"ID\":\"%s\",\"V\":{\"%s\":\"%f\",\"%s\":\"%f\",\"%s\":\"%f\",\"%s\":\"%f\",\"%s\":\"%f\"}}\n", did, inputid, value, inputid1, value1, inputid2, value2, inputid3, value3, inputid4, value4);
	
}
/*���;�����Ϣ*/
void alare(char *p) {
    printf("{\"M\":\"alert\",\"ID\":\"1476\",\"C\":\"%s\"}\n",p);
	
}
/*��ѯ������ʱ��*/
void gettime(void){
		printf("{\"M\":\"time\",\"F\":\"Y-m-d H:i\"}\n");
}
//�ַ���ת����
long str2int (char s[])
{
	long i=0,n;
	for(n=0;isdigit(s[i]);i++)
	n=10*n+(s[i]-'0');
	return n;
}
/*��CJSON������յ�����Ϣ*/
int processMessage(char *msg) {
    cJSON *jsonObj = cJSON_Parse(msg);
    cJSON *method;
    char *m;
		char mestemp[50];
    //json�ַ�������ʧ�ܣ�ֱ���˳�
    if(!jsonObj)
    {
        //uart1.printf("json string wrong!");
        return 0;
    }
    method = cJSON_GetObjectItem(jsonObj, "M");
    m = method->valuestring;
		if(strncmp(m,"time",4) == 0)	//�������ʱ��
		{
			char *content = cJSON_GetObjectItem(jsonObj, "T")->valuestring;
			LCD_DisplayString(106,275,16,content);
		}
		if(strncmp(m,"checkinok",7) == 0)		//��¼�ɹ�
		{
			LCD_DisplayString(10,275,16,"->");
		}
    if(strncmp(m, "WELCOME", 7) == 0)		//��������δ��¼
    {
        //��ֹ�豸����״̬δ�������ȵǳ�
        checkout();
        //��ֹ��������ָ�����
        delay_ms(500);
        checkin();
    }
    if(strncmp(m, "connected", 9) == 0)
    {
        checkout();
        delay_ms(500);
        checkin();
    }
    //���豸���û���¼�����ͻ�ӭ��Ϣ
    if(strncmp(m, "login", 5) == 0)
    {
        char *from_id = cJSON_GetObjectItem(jsonObj, "ID")->valuestring;
        char new_content[] = "Dear friend, welcome to BIGIOT !";
        say(from_id, new_content);
				LCD_DisplayString(10,291,16,"                 ");
				LCD_DisplayString(10,291,16,from_id);
				LCD_DisplayString(60,291,16,"is connection");

    }
		if(strncmp(m, "logout", 6) == 0)
    {
        char *from_id = cJSON_GetObjectItem(jsonObj, "ID")->valuestring;
				LCD_DisplayString(10,291,16,"                  ");
				LCD_DisplayString(10,291,16,from_id);
				LCD_DisplayString(60,291,16,"is logout          ");

    }
		
    //�յ�sayָ�ִ����Ӧ��������������Ӧ�ظ�
    if(strncmp(m, "say", 3) == 0 && GetTimerCount() - lastSayTime > 10)
    {
        char *content = cJSON_GetObjectItem(jsonObj, "C")->valuestring;
        char *from_id = cJSON_GetObjectItem(jsonObj, "ID")->valuestring;
				int number;
        lastSayTime = GetTimerCount();
				LCD_DisplayString(10,291,16,"                    ");
				LCD_DisplayString(10,291,16,content);
			//�������д���
				if(strncmp(content, "fre1", 4) == 0)
				{
					char new_content[] = "enter the number to 1#:";
					say(from_id, new_content);
					change = 1;
				}else if(strncmp(content, "fre2", 4) == 0){
					
					char new_content[] = "enter the number to 2#:";
					say(from_id, new_content);
					change = 2;
				}else if(strncmp(content, "fre3", 4) == 0){
					
					char new_content[] = "enter the number to 3#:";
					say(from_id, new_content);
					change = 3;
				}else if(strncmp(content, "fre4", 4) == 0){
				
					char new_content[] = "enter the number to 4#:";
					say(from_id, new_content);
					change = 4;
				}else if(strncmp(content, "fre5", 4) == 0){
					
					char new_content[] = "enter the number to 5#:";
					say(from_id, new_content);
					change = 5;
				}else if(strncmp(content, "freall", 6) == 0){
					
					char new_content[] = "enter the number to 1-5#:";
					say(from_id, new_content);
					change = 6;
				}else if (strncmp(content, "alarm",5) == 0){
					char new_content[] = "enter the alarm value:";
					say(from_id, new_content);
					change = 7;
				}
				else if(change!=0){//�û�������ֵ
					
					number = str2int(content);
					if(number==0){
						say(from_id, "Input Error!!!");
					}else{
						switch(change){
							case 1:ch1_fre = number;sprintf(datatemp, "%d",ch1_fre);AT24C02_Write(0,datatemp,6);break;
							case 2:ch2_fre = number;sprintf(datatemp, "%d",ch2_fre);AT24C02_Write(8,datatemp,6);break;
							case 3:ch3_fre = number;sprintf(datatemp, "%d",ch3_fre);AT24C02_Write(16,datatemp,6);break;
							case 4:ch4_fre = number;sprintf(datatemp, "%d",ch4_fre);AT24C02_Write(24,datatemp,6);break;
							case 5:ch5_fre = number;sprintf(datatemp, "%d",ch5_fre);AT24C02_Write(32,datatemp,6);break;
							case 6:	ch1_fre=number;
											ch2_fre=number;
											ch3_fre=number;
											ch4_fre=number;
											ch5_fre =number;
											sprintf(datatemp, "%d",ch1_fre);AT24C02_Write(0,datatemp,6);
											sprintf(datatemp, "%d",ch2_fre);AT24C02_Write(8,datatemp,6);
											sprintf(datatemp, "%d",ch3_fre);AT24C02_Write(16,datatemp,6);
											sprintf(datatemp, "%d",ch4_fre);AT24C02_Write(24,datatemp,6);
											sprintf(datatemp, "%d",ch5_fre);AT24C02_Write(32,datatemp,6);break;
							case 7: alarm_value = number;break;
							default:break;
						}
					}
					change=0;
				}else if(strncmp(content, "open1", 5) == 0){	//��������ˮ��
					K1_OUT=1;ch1_close=0;
					say(from_id, "1# is open");
				}else if(strncmp(content, "open2", 5) == 0){
					K2_OUT=1;ch2_close=0;
					say(from_id, "2# is open");
				}else if(strncmp(content, "open3", 5) == 0){
					K3_OUT=1;ch3_close=0;
					say(from_id, "3# is open");
				}else if(strncmp(content, "open4", 5) == 0){
					K4_OUT=1;ch4_close=0;
					say(from_id, "4# is open");
				}else if(strncmp(content, "open5", 5) == 0){
					K5_OUT=1;ch5_close=0;
					say(from_id, "5# is open");
				}else if(strncmp(content, "close1", 6) == 0){		//�رյ���ˮ��
					K1_OUT=0;ch1_close=1;
					say(from_id, "1# is close");
				}else if(strncmp(content, "close2", 6) == 0){
					K2_OUT=0;ch2_close=1;
					say(from_id, "2# is close");
				}else if(strncmp(content, "close3", 6) == 0){
					K3_OUT=0;ch3_close=1;
					say(from_id, "3# is close");
				}else if(strncmp(content, "close4", 6) == 0){
					K4_OUT=0;ch4_close=1;
					say(from_id, "4# is close");
				}else if(strncmp(content, "close5", 6) == 0){
					K5_OUT=0;ch5_close=1;
					say(from_id, "5# is close");
				}else if(strncmp(content, "openall", 7) == 0){	//��������ˮ��
					K1_OUT=1;K2_OUT=1;K3_OUT=1;K4_OUT=1;K5_OUT=1;
					ch1_close=0;ch2_close=0;ch3_close=0;ch4_close=0;ch5_close=0;
					say(from_id, "All open");
				}else if(strncmp(content, "closeall", 8) == 0){		//�ر�����ˮ��
					K1_OUT=0;K2_OUT=0;K3_OUT=0;K4_OUT=0;K5_OUT=0;
					ch1_close=1;ch2_close=1;ch3_close=1;ch4_close=1;ch5_close=1;
					say(from_id, "All close");
				}else if(strncmp(content, "sx", 2) == 0){		//�����ϴ���������
					update1(DEVICEID,sm1,ch1_val,sm2,ch2_val,sm3,ch3_val,sm4,ch4_val,sm5,ch5_val);
					lastUploadTime = GetTimerCount();
				}else if(strncmp(content, "save", 2) == 0){		//������Դ���
					sprintf(datatemp, "%d",ch1_fre);AT24C02_Write(0,datatemp,6);
					sprintf(datatemp, "%d",ch2_fre);AT24C02_Write(8,datatemp,6);
					sprintf(datatemp, "%d",ch3_fre);AT24C02_Write(16,datatemp,6);
					sprintf(datatemp, "%d",ch4_fre);AT24C02_Write(24,datatemp,6);
					sprintf(datatemp, "%d",ch5_fre);AT24C02_Write(32,datatemp,6);
				}else if(strncmp(content, "clear", 5) == 0){		//��ղ��Դ���
						AT24C02_Write(0,"000000",6);
						AT24C02_Write(8,"000000",6);
						AT24C02_Write(16,"000000",6);
						AT24C02_Write(24,"000000",6);
						AT24C02_Write(32,"000000",6);
						ch1_fre=0;ch2_fre=0;ch3_fre=0;ch4_fre=0;ch5_fre=0;
						LCD_DisplayNum_color(24+24*2+4+24*3,26,ch1_fre,6,24,0,WHITE,BBLUED);
						LCD_DisplayNum_color(24+24*2+4+24*3,76,ch2_fre,6,24,0,WHITE,BBLUED);
						LCD_DisplayNum_color(24+24*2+4+24*3,126,ch3_fre,6,24,0,WHITE,BBLUED);
						LCD_DisplayNum_color(24+24*2+4+24*3,176,ch4_fre,6,24,0,WHITE,BBLUED);
						LCD_DisplayNum_color(24+24*2+4+24*3,226,ch5_fre,6,24,0,WHITE,BBLUED);
				}
				else if(strncmp(content, "cx", 2) == 0){		//��ѯ���Դ���
					sprintf(mestemp, "F1:%d-F2:%d-F3:%d-F4:%d-F5:%d",ch1_fre,ch2_fre,ch3_fre,ch4_fre,ch5_fre);
						
						say(from_id,mestemp);
				}
				else if(strncmp(content, "bgoff", 5) == 0){		//�ر���ʾ������
					LCD_BACK=0;
					say(from_id,"Background Light OFF");
				}
				else if(strncmp(content, "bgon", 4) == 0){		//����ʾ������
					LCD_BACK=1;
					say(from_id,"Background Light ON");

				}else if(strncmp(content, "ll", 2) == 0){		//��ѯ����
					sprintf(mestemp, "F1:%d-F2:%d-F3:%d-F4:%d-F5:%d",ch1_val,ch2_val,ch3_val,ch4_val,ch5_val);
						
						say(from_id,mestemp);
				}
				
    }
    if(jsonObj)cJSON_Delete(jsonObj);
    return 1;
}
void setup(void)
{
	ESP8266_init();
  uart4_init(115200);
	TIM2_Init(9,8399);
}

void EXTI0_IRQHandler(void)
{
	 if(EXTI_GetITStatus(EXTI_Line0)!=RESET)  
    {  
			if(K_IN==0)
			ch1++;
    }   
		EXTI_ClearFlag(EXTI_Line0);		
		EXTI_ClearITPendingBit(EXTI_Line0); //���LINE0�ϵ��жϱ�־λ 
}
void EXTI1_IRQHandler(void)
{
	 if(EXTI_GetITStatus(EXTI_Line1)!=RESET)  
    {  
			if(K_IN==0)
			ch2++;
    }   
		EXTI_ClearFlag(EXTI_Line1);		
		EXTI_ClearITPendingBit(EXTI_Line1); //���LINE0�ϵ��жϱ�־λ 
}
void EXTI3_IRQHandler(void)
{
	 if(EXTI_GetITStatus(EXTI_Line3)!=RESET)  
    {  
			if(K_IN==0)
			ch3++;
    }   
		EXTI_ClearFlag(EXTI_Line3);		
		EXTI_ClearITPendingBit(EXTI_Line3); //���LINE0�ϵ��жϱ�־λ 
}
void EXTI4_IRQHandler(void)
{
	 if(EXTI_GetITStatus(EXTI_Line4)!=RESET)  
    {  
			if(K_IN==0)
			ch4++;
    }   
		EXTI_ClearFlag(EXTI_Line4);		
		EXTI_ClearITPendingBit(EXTI_Line4); //���LINE0�ϵ��жϱ�־λ 
}
void EXTI9_5_IRQHandler(void)
{
	 if(EXTI_GetITStatus(EXTI_Line5)!=RESET)  
    {  
			if(K_IN==0)
			ch5++;
    }   
		EXTI_ClearFlag(EXTI_Line5);		
		EXTI_ClearITPendingBit(EXTI_Line5); //���LINE0�ϵ��жϱ�־λ 
}

void EXITGPIO_Init(void)
{
	
	GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG,ENABLE);//ʹ��GPIOGʱ��
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5; //G0 G1 G3 G4 G5��Ӧ����
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//��ͨ����ģʽ
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOG, &GPIO_InitStructure);//��ʼ��GPIOG0/1/3/4/5
	
	 
  //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;//WK_UP��Ӧ����PA0
  //GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN ;//����
  //GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA0
 
} 
void EXTIX_Init(void)
{
	NVIC_InitTypeDef	NVIC_InitStructure;
	EXTI_InitTypeDef	EXTI_InitStructure;
	
	EXITGPIO_Init();
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);
	
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG,EXTI_PinSource0);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG,EXTI_PinSource1);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG,EXTI_PinSource3);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG,EXTI_PinSource4);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG,EXTI_PinSource5);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1 | EXTI_Line3 | EXTI_Line4 | EXTI_Line5;
	EXTI_InitStructure.EXTI_Mode =EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger =EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel =EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x05;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority= 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd =ENABLE;
	NVIC_Init(&NVIC_InitStructure); 

	NVIC_InitStructure.NVIC_IRQChannel =EXTI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x04;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority= 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd =ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
	
		NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x03;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority= 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd =ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
	
		NVIC_InitStructure.NVIC_IRQChannel =EXTI4_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority= 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd =ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
	
		NVIC_InitStructure.NVIC_IRQChannel =EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority= 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd =ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
}
void display(void){
	//���⡰ˮ����������ƽ̨��
	LCD_Show_CH_Font24(24,0,0,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*1,0,1,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*2,0,2,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*3,0,3,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*4,0,4,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*5,0,5,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*6,0,6,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*7,0,7,WHITE,BLACK);
	//��ɫ�׿�
	LCD_Fill_onecolor(0,26,239,26+48,BBLUED);
	LCD_Fill_onecolor(24+24*2+2,26+24,239,26+24,BLACK);
	LCD_Fill_onecolor(0,76,239,76+48,BBLUED);
	LCD_Fill_onecolor(24+24*2+2,76+24,239,76+24,BLACK);
	LCD_Fill_onecolor(0,126,239,126+48,BBLUED);
	LCD_Fill_onecolor(24+24*2+2,126+24,239,126+24,BLACK);
	LCD_Fill_onecolor(0,176,239,176+48,BBLUED);
	LCD_Fill_onecolor(24+24*2+2,176+24,239,176+24,BLACK);
	LCD_Fill_onecolor(0,226,239,226+48,BBLUED);
	LCD_Fill_onecolor(24+24*2+2,226+24,239,226+24,BLACK);
	LCD_Fill_onecolor(24+24*2+2,24,24+24*2+2,226+48,BLACK);
	//1-5�ű��
	LCD_DisplayNum_color(12,26,1,1,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(12,76,2,1,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(12,126,3,1,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(12,176,4,1,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(12,226,5,1,24,0,WHITE,BBLUED);
	LCD_Show_CH_Font24(12+24*1,26,8,WHITE,BBLUED);
	LCD_Show_CH_Font24(12+24*1,76,8,WHITE,BBLUED);
	LCD_Show_CH_Font24(12+24*1,126,8,WHITE,BBLUED);
	LCD_Show_CH_Font24(12+24*1,176,8,WHITE,BBLUED);
	LCD_Show_CH_Font24(12+24*1,226,8,WHITE,BBLUED);
	//���������Сֵ
	LCD_DisplayString_color(4,26+25,12,"Max:",WHITE,BBLUED);
	LCD_DisplayString_color(4,26+25+12,12,"Min:",WHITE,BBLUED);
	LCD_DisplayString_color(4,76+25,12,"Max:",WHITE,BBLUED);
	LCD_DisplayString_color(4,76+25+12,12,"Min:",WHITE,BBLUED);
	LCD_DisplayString_color(4,126+25,12,"Max:",WHITE,BBLUED);
	LCD_DisplayString_color(4,126+25+12,12,"Min:",WHITE,BBLUED);
	LCD_DisplayString_color(4,176+25,12,"Max:",WHITE,BBLUED);
	LCD_DisplayString_color(4,176+25+12,12,"Min:",WHITE,BBLUED);
	LCD_DisplayString_color(4,226+25,12,"Max:",WHITE,BBLUED);
	LCD_DisplayString_color(4,226+25+12,12,"Min:",WHITE,BBLUED);
	//��������������������
	LCD_Show_CH_Font24(24+24*2+4,26,9,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24,26,10,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24*2,26,13,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4,26+25,11,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24,26+25,12,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24*2,26+25,13,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4,76,9,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24,76,10,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24*2,76,13,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4,76+25,11,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24,76+25,12,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24*2,76+25,13,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4,126,9,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24,126,10,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24*2,126,13,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4,126+25,11,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24,126+25,12,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24*2,126+25,13,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4,176,9,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24,176,10,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24*2,176,13,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4,176+25,11,WHITE,BBLUED);	
	LCD_Show_CH_Font24(24+24*2+4+24,176+25,12,WHITE,BBLUED);	
	LCD_Show_CH_Font24(24+24*2+4+24*2,176+25,13,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4,226,9,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24,226,10,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24*2,226,13,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4,226+25,11,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24,226+25,12,WHITE,BBLUED);
	LCD_Show_CH_Font24(24+24*2+4+24*2,226+25,13,WHITE,BBLUED);
}

void auto_close(void){
	if(W1_IN==0&&ch1_close==0){
		delay_ms(100);
		if(W1_IN==0){K1_OUT=0;ch1_close=1;alare("1# touch water error");
		//��ʾ��©ˮ������
		LCD_Show_CH_Font24(24+24*2+24*3,26+25,14,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*4,26+25,0,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*5,26+25,15,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*6,26+25,16,RED,WHITE);
		}//��⵽©ˮ���Ͽ��̵�����ѹ������������־
	}
	if(W2_IN==0&&ch2_close==0){
		delay_ms(100);
		if(W2_IN==0){K2_OUT=0;ch2_close=1;alare("2# touch water error");
		//��ʾ��©ˮ������
		LCD_Show_CH_Font24(24+24*2+24*3,26+25+50,14,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*4,26+25+50,0,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*5,26+25+50,15,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*6,26+25+50,16,RED,WHITE);
		}
	}
	if(W3_IN==0&&ch3_close==0){
		delay_ms(100);
		if(W3_IN==0){K3_OUT=0;ch3_close=1;alare("3# touch water error");
		//��ʾ��©ˮ������
		LCD_Show_CH_Font24(24+24*2+24*3,26+25+100,14,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*4,26+25+100,0,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*5,26+25+100,15,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*6,26+25+100,16,RED,WHITE);
		}
	}
	if(W4_IN==0&&ch4_close==0){
		delay_ms(100);
		if(W4_IN==0){K4_OUT=0;ch4_close=1;alare("4# touch water error");
		//��ʾ��©ˮ������
		LCD_Show_CH_Font24(24+24*2+24*3,26+25+150,14,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*4,26+25+150,0,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*5,26+25+150,15,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*6,26+25+150,16,RED,WHITE);
		}
	}
	if(W5_IN==0&&ch5_close==0){
		delay_ms(100);
		if(W5_IN==0){K5_OUT=0;ch5_close=1;alare("5# touch water error");
		//��ʾ��©ˮ������
		LCD_Show_CH_Font24(24+24*2+24*3,26+25+200,14,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*4,26+25+200,0,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*5,26+25+200,15,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*6,26+25+200,16,RED,WHITE);
		}
	}
	if(ch1_val<=alarm_value&&ch1_close==0){
		
		if(ch1_val<=10){	
				ch1_count++;
				if(ch1_count>=10){
					ch1_count=0;
					K1_OUT=0;
					ch1_close=1;
					alare("1# no water error");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25,18,RED,WHITE);
				}	
		}else if(ch1_val<=120){
			ch1_count++;
			if(ch1_count>=3){
				ch1_count=0;
				K1_OUT=0;
				ch1_close=1;
				alare("1# little water warning");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25,18,RED,WHITE);
			}
		}
	}else{
			ch1_count=0;
		}
	if(ch2_val<=alarm_value&&ch2_close==0){
			
		if(ch2_val<=10){	
				ch2_count++;
				if(ch2_count>=10){
					ch2_count=0;
					K2_OUT=0;
					ch2_close=1;
					alare("2# no water error");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25+50,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25+50,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25+50,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25+50,18,RED,WHITE);
				}	
		}else if(ch2_val<=120){
			ch2_count++;
			if(ch2_count>=3){
				ch2_count=0;
				K2_OUT=0;
				ch2_close=1;
				alare("2# little water warning");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25+50,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25+50,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25+50,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25+50,18,RED,WHITE);
			}
		}
	}else{
			ch2_count=0;
		}
	if(ch3_val<=alarm_value&&ch3_close==0){
		
		if(ch3_val<=10){	
				ch3_count++;
				if(ch3_count>=10){
					ch3_count=0;
					K3_OUT=0;
					ch3_close=1;
					alare("3#no water error");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25+100,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25+100,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25+100,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25+100,18,RED,WHITE);
				}	
		}else if(ch3_val<=120){
			ch3_count++;
			if(ch3_count>=3){
				ch3_count=0;
				K3_OUT=0;
				ch3_close=1;
				alare("3# little water warning");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25+100,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25+100,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25+100,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25+100,18,RED,WHITE);
			}
		}
	}else{
			ch3_count=0;
		}
	if(ch4_val<=alarm_value&&ch4_close==0){
		
		if(ch4_val<=10){	
				ch4_count++;
				if(ch4_count>=10){
					ch4_count=0;
					K4_OUT=0;
					ch4_close=1;
					alare("4# no water error");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25+150,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25+150,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25+150,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25+150,18,RED,WHITE);
				}	
		}else if(ch4_val<=120){
			ch4_count++;
			if(ch4_count>=3){
				ch4_count=0;
				K4_OUT=0;
				ch4_close=1;
				alare("4# little water warning");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25+150,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25+150,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25+150,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25+150,18,RED,WHITE);
			}
		}
	}else{
			ch4_count=0;
		}
	if(ch5_val<=alarm_value&&ch5_close==0){
		
		if(ch5_val<=10){	
				ch5_count++;
				if(ch5_count>=10){
					ch5_count=0;
					K5_OUT=0;
					ch5_close=1;
					alare("5# no water error");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25+200,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25+200,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25+200,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25+200,18,RED,WHITE);
				}	
		}else if(ch5_val<=120){
			ch5_count++;
			if(ch5_count>=3){
				ch5_count=0;
				K5_OUT=0;
				ch5_close=1;
				alare("5# little water warning");
					//��ʾ���������㡱
					LCD_Show_CH_Font24(24+24*2+24*3,26+25+200,11,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*4,26+25+200,12,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*5,26+25+200,17,RED,WHITE);
					LCD_Show_CH_Font24(24+24*2+24*6,26+25+200,18,RED,WHITE);
			}
		}
	}else{
			ch5_count=0;
		}
}

int main(void)
{ 

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	setup();							//��ʼ������wifi
	delay_init();         //��ʼ����ʱ����
	LED_Init();						//��ʼ��LED 
	KEY_Init();						//LCD��ʼ��
	EXTIX_Init();					//��ʼ���ⲿ�ж�
	AT24C02_Init();				//AT24C02��ʼ�� 
	LCD_Init();					  //��ʼ��LCD
	BRUSH_COLOR=WHITE;
	display();						//��ʾ����
	//��ȡ����Ĵ���
	AT24C02_Read(0,datatemp,6);
	ch1_fre = str2int(datatemp);
	AT24C02_Read(8,datatemp,6);
	ch2_fre = str2int(datatemp);
	AT24C02_Read(16,datatemp,6);
	ch3_fre = str2int(datatemp);
	AT24C02_Read(24,datatemp,6);
	ch4_fre = str2int(datatemp);
	AT24C02_Read(32,datatemp,6);
	ch5_fre = str2int(datatemp);	
	//��ʾ���Դ���
	LCD_DisplayNum_color(24+24*2+4+24*3,26,ch1_fre,6,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(24+24*2+4+24*3,76,ch2_fre,6,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(24+24*2+4+24*3,126,ch3_fre,6,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(24+24*2+4+24*3,176,ch4_fre,6,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(24+24*2+4+24*3,226,ch5_fre,6,24,0,WHITE,BBLUED);
	
	while (1)
    {
				key_scan(0);	//ɨ��̵����Ƿ��ͨ
				
				if(keyup_data==K1_DATA)   //�Ͽ�������ִ�м���
					{
						delay_ms(1000);
						if(K_IN==1){
						ch1_val=ch1;//ȡ������ֵ����
						ch2_val=ch2;
						ch3_val=ch3;
						ch4_val=ch4;						
						ch5_val=ch5;
						//��������
						ch1=0;
						ch2=0;
						ch3=0;
						ch4=0;
						ch5=0;
						auto_close();//����©ˮ���������
						if(ch1_close==0){//δ����©ˮ������²ż���
							ch1_fre++;//���Դ����ۼ�
							if(ch1_fre%100==0){
								//ÿ��100�����򱣴�eeproom
								sprintf(datatemp, "%d",ch1_fre);AT24C02_Write(0,datatemp,6);
							}
						}
						if(ch2_close==0){
							ch2_fre++;
							if(ch2_fre%100==0){
								sprintf(datatemp, "%d",ch2_fre);AT24C02_Write(8,datatemp,6);
							}
						}						
						if(ch3_close==0){
							ch3_fre++;
							if(ch3_fre%100==0){
								sprintf(datatemp, "%d",ch3_fre);AT24C02_Write(16,datatemp,6);
							}
						}						
						if(ch4_close==0){
							ch4_fre++;
							if(ch4_fre%100==0){
								sprintf(datatemp, "%d",ch4_fre);AT24C02_Write(24,datatemp,6);
							}
						}						
						if(ch5_close==0){
							ch5_fre++;
							if(ch5_fre%100==0){
								sprintf(datatemp, "%d",ch5_fre);AT24C02_Write(32,datatemp,6);
							}
						}						
						
						//�Ա����������Сֵ
						if(ch1_val>ch1_max)ch1_max=ch1_val;
						else if((ch1_val<ch1_min) && (ch1_val>10))ch1_min=ch1_val;
						if(ch2_val>ch2_max)ch2_max=ch2_val;
						else if((ch2_val<ch2_min) && (ch2_val>10))ch2_min=ch2_val;
						if(ch3_val>ch3_max)ch3_max=ch3_val;
						else if((ch3_val<ch3_min) && (ch3_val>10))ch3_min=ch3_val;
						if(ch4_val>ch4_max)ch4_max=ch4_val;
						else if((ch4_val<ch4_min) && (ch4_val>10))ch4_min=ch4_val;
						if(ch5_val>ch5_max)ch5_max=ch5_val;
						else if((ch5_val<ch5_min) && (ch5_val>10))ch5_min=ch5_val;
						//��ʾ����������
						if(ch1_close==0)
						LCD_DisplayNum_color(24+24*2+4+24*3,26,ch1_fre,6,24,0,WHITE,BBLUED);
						else	LCD_DisplayNum_color(24+24*2+4+24*3,26,ch1_fre,6,24,0,RED,WHITE);
						if(ch2_close==0)
						LCD_DisplayNum_color(24+24*2+4+24*3,76,ch2_fre,6,24,0,WHITE,BBLUED);
						else	LCD_DisplayNum_color(24+24*2+4+24*3,76,ch2_fre,6,24,0,RED,WHITE);
						if(ch3_close==0)
						LCD_DisplayNum_color(24+24*2+4+24*3,126,ch3_fre,6,24,0,WHITE,BBLUED);
						else	LCD_DisplayNum_color(24+24*2+4+24*3,126,ch3_fre,6,24,0,RED,WHITE);
						if(ch4_close==0)
						LCD_DisplayNum_color(24+24*2+4+24*3,176,ch4_fre,6,24,0,WHITE,BBLUED);
						else	LCD_DisplayNum_color(24+24*2+4+24*3,176,ch4_fre,6,24,0,RED,WHITE);
						if(ch5_close==0)
						LCD_DisplayNum_color(24+24*2+4+24*3,226,ch5_fre,6,24,0,WHITE,BBLUED);
						else	LCD_DisplayNum_color(24+24*2+4+24*3,226,ch5_fre,6,24,0,RED,WHITE);
						
					//	if(ch1_val>120)
							LCD_DisplayNum_color(24+24*2+4+24*3,26+25,ch1_val,6,24,0,WHITE,BBLUED);
					//	if(ch2_val>120)
							LCD_DisplayNum_color(24+24*2+4+24*3,76+25,ch2_val,6,24,0,WHITE,BBLUED);
					//	if(ch3_val>120)
							LCD_DisplayNum_color(24+24*2+4+24*3,126+25,ch3_val,6,24,0,WHITE,BBLUED);
					//	if(ch4_val>120)
							LCD_DisplayNum_color(24+24*2+4+24*3,176+25,ch4_val,6,24,0,WHITE,BBLUED);
					//	if(ch5_val>120)						
							LCD_DisplayNum_color(24+24*2+4+24*3,226+25,ch5_val,6,24,0,WHITE,BBLUED);
						//���������Сֵ
						LCD_DisplayNum_color(28,26+25,ch1_max,4,12,0,WHITE,BBLUED);
						LCD_DisplayNum_color(28,26+25+12,ch1_min,4,12,0,WHITE,BBLUED);
						LCD_DisplayNum_color(28,76+25,ch2_max,4,12,0,WHITE,BBLUED);
						LCD_DisplayNum_color(28,76+25+12,ch2_min,4,12,0,WHITE,BBLUED);
						LCD_DisplayNum_color(28,126+25,ch3_max,4,12,0,WHITE,BBLUED);
						LCD_DisplayNum_color(28,126+25+12,ch3_min,4,12,0,WHITE,BBLUED);
						LCD_DisplayNum_color(28,176+25,ch4_max,4,12,0,WHITE,BBLUED);
						LCD_DisplayNum_color(28,176+25+12,ch4_min,4,12,0,WHITE,BBLUED);
						LCD_DisplayNum_color(28,226+25,ch5_max,4,12,0,WHITE,BBLUED);
						LCD_DisplayNum_color(28,226+25+12,ch5_min,4,12,0,WHITE,BBLUED);
						}
					}
				
        if(keydown_data==KEY0_DATA)//KEY0����,���㱣��ļ���
					{ 
						AT24C02_Write(0,"000000",6);
						AT24C02_Write(8,"000000",6);
						AT24C02_Write(16,"000000",6);
						AT24C02_Write(24,"000000",6);
						AT24C02_Write(32,"000000",6);
						ch1_fre=0;ch2_fre=0;ch3_fre=0;ch4_fre=0;ch5_fre=0;
						LCD_DisplayNum_color(24+24*2+4+24*3,26,ch1_fre,6,24,0,WHITE,BBLUED);
						LCD_DisplayNum_color(24+24*2+4+24*3,76,ch2_fre,6,24,0,WHITE,BBLUED);
						LCD_DisplayNum_color(24+24*2+4+24*3,126,ch3_fre,6,24,0,WHITE,BBLUED);
						LCD_DisplayNum_color(24+24*2+4+24*3,176,ch4_fre,6,24,0,WHITE,BBLUED);
						LCD_DisplayNum_color(24+24*2+4+24*3,226,ch5_fre,6,24,0,WHITE,BBLUED);
					}	
					
				//��¼��������
        if (GetTimerCount() - lastCheckInTime > postingInterval || lastCheckInTime == 0) {
            checkin();
            lastCheckInTime = GetTimerCount();
        }
        //��ѯ����״̬
        if (GetTimerCount() - lastCheckStatusTime > statusInterval) {
            check_status();
            lastCheckStatusTime = GetTimerCount();
        }
				//�ϴ���������
				if (GetTimerCount() - lastUploadTime > uploadTime){
						update1(DEVICEID,sm1,ch1_val,sm2,ch2_val,sm3,ch3_val,sm4,ch4_val,sm5,ch5_val);
						lastUploadTime = GetTimerCount();
				}
				//��ȡ������ʱ��
				if(GetTimerCount() - lastcheckTime > checkTime||lastcheckTime == 0){
						gettime();
						lastcheckTime = GetTimerCount();
				}
        if(ReceiveState == 1)
        {
            ReceiveState = 0;
            processMessage(aRxBuffer);
            RxCounter = 0;
        }
        
    }
	

	
 //	
	//KEY_Init();           //��ʼ��KEY


	

	

}
