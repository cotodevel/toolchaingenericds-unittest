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
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dswnifi_lib.h"
#include "keypadTGDS.h"
#include "TGDSLogoLZSSCompressed.h"
#include "biosTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dldi.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "consoleTGDS.h"
#include "soundTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"
#include "click_raw.h"
#include "ima_adpcm.h"
#include "linkerTGDS.h"
#include "utils.twl.h"
#include "gui_console_connector.h"				  
#include "lzss9.h"
#include "busTGDS.h"
#include "clockTGDS.h"
#include "opmock.h"
#include "c_partial_mock_test.h"
#include "c_regression.h"
#include "cpptests.h"
#include "libndsFIFO.h"
#include "cartHeader.h"
#include "videoGL.h"
#include "videoTGDS.h"
#include "math.h"
#include "posixFilehandleTest.h"
#include "loader.h"
#include "ndsDisplayListUtils.h"
#include "microphoneShared.h"
#include "spitscTGDS.h"

// Includes
#include "WoopsiTemplate.h"
#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include "debugNocash.h"
#include <stdio.h>

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
	if(isNTROrTWLBinary(filename) == notTWLOrNTRBinary){
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

static inline void menuShow(){
	clrscr();
	printf("     ");
	printf("     ");
	printf("%s ", TGDSPROJECTNAME);
	printf("(Select): This menu. ");
	printf("(Start): FileBrowser : (A) Play WAV/IMA-ADPCM (Intel) strm ");
	printf("(D-PAD:UP/DOWN): Volume + / - ");
	printf("(D-PAD:LEFT): GDB Debugging. >%d", TGDSPrintfColor_Green);
	printf("(D-PAD:RIGHT): Demo Sound. >%d", TGDSPrintfColor_Yellow);
	printf("(B): Stop WAV/IMA-ADPCM file. ");
	printf("Current Volume: %d", (int)getVolume());
	if(internalCodecType == SRC_WAVADPCM){
		printf("ADPCM Play: >%d", TGDSPrintfColor_Red);
	}
	else if(internalCodecType == SRC_WAV){	
		printf("WAVPCM Play: >%d", TGDSPrintfColor_Green);
	}
	else{
		printf("Player Inactive");
	}
	printf("Available heap memory: %d >%d", getMaxRam(), TGDSPrintfColor_Cyan);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int do_sound(char *sound){
	return 0;
}

char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("Os")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	printf("              ");
	printf("              ");
	
	//xmalloc init removes args, so save them
	int i = 0;
	for(i = 0; i < argc; i++){
		argvs[i] = argv[i];
	}

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	//argv destroyed here because of xmalloc init, thus restore them
	for(i = 0; i < argc; i++){
		argv[i] = argvs[i];
	}

	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else{
		printf("FS Init error: %d", ret);
	}
	
	switch_dswnifi_mode(dswifi_idlemode);
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	/////////////////////////////////////////////////////////Reload TGDS Proj///////////////////////////////////////////////////////////
	char tmpName[256];
	char ext[256];
	if(__dsimode == true){
		char TGDSProj[256];
		char curChosenBrowseFile[256];
		strcpy(TGDSProj,"0:/");
		strcat(TGDSProj, "ToolchainGenericDS-multiboot");
		if(__dsimode == true){
			strcat(TGDSProj, ".srl");
		}
		else{
			strcat(TGDSProj, ".nds");
		}
		//Force ARM7 reload once 
		if( 
			(argc < 3) 
			&& 
			(strncmp(argv[1], TGDSProj, strlen(TGDSProj)) != 0) 	
		){
			REG_IME = 0;
			MPUSet();
			REG_IME = 1;
			char startPath[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(startPath,"/");
			strcpy(curChosenBrowseFile, TGDSProj);
			
			char thisTGDSProject[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(thisTGDSProject, "0:/");
			strcat(thisTGDSProject, TGDSPROJECTNAME);
			if(__dsimode == true){
				strcat(thisTGDSProject, ".srl");
			}
			else{
				strcat(thisTGDSProject, ".nds");
			}
			
			//Boot .NDS file! (homebrew only)
			strcpy(tmpName, curChosenBrowseFile);
			separateExtension(tmpName, ext);
			strlwr(ext);
			
			//pass incoming launcher's ARGV0
			char arg0[256];
			int newArgc = 3;
			if (argc > 4) {
				printf(" ---- test");
				printf(" ---- test");
				printf(" ---- test");
				printf(" ---- test");
				printf(" ---- test");
				printf(" ---- test");
				printf(" ---- test");
				printf(" ---- test");
				
				//arg 0: original NDS caller
				//arg 1: this NDS binary
				//arg 2: this NDS binary's ARG0: filepath
				strcpy(arg0, (const char *)argv[2]);
				newArgc++;
			}
			//or else stub out an incoming arg0 for relaunched TGDS binary
			else {
				strcpy(arg0, (const char *)"0:/incomingCommand.bin");
				newArgc++;
			}
			//debug end
			
			char thisArgv[5][MAX_TGDSFILENAME_LENGTH];
			memset(thisArgv, 0, sizeof(thisArgv));
			strcpy(&thisArgv[0][0], thisTGDSProject);	//Arg0:	This Binary loaded
			strcpy(&thisArgv[1][0], curChosenBrowseFile);	//Arg1:	Chainload caller: TGDS-MB
			strcpy(&thisArgv[2][0], "0:/stub.bin");			//bugged arg slot
			strcpy(&thisArgv[3][0], (char*)&arg0[0]);//Arg3: NDS Binary reloaded through ChainLoad's ARG0
			strcpy(&thisArgv[4][0], thisTGDSProject);	//Arg4:	NDS Binary reloaded through ChainLoad
			newArgc++;
			addARGV(newArgc, (char*)&thisArgv);				
			if(TGDSMultibootRunNDSPayload(curChosenBrowseFile) == false){ //should never reach here, nor even return true. Should fail it returns false
				
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	setupDisabledExceptionHandler();
	REG_IME = 0;
	MPUSet();
	//TGDS-Projects -> legacy NTR TSC compatibility
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	REG_IME = 1;
	InstallSoundSys();
	initMIC();
	
	// Create Woopsi UI
	WoopsiTemplate WoopsiTemplateApp;
	WoopsiTemplateProc = &WoopsiTemplateApp;
	return WoopsiTemplateApp.main(argc, argv);
	
	while(1) {
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}

	return 0;
}
