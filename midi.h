/*
 * Copyright (c) 2014 Nicholas Parkanyi
 * See LICENSE
*/
#ifndef MIDI_H
#define MIDI_H

#include <glib.h>

enum errors{
  SUCCESS,
  FILE_IO_ERROR,
  FILE_INVALID,
  VLV_ERROR
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
  META_END_TRACK = 0x2F
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
  char id[4];
  guint32 size;
  guint16 format;
  guint16 num_tracks;
  guint16 time_div;
} MIDIHeader;

struct _MIDIEvent{
  EventType type;
  guint32 delta_time;
  struct _MIDIEvent * next;
  //pointer to struct of type determined by event type
  //for ghetto polymorphism 
  char * data;
}; 
typedef struct _MIDIEvent MIDIEvent;

typedef struct {
  char id[4];
  guint32 size;
} MIDITrackHeader;

typedef struct {
  MIDITrackHeader header;
  //track's events stored as linked list
  MIDIEvent * head;
} MIDITrack;

typedef struct {
  FILE * file;
  MIDIHeader header;
} MIDIFile;

//read in Variable Length Value used by some MIDI values
//never larger than 4 bytes
int VLV_read(FILE * buf, guint32 * val);

int MIDIFile_load(MIDIFile * midi, const char * filename);

int MIDIHeader_load(MIDIHeader * header, FILE * file);

int MIDITrack_load(MIDITrack * track, FILE * file);

#endif
