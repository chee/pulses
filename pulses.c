#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>
#include <stdbool.h>

typedef jack_default_audio_sample_t sample_t;

jack_client_t *client;
jack_port_t *analog_port;
unsigned long sr;
double last_bpm = 120;
double bpm = 120;
jack_nframes_t tone_length, wave_length;
sample_t *wave;
long offset = 0;
int transport_aware = 1;
jack_transport_state_t transport_state;
bool playing = false;

int setup_wavetable()
{
	int i;
	jack_nframes_t wl = (60 * sr / bpm) / 2;
	if (wl == 0) {
		return -1;
	}
	wave_length = wl;
	wave = (sample_t *)malloc(wave_length * sizeof(sample_t));

	for (i = 0; i < 5; i++) {
		wave[i] = 0.02;
	}

	for (i = 5; i < 200; i++) {
		wave[i] = 0.95;
	}

	for (i = 200; i < 400; i++) {
		wave[i] = -0.01 + ((float)i / 1000);
	}

	for (i = 400; i < (int)wave_length; i++) {
		wave[i] = 0.001;
	}

	return 0;
}

static void signal_handler(int sig) {
	jack_client_close(client);
	fprintf(stderr, "signal received (%i), exiting ...\n", sig);
	exit(0);
}

static void usage () {
	fprintf (stderr, "\nusage: pulses \n");
}

static void process_silence (jack_nframes_t nframes) {
	sample_t *buffer = (sample_t *) jack_port_get_buffer (analog_port, nframes);
	memset (buffer, 0, sizeof (jack_default_audio_sample_t) * nframes);
}

jack_nframes_t last_time;
jack_time_t last_micro_time;

static void process_audio (jack_nframes_t nframes) {
	sample_t *buffer = (sample_t *) jack_port_get_buffer (analog_port, nframes);
	jack_nframes_t frames_left = nframes;
	if (offset > wave_length) {
		return;
	}
	while (wave_length - offset < frames_left) {
		memcpy (buffer + (nframes - frames_left), wave + offset, sizeof (sample_t) * (wave_length - offset));
		frames_left -= wave_length - offset;
		offset = 0;
	}
	if (frames_left > 0) {
		memcpy (buffer + (nframes - frames_left), wave + offset, sizeof (sample_t) * frames_left);
		offset += frames_left;
	}
}

jack_transport_state_t last_state = 0;

static int process (jack_nframes_t nframes, void *arg) {
	jack_position_t pos;
	jack_transport_state_t state
		= jack_transport_query(client, &pos);
	bpm = pos.beats_per_minute;

	if (bpm != last_bpm) {
		last_bpm = bpm;
		setup_wavetable();
	}

	if (transport_aware) {
		if (state == JackTransportRolling && playing) {
			process_audio(nframes);
		} else if (state == JackTransportRolling && pos.beat == 1) {
			playing = true;
			process_audio(nframes);
		} else {
			playing = false;
			process_silence(nframes);
		}
		if (state == last_state) {
			offset = pos.frame % wave_length;
		} else {
			offset = 0;
		}
	} else {
		process_audio(nframes);
	}
	last_state = state;
	return 0;
}

jack_transport_state_t prev_sync_state = JackTransportStopped;

int main (int argc, char *argv[]) {
	int option_index;
	int opt;

	char *client_name = 0;
	int verbose = 0;
	jack_status_t status;

	const char *options = "th";
	struct option long_options[] =
	{
		{"transport", 0, 0, 't'},
		{"help", 0, 0, 'h'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != -1) {
		switch (opt) {
		case 't':
			transport_aware = 0;
			break;
		default:
			fprintf (stderr, "unknown option %c\n", opt);
		case 'h':
			return -1;
		}
	}

	if ((client = jack_client_open ("pulses", JackNoStartServer, &status)) == 0) {
		fprintf (stderr, "JACK server not running?\n");
		return 1;
	}
	jack_set_process_callback(client, process, 0);

	analog_port = jack_port_register (client, "2ppqn", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	sr = jack_get_sample_rate(client);
	setup_wavetable();

	if (jack_activate(client)) {
		fprintf (stderr, "cannot activate client\n");
		goto error;
	}

	signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);

	while (1) {
		sleep(1);
	};

	jack_client_close(client);

error:
	free(wave);
	exit (0);
}
