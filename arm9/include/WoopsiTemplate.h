#ifndef _DEMO_H_
#define _DEMO_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <_ansi.h>
#include <reent.h>
#include "soundTGDS.h"
#include "dswnifi_lib.h"
#include "videoGL.h"
#include "libndsFIFO.h"
#include "microphoneShared.h"

#ifdef __cplusplus
#include "alert.h"
#include "woopsi.h"
#include "woopsiheaders.h"
#include "filerequester.h"
#include "textbox.h"
#include "button.h"

#include <string>
#include <iostream>
#include <vector>
using namespace std;
using namespace WoopsiUI;

class WoopsiTemplate : public Woopsi, public GadgetEventHandler {
public:
	void startup(int argc, char **argv);
	void shutdown();
	void handleValueChangeEvent(const GadgetEventArgs& e);	//Handles UI events if they change
	void handleClickEvent(const GadgetEventArgs& e);	//Handles UI events when they take click action
	void waitForAOrTouchScreenButtonMessage(MultiLineTextBox* thisLineTextBox, const WoopsiString& thisText);
	void handleLidClosed();
	void handleLidOpen();
	void ReportAvailableMem();
	void ApplicationMainLoop();
	int currentFileRequesterIndex;
	
	//Top Screen Obj
	AmigaScreen* _topScreen;	//Bottom Screen object
	MultiLineTextBox* _MultiLineTextBoxLogger;
	
	//Bottom Screen Obj
	AmigaScreen* _controlsScreen;	//Top Screen object
	Button* _Idle; //2
	Button* _udpNifi; //3
	Button* _localNifi; //4
	Button* _sendNifiMsg; //5
	Alert* _udpNifiMsgBox; //6
	
	Button* _lzssCompress; //7
	Button* _lzssDecompress; //8
	Button* _dumpNDSSections; //10

	Button* _dumpARM7Memory; //11
	Button* _dumpDLDI; //12
	Button* _testLibNDSFifo; //13
	Button* _testcppFilesystem; //14

	Button* _toggle2D3DMode; //15
	int _3DMode; //1: 3D Mode (simple triangle), 2: 3D Mode (OpenGL DisplayList) 0: 2D Mode
	float rotateX;
	float rotateY;

	Button* _reportPayloadMode; //16
	
	bool _micRecording;
	Button* _micRecord; //9
	
	Button* _UnitTests; //17
	FileRequester* __UnitTestList; //18

	Button* _RandomGen; //19

	Button* _RunToolchainGenericDSMB; //20
	

	FileRequester* _fileReq;
	int _parentRefcon; //registers caller ID when interacting with _fileReq
};
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
extern WoopsiTemplate * WoopsiTemplateProc;
extern std::string getDldiDefaultPath();
extern vector<string> generateAndShuffleKeys(int keyCount);
#endif

extern u32 pendPlay;
extern char currentFileChosen[256+1];
extern char somebuf[frameDSsize];	//use frameDSsize as the sender buffer size, any other size won't be sent.
extern void returnMsgHandler(int bytes, void* user_data);
extern void InstallLibNDSFIFOSys9();
extern int printfWoopsi(const char *fmt, ...);

extern GLvoid BuildLists();
extern GLvoid ReSizeGLScene(GLsizei width, GLsizei height);
extern int textureArrayNDS[1];
extern int InitGL();

extern void sendTestLibNDSFIFO();

#ifdef __cplusplus
}
#endif
