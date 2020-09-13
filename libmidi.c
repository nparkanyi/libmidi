/*
 * Copyright (c) 2014 Nicholas Parkanyi
 * See LICENSE
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "libmidi.h"

//convert big-endian data to little endian in-place, does nothing on BE host
void be_to_le(void* vdata, int bytes)
{
//NOTE: these preprocessor variables are gcc-specific!
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    char tmp;
    unsigned char* data = (unsigned char*)vdata;

    for (int i = 0; i < bytes / 2; i++){
        tmp = *(data + i);
        *(data + i) = *(data + bytes - 1 - i);
        *(data + bytes - 1 - i) = tmp;
    }
#endif
}

int VLV_read(FILE * buf, uint32_t * val, int * bytes_read)
{
  uint8_t byte;
  uint8_t a;
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


int MIDIFile_load(MIDIFile * midi, const char * filename)
{
  int r;

  midi->file = fopen(filename, "rb");
  if (!midi->file)
    return FILE_IO_ERROR;

  r = MIDIHeader_load(&midi->header, midi->file);
  if (r != SUCCESS)
    fclose(midi->file);

  return r;
}

void MIDIFile_delete(MIDIFile * midi)
{
    fclose(midi->file);
}

int MIDIHeader_load(MIDIHeader * header, FILE * file)
{
  int i;
  char const * name = "MThd";

  if (fread(&header->id, sizeof(uint8_t), 4, file) < 1)
    return FILE_INVALID;
  if (fread(&header->size, sizeof(uint32_t), 1, file) < 1)
    return FILE_INVALID;
  if (fread(&header->format, sizeof(uint16_t), 1, file) < 1)
    return FILE_INVALID;
  if (fread(&header->num_tracks, sizeof(uint16_t), 1, file) < 1)
    return FILE_INVALID;
  if (fread(&header->time_div, sizeof(uint16_t), 1, file) < 1)
    return FILE_INVALID;

  //swap endianness (does nothing if host is BE)
  be_to_le(&header->size, sizeof(uint32_t));
  be_to_le(&header->format, sizeof(uint16_t));
  be_to_le(&header->num_tracks, sizeof(uint16_t));
  be_to_le(&header->time_div, sizeof(uint16_t));

  //if id is not "MThd", not a MIDI file
  for (i = 0; i < 4; i++){
    if (header->id[i] != name[i])
      return FILE_INVALID;
  }

  return SUCCESS;
}


uint32_t MIDIHeader_getTempoConversion(MIDIHeader * header, uint32_t tempo)
{
  if ((header->time_div & 0x8000) == 0){
    //metrical timing
    return (uint32_t)(tempo //microseconds per quarter note
                      / (header->time_div & 0x7FFF)); //ticks per quarter note
  } else {
    return 1;
  }
}


MIDIEventList * MIDIEventList_create()
{
  MIDIEventList * ret = (MIDIEventList*)malloc(sizeof(MIDIEventList));

  ret->tail = NULL;
  ret->head = NULL;

  return ret;
}


MIDIEventIterator MIDIEventList_get_start_iter(MIDIEventList * list)
{
  MIDIEventIterator iter = { list->head, list };

  return iter;
}


MIDIEventIterator MIDIEventList_get_end_iter(MIDIEventList * list)
{
  MIDIEventIterator iter = { list->tail, list };

  return iter;
}


MIDIEventIterator MIDIEventList_next_event(MIDIEventIterator iter)
{
  MIDIEventIterator _new;

  if (iter.node->next){
    _new.node = iter.node->next;
    return _new;
  } else {
    return iter;
  }
}


MIDIEvent * MIDIEventList_get_event(MIDIEventIterator iter)
{
  return &(iter.node->ev);
}


bool MIDIEventList_is_end_iter(MIDIEventIterator iter)
{
  //return (iter.node == iter.list->tail);
  return !(iter.node->next);
}


int MIDIEventList_insert(MIDIEventList * list, MIDIEventIterator iter,
                         MIDIEvent ev)
{
  MIDIEventNode * _new = (MIDIEventNode*)malloc(sizeof(MIDIEventNode));

  if (!_new)
    return MEMORY_ERROR;

  _new->ev = ev;
  if (iter.node){
    _new->prev = iter.node;
    _new->next = iter.node->next;
    iter.node->next = _new;
    if (_new->prev == iter.list->tail)
      iter.list->tail = _new;
  } else {
    _new->next = list->head;
    list->head = _new;
    if (!list->tail)
      list->tail = _new;
  }
  return SUCCESS;
}


int MIDIEventList_append(MIDIEventList * list, MIDIEvent ev)
{
  return MIDIEventList_insert(list, MIDIEventList_get_end_iter(list), ev);
}


void MIDIEventList_delete(MIDIEventList * list)
{
  MIDIEventNode * tmp;

  if (!list) return;

  while (list->head){
    tmp = list->head->next;
    free(list->head->ev.data);
    free(list->head);
    list->head = tmp;
  }
  free(list);
}


int MIDITrack_load(MIDITrack * track, FILE * file)
{
  int i;
  char const * name = "MTrk";

  if (fread(&track->header.id, sizeof(uint8_t), 4, file) < 4)
	return FILE_IO_ERROR;
  if (fread(&track->header.size, sizeof(uint32_t), 1, file) < 1)
    return FILE_IO_ERROR;

  //swap endianness
  be_to_le(&track->header.size, sizeof(uint32_t));

  //if id is not "MTrk", not a track
  for (i = 0; i < 4; i++){
    if (track->header.id[i] != name[i]){
      return FILE_INVALID;
    }
  }

  track->list = MIDIEventList_create();
  if (!track->list)
    return MEMORY_ERROR;

  //MIDITrack_load_events(track, file);
  return MIDITrack_load_events(track, file);
}


int MIDITrack_skip(FILE * file)
{
    uint32_t size;
    unsigned char c;
    char const * name = "MTrk";
    char name_check[4];
    int i;

    if (fread(name_check, sizeof(uint8_t), 4, file) < 4)
        return FILE_IO_ERROR;
    for (i = 0; i < 4; i++){
        if (name_check[i] != name[i])
            return FILE_INVALID;
    }
    if (fread(&size, sizeof(uint32_t), 1, file) < 1)
        return FILE_IO_ERROR;
    be_to_le(&size, sizeof(uint32_t));
    
    if (fseek(file, size, SEEK_CUR) != 0)
        return FILE_INVALID;

    return SUCCESS;
}


float hour_byte_to_fps(uint8_t byte)
{
  uint8_t rate = byte >> 5; //remove all but the rate bits

  switch(rate){
    case 0:
      return 24.0f;
    case 1:
      return 25.0f;
    case 2:
      return 29.97f;
    case 3:
      return 30.0f;
    default:
      return 0.0f;
  }
}

int MIDITrack_load_events(MIDITrack * track, FILE * file)
{
  int bytes_read = 0;
  int vlv_read;
  uint32_t ev_delta_time;
  //if a channel event, type and channel # packed into one byte
  //otherwise, entire byte represents the type :{
  uint8_t ev_type_channel;
  uint8_t ev_type;
  uint8_t ev_channel;
  uint8_t param1, param2;
  const int size = track->header.size;
  uint32_t meta_size;
  int r;

  do {
    if (VLV_read(file, &ev_delta_time, &vlv_read) == VLV_ERROR)
      return FILE_IO_ERROR;
    if (fread(&ev_type_channel, sizeof(uint8_t), 1, file) < 1)
      return FILE_IO_ERROR;
    //meta events
    if (ev_type_channel == 0xFF){
      if (fread(&ev_type, sizeof(uint8_t), 1, file) < 1)
        return FILE_IO_ERROR;

      if (VLV_read(file, &meta_size, &vlv_read) == VLV_ERROR)
        return FILE_IO_ERROR;

      if (ev_type == META_END_TRACK){
        if (meta_size != 0)
          return FILE_INVALID;
        r = MIDITrack_add_meta_event(track, ev_delta_time, META_END_TRACK, NULL);
      } else if (ev_type == META_TEMPO_CHANGE){
        if (meta_size != 3)
          return FILE_INVALID;

        uint32_t * tempo = NULL;
        tempo = (uint32_t*)malloc(sizeof(uint32_t));
        if (!tempo) return MEMORY_ERROR;
        *(char*)(tempo) = 0;
        if (fread((char*)(tempo) + 1, sizeof(uint8_t), 3, file) < 1){
          free(tempo);
          return FILE_IO_ERROR;
        }
        be_to_le(tempo, sizeof(uint32_t));
        r = MIDITrack_add_meta_event(track, ev_delta_time, META_TEMPO_CHANGE, tempo);
      } else if (ev_type == META_SMPTE_OFFSET){
        if (meta_size != 5)
          return FILE_INVALID;

        SMPTEData * smpte = NULL;
        smpte = (SMPTEData*)malloc(sizeof(SMPTEData));
        if (!smpte) return MEMORY_ERROR;

        if (fread(&(smpte->hours), sizeof(uint8_t), 1, file) < 1)
          return FILE_INVALID;

        smpte->framerate = hour_byte_to_fps(smpte->hours);
        if (smpte->framerate == 0.0f){
          free(smpte);
          return FILE_INVALID;
        }

        if (fread(&(smpte->minutes), sizeof(uint8_t), 1, file) < 1){
          free(smpte);
          return FILE_INVALID;
        }

        if (fread(&(smpte->seconds), sizeof(uint8_t), 1, file) < 1){
          free(smpte);
          return FILE_INVALID;
        }

        if (fread(&(smpte->frames), sizeof(uint8_t), 1, file) < 1){
          free(smpte);
          return FILE_INVALID;
        }

        if (fread(&(smpte->subframes), sizeof(uint8_t), 1, file) < 1){
          free(smpte);
          return FILE_INVALID;
        }

        r = MIDITrack_add_meta_event(track, ev_delta_time, META_SMPTE_OFFSET, smpte);
      } else {
        //for ignored events, skip their data bytes
        if (fseek(file, meta_size, SEEK_CUR) != 0)
          return FILE_INVALID;
      }
      continue;
    //sysex events, ignore all these
    } else if (ev_type_channel == 0xF0 || ev_type_channel == 0xF7){
      if (VLV_read(file, &meta_size, &vlv_read) == VLV_ERROR)
        return FILE_IO_ERROR;
      if (fseek(file, meta_size, SEEK_CUR) != 0)
        return FILE_INVALID;
      continue;
    }

    //channel events
    /* if first bit not zero, this is new status byte
     * otherwise, apply running status, this is 1st parameter byte */
    if ((ev_type_channel & 0x80) != 0){
      ev_type = (ev_type_channel & 0xF0) >> 4;
      ev_channel = ev_type_channel & 0x0F;
      if (fread(&param1, sizeof(uint8_t), 1, file) < 1)
        return FILE_IO_ERROR;
    } else {
      param1 = ev_type_channel;
    }

    //channel events that only have 1 parameter
    if (ev_type == EV_PROGRAM_CHANGE ||
        ev_type == EV_CHANNEL_AFTERTOUCH){
      r = MIDITrack_add_channel_event(track, ev_type, ev_channel,
                                      ev_delta_time, param1, 0);
    } else {
      if (fread(&param2, sizeof(uint8_t), 1, file) < 1)
        return FILE_IO_ERROR;

      r = MIDITrack_add_channel_event(track, ev_type, ev_channel,
          ev_delta_time, param1,
          param2);
    }
    if (r != SUCCESS)
      return r;
  } while (ev_type != META_END_TRACK);
  return SUCCESS;
}


int MIDITrack_add_channel_event(MIDITrack * track,
                                 uint8_t type, uint8_t channel,
                                 uint32_t delta, uint8_t param1,
                                 uint8_t param2)
{
  MIDIEvent temp;
  int r;

  temp.type = (EventType)type;
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


int MIDITrack_add_meta_event(MIDITrack * track, uint32_t delta, MetaType type,
                             void * data)
{
  MIDIEvent temp;

  temp.type = (EventType)type;
  temp.delta_time = delta;
  temp.data = data;

  return MIDIEventList_append(track->list, temp);
}


void MIDITrack_delete_events(MIDITrack * track)
{
  MIDIEventList_delete(track->list);
}

unsigned long SMPTE_to_milliseconds(SMPTEData smpte)
{
  return 3600000 * smpte.hours
        + 60000 * smpte.minutes
        + 1000 * smpte.seconds
        + (1000.0f / smpte.framerate) * smpte.frames
        + (10.0f / smpte.framerate) * smpte.subframes;
}
