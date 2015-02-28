/*
 * Copyright (c) 2014 Nicholas Parkanyi
 * See LICENSE
*/
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "midi.h"

int VLV_read(FILE * buf, guint32 * val, int * bytes_read){
  char byte;
  int i;
  
  *val = 0x00;

  for (i = 0; i < 4; i++){
    if (fread(&byte, 1, 1, buf) != 1)
      return FILE_IO_ERROR;

    *val = ((*val << 7) | (byte % 128));

    if ((byte & 0x80) == 0x00){
      if (bytes_read != NULL)
        *bytes_read = i + 1;
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

  if (fread(&header->id, sizeof(char), 4, file) < 1)
    return FILE_INVALID;
  if (fread(&header->size, sizeof(guint32), 1, file) < 1)
    return FILE_INVALID;
  if (fread(&header->format, sizeof(guint16), 1, file) < 1)
    return FILE_INVALID;
  if (fread(&header->num_tracks, sizeof(guint16), 1, file) < 1)
    return FILE_INVALID;
  if (fread(&header->time_div, sizeof(guint16), 1, file) < 1)
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

int MIDITrack_load(MIDITrack * track, FILE * file){
  int i;
  //int bytes_read = 0;
  char * name = "MTrk";

  track->head = NULL;
  track->tail = NULL;
  
  if (fread(&track->header.id, sizeof(char), 4, file) < 1)
    return FILE_IO_ERROR;
  if (fread(&track->header.size, sizeof(guint32), 1, file) < 1)
    return FILE_IO_ERROR;

  //swap endianness
  track->header.size = GUINT32_FROM_BE(track->header.size);
  
  //if id is not "MTrk", not a track
  for (i = 0; i < 4; i++){
      if (track->header.id[i] != name[i]){
        printf("MTrk not found\n");
        return FILE_INVALID;
      }
  }

  //MIDITrack_load_events(track, file);
  return MIDITrack_load_events(track, file);
}

int MIDITrack_load_events(MIDITrack * track, FILE * file){
  int bytes_read = 0;
  int vlv_read;
  guint32 ev_delta_time;
  //if a channel event, type and channel # packed into one byte
  //otherwise, entire byte represents the type :{
  char ev_type_channel;
  char ev_type;
  char ev_channel;
  const int size = track->header.size;
  guint32 skip_bytes;
  int r;

  do {
    assert(bytes_read < size);
    //printf("bytes_read: %d\n", bytes_read);
    if (VLV_read(file, &ev_delta_time, &vlv_read) == VLV_ERROR)
      return FILE_IO_ERROR;
    bytes_read += vlv_read;
    printf("VLV delta time: %d\n", ev_delta_time);

    if (fread(&ev_type_channel, sizeof(char), 1, file) < 1)
      return FILE_IO_ERROR;
    bytes_read++;

    //sys and meta events, ignoring these for now
    if (ev_type_channel == 0xF0 ||
        ev_type_channel == 0xFF){
      if (fread(&ev_type, sizeof(char), 1, file) < 1)
        return FILE_IO_ERROR;
      printf("meta event type: %x\n", ev_type);

      if(VLV_read(file, &skip_bytes, &vlv_read) == VLV_ERROR)
        return FILE_IO_ERROR;
      printf("skip_bytes: %d\n", skip_bytes);
      bytes_read += vlv_read;
      if(fseek(file, skip_bytes, SEEK_CUR) != 0)
        return FILE_INVALID;
      bytes_read += skip_bytes;
    } 

    //channel events
    ev_type = (ev_type_channel & 0xF0) >> 4;
    ev_channel = ev_type_channel & 0x0F;
    //printf("ev_type: %X\n", ev_type);
    r = MIDITrack_load_channel_event(track, &bytes_read, 
                                     ev_type, ev_channel,
                                     ev_delta_time, file);
    if (r != SUCCESS)
      return r;

//    assert(bytes_read < size);
  } while (bytes_read < size);
  printf("bytes_read: %d\n", bytes_read);
  return SUCCESS;
}

int MIDITrack_load_channel_event(MIDITrack * track, int * bytes_read,
                                 char type, char channel,
                                 guint32 delta, FILE * file){
  MIDIEvent * temp = NULL;
  MIDIChannelEventData * data = NULL;
  int r;

  temp = (MIDIEvent *)malloc(sizeof(MIDIEvent));
  if (temp == NULL)
    return MEMORY_ERROR;

  temp->type = type;
  temp->delta_time = delta;

  data = (MIDIChannelEventData *)malloc(sizeof(MIDIChannelEventData));
  if (data == NULL){
    r = MEMORY_ERROR;
    goto fail2;
  }

  data->channel = channel;
  if (fread(&data->param1, sizeof(char), 1, file) < 1){
    r = FILE_IO_ERROR;
    goto fail1;
  }
  //printf("read param1: %d\n", data->param1);
  (*bytes_read)++;

  switch (type){
    case EV_NOTE_ON:
    case EV_NOTE_OFF:
    case EV_NOTE_AFTERTOUCH:
    case EV_CONTROLLER:
    case EV_PITCH_BEND:
      if (fread(&data->param2, sizeof(char), 1, file) < 1){
        r = FILE_IO_ERROR;
        goto fail1;
      }
      //printf("read param2: %d\n", data->param2);
      (*bytes_read)++;
      break;
    case EV_PROGRAM_CHANGE:
    case EV_CHANNEL_AFTERTOUCH:
      data->param2 = 0;
      break;
  }

  temp->data = (void *)data;
  temp->next = NULL;
  //add the event to the track's linked list
  if (track->head == NULL){
    track->head = temp;
    track->tail = temp;
  } else {
    temp->prev = track->tail;
    track->tail->next = temp;
    track->tail = temp;
  }
  return SUCCESS;

fail1:
  free(data);
fail2:
  free(temp);
  return r;
}

void MIDITrack_delete_events(MIDITrack * track){
  MIDIEvent * temp = track->head;

  while (track->head != NULL){
    temp = track->head;
    track->head = track->head->next;
    free(temp->data);
    free(temp);
  }
  track->tail = NULL;
}
