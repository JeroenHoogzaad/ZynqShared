//======================================================================================================================
//  File:   HwCheck.h
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
#ifndef _HWCHECK_H_
#define _HWCHECK_H_

//======================================================================================================================
// Includes
//======================================================================================================================
// Library

// Bsp
#include "xil_types.h"

// Kernel

// Application

//======================================================================================================================
//  Definitions: Public Macros / Definitions
//  Description:
//      NAME    -   description of NAME
//======================================================================================================================

//======================================================================================================================
// Types
//======================================================================================================================
typedef enum
{
    STATUSDATA = 0,
    HWCHANGED  = 1,
    VLED       = 2,
    HWCHECK_OUT= 3,
    MAXOUTPUTS = 4,
} t_HwCheckOutput;

//======================================================================================================================
//  Variables: Public Data 
//      value_to_read   -   Is public variable to read
//======================================================================================================================
#ifndef _HWCHECK_C_

#endif
//======================================================================================================================
// Public functions
//======================================================================================================================
void HwCheckInit(void);
void HwCheckSetBit(u32 channel, u32 word, u32 bit, u8 value);
void HwCheckSetBitFromIsr(u32 channel, u32 word, u32 bit, u8 value);
u8   HwCheckGetBit(t_HwCheckOutput output, u32 channel, u32 word, u32 bit);

void HwCheckSetWord(u32 channel, u32 word, u32 mask, u32 value);
u32  HwCheckGetWord(t_HwCheckOutput output, u32 channel, u32 word);

void HwCheckHardwareChanged(void);

#endif //_HWCHECK_H_
//========================================================EOF===========================================================
