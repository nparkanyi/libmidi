/*
 * Copyright (c) 2014 Nicholas Parkanyi
 * See LICENSE
*/
#include <stdio.h>
#include <stdlib.h>
#include "midi.h"

int VLV_read(FILE * buf, guint32 * val, int * bytes_read){
  guint8 byte;
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


int MIDIFile_load(MIDIFile * midi, const guint8 * filename){
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
  guint8 * name = "MThd";

  if (fread(&header->id, sizeof(guint8), 4, file) < 1)
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

MIDIEventList * MIDIEventList_create(){
  MIDIEventList * ret = malloc(sizeof(MIDIEventList));

  ret->tail = NULL;
  ret->head = NULL;

  return ret;
}

MIDIEventIterator MIDIEventList_get_start_iter(MIDIEventList * list){
  MIDIEventIterator iter = { list->head, list };

  return iter;
}

MIDIEventIterator MIDIEventList_get_end_iter(MIDIEventList * list){
  MIDIEventIterator iter = { list->tail, list };

  return iter;
}

MIDIEventIterator MIDIEventList_next_event(MIDIEventIterator iter){
  MIDIEventIterator new;

  if (iter.node->next){
    new.node = iter.node->next;
    return new;
  } else {
    return iter;
  }
}

MIDIEvent * MIDIEventList_get_event(MIDIEventIterator iter){
  return &(iter.node->ev);
}

bool MIDIEventList_is_end_iter(MIDIEventIterator iter){
  //return (iter.node == iter.list->tail);
  return !(iter.node->next);
}

int MIDIEventList_insert(MIDIEventList * list, MIDIEventIterator iter,
                         MIDIEvent ev){
  MIDIEventNode * new = malloc(sizeof(MIDIEventNode));

  if (!new)
    return MEMORY_ERROR;

  new->ev = ev;
  if (iter.node){
    new->prev = iter.node;
    new->next = iter.node->next;
    iter.node->next = new;
    if (new->prev == iter.list->tail)
      iter.list->tail = new;
  } else {
    new->next = list->head;
    list->head = new;
    if (!list->tail)
      list->tail = new;
  }
  return SUCCESS;
}

int MIDIEventList_append(MIDIEventList * list, MIDIEvent ev){
  return MIDIEventList_insert(list, MIDIEventList_get_end_iter(list), ev);
}

void MIDIEventList_delete(MIDIEventList * list){
  MIDIEventNode * tmp;

  while (list->head){
    tmp = list->head->next;
    free(list->head);
    list->head = tmp;
  }
  free(list);
}

int MIDITrack_load(MIDITrack * track, FILE * file){
  int i;
  guint8 * name = "MTrk";

  if (fread(&track->header.id, sizeof(guint8), 4, file) < 1)
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

  track->list = MIDIEventList_create();
  if (!track->list)
    return MEMORY_ERROR;

  //MIDITrack_load_events(track, file);
  return MIDITrack_load_events(track, file);
}

int MIDITrack_load_events(MIDITrack * track, FILE * file){
  int bytes_read = 0;
  int vlv_read;
  guint32 ev_delta_time;
  //if a channel event, type and channel # packed into one byte
  //otherwise, entire byte represents the type :{
  guint8 ev_type_channel;
  guint8 ev_type;
  guint8 ev_channel;
  guint8 param1, param2;
  const int size = track->header.size;
  guint32 skip_bytes;
  int r;

  do {
    if (VLV_read(file, &ev_delta_time, &vlv_read) == VLV_ERROR)
      return FILE_IO_ERROR;

    if (fread(&ev_type_channel, sizeof(guint8), 1, file) < 1)
      return FILE_IO_ERROR;

    //sys and meta events, ignoring these for now, except for END_TRACK
    if (ev_type_channel == 0xF0 ||
        ev_type_channel == 0xFF){
      if (fread(&ev_type, sizeof(guint8), 1, file) < 1)
        return FILE_IO_ERROR;

      if (ev_type == META_END_TRACK){
        r = MIDITrack_add_end_track_event(track, ev_delta_time);
      }

      if (VLV_read(file, &skip_bytes, &vlv_read) == VLV_ERROR)
        return FILE_IO_ERROR;
      if (fseek(file, skip_bytes, SEEK_CUR) != 0)
        return FILE_INVALID;
      continue;
    }

    //channel events
    /* if first bit not zero, this is new status byte
     * otherwise, apply running status, this is 1st parameter byte */
    if ((ev_type_channel & 0x80) != 0){
      ev_type = (ev_type_channel & 0xF0) >> 4;
      ev_channel = ev_type_channel & 0x0F;
      if (fread(&param1, sizeof(guint8), 1, file) < 1)
        return FILE_IO_ERROR;
    } else {
      param1 = ev_type_channel;
    }

    if (fread(&param2, sizeof(guint8), 1, file) < 1)
      return FILE_IO_ERROR;

    r = MIDITrack_add_channel_event(track, ev_type, ev_channel,
                                            ev_delta_time, param1,
                                            param2);
    if (r != SUCCESS)
      return r;
  } while (ev_type != META_END_TRACK);
  return SUCCESS;
}

int MIDITrack_add_channel_event(MIDITrack * track,
                                 guint8 type, guint8 channel,
                                 guint32 delta, guint8 param1,
                                 guint8 param2){
  MIDIEvent temp;
  int r;

  temp.type = type;
  temp.delta_time = delta;

  temp.data = malloc(sizeof(MIDIChannelEventData));
  if (!temp.data){
    return MEMORY_ERROR;
  }

  ((MIDIChannelEventData*)(temp.data))->channel = channel;
  ((MIDIChannelEventData*)(temp.data))->param1 = param1;
  ((MIDIChannelEventData*)(temp.data))->param2 = param2;

  return MIDIEventList_append(track->list, temp);
}

int MIDITrack_add_end_track_event(MIDITrack * track, guint32 delta){
  MIDIEvent temp;

  temp.type = META_END_TRACK;
  temp.delta_time = delta;

  temp.data = NULL;

  return MIDIEventList_append(track->list, temp);
}

void MIDITrack_delete_events(MIDITrack * track){
  MIDIEventList_delete(track->list);
}

