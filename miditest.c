#include <stdio.h>
#include "midi.h"

int main(){
	int r;
	MIDIFile midi;

	r = MIDIFile_load(&midi, "liz_et3.mid");
	switch (r){
		case FILE_IO_ERROR:
			puts("ERROR: Failed to open file!");
			return 1;
		case FILE_INVALID:
			puts("ERROR: Invalid MIDI file!");
			return 1;
	}	

	printf("\n%c %d %d %d\n",midi.header.id[0],
			(int)midi.header.size, midi.header.format,
			midi.header.num_tracks, midi.header.time_div);

	fclose(midi.file);
	return 0;
}
