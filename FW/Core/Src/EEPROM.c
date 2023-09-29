
////////////////////////////////////////////////////////////////////////////////////////////////////
// SAVE / READ config into FLASH pages 126 and 127 (EEPROM emulation)
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "EEPROM.h"

uint16_t Crc16_up(uint16_t crc, uint8_t data)			// liczenie CRC
{
	uint8_t x = crc >> 8 ^ data;
	x ^= x>>4;
	crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
	return crc;
}

void Calc_config_crc(void)		// aktualizacja CRC struktury config
{
	config.checksum = 0xFFFF;
	uint8_t *p = (uint8_t*)&config;
	//printf("calc config crc, size of config: %i \r\n",sizeof(config));
	//for(int i=0; i<sizeof(config)-2; i++) config.checksum = Crc16_up(config.checksum, *p++);

	 uint16_t chk = 0xFFFF;
	    for(int i=0; i<sizeof(config)-2; i++) chk = Crc16_up(chk, *p++);
	    //printf("calc config crc: 0x%X \r\n",chk);
	    config.checksum=chk;

}

#define ADDR_FLASH_PAGE_0   ((uint32_t)0x08000000) /* Base @ of Page 0, 2 Kbytes */
#define FLASH_PAGE_ADDRESS 	(ADDR_FLASH_PAGE_0 | (FLASH_PAGE_SIZE * USE_FLASH_PAGE))

uint8_t Flash_write_block(uint32_t blk, uint8_t *buff, uint32_t len)
{
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PAGEError = 0;
	//WDR();
	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks       = FLASH_BANK_1;
	EraseInitStruct.Page        = blk;                                  // 2kb page
	EraseInitStruct.NbPages     = 1;
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)		// kasuj cala strone
	{
		HAL_FLASH_Lock();			// blad kasowania
		printf("Erase error at %u\r\n", (int)blk);
		return 0;
	}
    blk *= FLASH_PAGE_SIZE;
    blk += ADDR_FLASH_PAGE_0;
	uint64_t data64;
    for(uint32_t i=0; i<len/8 + 1; i++)          // zapisuj po 8 bajtow, o 8 bajtow wiecej niz trzeba
    {
        data64 = *(uint64_t*)buff;
        buff += 8;
        //printf("WRITE FLASH ADDR 0x%X \r\n",blk);
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 8*i + blk, data64) != HAL_OK)    // zapisz 8 bajtow do flasha
        {
            HAL_FLASH_Lock();
           // HAL_IWDG_Refresh(&hiwdg);
            printf("Programming error at %X\r\n",(unsigned int)(8*i + blk));
            return 1;                               // blad
        }
    }
	HAL_FLASH_Lock();
	//WDR();
	return 0;										// OK
}

void Flash_read(uint32_t adr, uint8_t *data, uint32_t size)
{
    uint8_t *psrc = (uint8_t*)(ADDR_FLASH_PAGE_0 + adr);
    for(uint32_t i=0; i<size; ++i) data[i] = *psrc++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#define USE_FLASH_PAGE      126

uint8_t FLASH_write_block(uint32_t adr, uint8_t *buff, uint32_t len)
{
    uint8_t err;
    adr &= 1;                   // adr = 0 -> page 62, adr = 1 -> page 63
    adr += USE_FLASH_PAGE;      // adr = page number
    for(int i=0; i<4; ++i)
    {
        err =  Flash_write_block(adr, buff, len);
        if(err == 0) break;
    }
    return err;
}

void FLASH_read_block(uint32_t adr, uint8_t *data, uint32_t size)
{
    adr &= 1;                   // adr = 0 -> page 62, adr = 1 -> page 63
    adr *= FLASH_PAGE_SIZE;     // adr = page number
    adr += (FLASH_PAGE_SIZE * USE_FLASH_PAGE);
    //printf("READ FLASH ADDR 0x%X \r\n",adr);
    Flash_read(adr, data, size);
}

uint8_t EEPROM_Save_config(void)
{
    uint8_t err = 0;
    uint16_t oldcrc = config.checksum;
    Calc_config_crc();
    if(config.checksum != oldcrc)
    {
        uint8_t *p = (uint8_t*)&config;
        err  = FLASH_write_block(0, p, sizeof(config));     // zapisz do 1 kopii
        err |= FLASH_write_block(1, p, sizeof(config));     // zapisz do 2 kopii
        err++;
    }
    return err;         // 0=NotNeed, 1=OK, 2=blad
}

void EEPROM_Load_defaults(void)
{
	memset((uint8_t*)&config, 0, sizeof(config));

	config.version = CONFIG_VERSION;
	config.bat_scale = 0.0505f;
	config.batt_alarm = BATT_ALARM_VOLTAGE;  // definicja w main.h
	config.TMP117_t_offset=0;
	config.SHT3_t_offset=0;
	config.SHT3_h_offset=0;
	config.MS8607_t_offset=0;
	config.MS8607_h_offset=0;
	config.MS8607_p_offset=0;
	config.BME280_t_offset=0;
	config.BME280_h_offset=0;
	config.BME280_p_offset=0;
	config.DPS368_t_offset=0;
	config.DPS368_p_offset=0;

	config.TMP117_config=0;
	config.SHT3_config=0;
	config.DPS368_config=0;
	config.MS8607_config=0;


	Calc_config_crc();

}

uint8_t EEPROM_Load_config(void)
{
    uint32_t eepok = 0;
    uint32_t len = sizeof(config);
    uint8_t *p = (uint8_t*)&config;
    FLASH_read_block(0, p, len);                                // odczyt pierwszej kopii
    //printf("load config crc: size of config: %i",sizeof(config));
    uint16_t chk = 0xFFFF;
    for(int i=0; i<len-2; i++) chk = Crc16_up(chk, *p++);
    //printf("read crc1: 0x%X \r\n",chk);
    if(chk == config.checksum) eepok |= 1;                      // zaznacz ze jest ok

    p = (uint8_t*)&config;
    FLASH_read_block(1, p, len);                                // odczyt drugiej kopii
    chk = 0xFFFF;
    for(int i=0; i<len-2; i++) chk = Crc16_up(chk, *p++);
    //printf("read crc2: 0x%X \r\n",chk);
    //printf("curr crc: 0x%X \r\n",config.checksum);
    if(chk == config.checksum) eepok |= 2;                      // zaznacz ze jest ok
    //printf("config chk: %X   eepok:%i \r\n", config.checksum, eepok);
    if(config.version != CONFIG_VERSION) eepok = 0;             // zmiana struktury -> laduj defaulty

    p = (uint8_t*)&config;
    switch(eepok)
    {
        case 0:                                                 // obie kopie zwalone
            EEPROM_Load_defaults();
            printf("config error, restoring to defaults \r\n");
            eepok = EEPROM_Save_config();

            if(eepok>1) return 3; else return 2;                // 2=zaladowano defaulty, 3=blad zapisu flash
            break;
        case 1:                                                 // 1 ok, 2 zwalona
            FLASH_read_block(0, p, len);
            printf("config bank 2 error,copying bank 1 to bank 2 \r\n");
            eepok = FLASH_write_block(1, p, len);               // przepisz do 2 kopii
            if(eepok) return 3;
            break;
        case 2:                                                 // 2 ok, 1 zwalona
            eepok = FLASH_write_block(0, p, len);               // przepisz do 1 kopii
            printf("config bank 1 error,copying bank 2 to bank 1 \r\n");
            if(eepok) return 3;
            break;
    }
    return 0;                                                   // config OK
}


void EEPROM_Print_config(void)
{
	printf("================================= \r\n");
	printf("Config version: %i \r\n", config.version);
	printf("Battery scale: %f \r\n", config.bat_scale);
	printf("Low Batt alarm: %i \r\n", config.batt_alarm);
	printf("TMP117 temp offset: %f \r\n", config.TMP117_t_offset);
	printf("SHT3 temp offset: %f \r\n", config.SHT3_t_offset);
	printf("SHT3 hum offset: %f \r\n", config.SHT3_h_offset);
	printf("MS8607 temp offset: %f \r\n", config.MS8607_t_offset);
	printf("MS8607 hum offset: %f  \r\n", config.MS8607_h_offset);
	printf("MS8607 press ofset: %f \r\n", config.MS8607_p_offset);
	printf("BME280 temp offset: %f \r\n", config.BME280_t_offset);
	printf("BME280 hum offset: %f \r\n", config.BME280_h_offset);
	printf("BME280 press ofset: %f \r\n", config.BME280_p_offset);
	printf("DPS368 temp offset: %f \r\n", config.DPS368_t_offset);
	printf("DPS368 press ofset: %f \r\n \r\n", config.DPS368_p_offset);

	printf("TMP117 config data: %i \r\n", 	config.TMP117_config);
	printf("SHT3 config data: %i \r\n", config.SHT3_config);
	printf("DPS368 config data: %i \r\n", config.DPS368_config);
	printf("MS8607 config data: %i \r\n", config.MS8607_config);
	printf("CHECKSUM: 0x%4X \r\n", config.checksum);
	printf("================================= \r\n");




}