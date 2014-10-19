#include <stdio.h>
#include "midi.h"

int MIDIFile_load(MIDIFile * midi, const char * filename){
	int r;

	midi->file = fopen(filename, "r");
	if (!midi->file) 
		return FILE_IO_ERROR;
	
	r = MIDIHeader_load(&midi->header, midi->file);
	if (r != SUCCESS)
		fclose(midi->file);
	
	return r;
}


int MIDIHeader_load(MIDIHeader * header, FILE * file){
	int i;
	char * name = "MThd";

	if (fread(header, sizeof(MIDIHeader), 1, file) < 1)
		return FILE_INVALID;
	
	//swap endianness (does nothing if host is BE)
	header->size = GUINT32_FROM_BE(header->size);
	header->format = GUINT16_FROM_BE(header->format);
	header->num_tracks = GUINT16_FROM_BE(header->num_tracks);
	header->time_div = GUINT16_FROM_BE(header->time_div);

	//if id is not "MThd", not a MIDI file
	for (i = 0; i < 4; i++){
		if (header->id[i] != name[i]) 
			return FILE_INVALID;
	}

	return SUCCESS;
}

