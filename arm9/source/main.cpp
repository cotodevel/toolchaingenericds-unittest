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
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
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
#include "opmock.h"
#include "c_partial_mock_test.h"
#include "c_regression.h"
#include "cpptests.h"
#include "libndsFIFO.h"
#include "xenofunzip.h"
#include "cartHeader.h"
#include "videoGL.h"
#include "videoTGDS.h"
#include "math.h"
#include "posixFilehandleTest.h"

struct FileClassList * menuIteratorfileClassListCtx = NULL;
char curChosenBrowseFile[256+1];
char globalPath[MAX_TGDSFILENAME_LENGTH+1];
static int curFileIndex = 0;
static bool pendingPlay = false;

int internalCodecType = SRC_NONE;//Internal because WAV raw decompressed buffers are used if Uncompressed WAV or ADPCM
static struct fd * _FileHandleVideo = NULL; 
static struct fd * _FileHandleAudio = NULL;

bool stopSoundStreamUser(){
	return stopSoundStream(_FileHandleVideo, _FileHandleAudio, &internalCodecType);
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

static inline void menuShow(){
	clrscr();
	printf("     ");
	printf("     ");
	printf("ToolchainGenericDS-CPPUnitTest: ");
	printf("(Select): This menu. ");
	printf("(Start): FileBrowser : (A) Play WAV/IMA-ADPCM (Intel) strm ");
	printf("(D-PAD:UP/DOWN): Volume + / - ");
	printf("(D-PAD:LEFT): GDB Debugging. >%d", TGDSPrintfColor_Green);
	printf("(D-PAD:RIGHT): Demo Sound. >%d", TGDSPrintfColor_Yellow);
	printf("(Y): Run CPPUTest. >%d", TGDSPrintfColor_Yellow);
	printf("(X): Test libnds FIFO subsystem. >%d", TGDSPrintfColor_Yellow);
	printf("(Touch): Move Simple 3D Triangle. >%d", TGDSPrintfColor_Cyan);
	
	printf("Available heap memory: %d >%d", getMaxRam(), TGDSPrintfColor_Cyan);
	printarm7DebugBuffer();
}

static bool ShowBrowserC(char * Path, char * outBuf, bool * pendingPlay, int * curFileIdx){	//MUST be same as the template one at "fileBrowse.h" but added some custom code
	scanKeys();
	while((keysDown() & KEY_START) || (keysDown() & KEY_A) || (keysDown() & KEY_B)){
		scanKeys();
		IRQWait(0, IRQ_VBLANK);
	}
	
	*pendingPlay = false;
	
	//Create TGDS Dir API context
	cleanFileList(menuIteratorfileClassListCtx);
	
	//Use TGDS Dir API context
	int pressed = 0;
	struct FileClass filStub;
	{
		filStub.type = FT_FILE;
		strcpy(filStub.fd_namefullPath, "");
		filStub.isIterable = true;
		filStub.d_ino = -1;
		filStub.parentFileClassList = menuIteratorfileClassListCtx;
	}
	char curPath[MAX_TGDSFILENAME_LENGTH+1];
	strcpy(curPath, Path);
	
	if(pushEntryToFileClassList(true, filStub.fd_namefullPath, filStub.type, -1, menuIteratorfileClassListCtx) != NULL){
		
	}
	
	int j = 1;
	int startFromIndex = 1;
	*curFileIdx = startFromIndex;
	struct FileClass * fileClassInst = NULL;
	fileClassInst = FAT_FindFirstFile(curPath, menuIteratorfileClassListCtx, startFromIndex);
	while(fileClassInst != NULL){
		//directory?
		if(fileClassInst->type == FT_DIR){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fileClassInst->fd_namefullPath);
			parseDirNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
		}
		//file?
		else if(fileClassInst->type  == FT_FILE){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fileClassInst->fd_namefullPath);
			parsefileNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
		}
		
		
		//more file/dir objects?
		fileClassInst = FAT_FindNextFile(curPath, menuIteratorfileClassListCtx);
	}
	
	//actual file lister
	clrscr();
	
	j = 1;
	pressed = 0 ;
	int lastVal = 0;
	bool reloadDirA = false;
	bool reloadDirB = false;
	char * newDir = NULL;
	
	#define itemsShown (int)(15)
	int curjoffset = 0;
	int itemRead=1;
	
	while(1){
		int fileClassListSize = getCurrentDirectoryCount(menuIteratorfileClassListCtx) + 1;	//+1 the stub
		int itemsToLoad = (fileClassListSize - curjoffset);
		
		//check if remaining items are enough
		if(itemsToLoad > itemsShown){
			itemsToLoad = itemsShown;
		}
		
		while(itemRead < itemsToLoad ){		
			if(getFileClassFromList(itemRead+curjoffset, menuIteratorfileClassListCtx)->type == FT_DIR){
				printfCoords(0, itemRead, "--- %s >%d",getFileClassFromList(itemRead+curjoffset, menuIteratorfileClassListCtx)->fd_namefullPath, TGDSPrintfColor_Yellow);
			}
			else{
				printfCoords(0, itemRead, "--- %s",getFileClassFromList(itemRead+curjoffset, menuIteratorfileClassListCtx)->fd_namefullPath);
			}
			itemRead++;
		}
		
		scanKeys();
		pressed = keysDown();
		if (pressed&KEY_DOWN && (j < (itemsToLoad - 1) ) ){
			j++;
			while(pressed&KEY_DOWN){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//downwards: means we need to reload new screen
		else if(pressed&KEY_DOWN && (j >= (itemsToLoad - 1) ) && ((fileClassListSize - curjoffset - itemRead) > 0) ){
			
			//list only the remaining items
			clrscr();
			
			curjoffset = (curjoffset + itemsToLoad - 1);
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysDown();
			while(pressed&KEY_DOWN){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//LEFT, reload new screen
		else if(pressed&KEY_LEFT && ((curjoffset - itemsToLoad) > 0) ){
			
			//list only the remaining items
			clrscr();
			
			curjoffset = (curjoffset - itemsToLoad - 1);
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysDown();
			while(pressed&KEY_LEFT){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//RIGHT, reload new screen
		else if(pressed&KEY_RIGHT && ((fileClassListSize - curjoffset - itemsToLoad) > 0) ){
			
			//list only the remaining items
			clrscr();
			
			curjoffset = (curjoffset + itemsToLoad - 1);
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysDown();
			while(pressed&KEY_RIGHT){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		else if (pressed&KEY_UP && (j > 1)) {
			j--;
			while(pressed&KEY_UP){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//upwards: means we need to reload new screen
		else if (pressed&KEY_UP && (j <= 1) && (curjoffset > 0) ) {
			//list only the remaining items
			clrscr();
			
			curjoffset--;
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysDown();
			while(pressed&KEY_UP){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//reload DIR (forward)
		else if( (pressed&KEY_A) && (getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->type == FT_DIR) ){
			struct FileClass * fileClassChosen = getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx);
			newDir = fileClassChosen->fd_namefullPath;
			reloadDirA = true;
			break;
		}
		
		//file chosen
		else if( (pressed&KEY_A) && (getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->type == FT_FILE) ){
			break;
		}
		
		//reload DIR (backward)
		else if(pressed&KEY_B){
			reloadDirB = true;
			break;
		}
		
		// Show cursor
		printfCoords(0, j, "*");
		if(lastVal != j){
			printfCoords(0, lastVal, " ");	//clean old
		}
		lastVal = j;
	}
	
	//enter a dir
	if(reloadDirA == true){
		//Free TGDS Dir API context
		//freeFileList(menuIteratorfileClassListCtx);	//can't because we keep the menuIteratorfileClassListCtx handle across folders
		
		enterDir((char*)newDir, Path);
		return true;
	}
	
	//leave a dir
	if(reloadDirB == true){
		//Free TGDS Dir API context
		//freeFileList(menuIteratorfileClassListCtx);	//can't because we keep the menuIteratorfileClassListCtx handle across folders
		
		//rewind to preceding dir in TGDSCurrentWorkingDirectory
		leaveDir(Path);
		return true;
	}
	
	strcpy((char*)outBuf, getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->fd_namefullPath);
	clrscr();
	printf("                                   ");
	if(getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->type == FT_DIR){
		//printf("you chose Dir:%s",outBuf);
	}
	else{
		*curFileIdx = (j+curjoffset);
		*pendingPlay = true;
	}
	
	//Free TGDS Dir API context
	//freeFileList(menuIteratorfileClassListCtx);
	return false;
}

#define ListSize (int)(4)
static char * TestList[ListSize] = {"c_partial_mock", "c_regression", "cpp_tests", "posix_filehandle_tests"};

static void returnMsgHandler(int bytes, void* user_data);

//Should ARM7 reply a message, then, it'll be received to test the Libnds FIFO implementation.
static void returnMsgHandler(int bytes, void* user_data)
{
	returnMsg msg;
	fifoGetDatamsg(FIFO_RETURN, bytes, (u8*) &msg);
	printf("ARM7 reply size: %d returnMsgHandler: ", bytes);
	printf("%s >%d", (char*)&msg.data[0], TGDSPrintfColor_Green);
}

static void InstallSoundSys()
{
	/* Install FIFO */
	fifoSetDatamsgHandler(FIFO_RETURN, returnMsgHandler, 0);
}

char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];

//ToolchainGenericDS-LinkedModule User implementation: Called if TGDS-LinkedModule fails to reload ARM9.bin from DLDI.
int TGDSProjectReturnFromLinkedModule() __attribute__ ((optnone)) {
	return -1;
}

int main(int argc, char **argv) __attribute__ ((optnone)) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	InstallSoundSys();
	
	//Init TGDS FS Directory Iterator Context(s). Mandatory to init them like this!! Otherwise several functions won't work correctly.
	menuIteratorfileClassListCtx = initFileList();
	cleanFileList(menuIteratorfileClassListCtx);
	
	memset(globalPath, 0, sizeof(globalPath));
	strcpy(globalPath,"/");
	
	menuShow();
	
	//Simple Triangle GL init
	float rotateX = 0.0;
	float rotateY = 0.0;
	{
		//set mode 0, enable BG0 and set it to 3D
		SETDISPCNT_MAIN(MODE_0_3D);

		//this should work the same as the normal gl call
		glViewPort(0,0,255,191);
		
		glClearColor(0,0,0);
		glClearDepth(0x7FFF);
		
		
	}
	
	/*
	//Tested untar code: takes a tar + gzipped archive file and creates a new folder, then decompress all files in archive in their respective directories
	clrscr();
	printf(" --");
	printf(" --");
	
	int argCount = 2;
	
	strcpy(&args[0][0], "-l");	//Arg0
	strcpy(&args[1][0], "0:/demo-simple.tar.gz");	//Arg1
	strcpy(&args[2][0], "d /");	//Arg2
	
	int i = 0;
	for(i = 0; i < argCount; i++){
		argvs[i] = (char*)&args[i][0];
	}
	
	extern int untgzmain(int argc,char **argv);
	
	untgzmain(argCount, argvs);
	
	printf("untgzmain end");
	while(1==1){
		swiDelay(1);
	}
	*/
	while(1) {
		if(pendingPlay == true){
			internalCodecType = playSoundStream(curChosenBrowseFile, _FileHandleVideo, _FileHandleAudio);
			if(internalCodecType == SRC_NONE){
				stopSoundStreamUser();
			}
			pendingPlay = false;
			menuShow();
		}
		
		scanKeys();
		
		if (keysDown() & KEY_X){
			
			returnMsg msg;
			sprintf((char*)&msg.data[0], "%s", "Test message using libnds FIFO API!");
			if(fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg) == true){
				//printf("X: Channel: %d Message Sent! ", FIFO_RETURN);
			}
			else{
				//printf("X: Channel: %d Message FAIL! ", FIFO_RETURN);
			}
			
			while(keysDown() & KEY_X){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_TOUCH){
			u8 channel = SOUNDSTREAM_FREE_CHANNEL;
			startSoundSample(11025, (u32*)&click_raw[0], click_raw_size, channel, 40, 63, 1);	//PCM16 sample
			while(keysDown() & KEY_TOUCH){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_SELECT){
			menuShow();
			while(keysDown() & KEY_SELECT){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_START){
			while( ShowBrowserC((char *)globalPath, (char *)&curChosenBrowseFile[0], &pendingPlay, &curFileIndex) == true ){	//as long you keep using directories ShowBrowser will be true
				
			}
			
			if(getCurrentDirectoryCount(menuIteratorfileClassListCtx) > 0){
				strcpy(curChosenBrowseFile, (const char *)getFileClassFromList(curFileIndex, menuIteratorfileClassListCtx)->fd_namefullPath);
				pendingPlay = true;
			}
			
			scanKeys();
			while(keysDown() & KEY_START){
				scanKeys();
			}
			menuShow();
		}
		
		if (keysDown() & KEY_L){
			struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
			if(getCurrentDirectoryCount(menuIteratorfileClassListCtx) > 0){
				if(curFileIndex > 1){	//+1 stub FileClass
					curFileIndex--;
				}
				struct FileClass * fileClassInst = getFileClassFromList(curFileIndex, menuIteratorfileClassListCtx);
				if(fileClassInst->type == FT_FILE){
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
					pendingPlay = true;
				}
				else{
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
				}
			}
			
			scanKeys();
			while(keysDown() & KEY_L){
				scanKeys();
				IRQWait(0, IRQ_VBLANK);
			}
			menuShow();
		}
		
		if (keysDown() & KEY_R){	
			//Play next song from current folder
			int lstSize = getCurrentDirectoryCount(menuIteratorfileClassListCtx);
			if(lstSize > 0){	
				if(curFileIndex < lstSize){
					curFileIndex++;
				}
				struct FileClass * fileClassInst = getFileClassFromList(curFileIndex, menuIteratorfileClassListCtx);
				if(fileClassInst->type == FT_FILE){
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
					pendingPlay = true;
				}
				else{
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
				}
			}
			
			scanKeys();
			while(keysDown() & KEY_R){
				scanKeys();
				IRQWait(0, IRQ_VBLANK);
			}
			menuShow();
		}
		
		if (keysDown() & KEY_UP){
			struct XYTscPos touchPos;
			XYReadScrPosUser(&touchPos);
			volumeUp(touchPos.touchXpx, touchPos.touchYpx);
			menuShow();
			scanKeys();
			while(keysDown() & KEY_UP){
				scanKeys();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		if (keysDown() & KEY_DOWN){
			struct XYTscPos touchPos;
			XYReadScrPosUser(&touchPos);
			volumeDown(touchPos.touchXpx, touchPos.touchYpx);
			menuShow();
			scanKeys();
			while(keysDown() & KEY_DOWN){
				scanKeys();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		if (keysDown() & KEY_B){
			scanKeys();
			stopSoundStreamUser();
			menuShow();
			while(keysDown() & KEY_B){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_RIGHT){
			strcpy(curChosenBrowseFile, (const char *)"0:/rain.ima");
			pendingPlay = true;
			
			scanKeys();
			while(keysDown() & KEY_RIGHT){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_Y){
			
			//Choose the test
			clrscr();
			
			int testIdx = 0;
			while(1==1){
				scanKeys();
				
				while(keysDown() & KEY_UP){
					if(testIdx < (ListSize-1)){
						testIdx++;
					}
					scanKeys();
					while(!(keysUp() & KEY_UP)){
						scanKeys();
					}
					printfCoords(0, 10, "                                                    ");	//clean old
					printfCoords(0, 11, "                                                    ");
				}
				
				while(keysDown() & KEY_DOWN){
					if(testIdx > 0){
						testIdx--;
					}
					scanKeys();
					while(!(keysUp() & KEY_DOWN)){
						scanKeys();
					}
					printfCoords(0, 10, "                                                    ");	//clean old
					printfCoords(0, 11, "                                                    ");
				}
				
				if(keysDown() & KEY_A){
					break;
				}
				
				printfCoords(0, 10, "Which Test? [%s] >%d", TestList[testIdx], TGDSPrintfColor_Yellow);
				printfCoords(0, 11, "Press (A) to start test");
			}
			
			switch(testIdx){
				case (0):{ //c_partial_mock
					opmock_test_suite_reset();
					opmock_register_test(test_fizzbuzz_with_15, "test_fizzbuzz_with_15");
					opmock_register_test(test_fizzbuzz_many_3, "test_fizzbuzz_many_3");
					opmock_register_test(test_fizzbuzz_many_5, "test_fizzbuzz_many_5");
					opmock_test_suite_run();
				}
				break;
				case (1):{ //c_regression
					opmock_test_suite_reset();
					opmock_register_test(test_push_pop_stack, "test_push_pop_stack");
					opmock_register_test(test_push_pop_stack2, "test_push_pop_stack2");
					opmock_register_test(test_push_pop_stack3, "test_push_pop_stack3");
					opmock_register_test(test_push_pop_stack4, "test_push_pop_stack4");
					opmock_register_test(test_verify, "test_verify");
					opmock_register_test(test_verify_with_matcher_cstr, "test_verify_with_matcher_cstr");
					opmock_register_test(test_verify_with_matcher_int, "test_verify_with_matcher_int");
					opmock_register_test(test_verify_with_matcher_float, "test_verify_with_matcher_float");
					opmock_register_test(test_verify_with_matcher_custom, "test_verify_with_matcher_custom");
					opmock_register_test(test_cmp_ptr_with_typedef, "test_cmp_ptr_with_typedef");
					opmock_register_test(test_cmp_ptr_with_typedef_fail, "test_cmp_ptr_with_typedef_fail");	
					opmock_test_suite_run();
				}
				break;
				case (2):{ //cpp_tests
					doCppTests(argc, argv);
				}
				break;
				case (3):{ //posix_filehandle_tests
					clrscr();
					printf(" - - ");
					printf(" - - ");
					printf(" - - ");
					
					if(testPosixFilehandle_fopen_fclose_method() == 0){
						printf("testPosixFilehandle_fopen_fclose_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_fopen_fclose_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
					if(testPosixFilehandle_sprintf_fputs_fscanf_method() == 0){
						printf("testPosixFilehandle_sprintf_fputs_fscanf_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_sprintf_fputs_fscanf_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
					if(testPosixFilehandle_fread_fwrite_method() == 0){
						printf("testPosixFilehandle_fread_fwrite_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_fread_fwrite_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
					if(testPosixFilehandle_fgetc_feof_method() == 0){
						printf("testPosixFilehandle_fgetc_feof_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_fgetc_feof_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
					if(testPosixFilehandle_fgets_method() == 0){
						printf("testPosixFilehandle_fgets_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_fgets_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
				}
				break;
			}
			
			
			printf("Tests done. Press (A) to exit.");
			while(1==1){
				scanKeys();	
				if(keysDown() & KEY_A){
					break;
				}	
			}
			menuShow();
		}
		
		// TSC Test.
		struct XYTscPos touch;
		XYReadScrPosUser(&touch);
		
		if( (touch.touchYpx == 0) && (touch.touchXpx == 0) ){
			printfCoords(0, 16, " No Press (X/Y):%d %d            -", touch.touchXpx, touch.touchYpx);
		}
		else{
			
			glReset();
	
			//any floating point gl call is being converted to fixed prior to being implemented
			gluPerspective(35, 256.0 / 192.0, 0.1, 40);

			gluLookAt(	0.0, 0.0, 1.0,		//camera possition 
						0.0, 0.0, 0.0,		//look at
						0.0, 1.0, 0.0);		//up

			glPushMatrix();

			//move it away from the camera
			glTranslate3f32(0, 0, floattof32(-1));
					
			glRotateX(rotateX);
			glRotateY(rotateY);			
			glMatrixMode(GL_MODELVIEW);
			
			//not a real gl function and will likely change
			glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
			
			//dead simple range check
			int sliceScreenX = (256/2);
			int sliceScreenY = (192/2);
			//Top Left
			if( ((sliceScreenX*1) >= touch.touchXpx) && ((sliceScreenY*1) > touch.touchYpx) ){
				rotateX -= 0.8;
				rotateY += 0.8;
				printfCoords(0, 16, " Top Left(X/Y):%d %d ", touch.touchXpx, touch.touchYpx);
			}
			//Bottom Left
			else if( ((sliceScreenX*1) >= touch.touchXpx) && ((sliceScreenY*1) < touch.touchYpx) ){
				rotateX -= 0.8;
				rotateY -= 0.8;
				printfCoords(0, 16, " Bottom Left(X/Y):%d %d ", touch.touchXpx, touch.touchYpx);
			}
			//Top Right
			else if( ((sliceScreenX*2) >= touch.touchXpx) && ((sliceScreenY*1) > touch.touchYpx) ){
				rotateX += 0.8;
				rotateY += 0.8;
				printfCoords(0, 16, " Top Right(X/Y):%d %d ", touch.touchXpx, touch.touchYpx);
			}
			//Bottom Right
			else if( ((sliceScreenX*2) >= touch.touchXpx) && ((sliceScreenY*1) < touch.touchYpx) ){
				rotateX += 0.8;
				rotateY -= 0.8;
				printfCoords(0, 16, " Bottom Right(X/Y):%d %d ", touch.touchXpx, touch.touchYpx);
			}
			
			//draw the obj
			glBegin(GL_TRIANGLE);
				
				glColor3b(31,0,0);
				glVertex3v16(inttov16(-1),inttov16(-1),0);

				glColor3b(0,31,0);
				glVertex3v16(inttov16(1), inttov16(-1), 0);

				glColor3b(0,0,31);
				glVertex3v16(inttov16(0), inttov16(1), 0);
				
			glEnd();
			glPopMatrix(1);
			glFlush();
		}
		
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}

	return 0;
}