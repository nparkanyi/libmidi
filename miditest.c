#include <stdio.h>
#include "midi.h"

int main(int argc, char * argv[]){
  int r, i;
  MIDIFile midi;
  MIDITrack track;
  MIDITrack track2;
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
  //fseek(midi.file, 217, SEEK_CUR);

  r = MIDITrack_load(&track, midi.file);
  switch (r){
    case FILE_INVALID:
      puts("track 1 ERROR reading track: Invalid data!");
      break;
    case FILE_IO_ERROR:
      puts("ERROR: file io problem!");
      break;
    case MEMORY_ERROR:
      puts("ERROR: failed to allocate memory!");
      break;
  }

  r = MIDITrack_load(&track2, midi.file);
  switch (r){
    case FILE_INVALID:
      puts("track 2 ERROR reading track: Invalid data!");
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
  printf("*******************\n");
//  printf("track2 size: %d\n", track2.header.size);

  ptr = track.head;
  while (ptr != NULL){
    printf("type: 0x%X\n", ptr->type);
    if (ptr->type == EV_NOTE_ON){
      printf("note: %d\n", ((MIDIChannelEventData*)(ptr->data))->param1);
    }
    if (ptr->type == EV_NOTE_OFF){
      printf("note: %d\n", ((MIDIChannelEventData*)(ptr->data))->param1);
    }
    ptr = ptr->next;
  }

  /*for (i = 0; i < 500; i++){
    printf("%d: %c\n", i, fgetc(midi.file));
  }*/
  puts("******track 2*********");
  ptr = track2.head;
  while (ptr != NULL){
    printf("type: 0x%X\n", ptr->type);
    if (ptr->type == EV_NOTE_ON){
      printf("note: %d\n", ((MIDIChannelEventData*)(ptr->data))->param1);
    }
    ptr = ptr->next;
  }

  fclose(midi.file);
  return 0;
}
