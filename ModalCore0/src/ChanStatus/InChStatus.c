//======================================================================================================================
//  File:   InChStatus.c
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
//      $[0[]0]
//  
//======================================================================================================================
#define _INCHSTATUS_C_

//======================================================================================================================
//  Includes
//======================================================================================================================
// Library

// Bsp
#include "xparameters.h"
#include "xil_io.h"

// Kernel

// Application
#include "ModuleCommonDefs.h"
#include "CommonDefs.h"
#include "HwCheck.h"
#include "InChStatus.h"
#include "IntrCtrl.h"
#include "InputChannel.h"

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

static u32 adcstatus_prev = 0;

//======================================================================================================================
//  Private functions (static)
//======================================================================================================================

//----------------------------------------------------------------------------------------------------------------------
//  Function: InChStatusIsr
//
//  Description:
//      ADC status report
//
//      7..0    FilterSaturated bit for each channel, read-only
//      15..8   ChipError bit for each channel, read-only
//      16      DataOverflow bit, read-only
//
//      FilterSaturated:
//      The filter saturated bit indicates that the filter output inside the Ad7768 ADC is clipping at either positive
//      or negative full scale.
//      When the bit is read as 1, the channel is in saturation, when it is 0 it is within normal bounds.
//      This will be reported in the HwCheck bit F_HW_CHK_LW_ADC_FILTER_SATURATED
//
//      ChipError:
//      The chip error bit indicates that a serious error has occurred. If this bit is set, a reset is required to
//      clear this bit. This bit indicates that the external clock is not detected, a memory map bit has unexpectedly
//      changed state, or an internal CRC error has been detected.
//      In the case where an external clock is not detected, the conversion results are output as all zeroes regardless
//      of the analog input voltages applied to the ADC channels.
//
//      DataOverflow
//      When a new sample was clocked in, while the previous sample hasn’t been read yet, this field is set to 1.
//      When it is set to 0, it indicates normal operation.
//
//      Note:
//      The flags mentioned above are OR’ed together over a period defined by the number of samples set in the BlockCount
//      register (INPUTCHANNEL_BLOCKCOUNT, also used for absmax determination
//      in HLS).
//      Whenever the defined number of samples has passed, the values are updated and an interrupt is triggered
//      This is normally set to 100 msec.
//
//      14-09-2016: Only report changes and direct to the HwCheck data through a special function HwCheckSetBitFromIsr()
//                  Do not use the CondItfTask to report, because this can get blocked for a long time when sequencing
//
//  Parameters: -
//  Returns: -
//----------------------------------------------------------------------------------------------------------------------
static void InChStatusIsr(void *callbackref)
{
    u32 i, adcstatus;
    adcstatus = Xil_In32(INPUTCHANNEL_ADC_STATUS);

    if (adcstatus_prev != adcstatus)
    {
        for(i=0; i<MAXCHANNELS; i++)
        {
            if ((adcstatus_prev&(1<<i)) != (adcstatus&(1<<i)))
            {
                HwCheckSetBitFromIsr(i,0,F_HW_CHK_LW_ADC_FILTER_SATURATED,(adcstatus>>i) & 0x00000001);
            }
        }
        adcstatus_prev = adcstatus;
    }

    if((adcstatus >> 8) & 0x1FF)
    {
        xil_printf("ADCERROR: %x", adcstatus);
    }
}


//======================================================================================================================
//  Variables: Public Data
//      var_name    -   explanation
//======================================================================================================================

//======================================================================================================================
//  Public functions
//======================================================================================================================
void InChStatusInit(void)
{
    u32 adcstatus = 0;

    IntrCtrlInstallHandler((fpIsr)InChStatusIsr, XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR, IRQ_DEFAULT_PRIORITY,
                           IRQ_TRIG_LEVEL_HIGH, NULL);
    // clear any pending interrupt before enabling interrupt
    adcstatus = Xil_In32(INPUTCHANNEL_ADC_STATUS);
    adcstatus = adcstatus;  // Remove warning
    adcstatus_prev = 0;
    IntrCtrlEnableIrq(XPAR_FABRIC_SYSTEM_IINPCHNIRQ_INTR);
}
//========================================================EOF===========================================================
