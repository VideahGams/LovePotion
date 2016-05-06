// This code is licensed under the MIT Open Source License.

// Copyright (c) 2015 Ruairidh Carmichael - ruairidhcarmichael@live.co.uk

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <compat-5.2.h>
#include <luaobj.h>

#include <sf2d.h>
#include <sfil.h>
#include <sftd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <3ds.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <3ds/services/cfgu.h>

#include <3ds/services/soc.h>
//#include <3ds/services/sslc.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#include <ivorbiscodec.h>
#include <ivorbisfile.h>

#include "util.h"

#define luaL_dobuffer(L, b, n, s) (luaL_loadbuffer(L, b, n, s) || lua_pcall(L, 0, LUA_MULTRET, 0))

#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)

typedef struct {
    int id;
} love_joystick;

typedef struct {
	sf2d_texture *texture;
	char *minFilter;
	char *magFilter;
} love_image;

typedef struct {
	sftd_font *font;
	int size;
} love_font;

typedef struct {
	FILE * file;
	const char * filename;
	const char * mode;
	long size;
	long position;
} love_file;

typedef enum {
	TYPE_UNKNOWN = -1,
	TYPE_OGG = 0,
	TYPE_WAV = 1
} love_source_type;

#define SAMPLERATE 44100
#define SAMPLESPERBUFFER (SAMPLERATE / 30)
#define BYTESPERSAMPLE 4

typedef struct {
	love_source_type type;

	OggVorbis_File vorbisFile;
	
	float rate;
	u32 channels;
	u32 encoding;
	u32 nsamples;
	u32 size;
	char * data;
	bool loop;
	int audiochannel;

	bool stream;

	const char * filename;

	size_t offset;
	
	u32 waveBufferPosition;

	float mix[12];
	ndspInterpType interp;
} love_source;

int streamCount;
love_source * * audioStreams;

typedef struct {
	int x;
	int y;
	int width;
	int height;
} love_quad;

bool initializeSocket;
typedef struct {
	int socket;
	struct sockaddr_in address;
	struct hostent * host;
} lua_socket;

extern lua_State *L;
extern int currentScreen;
extern int drawScreen;
extern char dsNames[32][32];
extern char keyNames[32][32];
extern touchPosition touch;
extern bool touchIsDown;
extern char *rootDir;
extern char sdmcPath[255];

extern char * filesystemCheckPath(char * luaString);
extern char * filesystemGetPath(char * luaString);

extern bool shouldQuit;
extern love_font *currentFont;
extern bool is3D;
extern const char *fontDefaultInit();
extern bool soundEnabled;
extern bool channelList[24];
extern u32 defaultFilter;
extern char *defaultMinFilter;
extern char *defaultMagFilter;