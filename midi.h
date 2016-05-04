/*
 * Copyright (c) 2014 Nicholas Parkanyi
 * See LICENSE
*/
#ifndef MIDI_H
#define MIDI_H

#include <stdbool.h>
#include <stdio.h>
#include <glib.h>

enum errors{
  SUCCESS,
  FILE_IO_ERROR,
  FILE_INVALID,
  VLV_ERROR,
  MEMORY_ERROR
};

//channel events
typedef enum {
  EV_NOTE_OFF = 0x8,
  EV_NOTE_ON,
  EV_NOTE_AFTERTOUCH,
  EV_CONTROLLER,
  EV_PROGRAM_CHANGE,
  EV_CHANNEL_AFTERTOUCH,
  EV_PITCH_BEND,
  EV_META = 0xFF
} EventType;

//meta event types
typedef enum {
  META_SEQUENCE_NUM,
  META_TEXT,
  META_COPYRIGHT,
  META_NAME,
  META_INSTRUMENT_NAME,
  META_LYRICS,
  META_MARKER,
  META_CUE_POINT,
  META_CHANNEL_PREFIX=0x20,
  META_END_TRACK = 0x2F,
  META_TEMPO_CHANGE = 0x51,
  META_SMPTE_OFFSET = 0x54,
  META_TIME_SIGNATURE = 0x58,
  META_KEY_SIGNATURE,
  META_SEQUENCER_SPECIFIC = 0x7F
} MetaType;

typedef enum {
  CTR_BANK_SELECT,
  CTR_MODULATION,
  CTR_BREATH,
  CTR_FOOT,
  CTR_PORTAMENTO_TIME,
  CTR_DATA_ENTRY,
  CTR_VOLUME,
  CTR_BALANCE,
  CTR_PAN = 0x0A,
  CTR_EXPRESSION,
  CTR_EFFECT_1,
  CTR_EFFECT_2,
  CTR_GENERAL_1 = 0x10,
  CTR_GENERAL_2,
  CTR_GENERAL_3,
  CTR_GENERAL_4,
  //0x20 to 0x3F: LSB for controllers 0-31
  CTR_DAMPER = 0x40,
  CTR_PORTAMENTO,
  CTR_SOSTENUTO,
  CTR_SOFT,
  CTR_LEGATO_FOOTSWITCH,
  CTR_HOLD2,
  //0x46 to 0x4F: sound controllers
  CTR_GENERAL_5 = 0x50,
  CTR_GENERAL_6,
  CTR_GENERAL_7,
  CTR_GENERAL_8,
  CTR_PORTAMENTO_CONTROL,
  CTR_EFFECT_1_DEPTH = 0x5B,
  CTR_EFFECT_2_DEPTH,
  CTR_EFFECT_3_DEPTH,
  CTR_EFFECT_4_DEPTH,
  CTR_EFFECT_5_DEPTH,
  CTR_DATA_INCR,
  CTR_DATA_DECR,
  CTR_NON_REG_PARAM_LSB,
  CTR_NON_REG_PARAM_MSB,
  CTR_REG_PARAM_LSB,
  CTR_REG_PARAM_MSB
  //0x79 to 0x7F: mode messages
} ControllerType;

typedef struct {
  guint8 id[4];
  guint32 size;
  guint16 format;
  guint16 num_tracks;
  guint16 time_div;
} MIDIHeader;

typedef struct {
  EventType type;
  guint32 delta_time;
  /* pointer to struct of type determined by event type
   * for ghetto polymorphism */
  void * data;
} MIDIEvent;

struct _MIDIEventNode {
  MIDIEvent ev;
  struct _MIDIEventNode * next;
  struct _MIDIEventNode * prev;
};
typedef struct _MIDIEventNode MIDIEventNode;

typedef struct {
  MIDIEventNode * head;
  MIDIEventNode * tail;
} MIDIEventList;

typedef struct {
  MIDIEventNode * node;
  MIDIEventList * list;
} MIDIEventIterator;

typedef struct {
  guint8 channel;
  guint8 param1;
  guint8 param2;
} MIDIChannelEventData;

typedef struct {
  guint8 id[4];
  guint32 size;
} MIDITrackHeader;

typedef struct {
  MIDITrackHeader header;
  MIDIEventList * list;
} MIDITrack;

typedef struct {
  FILE * file;
  MIDIHeader header;
} MIDIFile;

/* read Variable Length Value used by some MIDI values into val
 * never larger than 4 bytes
 * returns VLV_ERROR if fails
 * set bytes_read to NULL if you don't need it */
int VLV_read(FILE * buf, guint32 * val, int * bytes_read);

int MIDIFile_load(MIDIFile * midi, const char * filename);

int MIDIHeader_load(MIDIHeader * header, FILE * file);
//returns a factor that converts delta times to milliseconds,
// tempo in microseconds per quarter note (will be ignored if using timecodes).
// You must get a new conversion factor after any tempo change event.
float MIDIHeader_getTempoConversion(MIDIHeader * header, guint32 tempo);

MIDIEventList * MIDIEventList_create();
MIDIEventIterator MIDIEventList_get_start_iter(MIDIEventList * list);
MIDIEventIterator MIDIEventList_get_end_iter(MIDIEventList * list);
MIDIEventIterator MIDIEventList_next_event(MIDIEventIterator iter);
MIDIEvent * MIDIEventList_get_event(MIDIEventIterator iter);
bool MIDIEventList_is_end_iter(MIDIEventIterator iter);
/* inserts new event after the given iterator
 * set iter.node to NULL to insert at the very front */
int MIDIEventList_insert(MIDIEventList * list, MIDIEventIterator iter,
                         MIDIEvent ev);
int MIDIEventList_append(MIDIEventList * list, MIDIEvent ev);
void MIDIEventList_delete(MIDIEventList * list);

int MIDITrack_load(MIDITrack * track, FILE * file);
int MIDITrack_load_events(MIDITrack * track, FILE * file);
int MIDITrack_add_channel_event(MIDITrack * track,
                                guint8 type, guint8 channel,
                                guint32 delta, guint8 param1,
                                guint8 param2);

int MIDITrack_add_end_track_event(MIDITrack * track, guint32 delta);
int MIDITrack_add_tempo_change_event(MIDITrack * track, guint32 delta_time,
                                     guint32 tempo);
void MIDITrack_delete_events(MIDITrack * track);

#endif
