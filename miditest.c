#include <stdio.h>
#include "midi.h"

int main(int argc, char * argv[]){
	int r;
	MIDIFile midi;

	r = MIDIFile_load(&midi, argv[1]);
	switch (r){
		case FILE_IO_ERROR:
			puts("ERROR: Failed to open file!");
			return 1;
		case FILE_INVALID:
			puts("ERROR: Invalid MIDI file!");
			return 1;
	}	

	printf("header size: %d\n", (int)midi.header.size);
	printf("format: %d\n", midi.header.format);
	printf("tracks: %d\n", midi.header.num_tracks);
	printf("time div: %d\n", midi.header.time_div);

	fclose(midi.file);
	return 0;
}
