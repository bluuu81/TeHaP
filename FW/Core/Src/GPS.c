/**
******************************************************************************

* GPS from SIM868 driver
* Author: Romuald Bialy

******************************************************************************
**/

#include "Sim80xDrv.h"


bool GPS_TurnOnGLONASS()
{
	uint8_t answer = Sim80x_SendAtCommand("AT+CGNSCMD=0,\"$PMTK353,1,1,0,0,0*2B\"\r\n", 300, 2,"\r\nOK\r\n","\r\nERROR\r\n");
	if(answer == 2)
	{
		#if (_SIM80X_DEBUG==1)
		printf("\r\nGPS_TurnOnGLONASS() ---> ERROR\r\n");
		#endif
		return false;
	}
	return true;
}

bool GPS_TurnOnWAAS()
{
	uint8_t answer = Sim80x_SendAtCommand("AT+CGNSCMD=0,\"$PMTK301,2*2E\"\r\n", 300, 2,"\r\nOK\r\n","\r\nERROR\r\n");
	if(answer == 2)
	{
		#if (_SIM80X_DEBUG==1)
		printf("\r\nGPS_TurnOnWAAS() ---> ERROR\r\n");
		#endif
		return false;
	}
	return true;
}

bool GPS_TurnOnSBAS()
{
	uint8_t answer = Sim80x_SendAtCommand("AT+CGNSCMD=0,\"$PMTK313,1*2E\"\r\n", 300, 2,"\r\nOK\r\n","\r\nERROR\r\n");
	if(answer == 2)
	{
		#if (_SIM80X_DEBUG==1)
		printf("\r\nGPS_TurnOnSBAS() ---> ERROR\r\n");
		#endif
		return false;
	}
	return true;
}

bool GPS_TurnOnFitnessMode()
{
	uint8_t answer = Sim80x_SendAtCommand("AT+CGNSCMD=0,\"$PMTK886,1*29\"\r\n", 300, 2,"\r\nOK\r\n","\r\nERROR\r\n");
	if(answer == 2)
	{
		#if (_SIM80X_DEBUG==1)
		printf("\r\nGPS_TurnOnFitnessMode() ---> ERROR\r\n");
		#endif
		return false;
	}
	return true;
}

bool GPS_TurnOnURC()
{
	uint8_t answer = Sim80x_SendAtCommand("AT+CGNSURC=1\r\n",300,2,"\r\nOK\r\n","\r\nERROR\r\n");
	if(answer == 2)
	{
		#if (_SIM80X_DEBUG==1)
		printf("\r\nGPS_TurnOnURC() ---> ERROR\r\n");
		#endif
		return false;
	}
	return true;
}

bool GPS_SetBaud()
{
    const char *brtab[6] = {"115200","9600","19200","38400","57600","115200"};
    uint8_t tout;
    uint8_t baudrate = 255;        // default 115200 baud
    char str[20];
    while(1)
    {
        tout = 0;
        Sim80x.GPS.RunStatus = 0;
        uint8_t answer = Sim80x_SendAtCommand("AT+CGNSVER\r\n",500,2,"\r\nOK\r\n","\r\nERROR\r\n");  // zapytaj o wersje GPS
        if(answer == 1)
        {
            while(Sim80x.GPS.RunStatus == 0)   // czekaj na odpowiedz z GPS z wersja firmware.
            {                                   // odpowiedz przychodzi tylko jesli baudrate jest prawidlowe
                osDelay(8);
                if(++tout > 100) break;         // max 800ms na odpowiedz, normalnie prsychodzi po ok 320ms
            }
            if(Sim80x.GPS.RunStatus == 0)      // brak odpowiedzi lub pakietow URC, testuj kolejne baudrate
            {
                if(++baudrate > 5)  goto reterr;
                #if (_SIM80X_DEBUG==1)
                printf("GPS_Init():Checking %s baud...\r\n", brtab[baudrate]);
                #endif
                snprintf(str, sizeof(str), "AT+CGNSIPR=%s\r\n", brtab[baudrate]);
                answer = Sim80x_SendAtCommand(str, 500, 2, "\r\nOK\r\n","\r\nERROR\r\n");   // zmien baudrate
                if(answer == 2)  goto reterr;                                               // blad zmiany baud
                continue;
            }
            else                                // przyszla odpowiedz -> baudrate prawidlowe
            {
                #if (_SIM80X_DEBUG==1)
            	if(baudrate == 255) baudrate = 0;
                printf("\r\nGPS_Init():GPS Baudrate %s\r\n", brtab[baudrate]);
                #endif
                return true;
            }
        }
reterr:
        #if (_SIM80X_DEBUG==1)
        printf("\r\nGPS_Init():SetBaudRate ---> ERROR\r\n");
        #endif
        return false;
    }
}


bool  GPS_SetPower(bool TurnOn)
{
	uint8_t answer;
	if(!Sim80x.Status.Power)
	{
		#if (_SIM80X_DEBUG==1)
		printf("\r\nGPS_Init():Check power ---> ERROR\r\n");
		#endif
		return false;
	}
	if(TurnOn)
	{
	  // wylaczenie slow clk bo przeszkadza w raportach URC
	  Sim80x.Status.LowPowerMode = 0;
	  osDelay(100);
	  // zalaczenie zasilania GPS
	  answer = Sim80x_SendAtCommand("AT+CGNSPWR=1\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");
	  if(answer == 1)
	  {
		memset(&Sim80x.GPS, 0, sizeof(GPS_t));
		osDelay(700);
		if(!GPS_TurnOnURC()) goto OnErr;				// wlaczenie raportow URC
		osDelay(700);
	    if(!GPS_SetBaud()) goto OnErr;					// ustawienie baudrate dla portu 2 simcomma
		osDelay(100);
		// 600ms LED blink
		answer = Sim80x_SendAtCommand("AT+CGNSCMD=0,\"$PMTK285,4,600*3F\"\r\n", 500, 1,"\r\nOK\r\n");
		if(answer != 1) {
			osDelay(100);
			Sim80x_SendAtCommand("AT+CGNSCMD=0,\"$PMTK285,4,600*3F\"\r\n", 500, 1,"\r\nOK\r\n");
		}
		osDelay(20);
		GPS_TurnOnGLONASS();							// wlaczenie glonass'a
		osDelay(20);
		GPS_TurnOnWAAS();								// wlaczenie WAAS
		osDelay(20);
		GPS_TurnOnSBAS();								// wlaczenie SBAS
		osDelay(20);
		GPS_TurnOnFitnessMode();						// wlaczenie Fittness Mode (best acurracy)
		osDelay(50);
		#if (_SIM80X_DEBUG==1)
		printf("\r\nGPS_PowerOn() ---> OK\r\n");
		#endif
		return true;
	  }
	  else
	  {
OnErr:
		printf("\r\nGPS_PowerOn() ---> ERROR\r\n");
		if(Sim80x.GPS.RunStatus)
			Sim80x_SendAtCommand("AT+CGNSPWR=0\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");
		memset(&Sim80x.GPS, 0, sizeof(GPS_t));
		return false;
	  }
	}
	else
	{
	  if(Sim80x.GPS.RunStatus)
	  {
		  // wylacz zasilanie GPS
		  answer = Sim80x_SendAtCommand("AT+CGNSPWR=0\r\n",1000,2,"\r\nOK\r\n","\r\nERROR\r\n");
		  osDelay(50);
	  } else answer = 1;
	  // wlacz spowrotem slow clk (low power)
	  if(Sim80x.GPRS.Connection == GPRSConnection_Idle) Sim80x.Status.LowPowerMode = 1;
	  if(answer == 1)
	  {
		#if (_SIM80X_DEBUG==1)
		printf("\r\nGPS_PowerOff() ---> OK\r\n");
		#endif
		Sim80x.GPS.RunStatus = 0;
		Sim80x.GPS.SatInUse = 0;
		Sim80x.GPS.Fix = 0;
		return true;
	  }
	  else
	  {
		printf("\r\nGPS_PowerOff() ---> ERROR\r\n");
		Sim80x.GPS.RunStatus = 0;
		Sim80x.GPS.SatInUse = 0;
		Sim80x.GPS.Fix = 0;
		return false;
	  }
	}
}

// ===============================================================================================

