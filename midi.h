#include<glib.h>

enum errors{
	SUCCESS=0,
	FILE_IO_ERROR,
	FILE_INVALID
};

typedef struct {
	char id[4];
	guint32 size;
	guint16 format;
	guint16 num_tracks;
	guint16 time_div;
} MIDIHeader;

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
