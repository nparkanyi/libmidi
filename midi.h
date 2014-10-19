/*
Copyright (c) 2014 Nicholas Parkanyi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <glib.h>

enum errors{
	SUCCESS=0,
	FILE_IO_ERROR,
	FILE_INVALID
};

enum event_types{
	EV_NOTE_OFF = 0x8,
	EV_NOTE_ON,
	EV_NOTE_AFTERTOUCH,
	EV_CONTROLLER,
	EV_PROGRAM_CHANGE,
	EV_CHANNEL_AFTERTOUCH,
	EV_PITCH_BEND,
	EV_META = 0xFF
};

enum meta_types{
	META_SEQUENCE_NUM,
	META_TEXT,
	META_COPYRIGHT,
	META_NAME,
	META_INSTRUMENT_NAME,
	META_LYRICS,
	META_MARKER,
	META_CUE_POINT,
	META_CHANNEL_PREFIX=0x20,
	META_END_TRACK = 0x2F
};

typedef struct {
	char id[4];
	guint32 size;
	guint16 format;
	guint16 num_tracks;
	guint16 time_div;
} MIDIHeader;

typedef char EventType; 

typedef struct {
	EventType type;
	//pointer to struct of type determined by event type
	//for ghetto polymorphism 
	char * data;
} MIDIEvent;

typedef struct {
	char id[4];
	guint32 size;
} MIDITrackHeader;

typedef struct {
	MIDITrackHeader header;
	//track's events stored as linked list
	MIDIEvent * head;
}

typedef struct {
	FILE * file;

	MIDIHeader header;
} MIDIFile;

int MIDIFile_load(MIDIFile * midi, const char * filename);

int MIDIHeader_load(MIDIHeader * header, FILE * file);
