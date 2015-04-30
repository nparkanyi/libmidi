miditest: miditest.c midi.c
	cc -std=c99 miditest.c midi.c `pkg-config --cflags glib-2.0` -g -lfluidsynth -lSDL2 -o miditest

vlvtest: vlvtest.c midi.c
	cc -std=c99 midi.c vlvtest.c `pkg-config --cflags glib-2.0` -o vlvtest

run: miditest
	./miditest ./s054.mid
