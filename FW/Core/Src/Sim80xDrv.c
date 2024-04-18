/**
******************************************************************************

* SIM868 main driver
* Author: Romuald Bialy

******************************************************************************
**/
#include "main.h"
#include "cmsis_os.h"
#include "Sim80xDrv.h"
#include "Sim80XConfig.h"
#include "thp.h"

Sim80x_t      	Sim80x;
osThreadId		Sim80xTaskHandle;
osThreadId		Sim80xBuffTaskHandle;
void 	        StartSim80xTask(void const * argument);
void 	        StartSim80xBuffTask(void const * argument);

//######################################################################################################################
//######################################################################################################################
//######################################################################################################################
void	Sim80x_SendString(char *str)
{
	if(*str == 0) return;
  #if (_SIM80X_DMA_TRANSMIT==1)
	while(_SIM80X_USART.hdmatx->State != HAL_DMA_STATE_READY)
		osDelay(10);
	HAL_UART_Transmit_DMA(&_SIM80X_USART,(uint8_t*)str,strlen(str));
	while(_SIM80X_USART.hdmatx->State != HAL_DMA_STATE_READY)
		osDelay(10);
  #else
//    if(osMutexWait(GsmMutexId, 500) == osOK)
    {
        HAL_UART_Transmit(&_SIM80X_USART,(uint8_t*)str,strlen(str),100);
//        osMutexRelease(GsmMutexId);
    }
  osDelay(10);
  #endif
}
//######################################################################################################################
void  Sim80x_SendRaw(uint8_t *Data,uint16_t len)
{
  #if (_SIM80X_DMA_TRANSMIT==1)
	while(_SIM80X_USART.hdmatx->State != HAL_DMA_STATE_READY)
		osDelay(10);
	HAL_UART_Transmit_DMA(&_SIM80X_USART,Data,len);
	while(_SIM80X_USART.hdmatx->State != HAL_DMA_STATE_READY)
		osDelay(10);
  #else
//    if(osMutexWait(GsmMutexId, 500) == osOK)
    {
        HAL_UART_Transmit(&_SIM80X_USART,Data,len,400);
//        osMutexRelease(GsmMutexId);
    }
  osDelay(10);
  #endif

}
//######################################################################################################################
void	Sim80x_RxCallBack(uint8_t ch)
{
    Sim80x.UsartRxLastTime = HAL_GetTick();
    Sim80x.UsartRxBuffer[Sim80x.UsartRxIndex] = ch; //Sim80x.UsartRxTemp;
    if(Sim80x.UsartRxIndex < (_SIM80X_BUFFER_SIZE-1))
      Sim80x.UsartRxIndex++;
}
//######################################################################################################################
uint8_t     Sim80x_SendAtCommand(char *AtCommand, int32_t MaxWaiting_ms, uint8_t HowMuchAnswers,...)
{
  while(Sim80x.Status.Busy == 1)
  {
    osDelay(100);
  }
  Sim80x.AtCommand.LowPowerTime = HAL_GetTick() + MaxWaiting_ms + 50;
  if(GSM_DTR_READ) {
	  GSM_DTR_LOW;
	  osDelay(100);
	  Sim80x.AtCommand.LowPowerTime = HAL_GetTick() + 100;
  }
  Sim80x.Status.Busy = 1;
  Sim80x.AtCommand.FindAnswer = 0;
  //Sim80x.AtCommand.ReceiveAnswerExeTime=0;
  Sim80x.AtCommand.SendCommandStartTime = HAL_GetTick();
  Sim80x.AtCommand.ReceiveAnswerMaxWaiting = MaxWaiting_ms;
  memset(Sim80x.AtCommand.ReceiveAnswer,0,sizeof(Sim80x.AtCommand.ReceiveAnswer));
  va_list tag;
	va_start (tag,HowMuchAnswers);
	char *arg[HowMuchAnswers];
	for(uint8_t i=0; i<HowMuchAnswers ; i++)
	{
		arg[i] = va_arg (tag, char *);
		strncpy(Sim80x.AtCommand.ReceiveAnswer[i],arg[i],sizeof(Sim80x.AtCommand.ReceiveAnswer[0]));
	}
  va_end (tag);
  strncpy(Sim80x.AtCommand.SendCommand,AtCommand, 120);
  Sim80x_SendString(Sim80x.AtCommand.SendCommand);
  if(_SIM80X_DEBUG == 2) printf("SIM < %s", AtCommand);
  while( MaxWaiting_ms > 0)
  {
    osDelay(10);
    if(Sim80x.AtCommand.FindAnswer > 0)
      return Sim80x.AtCommand.FindAnswer;    
    MaxWaiting_ms-=10;
  }
  memset(Sim80x.AtCommand.ReceiveAnswer,0,sizeof(Sim80x.AtCommand.ReceiveAnswer));
  Sim80x.Status.Busy=0;
  Sim80x.AtCommand.LowPowerTime = HAL_GetTick() + 50;
  return Sim80x.AtCommand.FindAnswer;
}

//######################################################################################################################
//######################################################################################################################
//######################################################################################################################
bool Sim80x_InitValue(void)
{
  Sim80x_SendAtCommand("ATE1\r\n",200,1,"ATE1\r\r\nOK\r\n");
  Sim80x_SendAtCommand("AT+COLP=1\r\n",200,1,"AT+COLP=1\r\r\nOK\r\n");
  Sim80x_SendAtCommand("AT+CLIP=1\r\n",200,1,"AT+CLIP=1\r\r\nOK\r\n");
  Sim80x_SendAtCommand("AT+FSHEX=0\r\n",200,1,"AT+FSHEX=0\r\r\nOK\r\n");
  // wlaczenie idczytu czasu z gsm i ewentualny restart SIM868
  if(Sim80x_SendAtCommand("AT+CLTS?\r\n",200,2,"\r\n+CLTS: 0\r\n","\r\n+CLTS: 1\r\n") == 1) {
	  Sim80x_SendAtCommand("AT+CLTS=1\r\n",200,1,"AT+CLTS=1\r\r\nOK\r\n");
	  Sim80x_SaveParameters();
	  Sim80x_SetPower(0);
	  return false;
  }
  Sim80x_SendAtCommand("AT+CREG=1\r\n",200,1,"AT+CREG=1\r\r\nOK\r\n");
  Sim80x_SendAtCommand("AT+CGNSURC=0\r\n",200,1,"\r\nOK\r\n");
  Sim80x_SendAtCommand("AT+CGNSPWR=1\r\n",1000,1,"\r\nOK\r\n");
  Gsm_MsgSetMemoryLocation(GsmMsgMemory_OnModule);
  Gsm_MsgSetFormat(GsmMsgFormat_Text);
  Gsm_MsgSetTextModeParameter(17,167,0,0);
  Gsm_MsgGetCharacterFormat();
  Gsm_MsgGetFormat();
  if(Sim80x.Gsm.MsgFormat != GsmMsgFormat_Text)
    Gsm_MsgSetFormat(GsmMsgFormat_Text);
  Gsm_MsgGetServiceNumber();
  Gsm_MsgGetTextModeParameter();
//  if(!Sim80x_GetIMEI(NULL)) return;
  Sim80x.Status.LowPowerMode = 1;
  Sim80x_SendAtCommand("AT+CSCLK=1\r\n",500,2,"\r\nOK\r\n","\r\nERROR\r\n");
  Sim80x_SendAtCommand("AT+CREG?\r\n",200,1,"\r\n+CREG:");
  Sim80x_SendAtCommand("AT+CGNSPWR=0\r\n",500,1,"\r\nOK\r\n");
  return true;
}
//######################################################################################################################
void   Sim80x_SaveParameters(void)
{
  Sim80x_SendAtCommand("AT&W\r\n",1000,1,"AT&W\r\r\nOK\r\n");
  if(_SIM80X_DEBUG) printf("\r\nSim80x_SaveParameters() ---> OK\r\n");
}
//######################################################################################################################
bool Sim80x_SetPower(bool TurnOn)
{ 
  Sim80x.Status.LowPowerMode = 0;
  if(TurnOn==true) {
    Sim80x.Status.Busy = 0;
    Sim80x.Status.Power=1;
    if(Sim80x_SendAtCommand("AT\r\n",200,1,"AT\r\r\nOK\r\n") == 1) {
      osDelay(100);
      if(_SIM80X_DEBUG) printf("\r\nSim80x_SetPower(ON) ---> OK\r\n");
      bool stat = Sim80x_InitValue();
      Sim80x.Status.Power=1;
      return stat;
    } else {
      #if (_SIM80X_USE_POWER_KEY==1)  
      HAL_GPIO_WritePin(_SIM80X_POWER_KEY_GPIO,_SIM80X_POWER_KEY_PIN,GPIO_PIN_SET);		// obrotka
      osDelay(1200);
      HAL_GPIO_WritePin(_SIM80X_POWER_KEY_GPIO,_SIM80X_POWER_KEY_PIN,GPIO_PIN_RESET);	// obrotka
      #endif
      osDelay(2500);
      Sim80x.Status.Power=1;
      uint8_t retry = 10;
      while(Sim80x_SendAtCommand("AT\r\n",200,1,"AT\r\r\nOK\r\n") != 1) {if(--retry == 0) break;}
      if(retry) {
        osDelay(200);
        if(_SIM80X_DEBUG) printf("\r\nSim80x_SetPower(ON) ---> OK\r\n");
        bool stat = Sim80x_InitValue();
        Sim80x.Status.Power=1;
        return stat;
      }
      else
        Sim80x.Status.Power=0;
    }
  } else {
    if(Sim80x_SendAtCommand("AT\r\n",200,1,"AT\r\r\nOK\r\n") == 1) {
      if(_SIM80X_DEBUG) printf("\r\nSim80x_SetPower(OFF) ---> OK\r\n");
      Sim80x.Status.Power=0;
      Sim80x_SendAtCommand("AT+CPOWD=1\r\n",2000,1,"\r\nOK\r\n");
    }
  }
  return true;
}
//######################################################################################################################
void Sim80x_SetFactoryDefault(void)
{
  Sim80x_SendAtCommand("AT&F0\r\n",1000,1,"AT&F0\r\r\nOK\r\n");
  if(_SIM80X_DEBUG) printf("\r\nSim80x_SetFactoryDefault() ---> OK\r\n");
}
//######################################################################################################################
bool  Sim80x_GetIMEI(char *IMEI)
{
  Sim80x_SendAtCommand("AT+GSN\r\n",1000,1,"\r\nOK\r\n");
  if(IMEI) strncpy(IMEI, Sim80x.IMEI, 16);
  if(Sim80x.IMEI[0]) {
      if(_SIM80X_DEBUG) printf("\r\nSim80x_GetIMEI(%s) <--- OK\r\n",Sim80x.IMEI);
	  return true;
  }
  if(_SIM80X_DEBUG) printf("\r\nSim80x_GetIMEI <--- ERROR\r\n");
  return false;
}
//######################################################################################################################
bool  Sim80x_GetCIMI(char *CIMI)
{
  uint8_t answer = Sim80x_SendAtCommand("AT+CIMI\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");
  if(answer == 1) {
	  if(CIMI) strncpy(CIMI, Sim80x.CIMI, 16);
	  if(Sim80x.CIMI[0] && Sim80x.CIMI[0] != 'E') {
	      if(_SIM80X_DEBUG) printf("\r\nSim80x_GetCIMI(%s) <--- OK\r\n",Sim80x.CIMI);
		  return true;
	  }
  }
  if(_SIM80X_DEBUG) printf("\r\nSim80x_GetCIMI <--- ERROR\r\n");
  return false;
}
//######################################################################################################################
bool  Sim80x_GetTime(void)
{
  if(!Sim80x.Status.Power) return false;
  uint8_t answer;
  answer=Sim80x_SendAtCommand("AT+CCLK?\r\n",1000,2,"\r\nOK\r\n","\r\n+CME ERROR:");
  if(answer==1) {
    if(_SIM80X_DEBUG) printf("\r\nSim80x_GetTime <--- OK\r\n");
    return true;
  } else {
    if(_SIM80X_DEBUG) printf("\r\nSim80x_GetTime() <--- ERROR\r\n");
    return false;
  }
}
//######################################################################################################################
bool  Sim80x_SetTime(void)
{
  uint8_t answer;
  char str[40];
  char ts = '+';
  if(Sim80x.Gsm.Time.Zone < 0) ts = '-';
  snprintf(str,sizeof(str),"AT+CCLK=\"%02u/%02u/%02u,%02u:%02u:%02u%c%02u\"\r\n",
		  Sim80x.Gsm.Time.Year-2000, Sim80x.Gsm.Time.Month, Sim80x.Gsm.Time.Day,
		  Sim80x.Gsm.Time.Hour, Sim80x.Gsm.Time.Min, Sim80x.Gsm.Time.Sec, ts, abs(Sim80x.Gsm.Time.Zone));
  answer=Sim80x_SendAtCommand(str,2000,2,"\r\nOK\r\n","\r\nERROR\r\n");
  if(answer==1) {
    if(_SIM80X_DEBUG) printf("\r\nSim80x_SetTime ---> OK\r\n");
    return true;
  } else {
    if(_SIM80X_DEBUG) printf("\r\nSim80x_SetTime ---> ERROR\r\n");
    return false;
  }
}
//######################################################################################################################
//######################################################################################################################
//######################################################################################################################
void Sim80x_Init(osPriority Priority)
{
  memset(&Sim80x,0,sizeof(Sim80x));
  osThreadDef(Sim80xTask, StartSim80xTask, Priority, 0, 512);
  Sim80xTaskHandle = osThreadCreate(osThread(Sim80xTask), NULL);
  osDelay(10);
  osThreadDef(Sim80xBuffTask, StartSim80xBuffTask, Priority, 0, 512);
  Sim80xBuffTaskHandle = osThreadCreate(osThread(Sim80xBuffTask), NULL);
  osDelay(10);
  Sim80x.Gsm.Time.Year  = 2024;
  Sim80x.Gsm.Time.Month = 1;
  Sim80x.Gsm.Time.Day	= 1;
  if(!Sim80x_SetPower(true)) Sim80x_SetPower(true); 	//ponowne odpalenie po restarcie
  GPRS_DeactivatePDPContext();						// GPRS disconnect
}
//######################################################################################################################
void Sim80x_BufferProcess(void)
{
  char      *strStart,*str1,*str2;
  int32_t   tmp_int32_t;
  char      tmp_str[20];
  
  strStart = (char*)&Sim80x.UsartRxBuffer[0];  

  //##################################################
  //+++       Buffer Process
  //##################################################
  str1 = strstr(strStart,"\r\n+CREG:");
  if(str1!=NULL) {
    str1 = strchr(str1,',');
    str1++;
    if(atoi(str1)==1)
      Sim80x.Status.RegisterdToNetwork=1;
    else
      Sim80x.Status.RegisterdToNetwork=0;
  }
  //##################################################
  str1 = strstr(strStart,"\r\nCall Ready\r\n");
  if(str1!=NULL)
    Sim80x.Status.CallReady=1;  
  //##################################################
  str1 = strstr(strStart,"\r\nSMS Ready\r\n");
  if(str1!=NULL)
    Sim80x.Status.SmsReady=1;  
  //##################################################
  str1 = strstr(strStart,"\r\n+COLP:");
  if(str1!=NULL) {
    Sim80x.Gsm.GsmVoiceStatus = GsmVoiceStatus_MyCallAnswerd;
  }  
  //##################################################
  str1 = strstr(strStart,"\r\n+CLIP:");
  if(str1!=NULL) {
    str1 = strchr(str1,'"');
    str1++;
    str2 = strchr(str1,'"');
    strncpy(Sim80x.Gsm.CallerNumber,str1,str2-str1);
    Sim80x.Gsm.HaveNewCall=1;  
  }  
  //##################################################
  str1 = strstr(strStart,"\r\n+CSQ:");
  if(str1!=NULL) {
    str1 = strchr(str1,':');
    str1++;
    Sim80x.Status.Signal = atoi(str1);      
  }
  //##################################################
  str1 = strstr(strStart,"\r\n+CBC:");
  if(str1!=NULL) {
    str1 = strchr(str1,':');
    str1++;
    tmp_int32_t = atoi(str1);
    str1 = strchr(str1,',');
    str1++;
    Sim80x.Status.BatteryPercent = atoi(str1);
    str1 = strchr(str1,',');
    str1++;
    Sim80x.Status.BatteryVoltage = atof(str1)/1000;      
  }
  //##################################################
  str1 = strstr(strStart,"\r\nBUSY\r\n");
  if(str1!=NULL) {
    Sim80x.Gsm.GsmVoiceStatus=GsmVoiceStatus_ReturnBusy;
  }
  //##################################################
  str1 = strstr(strStart,"\r\nNO DIALTONE\r\n");
  if(str1!=NULL) {
    Sim80x.Gsm.GsmVoiceStatus=GsmVoiceStatus_ReturnNoDialTone;
  }
  //##################################################
  str1 = strstr(strStart,"\r\nNO CARRIER\r\n");
  if(str1!=NULL) {
    Sim80x.Gsm.GsmVoiceStatus=GsmVoiceStatus_ReturnNoCarrier;
  }
  //##################################################
  str1 = strstr(strStart,"\r\nNO ANSWER\r\n");
  if(str1!=NULL) {
    Sim80x.Gsm.GsmVoiceStatus=GsmVoiceStatus_ReturnNoAnswer;
  }
  //##################################################  
  str1 = strstr(strStart,"NORMAL POWER DOWN");
  if(str1!=NULL && Sim80x.Status.Power) {
	  memset(&Sim80x,0,sizeof(Sim80x));
	  osDelay(2000);
	  Sim80x_SetPower(true);
  }
  //##################################################
  str1 = strstr(strStart,"\r\n+CMGS:");
  if(str1!=NULL) {
    Sim80x.Gsm.MsgSent=1;
  }  
  //##################################################  
  str1 = strstr(strStart,"\r\n+CPMS:");
  if(str1!=NULL) {
    str1 = strchr(str1,':');
    str1++;
    str1++;
    if(*str1 == '"') {
      str1 = strchr(str1,',');
      str1++;
    }
    Sim80x.Gsm.MsgUsed = atoi(str1);
    str1 = strchr(str1,',');
    str1++;      
    Sim80x.Gsm.MsgCapacity = atoi(str1);
  }  
  //##################################################  
  str1 = strstr(strStart,"\r\n+CMGR:");
  if(str1!=NULL) {
    if(Sim80x.Gsm.MsgFormat == GsmMsgFormat_Text) {
      memset(Sim80x.Gsm.Msg,0,sizeof(Sim80x.Gsm.Msg));
      memset(Sim80x.Gsm.MsgDate,0,sizeof(Sim80x.Gsm.MsgDate));
      memset(Sim80x.Gsm.MsgNumber,0,sizeof(Sim80x.Gsm.MsgNumber));
      memset(Sim80x.Gsm.MsgTime,0,sizeof(Sim80x.Gsm.MsgTime));
      tmp_int32_t = sscanf(str1,"\r\n+CMGR: %*[^,],\"%[^\"]\",%*[^,],\"%[^,],%[^+-]%*d\"\r\n%[^\r]s\r\nOK\r\n",Sim80x.Gsm.MsgNumber,Sim80x.Gsm.MsgDate,Sim80x.Gsm.MsgTime,Sim80x.Gsm.Msg);      
      if(tmp_int32_t == 4)
        Sim80x.Gsm.MsgReadIsOK=1;
      else
        Sim80x.Gsm.MsgReadIsOK=0;
    }else if(Sim80x.Gsm.MsgFormat == GsmMsgFormat_PDU) {
        memset(Sim80x.Gsm.Msg,0,sizeof(Sim80x.Gsm.Msg));
        memset(Sim80x.Gsm.MsgDate,0,sizeof(Sim80x.Gsm.MsgDate));
        memset(Sim80x.Gsm.MsgNumber,0,sizeof(Sim80x.Gsm.MsgNumber));
        memset(Sim80x.Gsm.MsgTime,0,sizeof(Sim80x.Gsm.MsgTime));
        Sim80x.Gsm.MsgReadIsOK=1;
    }
  }
  //################################################## 
  str1 = strstr(strStart,"\r\n+CNUM:");
   if(str1!=NULL) {
     str1 = strchr(str1,',');
     str1++;
     str1++;
     str2 = strchr(str1,'"');
     strncpy(Sim80x.Gsm.MyNumber,str1,str2-str1);
   }
  //################################################## 
  str1 = strstr(strStart,"\r\n+CCLK:");
  if(str1!=NULL) {
    str1 = strchr(str1,'"');
    str1++;
	strncpy(tmp_str, str1+0, 2);
	tmp_str[2]=0;
	Sim80x.Gsm.Time.Year = atoi(tmp_str)+2000;	// year
	strncpy(tmp_str, str1+3, 2);
	Sim80x.Gsm.Time.Month = atoi(tmp_str);	// month
	strncpy(tmp_str, str1+6, 2);
	Sim80x.Gsm.Time.Day = atoi(tmp_str);	// day
	strncpy(tmp_str, str1+9, 2);
	Sim80x.Gsm.Time.Hour = atoi(tmp_str);	// hour
	strncpy(tmp_str, str1+12, 2);
	Sim80x.Gsm.Time.Min = atoi(tmp_str);	// min
	strncpy(tmp_str, str1+15, 2);
	Sim80x.Gsm.Time.Sec = atoi(tmp_str);	// sec
	strncpy(tmp_str, str1+17, 3);
	Sim80x.Gsm.Time.Zone = atoi(tmp_str); 	// timezone
	Sim80x.Gsm.Time.Millis = 0;
  }
  //################################################## 
  str1 = strstr(strStart,"\r\n+CMTI:");
  if(str1!=NULL) {
    str1 = strchr(str1,',');
    str1++;
    for(uint8_t i=0 ;i<sizeof(Sim80x.Gsm.HaveNewMsg) ; i++)
    {
      if(Sim80x.Gsm.HaveNewMsg[i]==0) {
        Sim80x.Gsm.HaveNewMsg[i] = atoi(str1);    
        break;
      }
    }
  }
  //##################################################  
  str1 = strstr(strStart,"\r\n+CSCA:");
  if(str1!=NULL) {
    memset(Sim80x.Gsm.MsgServiceNumber,0,sizeof(Sim80x.Gsm.MsgServiceNumber));
    str1 = strchr(str1,'"');
    str1++;
    str2 = strchr(str1,'"');
    strncpy(Sim80x.Gsm.MsgServiceNumber,str1,str2-str1);
  }
  //##################################################  
  str1 = strstr(strStart,"\r\n+CSMP:");
  if(str1!=NULL) {
    tmp_int32_t = sscanf(str1,"\r\n+CSMP: %hhd,%hhd,%hhd,%hhd\r\nOK\r\n",&Sim80x.Gsm.MsgTextModeParameterFo,&Sim80x.Gsm.MsgTextModeParameterVp,&Sim80x.Gsm.MsgTextModeParameterPid,&Sim80x.Gsm.MsgTextModeParameterDcs);
  }
  //##################################################  
  str1 = strstr(strStart,"\r\n+CUSD:");
  if(str1!=NULL) {
    sscanf(str1,"\r\n+CUSD: 0, \"%[^\r]s",Sim80x.Gsm.Msg);    
    tmp_int32_t = strlen(Sim80x.Gsm.Msg);
    if(tmp_int32_t > 5) {
      Sim80x.Gsm.Msg[tmp_int32_t-5] = 0;
    }
  }
  //##################################################  
  str1 = strstr(strStart,"AT+GSN\r");
  if(str1!=NULL) {
    sscanf(str1,"\nAT+GSN\r\r\n%[^\r]",Sim80x.IMEI);    
  }
  //##################################################  
  str1 = strstr(strStart,"AT+CIMI\r");
  if(str1!=NULL) {
    sscanf(str1,"\nAT+CIMI\r\r\n%[^\r]",Sim80x.CIMI);
  }
  //##################################################  
  //##################################################  
  //##################################################
  #if (_SIM80X_USE_GPRS==1)
  //##################################################  
  str1 = strstr(strStart,"\r\n+CSTT:");
  if(str1!=NULL) {
    sscanf(str1,"\r\n+CSTT: \"%[^\"]\",\"%[^\"]\",\"%[^\"]\"\r\n",Sim80x.GPRS.APN,Sim80x.GPRS.APN_UserName,Sim80x.GPRS.APN_Password);    
  }
  //##################################################  
  str1 = strstr(strStart,"AT+CIFSR\r\r\n");
  if(str1!=NULL) {
    sscanf(str1,"AT+CIFSR\r\r\n%[^\r]",Sim80x.GPRS.LocalIP);
  } 
  //##################################################  
  str1 = strstr(strStart,"\r\n+CIPMUX:");
  if(str1!=NULL) {
    str1 =strchr(str1,':');
    str1++;
    if(atoi(str1)==0)
      Sim80x.GPRS.MultiConnection=0;
    else
      Sim80x.GPRS.MultiConnection=1;
  } 
  //##################################################  
  str1 = strstr(strStart,"\r\nCONNECT OK\r\n");
  if(str1!=NULL) {
    Sim80x.GPRS.Connection = GPRSConnection_ConnectOK;
  } 
  //##################################################  
  str1 = strstr(strStart,"\r\nCLOSE OK\r\n");
  if(str1!=NULL) {
	Sim80x.GPRS.Connection = GPRSConnection_GPRSup;
  }
  str1 = strstr(strStart,"\r\nCLOSED\r\n");
  if(str1!=NULL) {
	Sim80x.GPRS.Connection = GPRSConnection_GPRSup;
  }
  //##################################################
  str1 = strstr(strStart,"\r\nCONNECT FAIL\r\n");
  if(str1!=NULL) {
    Sim80x.GPRS.Connection = GPRSConnection_ConnectFail;
  } 
  //##################################################  
  str1 = strstr(strStart,"\r\nALREADY CONNECT\r\n");
  if(str1!=NULL) {
    Sim80x.GPRS.Connection = GPRSConnection_AlreadyConnect;
  } 
  str1 = strstr(strStart,"\r\nSTATE: IP GPRSACT\r\n");
  if(str1!=NULL) {
    Sim80x.GPRS.Connection = GPRSConnection_AlreadyConnect;
  }
  //##################################################  
  str1 = strstr(strStart,"\r\nSHUT OK\r\n");
  if(str1!=NULL) {
    Sim80x.GPRS.Connection = GPRSConnection_Idle;
  }
  str1 = strstr(strStart,"\r\nSTATE: IP INITIAL\r\n");
  if(str1!=NULL) {
    Sim80x.GPRS.Connection = GPRSConnection_Idle;
  }
  //##################################################
  str1 = strstr(strStart,"\r\nSEND OK\r\n");
  if(str1!=NULL) {
    Sim80x.GPRS.SendStatus = GPRSSendData_SendOK;
  } 
  //##################################################  
  str1 = strstr(strStart,"\r\nSEND FAIL\r\n");
  if(str1!=NULL) {
    Sim80x.GPRS.SendStatus = GPRSSendData_SendFail;
  }
  //##################################################
  str1 = strstr(strStart,"\r\n+IPD");
  if(str1!=NULL) {
    str1 = strchr(str1,',');
    str1++;
    tmp_int32_t = atoi(str1);
    if(tmp_int32_t > sizeof(Sim80x.GPRS.ReceiveDataBuf))
    	tmp_int32_t = sizeof(Sim80x.GPRS.ReceiveDataBuf);
    str1 = strchr(str1,':');
    str1++;
    memcpy(Sim80x.GPRS.ReceiveDataBuf,str1,tmp_int32_t);
    Sim80x.GPRS.ReceiveDataLen = tmp_int32_t;
    if(_SIM80X_DEBUG) printf("\r\nGot IPD, %d bytes\r\n", Sim80x.GPRS.ReceiveDataLen);
  }
  //##################################################
  #endif  
  //##################################################  
  //##################################################  
  str1 = strstr(strStart,"\r\n+CGNSINF:");
  if(str1 == NULL)
	  str1 = strstr(strStart,"\r\n+UGNSINF:");
  if(str1 != NULL) {
	float tmp;
    str1 = strchr(str1,':');
    str1++;
    Sim80x.GPS.RunStatus = atoi(str1);
//    printf("GNS_RUN %d\r\n", Sim80x.GPS.RunStatus);
    if(Sim80x.GPS.RunStatus) {
		str1 = strchr(str1,',');
		str1++;
		str1 = strchr(str1,',');
		str1++;
		strncpy(tmp_str, str1+0, 4);
		tmp_str[4]=0;
		Sim80x.GPS.Time.Year = atoi(tmp_str);	// year
		strncpy(tmp_str, str1+4, 2);
		tmp_str[2]=0;
		tmp_str[3]=0;
		Sim80x.GPS.Time.Month = atoi(tmp_str);	// month
		strncpy(tmp_str, str1+6, 2);
		Sim80x.GPS.Time.Day = atoi(tmp_str);	// day
		strncpy(tmp_str, str1+8, 2);
		Sim80x.GPS.Time.Hour = atoi(tmp_str);	// hour
		strncpy(tmp_str, str1+10, 2);
		Sim80x.GPS.Time.Min = atoi(tmp_str);	// min
		strncpy(tmp_str, str1+12, 2);
		Sim80x.GPS.Time.Sec = atoi(tmp_str);	// sec
		strncpy(tmp_str, str1+15, 3);
		Sim80x.GPS.Time.Millis = atoi(tmp_str); // millis
		Sim80x.GPS.Time.Zone = 0;

		str1 = strchr(str1,',');
		str1++;
		tmp = atof(str1);	// lat
		Sim80x.GPS.Lat = tmp * 10000000;
		str1 = strchr(str1,',');
		str1++;
		tmp = atof(str1);	// lon
		Sim80x.GPS.Lon = tmp * 10000000;
		str1 = strchr(str1,',');
		str1++;
		tmp = atof(str1);	// alt
		Sim80x.GPS.Alt = tmp * 100;

		for(uint8_t i=0; i<3; i++)
		{
			str1 = strchr(str1,',');
			str1++;
		}
		Sim80x.GPS.Fix = atoi(str1);

		for(uint8_t i=0; i<7; i++)
		{
			str1 = strchr(str1,',');
			str1++;
		}
		Sim80x.GPS.SatInUse = atoi(str1);
    }
	Sim80x.GPS.NewData = 1;
  }
  //##################################################
  str1 = strstr(strStart,"\r\nAXN_");
  if(str1 != NULL) {
      Sim80x.GPS.RunStatus = 2;
  }
  //##################################################  
  //##################################################  
  //##################################################  
  for( uint8_t parameter=0; parameter<11; parameter++)
  {
    if((parameter==10) || (Sim80x.AtCommand.ReceiveAnswer[parameter][0]==0)) {
      Sim80x.AtCommand.FindAnswer=0;
      break;
    }
    str1 = strstr(strStart,Sim80x.AtCommand.ReceiveAnswer[parameter]);
    if(str1!=NULL) {
      Sim80x.AtCommand.FindAnswer = parameter+1;
//      Sim80x.AtCommand.ReceiveAnswerExeTime = HAL_GetTick()-Sim80x.AtCommand.SendCommandStartTime;
      break;
    }    
  }
  //##################################################
  //---       Buffer Process
  //##################################################
//  if(debug_level & 4) printf("%s",strStart);
  Sim80x.UsartRxIndex=0;
  memset(Sim80x.UsartRxBuffer,0,_SIM80X_BUFFER_SIZE);    
  Sim80x.Status.Busy=0;
  Sim80x.AtCommand.LowPowerTime = HAL_GetTick() + 50;
}

//######################################################################################################################
//######################################################################################################################
//######################################################################################################################
void StartSim80xBuffTask(void const * argument)
{ 
  printf("SIM BuffTaskStart\r\n");
  while(1)
  {
    if( ((Sim80x.UsartRxIndex>4) && (HAL_GetTick()-Sim80x.UsartRxLastTime > 20)) ) {
      if(_SIM80X_DEBUG==2) printf("SIM > %s", Sim80x.UsartRxBuffer);
      Sim80x_BufferProcess();      
    }
    osDelay(Sim80x.Status.Power ? 3 : 50);
  }    
}
//######################################################################################################################
void StartSim80xTask(void const * argument)
{ 
  uint32_t TimeForSlowRun=0;
  #if( _SIM80X_USE_GPRS==1)
  uint32_t TimeForSlowRunGPRS=0;
  #endif
  uint8_t UnreadMsgCounter=1;
  printf("SIM TaskStart\r\n");

  while(1)
  {
    if(Sim80x.Status.Power == 0 && Sim80x.GPRS.ReceiveDataLen == 0) {
	  osDelay(100);
	  continue;
    }
    //###########################################
    #if( _SIM80X_USE_GPRS==1)
    //###########################################
    if(HAL_GetTick()-TimeForSlowRunGPRS > 5000) {
      TimeForSlowRunGPRS=HAL_GetTick();
    }
    //###########################################
    if(Sim80x.GPRS.ReceiveDataLen > 0) {                                        // POPRAWKA NA GUBIONE DANE Z IPD
      char *mptr = malloc(Sim80x.GPRS.ReceiveDataLen);                          // tymczasowy bufor
      uint16_t mlen = Sim80x.GPRS.ReceiveDataLen;                                    // zapamietaj dlugosc
      memcpy(mptr, Sim80x.GPRS.ReceiveDataBuf, Sim80x.GPRS.ReceiveDataLen);     // skopiuj dane IPD do tymczasowego
      Sim80x.GPRS.ReceiveDataLen = 0;                                           // zaznacz ze odebrano dane
      if(_SIM80X_DEBUG) printf("Sending IPD data to app\r\n");
      GPRS_UserNewData(mptr, mlen);                                             // interpretacja w trakcie kt�rej mog�
      free(mptr);                                                               // przyjsc nowe dane i nie zostan� utracone
    }
    //###########################################
    #endif
    //###########################################
    for(uint8_t msgnum=0 ;msgnum<sizeof(Sim80x.Gsm.HaveNewMsg) ; msgnum++)
    {
      if(Sim80x.Gsm.HaveNewMsg[msgnum] > 0) {
        if(Gsm_MsgRead(Sim80x.Gsm.HaveNewMsg[msgnum])==true) {
//          osDelay(100);
//          Gsm_UserNewMsg(Sim80x.Gsm.MsgNumber,Sim80x.Gsm.MsgDate,Sim80x.Gsm.MsgTime,Sim80x.Gsm.Msg);
          osDelay(100);
          Gsm_MsgDelete(Sim80x.Gsm.HaveNewMsg[msgnum]);
          osDelay(100);
        }
        Gsm_MsgGetMemoryStatus();
        Sim80x.Gsm.HaveNewMsg[msgnum] = 0;
      }        
    }    
    //###########################################
    if(Sim80x.Gsm.MsgUsed > 0) {
      if(Gsm_MsgRead(UnreadMsgCounter)==true) {
//        osDelay(100);
//        Gsm_UserNewMsg(Sim80x.Gsm.MsgNumber,Sim80x.Gsm.MsgDate,Sim80x.Gsm.MsgTime,Sim80x.Gsm.Msg);
        osDelay(100);
        Gsm_MsgDelete(UnreadMsgCounter);
        osDelay(100);
        Gsm_MsgGetMemoryStatus();
      }
      UnreadMsgCounter++;
      if(UnreadMsgCounter==150)
        UnreadMsgCounter=0;      
    }
    //###########################################
    if(Sim80x.Gsm.HaveNewCall == 1) {
      Sim80x.Gsm.GsmVoiceStatus = GsmVoiceStatus_Ringing;
      Sim80x.Gsm.HaveNewCall = 0;

      osDelay(500);
	  Gsm_CallDisconnect();		// rozlacz po 3 sekundach

//      Gsm_UserNewCall(Sim80x.Gsm.CallerNumber);
    }    
    //###########################################
    if(HAL_GetTick() - TimeForSlowRun > (Sim80x.Status.RegisterdToNetwork ? 45000:10000) && Sim80x.GPRS.Connection != GPRSConnection_ConnectOK) { // nie przy aktywnym GPRS
      Sim80x_SendAtCommand("AT+CSQ\r\n",200,1,"\r\n+CSQ:");
      Sim80x_SendAtCommand("AT+CREG?\r\n",200,1,"\r\n+CREG:");
      Gsm_MsgGetMemoryStatus();
      SysTimeSync();
      TimeForSlowRun = HAL_GetTick();
    }

    if(GSM_DTR_READ == 0 && HAL_GetTick() > Sim80x.AtCommand.LowPowerTime && Sim80x.Status.LowPowerMode) {
      GSM_DTR_HI;
    }
    //###########################################
//    Gsm_User(HAL_GetTick());
    //###########################################
    osDelay(5);
  }    
}



