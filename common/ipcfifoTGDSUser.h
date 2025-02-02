/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

//TGDS required version: IPC Version: 1.3

//IPC FIFO Description: 
//		struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress; 														// Access to TGDS internal IPC FIFO structure. 		(ipcfifoTGDS.h)
//		struct sIPCSharedTGDSSpecific * TGDSUSERIPC = (struct sIPCSharedTGDSSpecific *)TGDSIPCUserStartAddress;		// Access to TGDS Project (User) IPC FIFO structure	(ipcfifoTGDSUser.h)

//inherits what is defined in: ipcfifoTGDS.h
#ifndef __ipcfifoTGDSUser_h__
#define __ipcfifoTGDSUser_h__

#include "dsregs.h"
#include "dsregs_asm.h"
#include "ipcfifoTGDS.h"

#ifdef ARM9
#include "posixHandleTGDS.h"
#endif

//---------------------------------------------------------------------------------
typedef struct sIPCSharedTGDSSpecific{
	
}  IPCSharedTGDSSpecific	__attribute__((aligned (4)));

//libnds FIFO
typedef struct
{
	u8 count;
	u8 data[96];
	u8 channel;
} returnMsg;

#define FIFO_SNDSYS FIFO_USER_01
#define FIFO_RETURN FIFO_USER_02

//TGDS Memory Layout ARM7/ARM9 Cores (WoopsiSDK relies on IWRAM to both play IMA-ADPCM / Run TGDS-Multiboot v3 / Run ARM7 VRAM Core)
#define TGDS_ARM7_MALLOCSTART (u32)(0x06018000)
#define TGDS_ARM7_MALLOCSIZE (int)(512)
#define TGDS_ARM7_AUDIOBUFFER_STREAM (u32)(TGDS_ARM7_MALLOCSTART + TGDS_ARM7_MALLOCSIZE) //16K
#define TGDSDLDI_ARM7_ADDRESS (u32)((int)TGDS_ARM7_AUDIOBUFFER_STREAM + (16*1024))	//ARM7DLDI: 16K

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern  struct sIPCSharedTGDSSpecific* getsIPCSharedTGDSSpecific();
//NOT weak symbols : the implementation of these is project-defined (here)
extern void HandleFifoNotEmptyWeakRef(uint32 cmd1, uint32 cmd2);
extern void HandleFifoEmptyWeakRef(uint32 cmd1, uint32 cmd2);

#ifdef __cplusplus
}
#endif