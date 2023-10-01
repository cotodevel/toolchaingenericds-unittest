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

#ifndef __main9_h__
#define __main9_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"
#include "videoGL.h"
#include "WoopsiTemplate.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern int main(int argc, char **argv);
extern void ApplicationMainLoop();

//TGDS Soundstreaming API
extern int internalCodecType;
extern struct fd * _FileHandleVideo; 
extern struct fd * _FileHandleAudio;
extern bool stopSoundStreamUser();
extern void closeSoundUser();
extern char args[8][MAX_TGDSFILENAME_LENGTH];
extern char *argvs[8];
extern bool dumpARM7ARM9Binary(char * filename);
extern void initMIC();
extern int do_sound(char *sound);

extern bool get_pen_delta( int *dx, int *dy );
extern GLuint	texture[1];			// Storage For 1 Texture
extern GLuint	box;				// Storage For The Box Display List
extern GLuint	top;				// Storage For The Top Display List
extern GLuint	xloop;				// Loop For X Axis
extern GLuint	yloop;				// Loop For Y Axis
extern GLfloat	xrot;				// Rotates Cube On The X Axis
extern GLfloat	yrot;				// Rotates Cube On The Y Axis
extern GLfloat boxcol[5][3];
extern GLfloat topcol[5][3];
extern float rotateX;
extern float rotateY;
extern float camMov;

extern int fcopy(FILE *f1, FILE *f2, int maxFileSize);

#ifdef __cplusplus
}
#endif
