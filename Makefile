miditest: miditest.c midi.c
	cc miditest.c midi.c `pkg-config --cflags glib-2.0` -o miditest

vlvtest: vlvtest.c midi.c
	cc midi.c vlvtest.c `pkg-config --cflags glib-2.0` -o vlvtest
