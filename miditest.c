#include <stdio.h>
#include <fluidsynth.h>
#include <SDL2/SDL.h>
#include "midi.h"

int main(int argc, char * argv[]){
  int r, i, sfHandle;
  MIDIFile midi;
  MIDITrack track;
  MIDITrack track2;
  MIDITrack track3;
  MIDIEvent * ptr;
  fluid_settings_t * settings;
  fluid_synth_t* synth;
  fluid_audio_driver_t * adriver;
  guint32 ticks = 0;
  long conversion;

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
  conversion = 60000 / (120 * midi.header.time_div);
//  printf("track2 size: %d\n", track2.header.size);

  //set up fluidsynth
  settings = new_fluid_settings();
  synth = new_fluid_synth(settings);
  fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
  adriver = new_fluid_audio_driver(settings, synth);
  sfHandle = fluid_synth_sfload(synth, "font.sf2", 1);
  fluid_synth_bank_select(synth, 0, 0);

  SDL_Init(SDL_INIT_TIMER);

  ptr = track.head;
  while (ptr != NULL){
    if (ptr->type == EV_NOTE_ON && ptr->delta_time * conversion <= SDL_GetTicks() - ticks){
      fluid_synth_noteon(synth, 0, ((MIDIChannelEventData*)(ptr->data))->param1,
                                   ((MIDIChannelEventData*)(ptr->data))->param2);
      ptr = ptr->next;
      ticks = SDL_GetTicks();
    } else if (ptr->type == EV_NOTE_OFF && ptr->delta_time * conversion <= SDL_GetTicks() - ticks){
      fluid_synth_noteoff(synth, 0, ((MIDIChannelEventData*)(ptr->data))->param1);
      ptr = ptr->next;
      ticks = SDL_GetTicks();
    } else if (ptr->type != EV_NOTE_OFF && ptr->type != EV_NOTE_ON){
      ptr = ptr->next;
    }
  }

  /*for (i = 0; i < 500; i++){
    printf("%d: %c\n", i, fgetc(midi.file));
  }*/
  puts("******track 2*********");
  ptr = track2.head;
  while (ptr != NULL){
    //printf("type: 0x%X\n", ptr->type);
    if (ptr->type == EV_NOTE_ON && ptr->delta_time * conversion <= SDL_GetTicks() - ticks){
      fluid_synth_noteon(synth, 0, ((MIDIChannelEventData*)(ptr->data))->param1,
                                   ((MIDIChannelEventData*)(ptr->data))->param2);
      ptr = ptr->next;
      ticks = SDL_GetTicks();
    } else if (ptr->type == EV_NOTE_OFF && ptr->delta_time * conversion <= SDL_GetTicks() - ticks){
      fluid_synth_noteoff(synth, 0, ((MIDIChannelEventData*)(ptr->data))->param1);
      ptr = ptr->next;
      ticks = SDL_GetTicks();
    } else if (ptr->type != EV_NOTE_OFF && ptr->type != EV_NOTE_ON){
      ptr = ptr->next;
    }
  }

  SDL_Quit();

  fluid_synth_sfunload(synth, sfHandle, 0);
  delete_fluid_audio_driver(adriver);
  delete_fluid_synth(synth);
  delete_fluid_settings(settings);
  fclose(midi.file);
  return 0;
}
