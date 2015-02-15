#include <stdio.h>
#include "midi.h"

int main(int argc, char * argv[]){
  int r;
  MIDIFile midi;
  MIDITrack track;
  MIDIEvent * ptr;

  r = MIDIFile_load(&midi, argv[1]);
  switch (r){
    case FILE_IO_ERROR:
      puts("ERROR: Failed to open file!");
      return 1;
    case FILE_INVALID:
      puts("ERROR: Invalid MIDI file!");
      return 1;
  } 

  r = MIDITrack_load(&track, midi.file);
  switch (r){
    case FILE_INVALID:
      puts("ERROR reading track: Invalid data!");
      break;
    case FILE_IO_ERROR:
      puts("ERROR: file io problem!");
      break;
    case MEMORY_ERROR:
      puts("ERROR: failed to allocate memory!");
      break;
  }

  printf("header size: %d\n", (int)midi.header.size);
  printf("format: %d\n", midi.header.format);
  printf("tracks: %d\n", midi.header.num_tracks);
  printf("time div: %d\n", midi.header.time_div);
  printf("track size: %d\n", track.header.size);

  ptr = track.head;
  while (ptr != NULL){
    if (ptr->type == EV_NOTE_ON){
      printf("note: %d\n", ((MIDIChannelEventData*)(ptr->data))->param1);
    }
    ptr = ptr->next;
  }

  fclose(midi.file);
  return 0;
}
