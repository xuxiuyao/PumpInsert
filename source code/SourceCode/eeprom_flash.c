/*****************************************************************************
 *   eeprom.c:  EEPROM module file for NXP LPC17xx Family Microprocessors
 *
 *   Copyright(C) 2009, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2009.05.25  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#define __FLASH__
#include "include.h"
/*
#include "lpc17xx.h"
#include <stdio.h>				   	

#include "type.h"
#include "struct.h"
#include "define.h"
#include "spi_flash.h"
#include "eeprom_flash.h"
#include "rtc.h"
#if(_PANEL_VERSION == _RX_V01)
#include "uifunc.h"
#elif(_PANEL_VERSION == _RX_V02)
#include "uifunc_v02.h"
#endif
#include "ui.h"
#include "power.h"
#include "eeprom_default.h"
#include "msd.h"
#include "main.h"
#include "systick.h"
#include "uifunc_v03.h"
*/

/*****************************************************************************
** Function name:		Reset Funtion
**
** Descriptions:		Reset setting
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
//=================================
//ÿ��sector ����Ϊ��С��λΪ32BYTE ��С����(0-2040)
//ÿ��sector �����һ��page ��ŵ�ǰ��С�����õ������index
//����flash �Ķ�д���ԣ������һ��page ��ÿһλ�����������index
//=================================


void CFlashSaveIndex(u8 Flash_num, u32 Page, u32 Index)
{
    int i=0;
    int j=0;
    
    if(Index == (2040-1))
    {
        CFlashClearFlashMoveBuff();
        SPIFlash_Read(Flash_num, flash_buf, (Page+254)*PAGE_SIZE, PAGE_SIZE);
        for(i=0;i<32;i++)
            g_u8MoveTable[i]=flash_buf[224+i];
        SPIFlash_EraseSector(Flash_num, Page*PAGE_SIZE);
        SPIFlash_WritePage(Flash_num, g_u8MoveTable, Page*PAGE_SIZE, PAGE_SIZE);
        Index = 1;
    }
    else
    {
        Index++;
    }
    CFlashClearFlashBuff();
    for(i=0;i<(Index/8);i++) //���һ��pageΪindex page�����Լ�1
    {
        for(j=0;j<8;j++)
        {
            flash_buf[255-i] = flash_buf[255-i] & (~(1<<j));
        }
    }

    for(j=0;j<(Index%8);j++)
    {
        flash_buf[255-i] = flash_buf[255-i] & (~(1<<j));
    }
    SPIFlash_WritePage(Flash_num, flash_buf, (Page+255)*PAGE_SIZE, PAGE_SIZE);
}
u32 CFlashLoadIndex(u8 Flash_num, u32 Page)
{
    u32 Index = 2039;
    int i=0;
    int j=0;
    
    SPIFlash_Read(Flash_num, flash_buf, (Page+255)*PAGE_SIZE, PAGE_SIZE);
    for(i=0;i<(0x100-1);i++) //���һ��pageΪindex page�����Լ�1
    {
        for(j=0;j<8;j++)
        {
            if(flash_buf[255-i] & (1<<j))
            {
                Index = (i*8)+j;
                return Index;
            }
        }
    }

    return Index;
}
void CFlashGetCell(u8 Flash_num, u32 Page)
{
    g_u32CellIndex = CFlashLoadIndex(Flash_num, Page);
    if(g_u32CellIndex == 0)
    {
        g_u8InitSector = 1;
    }
    else
    {
        g_u32PageLast = (g_u32CellIndex-1)/SAVE_NUM_OF_PAGE;
        g_u32PageNew= g_u32CellIndex/SAVE_NUM_OF_PAGE;
        g_u8CellLast = (g_u32CellIndex-1)%8;
        g_u8CellNew = g_u32CellIndex%8;
    }
}
void CFlashLoadCheckSum(void)
{
    int i;
    
    SPIFlash_Read(_FLASH_NUM0, flash_buf, PAGE_CKS*PAGE_SIZE, PAGE_SIZE);
    for(i=0;i<INDEX_CKS_NUM;i++)
        CheckSumData[i] = flash_buf[i];
}
void CFlashSaveCheckSum(void)
{
    int i;
    
    for(i=0;i<INDEX_CKS_NUM;i++)
        flash_buf[i] = CheckSumData[i];    
    SPIFlash_EraseSector(_FLASH_NUM0, SECTOR_CKS*SECTOR_SIZE);
    SPIFlash_WritePage(_FLASH_NUM0, flash_buf, PAGE_CKS*PAGE_SIZE, PAGE_SIZE);
}
void CFlashStartupCheck(void)
{
    CFlashCheck();
    CFlashLoadCheckSum();
    //CFlashRTCCheck();
    CFlashUICheck();
    CFlashTPCheck();
    CFlashSaveCheckSum();
    //g_uiBacklightOffCounter = BacklightOffTime[g_stUISetting.BacklightOff];
    //g_uiAutoPowerOffCounter = AutoPowerOffTime[g_stUISetting.AutoPowerOff];
    //g_uiPanelStatus = 1;
    //Delay10ms(100);
    return;
   
}
void CFlashCheck(void)
{
    
    SPIFlash_Read(_FLASH_NUM0, flash_buf, PAGE_SPI_FLASH_CKS*PAGE_SIZE, PAGE_SIZE);

    if(flash_buf[0]!= SPI_FLASH_CKS)
    {
        SPIFlash_EraseBulk(_FLASH_NUM0);
        g_8Flash0BulkErase = 1;
        CFlashClearFlashBuff();
        flash_buf[0] = SPI_FLASH_CKS;
        SPIFlash_WritePage(_FLASH_NUM0, flash_buf, PAGE_SPI_FLASH_CKS*PAGE_SIZE, PAGE_SIZE);
    }
}
void CFlashRTCCheck(void)
{
    if(CheckSumData[INDEX_RTC_CKS]!=RTC_CKS)
    {
        RTCSetTimeDefault();
        CheckSumData[INDEX_RTC_CKS] = RTC_CKS;
    }
    else
    {
        g_stCurrent_time = RTCGetTime();
    }
}
void CFlashUICheck(void)
{
    if(CheckSumData[INDEX_UI_CKS]!= UI_CKS)
    {
        CFlashLoadUISettingDefault();
        CheckSumData[INDEX_UI_CKS] = UI_CKS;
    }
    else
    {
        CFlashLoadUISetting();
    }
}
void CFlashTPCheck(void)
{
    if(CheckSumData[INDEX_TP_CKS]!= TP_CKS)
    {
        CFlashLoadTouchPointDefault();
        CheckSumData[INDEX_TP_CKS] = TP_CKS;
    }
    else
    {
        CFlashLoadTouchPoint();
    }
}
void CFlashClearFlashBuff(void)
{
    int j;

    for(j=0;j<PAGE_SIZE;j++)
        flash_buf[j] = 0xff;
}
void CFlashClearFlashMoveBuff(void)
{
    int j;

    for(j=0;j<PAGE_SIZE;j++)
        g_u8MoveTable[j] = 0xff;
}
void CEepromClearFlashIDBuff(void)
{
    int j;

    for(j=0;j<PAGE_SIZE;j++)
    {
        if(j<12)
            flash_buf[j] = 0x20;
        else
            flash_buf[j] = 0xff;
    }
}
void CFlashSaveUISetting(void)
{
    CFlashGetCell(_FLASH_NUM0, PAGE_UI_SETTING);
    if(g_u8CellNew == 0)
    {
        CFlashClearFlashBuff();
        flash_buf[0] = g_stUISetting.DTFormat;
        flash_buf[1] = g_stUISetting.BacklightOff;
        flash_buf[2] = g_stUISetting.AutoPowerOff;
        flash_buf[3] = g_stUISetting.AudioKey;
        flash_buf[4] = g_stUISetting.AudioTip;
        flash_buf[5] = g_stUISetting.PasswordOn;
        flash_buf[6] = g_stUISetting.Language;

    }
    else
    {
        SPIFlash_Read(_FLASH_NUM0, flash_buf, (PAGE_UI_SETTING+g_u32PageLast)*PAGE_SIZE, PAGE_SIZE);
        flash_buf[(CELL_SIZE*g_u8CellNew)+0] = g_stUISetting.DTFormat;
        flash_buf[(CELL_SIZE*g_u8CellNew)+1] = g_stUISetting.BacklightOff;
        flash_buf[(CELL_SIZE*g_u8CellNew)+2] = g_stUISetting.AutoPowerOff;
        flash_buf[(CELL_SIZE*g_u8CellNew)+3] = g_stUISetting.AudioKey;
        flash_buf[(CELL_SIZE*g_u8CellNew)+4] = g_stUISetting.AudioTip;
        flash_buf[(CELL_SIZE*g_u8CellNew)+5] = g_stUISetting.PasswordOn;
        flash_buf[(CELL_SIZE*g_u8CellNew)+6] = g_stUISetting.Language;

    }
    SPIFlash_WritePage(_FLASH_NUM0, flash_buf, (PAGE_UI_SETTING+g_u32PageNew)*PAGE_SIZE, PAGE_SIZE);
    CFlashSaveIndex(_FLASH_NUM0, PAGE_UI_SETTING, g_u32CellIndex);
}
void CFlashLoadUISetting(void)
{
    CFlashGetCell(_FLASH_NUM0, PAGE_UI_SETTING);
    
    SPIFlash_Read(_FLASH_NUM0, flash_buf, (PAGE_UI_SETTING+g_u32PageLast)*PAGE_SIZE, PAGE_SIZE);
    g_stUISetting.DTFormat = flash_buf[(CELL_SIZE*g_u8CellLast)+0];
    g_stUISetting.BacklightOff = flash_buf[(CELL_SIZE*g_u8CellLast)+1];
    g_stUISetting.AutoPowerOff = flash_buf[(CELL_SIZE*g_u8CellLast)+2];
    g_stUISetting.AudioKey = flash_buf[(CELL_SIZE*g_u8CellLast)+3];
    g_stUISetting.AudioTip = flash_buf[(CELL_SIZE*g_u8CellLast)+4];
    g_stUISetting.PasswordOn = flash_buf[(CELL_SIZE*g_u8CellLast)+5];
    g_stUISetting.Language = flash_buf[(CELL_SIZE*g_u8CellLast)+6];

}
void CFlashLoadUISettingDefault(void)
{
    g_stUISetting = tEEPROM_UISetting_DEFAULT;
    CFlashClearFlashBuff();
    flash_buf[0] = g_stUISetting.DTFormat;
    flash_buf[1] = g_stUISetting.BacklightOff;
    flash_buf[2] = g_stUISetting.AutoPowerOff;
    flash_buf[3] = g_stUISetting.AudioKey;
    flash_buf[4] = g_stUISetting.AudioTip;
    flash_buf[5] = g_stUISetting.PasswordOn;
    flash_buf[6] = g_stUISetting.Language;

    
    SPIFlash_EraseSector(_FLASH_NUM0, SECTOR_UI_SETTING*SECTOR_SIZE);
    SPIFlash_WritePage(_FLASH_NUM0, flash_buf, PAGE_UI_SETTING*PAGE_SIZE, PAGE_SIZE);
    CFlashSaveIndex(_FLASH_NUM0, PAGE_UI_SETTING, 0);
}
void CFlashSaveTouchPoint(void)
{
    CFlashGetCell(_FLASH_NUM0, PAGE_TP_VALUE);
    if(g_u8CellNew == 0)
    {
        CFlashClearFlashBuff();
        flash_buf[0] =(g_stTouchPoint.xMin>>8)&0x00ff;
        flash_buf[1] = g_stTouchPoint.xMin&0x00ff;
        flash_buf[2] =(g_stTouchPoint.xMax>>8)&0x00ff;
        flash_buf[3] = g_stTouchPoint.xMax&0x00ff;
        flash_buf[4] =(g_stTouchPoint.yMin>>8)&0x00ff;
        flash_buf[5] = g_stTouchPoint.yMin&0x00ff;
        flash_buf[6] =(g_stTouchPoint.yMax>>8)&0x00ff;
        flash_buf[7] = g_stTouchPoint.yMax&0x00ff;
    }
    else
    {
        SPIFlash_Read(_FLASH_NUM0, flash_buf, (PAGE_TP_VALUE+g_u32PageLast)*PAGE_SIZE, PAGE_SIZE);
        flash_buf[(CELL_SIZE*g_u8CellNew)+0] = (g_stTouchPoint.xMin>>8)&0x00ff;
        flash_buf[(CELL_SIZE*g_u8CellNew)+1] =  g_stTouchPoint.xMin&0x00ff;
        flash_buf[(CELL_SIZE*g_u8CellNew)+2] = (g_stTouchPoint.xMax>>8)&0x00ff;
        flash_buf[(CELL_SIZE*g_u8CellNew)+3] =  g_stTouchPoint.xMax&0x00ff;
        flash_buf[(CELL_SIZE*g_u8CellNew)+4] = (g_stTouchPoint.yMin>>8)&0x00ff;
        flash_buf[(CELL_SIZE*g_u8CellNew)+5] = g_stTouchPoint.yMin&0x00ff;
        flash_buf[(CELL_SIZE*g_u8CellNew)+6] = (g_stTouchPoint.yMax>>8)&0x00ff;
        flash_buf[(CELL_SIZE*g_u8CellNew)+7] =  g_stTouchPoint.yMax&0x00ff;

    }
    SPIFlash_WritePage(_FLASH_NUM0, flash_buf, (PAGE_TP_VALUE+g_u32PageNew)*PAGE_SIZE, PAGE_SIZE);
    CFlashSaveIndex(_FLASH_NUM0, PAGE_TP_VALUE, g_u32CellIndex);
}
void CFlashLoadTouchPoint(void)
{
    u16 Temp;
    
    CFlashGetCell(_FLASH_NUM0, PAGE_UI_SETTING);
    
    SPIFlash_Read(_FLASH_NUM0, flash_buf, (PAGE_TP_VALUE+g_u32PageLast)*PAGE_SIZE, PAGE_SIZE);
    Temp = flash_buf[(CELL_SIZE*g_u8CellLast)+0];
    g_stTouchPoint.xMin= (Temp<<8)|flash_buf[(CELL_SIZE*g_u8CellLast)+1];
    Temp = flash_buf[(CELL_SIZE*g_u8CellLast)+2];
    g_stTouchPoint.xMax= (Temp<<8)|flash_buf[(CELL_SIZE*g_u8CellLast)+3];
    Temp = flash_buf[(CELL_SIZE*g_u8CellLast)+4];
    g_stTouchPoint.yMin= (Temp<<8)|flash_buf[(CELL_SIZE*g_u8CellLast)+5];
    Temp = flash_buf[(CELL_SIZE*g_u8CellLast)+6];
    g_stTouchPoint.yMax= (Temp<<8)|flash_buf[(CELL_SIZE*g_u8CellLast)+7];

}
void CFlashLoadTouchPointDefault(void)
{
    g_stTouchPoint= tEEPROM_TouchPoint_DEFAULT;
    CFlashClearFlashBuff();
    flash_buf[0] =(g_stTouchPoint.xMin>>8)&0x00ff;
    flash_buf[1] = g_stTouchPoint.xMin&0x00ff;
    flash_buf[2] =(g_stTouchPoint.xMax>>8)&0x00ff;
    flash_buf[3] = g_stTouchPoint.xMax&0x00ff;
    flash_buf[4] =(g_stTouchPoint.yMin>>8)&0x00ff;
    flash_buf[5] = g_stTouchPoint.yMin&0x00ff;
    flash_buf[6] =(g_stTouchPoint.yMax>>8)&0x00ff;
    flash_buf[7] = g_stTouchPoint.yMax&0x00ff;


    
    SPIFlash_EraseSector(_FLASH_NUM0, SECTOR_TP_VALUE*SECTOR_SIZE);
    SPIFlash_WritePage(_FLASH_NUM0, flash_buf, PAGE_TP_VALUE*PAGE_SIZE, PAGE_SIZE);
    CFlashSaveIndex(_FLASH_NUM0, PAGE_TP_VALUE, 0);
}

void CFlashLoadDefault(void)
{
    CFlashLoadUISettingDefault();
}


//====================end of line==========================

