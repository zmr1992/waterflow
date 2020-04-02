#include <stdio.h>
#include <ctype.h>
#include "led.h"
#include "lcd.h"
#include "24c02.h"
#include "key.h"
#include "cjSON.h"
#include "usart1.h"
/*********************************************************************************
*********************启明欣欣 STM32F407应用开发板(高配版)*************************
**********************************************************************************
* 文件名称: 水泵寿命测试平台                                                       *
* 文件简述：                                                         						 *
* 创建日期：2019.04.29                                                           *
* 版    本：V1.0                                                                 *
* 作    者：zmr                                                            				*
* 说    明：通过8266模块连接贝壳物联平台实现远程监控测试状态		
*	接线方式：
*						PC5										----	继电器输入信号，常态为高电平输入，上升沿后触发																														*
*           PF0/1/12/13/PA15				----	五路漏水电平输入，漏水时为低电平   
*						PF15/PD11/12/13/PA7		----	五路继电器输出，低电平断开供电
*						PG0/1/3/4/5						----	五路流量计外部中断
*                                           
**********************************************************************************
*********************************************************************************/
volatile u8 aRxBuffer[1024]={0x00};
volatile u8 RxCounter=0;
volatile u8 ReceiveState=0;
//计数锁定
u8 ch1_close = 0;
u8 ch2_close = 0;
u8 ch3_close = 0;
u8 ch4_close = 0;
u8 ch5_close = 0;
//流量计数
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
//流量最大最小值
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
//循环次数
uint32_t ch1_fre=0;
uint32_t ch2_fre=0;
uint32_t ch3_fre=0;
uint32_t ch4_fre=0;
uint32_t ch5_fre=0;
//时间值
uint32_t lastCheckInTime = 0; 
uint32_t lastCheckStatusTime = 0; 
uint32_t lastUploadTime = 0;
uint32_t lastcheckTime = 0;
uint32_t lastSayTime = 0; 
uint32_t uploadTime = 60000*5;
uint32_t checkTime = 60000;
uint32_t postingInterval = 4000; 
uint32_t statusInterval = 10000; 
//贝壳物联密钥
char *DEVICEID = "10362";
char *APIKEY = "d3851f09b";
//5个流量数据ID
char *sm1 = "9200";
char *sm2 = "9203";
char *sm3 = "9291";
char *sm4 = "9294";
char *sm5 = "9295";
u8 change = 0;
u8 datatemp[6];	
/*发送登录信息*/
void checkin(void)
{
    printf("{\"M\":\"checkin\",\"ID\":\"%s\",\"K\":\"%s\"}\n", DEVICEID, APIKEY);
}
/*退出登录*/
void checkout(void)
{
    printf("{\"M\":\"checkout\",\"ID\":\"%s\",\"K\":\"%s\"}\n", DEVICEID, APIKEY);
}
/*检查当前登录状态*/
void check_status(void)
{
    printf("{\"M\":\"status\"}\n");
}
/*发送指令到目标单位*/
void say(char *toID, char *content)
{
    printf("{\"M\":\"say\",\"ID\":\"%s\",\"C\":\"%s\"}\n", toID, content);
}
/*上传一个接口的实时数据*/
void update1(char *did, char *inputid, float value,char *inputid1, float value1,char *inputid2, float value2,char *inputid3, float value3,char *inputid4, float value4) {
    printf("{\"M\":\"update\",\"ID\":\"%s\",\"V\":{\"%s\":\"%f\",\"%s\":\"%f\",\"%s\":\"%f\",\"%s\":\"%f\",\"%s\":\"%f\"}}\n", did, inputid, value, inputid1, value1, inputid2, value2, inputid3, value3, inputid4, value4);
	
}
/*发送警报信息*/
void alare(char *p) {
    printf("{\"M\":\"alert\",\"ID\":\"1476\",\"C\":\"%s\"}\n",p);
	
}
/*查询服务器时间*/
void gettime(void){
		printf("{\"M\":\"time\",\"F\":\"Y-m-d H:i\"}\n");
}
//字符串转数字
long str2int (char s[])
{
	long i=0,n;
	for(n=0;isdigit(s[i]);i++)
	n=10*n+(s[i]-'0');
	return n;
}
/*用CJSON处理接收到的信息*/
int processMessage(char *msg) {
    cJSON *jsonObj = cJSON_Parse(msg);
    cJSON *method;
    char *m;
		char mestemp[50];
    //json字符串解析失败，直接退出
    if(!jsonObj)
    {
        //uart1.printf("json string wrong!");
        return 0;
    }
    method = cJSON_GetObjectItem(jsonObj, "M");
    m = method->valuestring;
		if(strncmp(m,"time",4) == 0)	//输出网络时间
		{
			char *content = cJSON_GetObjectItem(jsonObj, "T")->valuestring;
			LCD_DisplayString(106,275,16,content);
		}
		if(strncmp(m,"checkinok",7) == 0)		//登录成功
		{
			LCD_DisplayString(10,275,16,"->");
		}
    if(strncmp(m, "WELCOME", 7) == 0)		//已联网但未登录
    {
        //防止设备在线状态未消除，先登出
        checkout();
        //防止连续发送指令过快
        delay_ms(500);
        checkin();
    }
    if(strncmp(m, "connected", 9) == 0)
    {
        checkout();
        delay_ms(500);
        checkin();
    }
    //有设备或用户登录，发送欢迎信息
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
		
    //收到say指令，执行相应动作，并进行相应回复
    if(strncmp(m, "say", 3) == 0 && GetTimerCount() - lastSayTime > 10)
    {
        char *content = cJSON_GetObjectItem(jsonObj, "C")->valuestring;
        char *from_id = cJSON_GetObjectItem(jsonObj, "ID")->valuestring;
				int number;
        lastSayTime = GetTimerCount();
				LCD_DisplayString(10,291,16,"                    ");
				LCD_DisplayString(10,291,16,content);
			//设置运行次数
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
				else if(change!=0){//用户发来数值
					
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
				}else if(strncmp(content, "open1", 5) == 0){	//启动单个水泵
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
				}else if(strncmp(content, "close1", 6) == 0){		//关闭单个水泵
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
				}else if(strncmp(content, "openall", 7) == 0){	//启动所有水泵
					K1_OUT=1;K2_OUT=1;K3_OUT=1;K4_OUT=1;K5_OUT=1;
					ch1_close=0;ch2_close=0;ch3_close=0;ch4_close=0;ch5_close=0;
					say(from_id, "All open");
				}else if(strncmp(content, "closeall", 8) == 0){		//关闭所有水泵
					K1_OUT=0;K2_OUT=0;K3_OUT=0;K4_OUT=0;K5_OUT=0;
					ch1_close=1;ch2_close=1;ch3_close=1;ch4_close=1;ch5_close=1;
					say(from_id, "All close");
				}else if(strncmp(content, "sx", 2) == 0){		//立刻上传流量数据
					update1(DEVICEID,sm1,ch1_val,sm2,ch2_val,sm3,ch3_val,sm4,ch4_val,sm5,ch5_val);
					lastUploadTime = GetTimerCount();
				}else if(strncmp(content, "save", 2) == 0){		//保存测试次数
					sprintf(datatemp, "%d",ch1_fre);AT24C02_Write(0,datatemp,6);
					sprintf(datatemp, "%d",ch2_fre);AT24C02_Write(8,datatemp,6);
					sprintf(datatemp, "%d",ch3_fre);AT24C02_Write(16,datatemp,6);
					sprintf(datatemp, "%d",ch4_fre);AT24C02_Write(24,datatemp,6);
					sprintf(datatemp, "%d",ch5_fre);AT24C02_Write(32,datatemp,6);
				}else if(strncmp(content, "clear", 5) == 0){		//清空测试次数
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
				else if(strncmp(content, "cx", 2) == 0){		//查询测试次数
					sprintf(mestemp, "F1:%d-F2:%d-F3:%d-F4:%d-F5:%d",ch1_fre,ch2_fre,ch3_fre,ch4_fre,ch5_fre);
						
						say(from_id,mestemp);
				}
				else if(strncmp(content, "bgoff", 5) == 0){		//关闭显示屏背光
					LCD_BACK=0;
					say(from_id,"Background Light OFF");
				}
				else if(strncmp(content, "bgon", 4) == 0){		//打开显示屏背光
					LCD_BACK=1;
					say(from_id,"Background Light ON");

				}else if(strncmp(content, "ll", 2) == 0){		//查询流量
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
		EXTI_ClearITPendingBit(EXTI_Line0); //清除LINE0上的中断标志位 
}
void EXTI1_IRQHandler(void)
{
	 if(EXTI_GetITStatus(EXTI_Line1)!=RESET)  
    {  
			if(K_IN==0)
			ch2++;
    }   
		EXTI_ClearFlag(EXTI_Line1);		
		EXTI_ClearITPendingBit(EXTI_Line1); //清除LINE0上的中断标志位 
}
void EXTI3_IRQHandler(void)
{
	 if(EXTI_GetITStatus(EXTI_Line3)!=RESET)  
    {  
			if(K_IN==0)
			ch3++;
    }   
		EXTI_ClearFlag(EXTI_Line3);		
		EXTI_ClearITPendingBit(EXTI_Line3); //清除LINE0上的中断标志位 
}
void EXTI4_IRQHandler(void)
{
	 if(EXTI_GetITStatus(EXTI_Line4)!=RESET)  
    {  
			if(K_IN==0)
			ch4++;
    }   
		EXTI_ClearFlag(EXTI_Line4);		
		EXTI_ClearITPendingBit(EXTI_Line4); //清除LINE0上的中断标志位 
}
void EXTI9_5_IRQHandler(void)
{
	 if(EXTI_GetITStatus(EXTI_Line5)!=RESET)  
    {  
			if(K_IN==0)
			ch5++;
    }   
		EXTI_ClearFlag(EXTI_Line5);		
		EXTI_ClearITPendingBit(EXTI_Line5); //清除LINE0上的中断标志位 
}

void EXITGPIO_Init(void)
{
	
	GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG,ENABLE);//使能GPIOG时钟
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5; //G0 G1 G3 G4 G5对应引脚
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输入模式
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化GPIOG0/1/3/4/5
	
	 
  //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;//WK_UP对应引脚PA0
  //GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN ;//下拉
  //GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA0
 
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
	//标题“水泵寿命测试平台”
	LCD_Show_CH_Font24(24,0,0,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*1,0,1,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*2,0,2,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*3,0,3,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*4,0,4,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*5,0,5,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*6,0,6,WHITE,BLACK);
	LCD_Show_CH_Font24(24+24*7,0,7,WHITE,BLACK);
	//蓝色底框
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
	//1-5号编号
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
	//流量最大最小值
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
	//“流量：”“次数：”
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
		//显示“漏水触发”
		LCD_Show_CH_Font24(24+24*2+24*3,26+25,14,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*4,26+25,0,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*5,26+25,15,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*6,26+25,16,RED,WHITE);
		}//检测到漏水，断开继电器电压，锁定计数标志
	}
	if(W2_IN==0&&ch2_close==0){
		delay_ms(100);
		if(W2_IN==0){K2_OUT=0;ch2_close=1;alare("2# touch water error");
		//显示“漏水触发”
		LCD_Show_CH_Font24(24+24*2+24*3,26+25+50,14,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*4,26+25+50,0,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*5,26+25+50,15,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*6,26+25+50,16,RED,WHITE);
		}
	}
	if(W3_IN==0&&ch3_close==0){
		delay_ms(100);
		if(W3_IN==0){K3_OUT=0;ch3_close=1;alare("3# touch water error");
		//显示“漏水触发”
		LCD_Show_CH_Font24(24+24*2+24*3,26+25+100,14,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*4,26+25+100,0,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*5,26+25+100,15,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*6,26+25+100,16,RED,WHITE);
		}
	}
	if(W4_IN==0&&ch4_close==0){
		delay_ms(100);
		if(W4_IN==0){K4_OUT=0;ch4_close=1;alare("4# touch water error");
		//显示“漏水触发”
		LCD_Show_CH_Font24(24+24*2+24*3,26+25+150,14,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*4,26+25+150,0,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*5,26+25+150,15,RED,WHITE);
		LCD_Show_CH_Font24(24+24*2+24*6,26+25+150,16,RED,WHITE);
		}
	}
	if(W5_IN==0&&ch5_close==0){
		delay_ms(100);
		if(W5_IN==0){K5_OUT=0;ch5_close=1;alare("5# touch water error");
		//显示“漏水触发”
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
					//显示“流量不足”
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
					//显示“流量不足”
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
					//显示“流量不足”
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
					//显示“流量不足”
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
					//显示“流量不足”
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
					//显示“流量不足”
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
					//显示“流量不足”
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
					//显示“流量不足”
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
					//显示“流量不足”
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
					//显示“流量不足”
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

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	setup();							//初始化串口wifi
	delay_init();         //初始化延时函数
	LED_Init();						//初始化LED 
	KEY_Init();						//LCD初始化
	EXTIX_Init();					//初始化外部中断
	AT24C02_Init();				//AT24C02初始化 
	LCD_Init();					  //初始化LCD
	BRUSH_COLOR=WHITE;
	display();						//显示界面
	//读取储存的次数
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
	//显示测试次数
	LCD_DisplayNum_color(24+24*2+4+24*3,26,ch1_fre,6,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(24+24*2+4+24*3,76,ch2_fre,6,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(24+24*2+4+24*3,126,ch3_fre,6,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(24+24*2+4+24*3,176,ch4_fre,6,24,0,WHITE,BBLUED);
	LCD_DisplayNum_color(24+24*2+4+24*3,226,ch5_fre,6,24,0,WHITE,BBLUED);
	
	while (1)
    {
				key_scan(0);	//扫描继电器是否接通
				
				if(keyup_data==K1_DATA)   //断开后马上执行计数
					{
						delay_ms(1000);
						if(K_IN==1){
						ch1_val=ch1;//取出流量值计数
						ch2_val=ch2;
						ch3_val=ch3;
						ch4_val=ch4;						
						ch5_val=ch5;
						//计数清零
						ch1=0;
						ch2=0;
						ch3=0;
						ch4=0;
						ch5=0;
						auto_close();//进行漏水及流量检测
						if(ch1_close==0){//未触发漏水的情况下才计数
							ch1_fre++;//测试次数累加
							if(ch1_fre%100==0){
								//每满100计数则保存eeproom
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
						
						//对比流量最大最小值
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
						//显示次数、流量
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
						//流量最大最小值
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
				
        if(keydown_data==KEY0_DATA)//KEY0按下,清零保存的计数
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
					
				//登录贝壳物联
        if (GetTimerCount() - lastCheckInTime > postingInterval || lastCheckInTime == 0) {
            checkin();
            lastCheckInTime = GetTimerCount();
        }
        //查询网络状态
        if (GetTimerCount() - lastCheckStatusTime > statusInterval) {
            check_status();
            lastCheckStatusTime = GetTimerCount();
        }
				//上传流量数据
				if (GetTimerCount() - lastUploadTime > uploadTime){
						update1(DEVICEID,sm1,ch1_val,sm2,ch2_val,sm3,ch3_val,sm4,ch4_val,sm5,ch5_val);
						lastUploadTime = GetTimerCount();
				}
				//获取服务器时间
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
	//KEY_Init();           //初始化KEY


	

	

}
