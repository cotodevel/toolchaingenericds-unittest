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

#include "main.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "gui_console_connector.h"
#include "TGDSLogoLZSSCompressed.h"
#include "ipcfifoTGDSUser.h"
#include "dldi.h"
#include "loader.h"
#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "consoleTGDS.h"
#include "soundTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"
#include "ima_adpcm.h"
#include "linkerTGDS.h"
#include "dldi.h"
#include "utils.twl.h"
#include "spitscTGDS.h"
#include "loader.h"

// Includes
#include "WoopsiTemplate.h"
#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include <stdio.h>

//ARM7 VRAM core
#include "arm7bootldr.h"
#include "arm7bootldr_twl.h"

u32 * getTGDSMBV3ARM7Bootloader(){
	if(__dsimode == false){
		return (u32*)&arm7bootldr[0];	
	}
	else{
		return (u32*)&arm7bootldr_twl[0];
	}
}

float rotateX = 0.0;
float rotateY = 0.0;
float camMov = -1.0;

//true: pen touch
//false: no tsc activity
bool get_pen_delta( int *dx, int *dy ){
	static int prev_pen[2] = { 0x7FFFFFFF, 0x7FFFFFFF };
	
	// TSC Test.
	struct touchPosition touch;
	XYReadScrPosUser(&touch);
	
	if( (touch.px == 0) && (touch.py == 0) ){
		prev_pen[0] = prev_pen[1] = 0x7FFFFFFF;
		*dx = *dy = 0;
		return false;
	}
	else{
		if( prev_pen[0] != 0x7FFFFFFF ){
			*dx = (prev_pen[0] - touch.px);
			*dy = (prev_pen[1] - touch.py);
		}
		prev_pen[0] = touch.px;
		prev_pen[1] = touch.py;
	}
	return true;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int fcopy(FILE *f1, FILE *f2, int maxFileSize){
    char            buffer[BUFSIZ];
    size_t          n=0,copiedBytes=0;
    while ( ((n = fread(buffer, sizeof(char), sizeof(buffer), f1)) > 0) && (copiedBytes < maxFileSize) ){
        if (fwrite(buffer, sizeof(char), n, f2) != n){
            printf("fcopy: write failed. Halt hardware.");
			while(1==1){}
		}
		copiedBytes += n;
    }
	return copiedBytes;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
bool dumpARM7ARM9Binary(char * filename){
	char debugBuf[256];
	bool isTGDSTWLHomebrew = false;
	if(isNTROrTWLBinary(filename, &isTGDSTWLHomebrew) == notTWLOrNTRBinary){
		return false;
	}
	sprintf(debugBuf, "payload open OK\n");
	nocashMessage(debugBuf);
	FILE * fh = fopen(filename, "r");
	if(fh != NULL){
		int headerSize = sizeof(struct sDSCARTHEADER);
		u8 * NDSHeader = (u8 *)TGDSARM9Malloc(headerSize*sizeof(u8));
		if (fread(NDSHeader, 1, headerSize, fh) != headerSize){
			TGDSARM9Free(NDSHeader);
			fclose(fh);
			return false;
		}
		sprintf(debugBuf, "header OK\n");
		nocashMessage(debugBuf);
		struct sDSCARTHEADER * NDSHdr = (struct sDSCARTHEADER *)NDSHeader;
		//ARM7
		int arm7BootCodeSize = NDSHdr->arm7size;
		u32 arm7BootCodeOffsetInFile = NDSHdr->arm7romoffset;
		u32 arm7entryaddress = NDSHdr->arm7entryaddress;
		sprintf(debugBuf, "ARM7: %d \n", arm7BootCodeSize);
		nocashMessage(debugBuf);
		fseek(fh, arm7BootCodeOffsetInFile, SEEK_SET);
		u8* alloc = (u8*)TGDSARM9Malloc(arm7BootCodeSize);
		fread(alloc, 1, arm7BootCodeSize, fh);
		FILE * fout = fopen("0:/arm7.bin", "w+");
		if(fout != NULL){
			fwrite(alloc, 1, arm7BootCodeSize, fout);
			fclose(fout);
		}
		else{
			sprintf(debugBuf, "fail opening arm7.bin");
			nocashMessage(debugBuf);
		}
		TGDSARM9Free(alloc);
		//ARM9
		int arm9BootCodeSize = NDSHdr->arm9size;
		u32 arm9BootCodeOffsetInFile = NDSHdr->arm9romoffset;
		u32 arm9entryaddress = NDSHdr->arm9entryaddress;
		sprintf(debugBuf, "ARM9: %d \n", arm9BootCodeSize);
		nocashMessage(debugBuf);
		fout = fopen("0:/arm9.bin", "w+");
		fseek(fh, arm9BootCodeOffsetInFile, SEEK_SET);
		int arm9written = fcopy(fh, fout, arm9BootCodeSize);
		sprintf(debugBuf, "ARM9 written: %d \n", arm9written);
		nocashMessage(debugBuf);

		TGDSARM9Free(NDSHeader);
		fclose(fout);
		fclose(fh);
		
		//layout-filename.txt
		char fname[256+1];
		sprintf(fname, "%s%s%s", "0:/", "layout-NDSBinary", ".txt");
		char tmpBuf[256+1];
		strcpy(tmpBuf, fname);
		strcpy(fname, tmpBuf);
		fh = fopen(fname, "w+");
		if(fh != NULL){
			char buff[256+1];
			sprintf(buff, "%s Sections: %s",filename, "\n");
			fputs(buff, fh);
			sprintf(buff, "[arm7.bin]:%s%x -- %s -> %d bytes [@ EntryAddress: 0x%x] %s", "arm7BootCodeOffsetInFile: 0x", arm7BootCodeOffsetInFile, "arm7BootCodeSize: ", arm7BootCodeSize, arm7entryaddress, "\n");
			fputs(buff, fh);
			sprintf(buff, "[arm9.bin]:%s%x -- %s -> %d bytes [@ EntryAddress: 0x%x] %s", "arm9BootCodeOffsetInFile: 0x", arm9BootCodeOffsetInFile, "arm9BootCodeSize: ", arm9BootCodeSize, arm9entryaddress, "\n");
			fputs(buff, fh);
			fclose(fh);
		}
		return true;
	}
	return false;
}

#define ListSize (int)(6)
static char * TestList[ListSize] = {"c_partial_mock", "c_regression", "cpp_tests", "posix_filehandle_tests", "ndsDisplayListUtils_tests", "argv_chainload_test"};

void initMIC(){
	char * recFile = "0:/RECNDS.WAV";
	startRecording(recFile);
	endRecording();
}

//TGDS Soundstreaming API
int internalCodecType = SRC_NONE; //Returns current sound stream format: WAV, ADPCM or NONE
struct fd * _FileHandleVideo = NULL; 
struct fd * _FileHandleAudio = NULL;

void menuShow(){
	clrscr();
	printf("                              ");
	printf("ToolchainGenericDS-WoopsiSDK stub:");
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
bool stopSoundStreamUser() {
	return stopSoundStream(_FileHandleVideo, _FileHandleAudio, &internalCodecType);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void closeSoundUser() {
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

int do_sound(char *sound){
	return 0;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	//Save Stage 1: IWRAM ARM7 payload: NTR/TWL (0x03800000)
	memcpy((void *)TGDS_MB_V3_ARM7_STAGE1_ADDR, (const void *)0x02380000, (int)(96*1024));	//
	coherent_user_range_by_size((uint32)TGDS_MB_V3_ARM7_STAGE1_ADDR, (int)(96*1024)); //		also for TWL binaries 
	
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	
	printf("   ");
	printf("   ");
	
	int ret=FS_init();
	if (ret != 0){
		printf("%s: FS Init error: %d >%d", TGDSPROJECTNAME, ret, TGDSPrintfColor_Red);
		while(1==1){
			swiDelay(1);
		}
	}
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	REG_IME = 0;
	set0xFFFF0000FastMPUSettings();
	//TGDS-Projects -> legacy NTR TSC compatibility
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	setupDisabledExceptionHandler();
	REG_IME = 1;
	
	//ARGV Implementation test
	if(getTGDSDebuggingState() == true){
		if (0 != argc ) {
			int i;
			for (i=0; i<argc; i++) {
				if (argv[i]) {
					printf("[%d] %s ", i, argv[i]);
				}
			}
		} 
		else {
			printf("No arguments passed!");
		}
	}
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	menuShow();
	
	// Create Woopsi UI
	WoopsiTemplate WoopsiTemplateApp;
	WoopsiTemplateProc = &WoopsiTemplateApp;
	return WoopsiTemplateApp.main(argc, argv);
	
	while (1){
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
	return 0;
}
