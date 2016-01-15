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

// http://hellomico.com/getting-started/convert-audio-to-raw/

#include "../shared.h"

#define CLASS_TYPE  LUAOBJ_TYPE_SOURCE
#define CLASS_NAME  "Source"

bool channelList[32];

int getOpenChannel() {

	int i;
	for (i = 0; i < 32; i++) {
		if (!channelList[i]) {
			channelList[i] = true;
			return i + 8;
		}
	}

	return NULL;

}

const char *sourceInit(love_source *self, const char *filename) {

	if (fileExists(filename)) {

		const char *ext = fileExtension(filename);

		if (strncmp(ext, "raw", 3) == 0) {

			FILE *file = fopen(filename, "rb");

			if (file) {

				fseek(file, 0, SEEK_END);
				self->size = ftell(file);
				fseek(file, 0, SEEK_SET);
				self->buffer = linearAlloc(self->size);

				if (!self->buffer) {
					fclose(file);
					return "Could not allocate sound buffer";
				}

				fread(self->buffer, 1, self->size, file);

				self->format = SOUND_FORMAT_16BIT;
				self->used = false;
				self->channel = getOpenChannel();
				self->samplerate = 44100;
				self->extension = ext;
				self->loop = false;

			}

			fclose(file);

		} else if (strncmp(ext, "wav", 3) == 0) {

			FILE *file = fopen(filename, "rb");

			if (file) {

				fseek(file, 0, SEEK_END); // Seek to the end of the .wav file.
				self->size = ftell(file); // Get the total size of the .wav file.
				fseek(file, 0, SEEK_SET); // Seek back to the start.

				self->buffer = linearAlloc(self->size - 44); // Allocate the buffer, remove 44 bytes because we're not storing the header in here (which is 44 bytes)

				if (!self->buffer) {
					fclose(file);
					return "Could not allocate sound buffer";
				}

				fseek(file, 44, SEEK_SET); // Seek to 44 bytes (Where the sound data starts)
				fread(self->buffer, 1, self->size - 44, file); // Read all of the sound data into the buffer
				fseek(file, 0, SEEK_SET); // Not sure if this is necessary, doing it just in case.

				fseek(file, 24, SEEK_SET); // Seek to 24 bytes (This is where the samplerate is located in the header)
				fread(&self->samplerate, 1, 4, file); // Read the samplerate into a variable.
				fseek(file, 0, SEEK_SET);

				self->format = SOUND_FORMAT_16BIT;
				self->used = false;
				self->channel = getOpenChannel();
				self->extension = ext;
				self->loop = false;
				self->samplerate *= 2; // You need to times the sample rate by two (not sure why)

			}

			fclose(file);

		} else return "Unknown audio type";

	} else return "Could not open source, does not exist.";

	return NULL;

}

int sourceNew(lua_State *L) { // love.audio.newSource()

	const char *filename = luaL_checkstring(L, 1);

	love_source *self = luaobj_newudata(L, sizeof(*self));
	luaobj_setclass(L, CLASS_TYPE, CLASS_NAME);

	const char *error = sourceInit(self, filename);

	if (error) luaError(L, error);

	return 1;

}

int sourcePlay(lua_State *L) { // source:play()

	love_source *self = luaobj_checkudata(L, 1, CLASS_TYPE);

	if (!self || !self->buffer || !self->format || !self->samplerate || !soundEnabled) {

		luaError(L, "There was an error playing the source, sound may not be available.");
		return;

	}

	u8 playing;
	csndIsPlaying(self->channel, &playing);

	if (!playing) {

		u32 shouldLoop = self->loop ? SOUND_REPEAT: SOUND_ONE_SHOT;
		csndPlaySound(self->channel, self->format | shouldLoop, self->samplerate, 1, 0, self->buffer, self->buffer, self->size);
		CSND_UpdateInfo(0);

		self->used = true;

	}

	return 0;

}

int sourceStop(lua_State *L) { // source:stop()

	love_source *self = luaobj_checkudata(L, 1, CLASS_TYPE);

	if (!self || !self->buffer || !self->format || !self->samplerate || !soundEnabled) return;

	CSND_SetPlayState(self->channel, false);
	CSND_UpdateInfo(0);

	return 0;

}

int sourceIsPlaying(lua_State *L) { // source:isPlaying()

	love_source *self = luaobj_checkudata(L, 1, CLASS_TYPE);

	u8 playing;
	csndIsPlaying(self->channel, &playing);
	if (playing) {
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}

	return 1;

}

int sourceSetLooping(lua_State *L) { // source:setLooping()

	love_source *self = luaobj_checkudata(L, 1, CLASS_TYPE);
	self->loop = lua_toboolean(L, 2);
	CSND_UpdateInfo(0);

	return 0;

}

int sourceIsLooping(lua_State *L) { // source:isLooping()

	love_source *self = luaobj_checkudata(L, 1, CLASS_TYPE);

	lua_pushboolean(L, self->loop);

	return 1;

}

int sourceGC(lua_State *L) { // Garbage Collection

	love_source *self = luaobj_checkudata(L, 1, CLASS_TYPE);

	if (!self->buffer) return 0;

	linearFree(self->buffer);
	self->buffer = NULL;

	channelList[self->channel - 8] = false;

	return 0;

}

int sourceUnload(lua_State *L) { // source:Unload(). Stops the sound and frees up memory.

	love_source *self = luaobj_checkudata(L, 1, CLASS_TYPE);
	
    CSND_SetPlayState(self->channel, false);
	CSND_UpdateInfo(0);

	if (self->buffer != NULL) {
		GSPGPU_FlushDataCache(self->buffer, self->size - 44);
	    linearFree(self->buffer);
		
		channelList[self->channel] = false;
	} else {
		return 0;
    }
    
    CSND_UpdateInfo(0);

	return 0;

}

int initSourceClass(lua_State *L) {

	luaL_Reg reg[] = {
		{"new",			sourceNew	},
		{"__gc",		sourceGC	},
		{"play",		sourcePlay	},
		{"stop",		sourceStop	},
		{"isPlaying",	sourceIsPlaying},
		{"setLooping",	sourceSetLooping},
		{"unload",	    sourceUnload},
		{ 0, 0 },
	};

	luaobj_newclass(L, CLASS_NAME, NULL, sourceNew, reg);

	return 1;
}