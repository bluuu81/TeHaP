/**
******************************************************************************

* GPRS functions
* Author: Romuald Bialy

******************************************************************************
**/

#include "Sim80xDrv.h"
#if (_SIM80X_USE_GPRS==1)

//####################################################################################################
bool  GPRS_StartUpGPRS(void)
{
  uint8_t answer;
  answer = Sim80x_SendAtCommand("AT+CIICR\r\n",85000,2,"\r\nOK\r\n","\r\nERROR\r\n");
  if(answer == 1)
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_StartUpGPRS() ---> OK\r\n");
    #endif
    return true;
  }
  else  
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_StartUpGPRS() --->ERROR\r\n");
    #endif
    return false;
  }
}  
//####################################################################################################
bool  GPRS_GetAPN(char *Name,char *username,char *password)
{
  uint8_t answer;
  answer = Sim80x_SendAtCommand("AT+CSTT?\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");
  if(answer==1)
  {
    if(Name!=NULL)
      strcpy(Name,Sim80x.GPRS.APN);
    if(username!=NULL)
      strcpy(username,Sim80x.GPRS.APN_UserName);
    if(password!=NULL)
      strcpy(password,Sim80x.GPRS.APN_Password);
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_GetAPN(%s,%s,%s) <--- OK\r\n",Sim80x.GPRS.APN,Sim80x.GPRS.APN_UserName,Sim80x.GPRS.APN_Password);
    #endif
    return true;
  }
  else
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_GetAPN() <--- ERROR\r\n");
    #endif
    return false;
  }
}
//####################################################################################################
bool  GPRS_SetAPN(char *Name,char *username,char *password)
{
  char str[64];
  uint8_t answer;
  sprintf(str,"AT+CSTT=\"%s\",\"%s\",\"%s\"\r\n",Name,username,password);
  answer = Sim80x_SendAtCommand(str,1000,2,"\r\nOK\r\n","\r\nERROR\r\n");
  if(answer == 1)
  {
    strcpy(Sim80x.GPRS.APN,Name);
    strcpy(Sim80x.GPRS.APN_UserName,username);
    strcpy(Sim80x.GPRS.APN_Password,password);
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_SetAPN(\"%s\",\"%s\",\"%s\") ---> OK\r\n",Name,username,password);
    #endif    
    return true;
  }
  else
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_SetAPN(\"%s\",\"%s\",\"%s\") ---> ERROR\r\n",Name,username,password);
    #endif    
    return false;  
  }  
}
//####################################################################################################
bool  GPRS_DeactivatePDPContext(void)
{
  uint8_t answer;
  answer = Sim80x_SendAtCommand("AT+CIPSHUT\r\n",10000,2,"\r\nSHUT OK\r\n","\r\nERROR\r\n");
  if(answer == 1)
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_DeactivatePDPContext() ---> OK\r\n");
    #endif    
    return true;
  }
  else
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_DeactivatePDPContext() ---> ERROR\r\n");
    #endif    
    return false;  
  }
}
//####################################################################################################
void  GPRS_GetLocalIP(char *IP)
{
  uint8_t answer;
  answer = Sim80x_SendAtCommand("AT+CIFSR\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");
  if((IP!=NULL) && (answer==1))
    strcpy(IP,Sim80x.GPRS.LocalIP);
  #if (_SIM80X_DEBUG==1)
  printf("\r\nGPRS_GetLocalIP(%s) <--- OK\r\n",Sim80x.GPRS.LocalIP);
  #endif    
}
//####################################################################################################
void  GPRS_GetCurrentConnectionStatus(void)
{
  Sim80x_SendAtCommand(" AT+CIPSTATUS\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");  
  #if (_SIM80X_DEBUG==1)
  printf("\r\nGPRS_GetCurrentConnectionStatus() <--- OK\r\n");
  #endif      
}
//####################################################################################################
bool  GPRS_GetMultiConnection(void)
{
  Sim80x_SendAtCommand(" AT+CIPMUX?\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");  
  #if (_SIM80X_DEBUG==1)
  printf("\r\nGPRS_GetMultiConnection(%d) <--- OK\r\n",Sim80x.GPRS.MultiConnection);
  #endif  
  return Sim80x.GPRS.MultiConnection;
}
//####################################################################################################
bool  GPRS_SetMultiConnection(bool Enable)
{
  uint8_t answer;
  if(Enable==true)
    answer = Sim80x_SendAtCommand(" AT+CIPMUX=1\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");  
  else
    answer = Sim80x_SendAtCommand(" AT+CIPMUX=0\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");  
  if(answer == 1)
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_SetMultiConnection(%d) ---> OK\r\n",Enable);
    #endif  
    Sim80x.GPRS.MultiConnection=Enable;
    return true;
  }
  else
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_SetMultiConnection(%d) ---> ERROR\r\n",Enable);
    #endif  
    return false;
  }
}
//####################################################################################################
//####################################################################################################
//####################################################################################################
bool  GPRS_ConnectToNetwork(char *Name,char *username,char *password,bool EnableMultiConnection)
{
  
  if(GPRS_DeactivatePDPContext()==false)
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_ConnectToNetwork() ---> ERROR\r\n");
    #endif 
    return false;
  }
  GPRS_SetMultiConnection(EnableMultiConnection);
  if(GPRS_SetAPN(Name,username,password)==false)
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_ConnectToNetwork() ---> apn ERROR\r\n");
    #endif 
    return false;
  }
  if(GPRS_StartUpGPRS()==false)
  {
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_ConnectToNetwork() ---> startup ERROR\r\n");
    #endif 
    return false;
  }
  GPRS_GetLocalIP(NULL);  
  #if (_SIM80X_DEBUG==1)
  printf("\r\nGPRS_ConnectToNetwork() ---> OK\r\n");
  #endif

  Sim80x_SendAtCommand("AT+CIPHEAD=1\r\n", 500, 1, "\r\nOK\r\n");
  return true;
}
//####################################################################################################
bool GPRS_ConnectToServer(char *ip, uint16_t port)
{
	uint8_t answer;
	char buf[64];
	snprintf(buf, sizeof(buf), "AT+CIPSTART=\"TCP\",\"%s\",\"%u\"\r\n", ip, port);
	answer = Sim80x_SendAtCommand(buf, 10000, 2, "\r\nOK\r\n","\r\nERROR\r\n");
	if(answer == 1)
	{
		return true;
	}
	else
	{
		GPRS_GetCurrentConnectionStatus();
		if(Sim80x.GPRS.Connection == GPRSConnection_AlreadyConnect)
		{
			return true;
		}
	}
	if(Sim80x.GPRS.Connection) Sim80x.GPRS.Connection = GPRSConnection_GPRSup;
	return false;
}
//#################################################################################################################
bool GPRS_DisconnectFromServer()
{
	uint8_t answer;
	answer = Sim80x_SendAtCommand("AT+CIPCLOSE=0\r\n", 5000, 1, "\r\nOK\r\n");
	if(answer == 1)
	{
		if(Sim80x.GPRS.Connection) Sim80x.GPRS.Connection = GPRSConnection_GPRSup;
		return true;
	}
	return false;
}
//#################################################################################################################
bool  GPRS_SendString(char *DataString)
{
  uint8_t answer;
  char str[4];
  Sim80x.GPRS.SendStatus = GPRSSendData_SendInProgress;
  answer = Sim80x_SendAtCommand("AT+CIPSEND\r\n",1500,2,"\r\r\n> ","\r\nERROR\r\n");
  if(answer == 1)
  {
    Sim80x_SendString(DataString);
    sprintf(str,"%c",26);
    answer = Sim80x_SendAtCommand(str,2000,2,"\r\nSEND OK\r\n","\r\nERROR\r\n");
    if(answer == 1) return true;
  }
  Sim80x.GPRS.SendStatus = GPRSSendData_SendFail;
  return false;
}
//#################################################################################################################
bool  GPRS_SendRaw(uint8_t *Data, uint16_t len)
{
  uint8_t answer;
  char str[20];
  Sim80x.GPRS.SendStatus = GPRSSendData_SendInProgress;
  snprintf(str,sizeof(str),"AT+CIPSEND=%d\r\n",len);
  answer = Sim80x_SendAtCommand(str,1500,2,"\r\r\n> ","\r\nERROR\r\n");
  if(answer == 1)
  {
    Sim80x_SendRaw(Data, len);
    osDelay(30);
    return true;
//    str[0] = 0;
//    answer = Sim80x_SendAtCommand(str,2000,2,"\r\nSEND OK\r\n","\r\nERROR\r\n");
//    if(answer == 1) return true;
  }
  Sim80x.GPRS.SendStatus = GPRSSendData_SendFail;
  return false;
}
//####################################################################################################
/*
bool  GPRS_HttpGet(char *URL)
{
  uint16_t timeout;
  uint8_t answer;
  char str[100];
  answer = Sim80x_SendAtCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r\n",1000,1,"\r\nOK\r\n");
  if(answer!=1)
    goto Error;
  answer = Sim80x_SendAtCommand("AT+SAPBR=1,1\r\n",10000,1,"\r\nOK\r\n");
  if(answer!=1)
    goto Error;
  answer = Sim80x_SendAtCommand("AT+HTTPINIT\r\n",1000,1,"\r\nOK\r\n");
  if(answer!=1)
    goto Error;
  answer = Sim80x_SendAtCommand("AT+HTTPPARA=\"CID\",1\r\n",1000,1,"\r\nOK\r\n");
  if(answer!=1)
    goto Error;
  
  snprintf(str,sizeof(str),"AT+HTTPPARA=\"URL\",\"%s\"\r\n",URL);
  answer = Sim80x_SendAtCommand(str,10000,2,"\r\nOK\r\n","\r\n+CME ERROR");
  if(answer!=1)
    goto Error;
  
  Sim80x.GPRS.HttpAction.ResultCode=0;
  answer = Sim80x_SendAtCommand("AT+HTTPACTION=0\r\n",1000,1,"\r\nOK\r\n");
  if(answer!=1)
    goto Error;
  timeout=0;
  while(Sim80x.GPRS.HttpAction.ResultCode==0)
  {
    osDelay(1000); 
    timeout++;  
    if(timeout == 120)
      goto Error;    
  }
  if(Sim80x.GPRS.HttpAction.ResultCode==200)
  {
    Sim80x.GPRS.HttpAction.TransferStartAddress=0;    
    for(uint32_t start=0 ;start<Sim80x.GPRS.HttpAction.DataLen ; start+=256 )
    {
      Sim80x.GPRS.HttpAction.CopyToBuffer=0;
      sprintf(str,"AT+HTTPREAD=%d,%d\r\n",(unsigned int)start,256);
      Sim80x_SendString(str);
      timeout=0;
      while(Sim80x.GPRS.HttpAction.CopyToBuffer==0)
      {
        osDelay(10);
        timeout++;
        if(timeout == 100)
          goto Error;
      } 
      Sim80x.GPRS.HttpAction.TransferStartAddress =  start;
      GPRS_UserHttpGetAnswer(Sim80x.GPRS.HttpAction.Data,Sim80x.GPRS.HttpAction.TransferStartAddress,Sim80x.GPRS.HttpAction.TransferDataLen);        
    }    
    answer = Sim80x_SendAtCommand("AT+HTTPTERM\r\n",1000,1,"\r\nOK\r\n");
    answer = Sim80x_SendAtCommand("AT+SAPBR=0,1\r\n",1000,1,"\r\nOK\r\n");
    #if (_SIM80X_DEBUG==1)
    printf("\r\nGPRS_HttpGet(%s) ---> OK\r\n",URL);
    #endif 
    return true;
  }
  Error:
  answer = Sim80x_SendAtCommand("AT+HTTPTERM\r\n",1000,1,"\r\nOK\r\n");
  answer = Sim80x_SendAtCommand("AT+SAPBR=0,1\r\n",1000,1,"\r\nOK\r\n");
  #if (_SIM80X_DEBUG==1)
  printf("\r\nGPRS_HttpGet(%s) ---> ERROR\r\n",URL);
  #endif 
  return false;  
}
*/
//####################################################################################################
#endif
