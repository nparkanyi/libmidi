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
#include <stdio.h>
#include "midi.h"

int VLV_read(FILE * buf, guint32 * val){
	char byte;
	int i;
	
	*val = 0x00;

	for (i = 0; i < 4; i++){
		if (fread(&byte, 1, 1, buf) != 1)
			return FILE_IO_ERROR;

		*val = ((*val << 7) | (byte % 128));

		//is last byte?
		if ((byte & 0x80) == 0x00){
			return SUCCESS;
		}
	}	
	//VLVs smaller than 4 bytes, should never reach here
	return VLV_ERROR;
}

	
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
