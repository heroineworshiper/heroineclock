// convert a midi file into clock data
// copied from aplaymidi.c


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


/** Sequencer event type */
enum snd_seq_event_type {
	/** system status; event data type = #snd_seq_result_t */
	SND_SEQ_EVENT_SYSTEM = 0,
	/** returned result status; event data type = #snd_seq_result_t */
	SND_SEQ_EVENT_RESULT,

	/** note on and off with duration; event data type = #snd_seq_ev_note_t */
	SND_SEQ_EVENT_NOTE = 5,
	/** note on; event data type = #snd_seq_ev_note_t */
	SND_SEQ_EVENT_NOTEON,
	/** note off; event data type = #snd_seq_ev_note_t */
	SND_SEQ_EVENT_NOTEOFF,
	/** key pressure change (aftertouch); event data type = #snd_seq_ev_note_t */
	SND_SEQ_EVENT_KEYPRESS,
	
	/** controller; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_CONTROLLER = 10,
	/** program change; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_PGMCHANGE,
	/** channel pressure; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_CHANPRESS,
	/** pitchwheel; event data type = #snd_seq_ev_ctrl_t; data is from -8192 to 8191) */
	SND_SEQ_EVENT_PITCHBEND,
	/** 14 bit controller value; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_CONTROL14,
	/** 14 bit NRPN;  event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_NONREGPARAM,
	/** 14 bit RPN; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_REGPARAM,

	/** SPP with LSB and MSB values; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_SONGPOS = 20,
	/** Song Select with song ID number; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_SONGSEL,
	/** midi time code quarter frame; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_QFRAME,
	/** SMF Time Signature event; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_TIMESIGN,
	/** SMF Key Signature event; event data type = #snd_seq_ev_ctrl_t */
	SND_SEQ_EVENT_KEYSIGN,
	        
	/** MIDI Real Time Start message; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_START = 30,
	/** MIDI Real Time Continue message; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_CONTINUE,
	/** MIDI Real Time Stop message; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_STOP,
	/** Set tick queue position; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_SETPOS_TICK,
	/** Set real-time queue position; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_SETPOS_TIME,
	/** (SMF) Tempo event; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_TEMPO,
	/** MIDI Real Time Clock message; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_CLOCK,
	/** MIDI Real Time Tick message; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_TICK,
	/** Queue timer skew; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_QUEUE_SKEW,
	/** Sync position changed; event data type = #snd_seq_ev_queue_control_t */
	SND_SEQ_EVENT_SYNC_POS,

	/** Tune request; event data type = none */
	SND_SEQ_EVENT_TUNE_REQUEST = 40,
	/** Reset to power-on state; event data type = none */
	SND_SEQ_EVENT_RESET,
	/** Active sensing event; event data type = none */
	SND_SEQ_EVENT_SENSING,

	/** Echo-back event; event data type = any type */
	SND_SEQ_EVENT_ECHO = 50,
	/** OSS emulation raw event; event data type = any type */
	SND_SEQ_EVENT_OSS,

	/** New client has connected; event data type = #snd_seq_addr_t */
	SND_SEQ_EVENT_CLIENT_START = 60,
	/** Client has left the system; event data type = #snd_seq_addr_t */
	SND_SEQ_EVENT_CLIENT_EXIT,
	/** Client status/info has changed; event data type = #snd_seq_addr_t */
	SND_SEQ_EVENT_CLIENT_CHANGE,
	/** New port was created; event data type = #snd_seq_addr_t */
	SND_SEQ_EVENT_PORT_START,
	/** Port was deleted from system; event data type = #snd_seq_addr_t */
	SND_SEQ_EVENT_PORT_EXIT,
	/** Port status/info has changed; event data type = #snd_seq_addr_t */
	SND_SEQ_EVENT_PORT_CHANGE,

	/** Ports connected; event data type = #snd_seq_connect_t */
	SND_SEQ_EVENT_PORT_SUBSCRIBED,
	/** Ports disconnected; event data type = #snd_seq_connect_t */
	SND_SEQ_EVENT_PORT_UNSUBSCRIBED,

	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR0 = 90,
	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR1,
	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR2,
	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR3,
	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR4,
	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR5,
	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR6,
	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR7,
	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR8,
	/** user-defined event; event data type = any (fixed size) */
	SND_SEQ_EVENT_USR9,

	/** system exclusive data (variable length);  event data type = #snd_seq_ev_ext_t */
	SND_SEQ_EVENT_SYSEX = 130,
	/** error event;  event data type = #snd_seq_ev_ext_t */
	SND_SEQ_EVENT_BOUNCE,
	/** reserved for user apps;  event data type = #snd_seq_ev_ext_t */
	SND_SEQ_EVENT_USR_VAR0 = 135,
	/** reserved for user apps; event data type = #snd_seq_ev_ext_t */
	SND_SEQ_EVENT_USR_VAR1,
	/** reserved for user apps; event data type = #snd_seq_ev_ext_t */
	SND_SEQ_EVENT_USR_VAR2,
	/** reserved for user apps; event data type = #snd_seq_ev_ext_t */
	SND_SEQ_EVENT_USR_VAR3,
	/** reserved for user apps; event data type = #snd_seq_ev_ext_t */
	SND_SEQ_EVENT_USR_VAR4,

	/** NOP; ignored in any case */
	SND_SEQ_EVENT_NONE = 255
};

/*
 * A MIDI event after being parsed/loaded from the file.
 * There could be made a case for using snd_seq_event_t instead.
 */
struct event {
	struct event *next;		/* linked list */

	unsigned char type;		/* SND_SEQ_EVENT_xxx */
	unsigned char port;		/* port index */
	unsigned int tick;
	union {
		unsigned char d[3];	/* channel and data bytes */
		int tempo;
		unsigned int length;	/* length of sysex data */
	} data;
	unsigned char sysex[0];
};

struct track {
	struct event *first_event;	/* list of all events in this track */
	int end_tick;			/* length of this track */

	struct event *current_event;	/* used while loading and playing */
};

FILE *file = 0;
char *file_name = 0;
int file_offset = 0;
int num_tracks = 0;
struct track *tracks;
int smpte_timing;
int port_count = 16;




/* prints an error message to stdout */
static void errormsg(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vfprintf(stdout, msg, ap);
	va_end(ap);
	fputc('\n', stdout);
}


/* memory allocation error handling */
static void check_mem(void *p)
{
	if (!p)
		printf("Out of memory\n");
}


static int read_byte(void)
{
	++file_offset;
	return getc(file);
}

/* reads a little-endian 32-bit integer */
static int read_32_le(void)
{
	int value;
	value = read_byte();
	value |= read_byte() << 8;
	value |= read_byte() << 16;
	value |= read_byte() << 24;
	return !feof(file) ? value : -1;
}

/* reads a 4-character identifier */
static int read_id(void)
{
	return read_32_le();
}

#define MAKE_ID(c1, c2, c3, c4) ((c1) | ((c2) << 8) | ((c3) << 16) | ((c4) << 24))



/* reads a fixed-size big-endian number */
static int read_int(int bytes)
{
	int c, value = 0;

	do {
		c = read_byte();
		if (c == EOF)
			return -1;
		value = (value << 8) | c;
	} while (--bytes);
	return value;
}


/* reads a variable-length number */
static int read_var(void)
{
	int value, c;

	c = read_byte();
	value = c & 0x7f;
	if (c & 0x80) {
		c = read_byte();
		value = (value << 7) | (c & 0x7f);
		if (c & 0x80) {
			c = read_byte();
			value = (value << 7) | (c & 0x7f);
			if (c & 0x80) {
				c = read_byte();
				value = (value << 7) | c;
				if (c & 0x80)
					return -1;
			}
		}
	}
	return !feof(file) ? value : -1;
}

/* allocates a new event */
static struct event *new_event(struct track *track, int sysex_length)
{
	struct event *event;

	event = malloc(sizeof(struct event) + sysex_length);
	check_mem(event);

	event->next = NULL;

	/* append at the end of the track's linked list */
	if (track->current_event)
		track->current_event->next = event;
	else
		track->first_event = event;
	track->current_event = event;

	return event;
}

static void skip(int bytes)
{
	while (bytes > 0)
		read_byte(), --bytes;
}

/* reads one complete track from the file */
static int read_track(struct track *track, int track_end)
{
	int tick = 0;
	unsigned char last_cmd = 0;
	unsigned char port = 0;

	/* the current file position is after the track ID and length */
	while (file_offset < track_end) {
		unsigned char cmd;
		struct event *event;
		int delta_ticks, len, c;

		delta_ticks = read_var();
		if (delta_ticks < 0)
			break;
		tick += delta_ticks;

		c = read_byte();
		if (c < 0)
			break;

		if (c & 0x80) {
			/* have command */
			cmd = c;
			if (cmd < 0xf0)
				last_cmd = cmd;
		} else {
			/* running status */
			ungetc(c, file);
			file_offset--;
			cmd = last_cmd;
			if (!cmd)
				goto _error;
		}

		switch (cmd >> 4) {
			/* maps SMF events to ALSA sequencer events */
			static const unsigned char cmd_type[] = {
				[0x8] = SND_SEQ_EVENT_NOTEOFF,
				[0x9] = SND_SEQ_EVENT_NOTEON,
				[0xa] = SND_SEQ_EVENT_KEYPRESS,
				[0xb] = SND_SEQ_EVENT_CONTROLLER,
				[0xc] = SND_SEQ_EVENT_PGMCHANGE,
				[0xd] = SND_SEQ_EVENT_CHANPRESS,
				[0xe] = SND_SEQ_EVENT_PITCHBEND
			};

		case 0x8: /* channel msg with 2 parameter bytes */
		case 0x9:
		case 0xa:
		case 0xb:
		case 0xe:
			event = new_event(track, 0);
			event->type = cmd_type[cmd >> 4];
			event->port = port;
			event->tick = tick;
			event->data.d[0] = cmd & 0x0f;
			event->data.d[1] = read_byte() & 0x7f;
			event->data.d[2] = read_byte() & 0x7f;
			break;

		case 0xc: /* channel msg with 1 parameter byte */
		case 0xd:
			event = new_event(track, 0);
			event->type = cmd_type[cmd >> 4];
			event->port = port;
			event->tick = tick;
			event->data.d[0] = cmd & 0x0f;
			event->data.d[1] = read_byte() & 0x7f;
			break;

		case 0xf:
			switch (cmd) {
			case 0xf0: /* sysex */
			case 0xf7: /* continued sysex, or escaped commands */
				len = read_var();
				if (len < 0)
					goto _error;
				if (cmd == 0xf0)
					++len;
				event = new_event(track, len);
				event->type = SND_SEQ_EVENT_SYSEX;
				event->port = port;
				event->tick = tick;
				event->data.length = len;
				if (cmd == 0xf0) {
					event->sysex[0] = 0xf0;
					c = 1;
				} else {
					c = 0;
				}
				for (; c < len; ++c)
					event->sysex[c] = read_byte();
				break;

			case 0xff: /* meta event */
				c = read_byte();
				len = read_var();
				if (len < 0)
					goto _error;

				switch (c) {
				case 0x21: /* port number */
					if (len < 1)
						goto _error;
					port = read_byte() % port_count;
					skip(len - 1);
					break;

				case 0x2f: /* end of track */
					track->end_tick = tick;
					skip(track_end - file_offset);
					return 1;

				case 0x51: /* tempo */
					if (len < 3)
						goto _error;
					if (smpte_timing) {
						/* SMPTE timing doesn't change */
						skip(len);
					} else {
						event = new_event(track, 0);
						event->type = SND_SEQ_EVENT_TEMPO;
						event->port = port;
						event->tick = tick;
						event->data.tempo = read_byte() << 16;
						event->data.tempo |= read_byte() << 8;
						event->data.tempo |= read_byte();
						skip(len - 3);
					}
					break;

				default: /* ignore all other meta events */
					skip(len);
					break;
				}
				break;

			default: /* invalid Fx command */
				goto _error;
			}
			break;

		default: /* cannot happen */
			goto _error;
		}
	}
_error:
	errormsg("%s: invalid MIDI data (offset %#x)", file_name, file_offset);
	return 0;
}


/* reads an entire MIDI file */
static int read_smf(void)
{
	int header_len, type, time_division, i, err;

	/* the curren position is immediately after the "MThd" id */
	header_len = read_int(4);
	if (header_len < 6) {
invalid_format:
		errormsg("%s: invalid file format", file_name);
		return 0;
	}

	type = read_int(2);
	if (type != 0 && type != 1) {
		errormsg("%s: type %d format is not supported", file_name, type);
		return 0;
	}

	num_tracks = read_int(2);
	if (num_tracks < 1 || num_tracks > 1000) {
		errormsg("%s: invalid number of tracks (%d)", file_name, num_tracks);
		num_tracks = 0;
		return 0;
	}
	tracks = calloc(num_tracks, sizeof(struct track));
	if (!tracks) {
		errormsg("out of memory");
		num_tracks = 0;
		return 0;
	}

	time_division = read_int(2);
	if (time_division < 0)
		goto invalid_format;

	/* interpret and set tempo */
	smpte_timing = !!(time_division & 0x8000);
	if (!smpte_timing) {
	} else {
		/* upper byte is negative frames per second */
		i = 0x80 - ((time_division >> 8) & 0x7f);
		/* lower byte is ticks per frame */
		time_division &= 0xff;
		/* now pretend that we have quarter-note based timing */
		switch (i) {
		case 24:
			break;
		case 25:
			break;
		case 29: /* 30 drop-frame */
			break;
		case 30:
			break;
		default:
			errormsg("%s: invalid number of SMPTE frames per second (%d)",
				 file_name, i);
			return 0;
		}
	}


	/* read tracks */
	for (i = 0; i < num_tracks; ++i) {
		int len;

		/* search for MTrk chunk */
		for (;;) {
			int id = read_id();
			len = read_int(4);
			if (feof(file)) {
				errormsg("%s: unexpected end of file", file_name);
				return 0;
			}
			if (len < 0 || len >= 0x10000000) {
				errormsg("%s: invalid chunk length %d", file_name, len);
				return 0;
			}
			if (id == MAKE_ID('M', 'T', 'r', 'k'))
				break;
			skip(len);
		}
		if (!read_track(&tracks[i], file_offset + len))
			return 0;
	}
	return 1;
}

static int read_riff(void)
{
	/* skip file length */
	read_byte();
	read_byte();
	read_byte();
	read_byte();

	/* check file type ("RMID" = RIFF MIDI) */
	if (read_id() != MAKE_ID('R', 'M', 'I', 'D')) {
invalid_format:
		errormsg("%s: invalid file format", file_name);
		return 0;
	}
	/* search for "data" chunk */
	for (;;) {
		int id = read_id();
		int len = read_32_le();
		if (feof(file)) {
data_not_found:
			errormsg("%s: data chunk not found", file_name);
			return 0;
		}
		if (id == MAKE_ID('d', 'a', 't', 'a'))
			break;
		if (len < 0)
			goto data_not_found;
		skip((len + 1) & ~1);
	}
	/* the "data" chunk must contain data in SMF format */
	if (read_id() != MAKE_ID('M', 'T', 'h', 'd'))
		goto invalid_format;
	return read_smf();
}

// oscillator states
typedef struct
{
// MIDI freq index
	int freq;
// MIDI tick when it turned on
	int time;
	int on;
} osc_t;

#define OSCS 3
// clock HZ
#define HZ 250
#define AUDIO_PERIOD 0xff
#define MAX_VOLUME (AUDIO_PERIOD / 2)




osc_t osc[OSCS];

void dump()
{
	printf("tracks: %d\n", num_tracks);
	int i;
	for(i = 0; i < num_tracks; i++)
	{
		struct track *track = &tracks[i];
		struct event *e = track->first_event;
		
		printf("  track %d:\n", i);
		while(e)
		{
			printf("    event=%d port=%d tick=%d", e->type, e->port, e->tick);
			switch(e->type)
			{
				case SND_SEQ_EVENT_NOTEON:
					printf(" NOTE ON %02x %02x %02x\n", e->data.d[0], e->data.d[1], e->data.d[2]);
					break;
				case SND_SEQ_EVENT_NOTEOFF:
					printf(" NOTE OFF %02x %02x %02x\n", e->data.d[0], e->data.d[1], e->data.d[2]);
					break;
				default:
					printf("\n");
					break;
			}
			e = e->next;
		}
		
	}
}

// convert MIDI freq index to clock freq index
int freq_to_clock(int note)
{
	int result = note - 48;
	if(result < 0)
	{
		result = 0;
	}
	else
	if(result > 36)
	{
		result = 36;
	}
	return result;
}

// convert MIDI ticks to clock HZ / 10
int tick_to_clock(int tick)
{
	return tick * (HZ / 10) / 1000;
}

void convert()
{
	int i, j;
// tick of last event
	int last_tick = 0;
	int got_it = 0;
	for(i = 0; i < num_tracks; i++)
	{
		struct track *track = &tracks[i];
		struct event *e = track->first_event;
		
		while(e)
		{
			int freq = e->data.d[1];

			switch(e->type)
			{
				case SND_SEQ_EVENT_NOTEON:
					got_it = 0;
// update oscillator using same frequency
					for(j = 0; j < OSCS; j++)
					{
						if(osc[j].on && osc[j].freq == freq)
						{
							osc[j].freq = freq;
							osc[j].time = e->tick;
// generate command to restart oscillator
							printf("    { %d, %d, %d, MAX_VOLUME / 3 },\n", 
								tick_to_clock(e->tick - last_tick), 
								j,
								freq_to_clock(freq));
							last_tick = e->tick;
							got_it = 1;
							break;
						}
					}
					
// take unused oscillator
					if(!got_it)
					{
						for(j = 0; j < OSCS; j++)
						{
							if(!osc[j].on)
							{
								osc[j].on = 1;
								osc[j].freq = freq;
								osc[j].time = e->tick;
// generate command to change oscillator
								printf("    { %d, %d, %d, MAX_VOLUME / 3 },\n", 
									tick_to_clock(e->tick - last_tick), 
									j,
									freq_to_clock(freq));
								last_tick = e->tick;
								got_it = 1;
								break;
							}
						}
					}
					
// overwrite oldest oscillator
					if(!got_it)
					{
						int oldest = -1;
						int oldest_time = -1;
						for(j = 0; j < OSCS; j++)
						{
							if(oldest < 0 || osc[j].time < oldest_time)
							{
								oldest = j;
								oldest_time = osc[j].time;
							}
						}
						
						osc[oldest].freq = freq;
						osc[oldest].time = e->tick;
// generate command to change oscillator
						printf("    { %d, %d, %d, MAX_VOLUME / 3 },\n", 
							tick_to_clock(e->tick - last_tick), 
							oldest,
							freq_to_clock(freq));
						last_tick = e->tick;
						got_it = 1;

					}
					break;


#if 0
				case SND_SEQ_EVENT_NOTEOFF:
// find relevant oscillator
					for(j = 0; j < OSCS; j++)
					{
						if(osc[j].on && osc[j].freq == freq)
						{
							osc[j].on = 0;
// generate command to turn off oscillator
							printf("    { %d, %d, 0, 0 },\n", 
								tick_to_clock(e->tick - last_tick), 
								j);
							last_tick = e->tick;
							break;
						}
					}
					break;
#endif

			}
			
			e = e->next;
		}
	}

// end code
	printf("    { 0xff, 0xff, 0xff, 0xff },\n");
}


void main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("Need a filename, you Mor*on!\n");
		return;
	}
	
	file_name = argv[1];
	
	file = fopen(file_name, "r");
	
	if(file != 0)
	{
		switch (read_id()) {
		case MAKE_ID('M', 'T', 'h', 'd'):
			read_smf();
			break;
		case MAKE_ID('R', 'I', 'F', 'F'):
			read_riff();
			break;
		default:
			errormsg("%s is not a Standard MIDI File", file_name);
			break;
		}
		
	}
	
	dump();

	convert();	
}







