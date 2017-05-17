//======================================================================================================================
//  File:   HwCheck.c
//
//  Classification:
//      Confidential
//
//  Legal:
//      Copyright (C) Siemens Industry Software B.V.  All Rights Reserved,
//      Druivenstraat 47, 4816 KB Breda, The Netherlands
//
//  Author:     
//      Olaf van den Berg
//
//  Created:    
//      August 04, 2016
//
//  Description:
//      
//  
//======================================================================================================================
#define _HWCHECK_C_

//======================================================================================================================
//  Includes
//======================================================================================================================
// Library

// Bsp
#include "xil_io.h"
#include "xil_assert.h"
#include "xstatus.h"

// Kernel
#include "FreeRTOS.h"
#include "semphr.h"

//  Application Common
#include "ModuleCommonDefs.h"
#include "MsgService.h"

// Application
#include "IntrCtrl.h"
#include "CommonDefs.h"
#include "LibDefs.h"
#include "HwCheck.h"
#include "Debug.h"

#undef DBG
#define DBG DBG_OFF
//======================================================================================================================
//  Definitions: Private Macros / Definitions
//  Description:
//      DEFINE  -   Explanation of DEFINE
//======================================================================================================================

//======================================================================================================================
//  Types
//======================================================================================================================

//======================================================================================================================
//  Private function prototypes (static)
//======================================================================================================================

//======================================================================================================================
//  Variables: Private data
//      var_name    -   explanation
//======================================================================================================================
static u32 HwCheck[                  F_HW_CHECK_WORDS_PER_CH*MAXCHANNELS];
static u32 HwCheckOutput[MAXOUTPUTS][F_HW_CHECK_WORDS_PER_CH*MAXCHANNELS];

static u32 send_hwchanged = 0;
static xSemaphoreHandle mutex;

//======================================================================================================================
//  Private functions (static)
//======================================================================================================================
static void HwCheckClrBit(t_HwCheckOutput output, u32 channel, u32 word, u32 bit)
{
    if (HwCheck[channel*F_HW_CHECK_WORDS_PER_CH+word] & ((u32)1 << bit))
    {
        HwCheckOutput[output][channel*F_HW_CHECK_WORDS_PER_CH+word] |= ((u32)1 << bit);
    }
    else
    {
        HwCheckOutput[output][channel*F_HW_CHECK_WORDS_PER_CH+word] &= ~((u32)1 << bit);
    }
}

static void HwCheckClrWord(t_HwCheckOutput output, u32 channel, u32 word)
{
    HwCheckOutput[output][channel*F_HW_CHECK_WORDS_PER_CH+word] = HwCheck[channel*F_HW_CHECK_WORDS_PER_CH+word];
}

//======================================================================================================================
//  Variables: Public Data
//      var_name    -   explanation
//======================================================================================================================

//======================================================================================================================
//  Public functions
//======================================================================================================================
void HwCheckInit(void)
{
    u32 i,j;

    // Create mutex for locking this resource
    mutex = xSemaphoreCreateMutex();
    Xil_AssertVoid(mutex);

    for(i=0;i<(F_HW_CHECK_WORDS_PER_CH*MAXCHANNELS);i++)
    {
        HwCheck[i]              = 0;
        for(j=0;j<MAXOUTPUTS;j++)
        {
            HwCheckOutput[j][i] = 0;
        }
    }
}

void HwCheckSetBit(u32 channel, u32 word, u32 bit, u8 value)
{
    u32 i;

    // Get the mutex
    xSemaphoreTake(mutex, portMAX_DELAY);
        // Disable the InChStatus to prevent this IRQ to access the HwCheck through HwCheckSetBitFromIsr()
        IntrCtrlDisableIrq(XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR);

        if (word == 0)
        {
            if (((value<<bit) & HWCHANGED_MASK) && ((HwCheck[channel*F_HW_CHECK_WORDS_PER_CH]&HWCHANGED_MASK)==0))
            {
                send_hwchanged = 1;
            }
        }

        HwCheck[channel*F_HW_CHECK_WORDS_PER_CH+word] |= (  ((value)?1:0) << bit );
        HwCheck[channel*F_HW_CHECK_WORDS_PER_CH+word] &= (~(((value)?0:1) << bit));
        for(i=0; i<MAXOUTPUTS; i++)
        {
            HwCheckOutput[i][channel*F_HW_CHECK_WORDS_PER_CH+word] |= (  ((value)?1:0) << bit );
        }
        // Enable the InChStatus irq
        IntrCtrlEnableIrq(XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR);
    // Release the mutex
    xSemaphoreGive(mutex);
}

//----------------------------------------------------------------------------------------------------------------------
//  Function: HwCheckSetBitFromIsr
//
//  Description:
//      Set HwCheck bits from ISR.  We cannot take the mutex from an ISR, so we disable the IRQ
//      when the mutex is taken in the other Set/Get functions
//
//  Parameters:
//      u32 channel - ADC channel to set bit on
//      u32 word    - HwCheck word to set the bit on (2 words at the moment)
//      u32 bit     - Bit nr to set
//      u32 value   - 0 or 1 to set
//
//  Returns: -
//----------------------------------------------------------------------------------------------------------------------
void HwCheckSetBitFromIsr(u32 channel, u32 word, u32 bit, u8 value)
{
    u32 i;

    HwCheck[channel*F_HW_CHECK_WORDS_PER_CH+word] |= (  ((value)?1:0) << bit );
    HwCheck[channel*F_HW_CHECK_WORDS_PER_CH+word] &= (~(((value)?0:1) << bit));
    for(i=0; i<MAXOUTPUTS; i++)
    {
        HwCheckOutput[i][channel*F_HW_CHECK_WORDS_PER_CH+word] |= (  ((value)?1:0) << bit );
    }
}

u8   HwCheckGetBit(t_HwCheckOutput output, u32 channel, u32 word, u32 bit)
{
    // Get the mutex
    xSemaphoreTake(mutex, portMAX_DELAY);
        // Disable the InChStatus to prevent this IRQ to access the HwCheck through HwCheckSetBitFromIsr()
        IntrCtrlDisableIrq(XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR);
        u8 value = (HwCheckOutput[output][channel*F_HW_CHECK_WORDS_PER_CH+word] & (1 << bit))?1:0;
        HwCheckClrBit(output, channel, word, bit);
        // Enable the InChStatus irq
        IntrCtrlEnableIrq(XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR);
    // Release the mutex
    xSemaphoreGive(mutex);

    return value;
}

void HwCheckSetWord(u32 channel, u32 word, u32 mask, u32 value)
{
    u32 i;

    // Get the mutex
    xSemaphoreTake(mutex, portMAX_DELAY);

        // Disable the InChStatus to prevent this IRQ to access the HwCheck through HwCheckSetBitFromIsr()
        IntrCtrlDisableIrq(XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR);

        if( word == 0)
        {
            if ((value & mask & HWCHANGED_MASK) && ((HwCheck[channel*F_HW_CHECK_WORDS_PER_CH]&HWCHANGED_MASK)==0))
            {
                DBG(("Set HwChanged To be sent!\r\n"));
                send_hwchanged = 1;
            }
        }

        HwCheck[channel*F_HW_CHECK_WORDS_PER_CH+word] |= (value & mask);
        HwCheck[channel*F_HW_CHECK_WORDS_PER_CH+word] &= (value | ~mask);

        DBG(("hwcheck %x\r\n",HwCheck[channel*F_HW_CHECK_WORDS_PER_CH+word]));
        for(i=0; i<MAXOUTPUTS; i++)
        {
            HwCheckOutput[i][channel*F_HW_CHECK_WORDS_PER_CH+word] |= (value & mask);
        }
        // Enable the InChStatus irq
        IntrCtrlEnableIrq(XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR);
    // Release the mutex
    xSemaphoreGive(mutex);
}

u32  HwCheckGetWord(t_HwCheckOutput output, u32 channel, u32 word)
{
    // Get the mutex
    xSemaphoreTake(mutex, portMAX_DELAY);
        // Disable the InChStatus to prevent this IRQ to access the HwCheck through HwCheckSetBitFromIsr()
        IntrCtrlDisableIrq(XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR);
        u32 value = HwCheckOutput[output][channel*F_HW_CHECK_WORDS_PER_CH+word];
        HwCheckClrWord(output,channel,word);
        // Enable the InChStatus irq
        IntrCtrlEnableIrq(XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR);
    // Release the mutex
    xSemaphoreGive(mutex);
    return value;
}

//----------------------------------------------------------------------------------------------------------------------
//  Function: HwCheckHardwareChanged
//
//  Description:
//      Check if we have to send a hardware changed message
//
//  Parameters: -
//  Returns:    -
//----------------------------------------------------------------------------------------------------------------------
void HwCheckHardwareChanged(void)
{
    if (send_hwchanged)
    {
        send_hwchanged = 0;

        t_msg OutboxMsg;
        OutboxMsg.src.id_main       = CONDITF;
        OutboxMsg.src.id_sub        = MESSAGE;
        OutboxMsg.dest              = HOSTPORT_MASTER;
        OutboxMsg.data.byte_count   = 1;
        OutboxMsg.data.field.value  = F_ERR_HARDWARE_CHANGED_EVENT;
        u32 result = MsgServicePostMsg(FALSE, &OutboxMsg);
        if(XST_FAILURE == result) xil_printf("MsgServicePostMsg() failed\r\n", result);
    }
}
//========================================================EOF===========================================================
