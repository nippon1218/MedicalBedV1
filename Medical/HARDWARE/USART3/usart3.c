#include "sys.h"
#include "usart3.h"	  
#include "stdarg.h"	 	 
#include "stdio.h"	 	 
#include "string.h"
#include "timer.h"  

u8 aRxBuffer[BUFFSIZE];//HAL��ʹ�õĴ��ڽ��ջ���
u8 USART3_RX_BUF[USART3_MAX_RECV_LEN]; 				//���ջ���,���USART3_MAX_RECV_LEN���ֽ�.
u8  USART3_TX_BUF[USART3_MAX_SEND_LEN]; 			//���ͻ���,���USART3_MAX_SEND_LEN�ֽ�

//ͨ���жϽ�������2���ַ�֮���ʱ������10ms�������ǲ���һ������������.
//���2���ַ����ռ������10ms,����Ϊ����1����������.Ҳ���ǳ���10msû�н��յ�
//�κ�����,���ʾ�˴ν������.
//���յ�������״̬
//[15]:0,û�н��յ�����;1,���յ���һ������.
//[14:0]:���յ������ݳ���
vu16 USART3_RX_STA=0;  
UART_HandleTypeDef UART3_Handler; //UART���
//TIM_HandleTypeDef TIM7_Handler;   //TIM���
void USART3_IRQHandler(void)
{
	u8 res;	      
	if(__HAL_UART_GET_FLAG(&UART3_Handler,UART_FLAG_RXNE)!=RESET)//���յ�����
	{	 
		HAL_UART_Receive(&UART3_Handler,&res,1,1000);
//		res=USART3->DR; 			 
		if((USART3_RX_STA&(1<<15))==0)//�������һ������,��û�б�����,���ٽ�����������
		{ 
			if(USART3_RX_STA<USART3_MAX_RECV_LEN)	//�����Խ�������
			{
//				__HAL_TIM_SetCounter(&TIM7_Handler,0);	
				TIM7->CNT=0;         				//���������	
				if(USART3_RX_STA==0) 				//ʹ�ܶ�ʱ��7���ж� 
				{
//					__HAL_RCC_TIM7_CLK_ENABLE();            //ʹ��TIM7ʱ��
					TIM7->CR1|=1<<0;     			//ʹ�ܶ�ʱ��7
				}
				USART3_RX_BUF[USART3_RX_STA++]=res;	//��¼���յ���ֵ	 
			}else 
			{
				USART3_RX_STA|=1<<15;				//ǿ�Ʊ�ǽ������
			} 
		}
	}  				 											 
}   

void usart3_init(u32 bound)
{  	 
	//UART ��ʼ������
	UART3_Handler.Instance=USART3;					    //USART1
	UART3_Handler.Init.BaudRate=bound;				    //������
	UART3_Handler.Init.WordLength=UART_WORDLENGTH_8B;   //�ֳ�Ϊ8λ���ݸ�ʽ
	UART3_Handler.Init.StopBits=UART_STOPBITS_1;	    //һ��ֹͣλ
	UART3_Handler.Init.Parity=UART_PARITY_NONE;		    //����żУ��λ
	UART3_Handler.Init.HwFlowCtl=UART_HWCONTROL_NONE;   //��Ӳ������
	UART3_Handler.Init.Mode=UART_MODE_TX_RX;		    //�շ�ģʽ
	HAL_UART_Init(&UART3_Handler);					    //HAL_UART_Init()��ʹ��UART3
	TIM7_Int_Init(1000-1,8400-1);		//100ms�ж�
	HAL_UART_Receive_IT(&UART3_Handler, (u8 *)aRxBuffer, BUFFSIZE);//�ú����Ὺ�������жϣ���־λUART_IT_RXNE���������ý��ջ����Լ����ջ���������������
	USART3_RX_STA=0;		//����
	TIM7->CR1&=~(1<<0);        //�رն�ʱ��7
//	__HAL_TIM_DISABLE(&TIM7_Handler);      //�رն�ʱ��7
}
//UART�ײ��ʼ����ʱ��ʹ�ܣ��������ã��ж�����
//�˺����ᱻHAL_UART_Init()����
//huart:���ھ��
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //GPIO�˿�����
	GPIO_InitTypeDef GPIO_Initure;
	
	__HAL_RCC_GPIOB_CLK_ENABLE();			//ʹ��GPIOAʱ��
	__HAL_RCC_USART3_CLK_ENABLE();			//ʹ��USART1ʱ��
	
	GPIO_Initure.Pin=GPIO_PIN_10;			//PB10
	GPIO_Initure.Mode=GPIO_MODE_AF_PP;		//�����������
	GPIO_Initure.Pull=GPIO_PULLUP;			//����
	GPIO_Initure.Speed=GPIO_SPEED_FAST;		//����
	GPIO_Initure.Alternate=GPIO_AF7_USART3;	//����ΪUSART1
	HAL_GPIO_Init(GPIOB,&GPIO_Initure);	   	//��ʼ��PB10

	GPIO_Initure.Pin=GPIO_PIN_11;			//PB11
	HAL_GPIO_Init(GPIOB,&GPIO_Initure);	   	//��ʼ��PB11
	
  __HAL_UART_DISABLE_IT(huart,UART_IT_TC);
	__HAL_UART_ENABLE_IT(huart,UART_IT_RXNE);		//���������ж�
	HAL_NVIC_EnableIRQ(USART3_IRQn);				//ʹ��USART3�ж�
	HAL_NVIC_SetPriority(USART3_IRQn,2,3);			//��ռ���ȼ�3�������ȼ�3	
}
//����3,printf ����
//ȷ��һ�η������ݲ�����USART3_MAX_SEND_LEN�ֽ�
void u3_printf(char* fmt,...)  
{  
	u16 i,j; 
	va_list ap; 
	va_start(ap,fmt);
	vsprintf((char*)USART3_TX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)USART3_TX_BUF);		//�˴η������ݵĳ���
	for(j=0;j<i;j++)							//ѭ����������
	{
		while((USART3->SR&0X40)==0);			//ѭ������,ֱ���������   
		USART3->DR=USART3_TX_BUF[j];  
	} 
}


































