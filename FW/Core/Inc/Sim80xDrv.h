/**
******************************************************************************

* SIM868 driver
* Author: Romuald Bialy

******************************************************************************
**/
#ifndef	_SIM80X_H
#define	_SIM80X_H

#include "main.h"
//#include "thp.h"
#include "Sim80xConfig.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cmsis_os.h"

//######################################################################################################################
//######################################################################################################################
//######################################################################################################################

typedef enum
{
  Sim80xTone_DialTone=1,
  Sim80xTone_CalledSubscriberBusy=2,
  Sim80xTone_Congestion=3,
  Sim80xTone_RadioPathAcknowledge=4,
  Sim80xTone_RadioPathNotAvailable_CallDropped=5,
  Sim80xTone_Error_SpecialInformation=6,
  Sim80xTone_CallWaitingTone=7,
  Sim80xTone_RingingTone=8,
  Sim80xTone_GeneralBeep=16,
  Sim80xTone_PositiveAcknowledgementTone=17,
  Sim80xTone_NegativeAcknowledgementOrErrorTone=18,
  Sim80xTone_IndianDialTone=19,
  Sim80xTone_AmericanDialTone=20,
  
}Sim80xTone_t;
//######################################################################################################################
typedef enum
{
  GsmVoiceStatus_Idle,
  GsmVoiceStatus_ReturnError,
  GsmVoiceStatus_ReturnNoDialTone,
  GsmVoiceStatus_ReturnNoCarrier,
  GsmVoiceStatus_ReturnNoAnswer,
  GsmVoiceStatus_ReturnBusy,
  GsmVoiceStatus_IAnswerTheCall,
  GsmVoiceStatus_MyCallAnswerd,
  GsmVoiceStatus_Ringing,
  GsmVoiceStatus_Calling,
  
}GsmVoiceStatus_t;
//######################################################################################################################
typedef enum
{
  GsmTECharacterSet_Error,
  GsmTECharacterSet_GSM,
  GsmTECharacterSet_UCS2,
  GsmTECharacterSet_IRA,
  GsmTECharacterSet_HEX,
  GsmTECharacterSet_PCCP,
  GsmTECharacterSet_PCDN,
  GsmTECharacterSet_8859_1,
	
}GsmTECharacterSet_t;
//######################################################################################################################
typedef enum
{
  GsmMsgMemory_Error,
  GsmMsgMemory_OnSim,
  GsmMsgMemory_OnModule,
	
}GsmMsgMemory_t;
//######################################################################################################################
typedef enum
{
  GsmMsgFormat_Error,
  GsmMsgFormat_PDU,
  GsmMsgFormat_Text,
	
}GsmMsgFormat_t;
//######################################################################################################################
typedef struct
{
  char                  SendCommand[128];
  char                  ReceiveAnswer[10][64];
  uint32_t              SendCommandStartTime;
  uint32_t				LowPowerTime;
  uint32_t              ReceiveAnswerMaxWaiting;
  uint8_t               FindAnswer; 
  
}Sim80xAtCommand_t;
//######################################################################################################################
typedef struct
{
  uint8_t               RegisterdToNetwork:1;
  uint8_t               Busy:1;
  uint8_t               Power:1;
  uint8_t               SmsReady:1;  
  uint8_t               CallReady:1;  
  uint8_t               LowPowerMode:1;
  uint8_t               FatalError:1;
  uint8_t               BatteryPercent;
  float                 BatteryVoltage;
  uint8_t               Signal; 
}Sim80xStatus_t;
//######################################################################################################################
typedef struct
{
   uint16_t				Year;
   uint8_t				Month;
   uint8_t              WeekDay;
   uint8_t				Day;
   uint8_t				Hour;
   uint8_t				Min;
   uint8_t				Sec;
   int8_t				Zone;
   uint16_t				Millis;
}Sim80x_Time_t;
//######################################################################################################################
typedef struct
{
  uint8_t               HaveNewCall:1;
  uint8_t               MsgReadIsOK:1;  
  uint8_t               MsgSent:1;  
  Sim80x_Time_t			Time;

  GsmVoiceStatus_t      GsmVoiceStatus;         
  char                  CallerNumber[17];
  char                  DiallingNumber[17];
  char					MyNumber[17];
  uint8_t               MsgTextModeParameterFo;
  uint8_t               MsgTextModeParameterVp;
  uint8_t               MsgTextModeParameterPid;
  uint8_t               MsgTextModeParameterDcs;
  char                  MsgServiceNumber[17];
  char                  MsgSentNumber[17];
  char                  MsgNumber[17];
  char                  MsgDate[9];
  char                  MsgTime[9];
  char                  Msg[256];
  GsmTECharacterSet_t   TeCharacterFormat;
  GsmMsgMemory_t        MsgMemory;
  GsmMsgFormat_t        MsgFormat;
  uint8_t               MsgCapacity;
  uint8_t               MsgUsed;
  uint8_t               HaveNewMsg[10];  
}Sim80xGsm_t;
//######################################################################################################################
typedef enum
{
  GPRSConnection_Idle=0,
  GPRSConnection_GPRSup,
  GPRSConnection_AlreadyConnect,
  GPRSConnection_ConnectOK,
  GPRSConnection_ConnectFail,    
  GPRSConnection_Timeout,
}GPRSConnection_t;
//######################################################################################################################
typedef enum
{
  GPRSSendData_Idle=0,
  GPRSSendData_SendInProgress,
  GPRSSendData_SendOK,
  GPRSSendData_SendFail,
  GPRSSendData_Timeout,
}GPRSSendData_t;
//######################################################################################################################
/*
typedef enum
{
  GPRSHttpMethod_GET=0,
  GPRSHttpMethod_POST=1,
  GPRSHttpMethod_HEAD=2,
  GPRSHttpMethod_DELETE=3,
}GPRSHttpMethod_t;
//######################################################################################################################

typedef struct 
{
  uint8_t                 CopyToBuffer;
  GPRSHttpMethod_t        Method;
  uint16_t                ResultCode;
  uint32_t                DataLen;
  uint32_t                TransferStartAddress;
  uint16_t                TransferDataLen;
  char                    Data[256];
}GPRSHttpAction_t;
*/
//######################################################################################################################
typedef struct
{
  uint8_t               MultiConnection:1;
  char                  APN[17];
  char                  APN_UserName[17];
  char                  APN_Password[17];
  char                  LocalIP[17]; 
  GPRSConnection_t      Connection;
  GPRSSendData_t        SendStatus;
  uint16_t              ReceiveDataLen;
  char                  ReceiveDataBuf[_SIM80X_BUFFER_SIZE];
//  GPRSHttpAction_t      HttpAction;
  
}GPRS_t;
//######################################################################################################################
typedef struct
{
   uint8_t				NewData;
   uint8_t 				RunStatus;
   uint8_t 				Fix;
   Sim80x_Time_t		Time;
   int32_t 				Lat;
   int32_t 				Lon;
   int32_t 				Alt;
   uint8_t				SatInUse;
}GPS_t;
//######################################################################################################################
typedef struct
{
  uint16_t	            UsartRxIndex;
  uint8_t		        UsartRxTemp;
  uint8_t		        UsartRxBuffer[_SIM80X_BUFFER_SIZE];
  uint64_t	            UsartRxLastTime;
  char                  IMEI[16];
  char                  CIMI[16];
  Sim80xStatus_t        Status;
  Sim80xAtCommand_t     AtCommand;
  Sim80xGsm_t           Gsm;
  GPRS_t                GPRS;
  GPS_t                 GPS;
}Sim80x_t;
//######################################################################################################################

extern Sim80x_t         Sim80x;

//######################################################################################################################
//######################################################################################################################
//######################################################################################################################
void	                Sim80x_SendString(char *str);
void                    Sim80x_SendRaw(uint8_t *Data,uint16_t len);
uint8_t                 Sim80x_SendAtCommand(char *AtCommand,int32_t  MaxWaiting_ms,uint8_t HowMuchAnswers,...);
//######################################################################################################################
void				    Sim80x_RxCallBack(uint8_t ch);
void				    Sim80x_Init(osPriority Priority);
void                    Sim80x_SaveParameters(void);
bool                    Sim80x_SetPower(bool TurnOn);
void                    Sim80x_SetFactoryDefault(void);
bool                    Sim80x_GetIMEI(char *IMEI);
bool                    Sim80x_GetCIMI(char *CIMI);
bool  					Sim80x_GetTime(void);
bool  					Sim80x_SetTime(void);
//######################################################################################################################
bool                    Gsm_Ussd(char *send,char *receive);

bool                    Gsm_CallDisconnect(void);
GsmMsgFormat_t          Gsm_MsgGetFormat(void);
bool                    Gsm_MsgSetFormat(GsmMsgFormat_t GsmMsgFormat);  
GsmMsgMemory_t          Gsm_MsgGetMemoryStatus(void);
bool                    Gsm_MsgSetMemoryLocation(GsmMsgMemory_t GsmMsgMemory);
GsmTECharacterSet_t     Gsm_MsgGetCharacterFormat(void);  
bool                    Gsm_MsgSetCharacterFormat(GsmTECharacterSet_t GsmTECharacterSet);
bool                    Gsm_MsgRead(uint8_t index);
bool                    Gsm_MsgDelete(uint8_t index);
bool                    Gsm_MsgGetServiceNumber(void);
bool                    Gsm_MsgSetServiceNumber(char *ServiceNumber);
bool                    Gsm_MsgGetTextModeParameter(void);
bool                    Gsm_MsgSetTextModeParameter(uint8_t fo,uint8_t vp,uint8_t pid,uint8_t dcs);
bool                    Gsm_MsgSendText(char *Number,char *msg);  
//######################################################################################################################
void  					GPRS_UserNewData(char *NewData, uint16_t len);
bool                    GPRS_DeactivatePDPContext(void);
//bool                    GPRS_GetAPN(char *Name,char *username,char *password);
//bool                    GPRS_SetAPN(char *Name,char *username,char *password);
bool                    GPRS_StartUpGPRS(void);
void                    GPRS_GetLocalIP(char *IP);
void                    GPRS_GetCurrentConnectionStatus(void);
bool                    GPRS_GetMultiConnection(void);
bool                    GPRS_SetMultiConnection(bool Enable);
//void                    GPRS_UserHttpGetAnswer(char *data,uint32_t StartAddress,uint16_t dataLen);
//bool                    GPRS_HttpGet(char *URL);
bool                    GPRS_ConnectToNetwork(char *Name,char *username,char *password,bool EnableMultiConnection);
bool 					GPRS_ConnectToServer(char *ip, uint16_t port);
bool 					GPRS_DisconnectFromServer();
bool  					GPRS_SendString(char *DataString);
bool  					GPRS_SendRaw(uint8_t *Data, uint16_t len);
//######################################################################################################################
bool  					GPS_SetPower(bool TurnOn);

//######################################################################################################################

#endif
