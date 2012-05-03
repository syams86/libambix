/* ambix-jplay.c -  play back an ambix file via jack            -*- c -*-

   Copyright © 2003-2001 Rohan Drape
   Copyright © 2012 IOhannes m zmölnig <zmoelnig@iem.at>.
         Institute of Electronic Music and Acoustics (IEM),
         University of Music and Dramatic Arts, Graz

   This file is based on 'jack.play' from Rohan Drape's "jack-tools" collection

   you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   ambix-jplay is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, see <http://www.gnu.org/licenses/>.
*/


#define HAVE_SAMPLERATE
//#define HAVE_AMBIX_SEEK


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include <jack/jack.h>
#include <jack/thread.h>

#include <ambix/ambix.h>

#ifdef HAVE_SAMPLERATE
# include <samplerate.h>
#endif /* HAVE_SAMPLERATE */


//#include "jcommon/file.h"
//#include "jcommon/int.h"
//#include "jcommon/jack-client.h"
#include "jcommon/jack-ringbuffer.h"
#include "jcommon/common.h"
//#include "jcommon/jack-port.h"
//#include "jcommon/jack-transport.h"
//#include "jcommon/memory.h"
//#include "jcommon/observe-signal.h"
//#include "jcommon/print.h"
//#include "jcommon/sound-file.h"

#include <stdlib.h>


jack_client_t *jack_client_unique_store(char *name)
{
  int n = (int)getpid();
  char uniq[64];
  snprintf(uniq, 64, "%s-%d", name, n);
  strncpy(name,uniq,64);
  jack_client_t *client = jack_client_open(uniq,JackNullOption,NULL);
  if(! client) {
    eprintf("jack_client_open() failed: %s\n", uniq);
    FAILURE;
  }
  return client;
}



struct player_opt
{
  int buffer_frames;
  int minimal_frames;
#ifdef HAVE_AMBIX_SEEK
  int64_t seek_request;
#endif /* HAVE_AMBIX_SEEK */
  bool transport_aware;
  int unique_name;
  double src_ratio;
  int rb_request_frames;
  int converter;
  char client_name[64];
};

static void interleave_data(float32_t*source1, uint32_t source1channels, float32_t*source2, uint32_t source2channels, float*destination, int64_t frames) {
#warning interleave a_buffer+e_buffer into d_buffer
  int64_t frame;
  for(frame=0; frame<frames; frame++) {
    uint32_t chan;
    for(chan=0; chan<source1channels; chan++)
      *destination++=*source1++;
    for(chan=0; chan<source2channels; chan++)
      *destination++=*source2++;
  }                    
}

struct player
{
  int buffer_bytes;
  int buffer_samples;
  float32_t *a_buffer, *e_buffer; /* ambix channels, extra channels */
  float *d_buffer; /* interleaved channels */
  float *j_buffer; /* re-sampled channels */
  float *k_buffer;
  ambix_t *sound_file;
  int channels, a_channels, e_channels;
  jack_port_t **output_port;
  float **out;
  jack_ringbuffer_t *rb;
  pthread_t disk_thread;
  int pipe[2];
  jack_client_t *client;
#ifdef HAVE_SAMPLERATE
  SRC_STATE *src;
#endif /* HAVE_SAMPLERATE */
  struct player_opt o;
};

/* Read the sound file from disk and write to the ring buffer until
   the end of file, at which point return. */

void *disk_proc(void *PTR)
{
  struct player *d = (struct player *)PTR;
  while(!observe_end_of_process()) {

#ifdef HAVE_AMBIX_SEEK
    /* Handle seek request. */
    if(d->o.seek_request >= 0) {
      sf_count_t err = sf_seek(d->sound_file,
                               (sf_count_t)d->o.seek_request, SEEK_SET);
      if(err == -1) {
        eprintf("ambix-jplay: seek request failed, %ld\n",
                (long)d->o.seek_request);
      }
      d->o.seek_request = -1;
    }
#endif /* HAVE_AMBIX_SEEK */

    /* Wait for write space at the ring buffer. */

    int nbytes = d->o.minimal_frames * sizeof(float) * d->channels;
    eprintf("nbytes=%d=%d*%d*%d\n", (int)nbytes, (int)d->o.minimal_frames, (int)sizeof(float), (int)d->channels);
    nbytes = jack_ringbuffer_wait_for_write(d->rb, nbytes, d->pipe[0]);
    eprintf("jack ringbuffer  returned %d nbytes\n", nbytes);

    /* Do not overflow the local buffer. */

    if(nbytes > d->buffer_bytes) {
      eprintf("ambix-jplay: impossible condition, write space (%d > %d).\n", (int)nbytes, (int)d->buffer_bytes);
      nbytes = d->buffer_bytes;
    }

    /* Read sound file data, which *must* be frame aligned. */

    int nframes =(nbytes / sizeof(float))/ d->channels;
    int nsamples = nframes * d->channels;
    int64_t err = ambix_readf_float32(d->sound_file,
                                      d->a_buffer,
                                      d->e_buffer,
                                      nframes);
    if(err == 0) {
      if(d->o.transport_aware) {
        memset(d->a_buffer, 0, nsamples * sizeof(float32_t));
        memset(d->e_buffer, 0, nsamples * sizeof(float32_t));
        err = nframes;
      } else {
        return NULL;
      }
    }

    interleave_data(d->a_buffer, d->a_channels, d->e_buffer, d->e_channels, d->d_buffer, err);
    /* Write data to ring buffer. */

    jack_ringbuffer_write(d->rb,
                          (char *)d->d_buffer,
                          (size_t)(err*d->channels) * sizeof(float));
  }

  return NULL;
}

int sync_handler(jack_transport_state_t state,
                 jack_position_t *position,
                 void *PTR)
{
  struct player *d = PTR;
#ifdef HAVE_AMBIX_SEEK
  d->o.seek_request = (int64_t)position->frame;
#endif /* HAVE_AMBIX_SEEK */
  return 1;
}

void signal_set(float **s, int n, int c, float z)
{
  int j;
  for(j = 0; j < c; j++) {
    int i;
    for(i = 0; i < n; i++) {
      s[j][i] = z;
    }
  }
}

/* Write data from the ring buffer to the JACK output ports.  If the
   disk thread is late, ie. the ring buffer is empty print a warning
   and zero the output ports.  */

int signal_proc(jack_nframes_t nframes, void *PTR)
{
  struct player *d = (struct player *)PTR;
  int nsamples = nframes * d->channels;
  int nbytes = nsamples * sizeof(float);

  /* Ensure the period size is workable. */

  if(nbytes >= d->buffer_bytes) {
    eprintf("ambix-jplay: period size exceeds limit\n");
    FAILURE;
    return 1;
  }

  /* Get port data buffers. */

  int i,j;
  for(i = 0; i < d->channels; i++) {
    d->out[i] = (float *)jack_port_get_buffer(d->output_port[i], nframes);
  }

  /* Write silence if the transport is stopped.  If stopped the disk
     thread will sleep and signals will be ignored, so check here
     also. */

  if(d->o.transport_aware && !jack_transport_is_rolling(d->client)) {
    if(observe_end_of_process ()) {
      FAILURE;
      return 1;
    } else {
      signal_set(d->out, nframes, d->channels, 0.0);
      return 0;
    }
  }

#ifdef HAVE_SAMPLERATE
  /* Get data from sample rate converter, this returns the number of
     frames acquired. */
  long err = src_callback_read (d->src,
                                d->o.src_ratio,
                                (long)nframes,
                                d->j_buffer);
  if(err==0) {
    eprintf("ambix-jplay: sample rate converter failed: %s\n",
            src_strerror(src_error(d->src)));
    FAILURE;
  }
#else
# warning no-samplerate callback
#endif /* HAVE_SAMPLERATE */


  /* Uninterleave available data to the output buffers. */

  for(i = 0; i < err; i++) {
    for(j = 0; j < d->channels; j++) {
      d->out[j][i] = d->j_buffer[(i*d->channels)+j];
    }
  }

  /* If any sample data is unavailable inform the user and zero the
     output buffers.  The print statement is not correct, a this
     should set a flag and have another thread take appropriate
     action. */

  if(err < nframes) {
    eprintf("ambix-jplay: disk thread late (%ld < %d)\n", err, nframes);
    for(i = err; i < nframes; i++) {
      for(j = 0; j < d->channels; j++) {
        d->out[j][i] = 0.0;
      }
    }
  }

  /* Indicate to the disk thread that the ring buffer has been read
     from.  This is done by writing a single byte to a communication
     pipe.  Once the disk thread gets so far ahead that the ring
     buffer is full it reads this communication channel to wait for
     space to become available.  So long as PIPE_BUF is not a
     pathologically small value this write operation is atomic and
     will not block.  The number of bytes that can accumulate in the
     pipe is a factor of the relative sizes of the ring buffer and the
     process callback, but should in no case be very large. */

  char b = 1;
  xwrite(d->pipe[1], &b, 1);

  return 0;
}

void usage(const char*filename)
{
  eprintf("Usage: %s [ options ] sound-file...\n", filename);
  eprintf("    -b N : Ring buffer size in frames (default=4096).\n");
#ifdef HAVE_SAMPLERATE
  eprintf("    -c N : ID of conversion algorithm (default=2, SRC_SINC_FASTEST).\n");
#endif /* HAVE_SAMPLERATE */
#ifdef HAVE_AMBIX_SEEK
  eprintf("    -i N : Initial disk seek in frames (default=0).\n");
#endif /* HAVE_AMBIX_SEEK */
  eprintf("    -m N : Minimal disk read size in frames (default=32).\n");
  eprintf("    -q N : Frames to request from ring buffer (default=64).\n");
  eprintf("    -r N : Resampling ratio multiplier (default=1.0).\n");
  eprintf("    -t   : Jack transport awareness.\n");
  FAILURE;
}

/* Get data from ring buffer.  Return number of frames read.  This
   could check the read size first, but then would still need to check
   the actual result size, and therefore have two error cases.  Since
   there is no alternative but to drop sample data in any case it does
   not matter much. */

long read_input_from_rb(void *PTR, float **buf)
{
  struct player *d = PTR;
  int nsamples = d->channels * d->o.rb_request_frames;
  int nbytes = (size_t)nsamples * sizeof(float);

  int err = jack_ringbuffer_read(d->rb,
                                 (char *)d->k_buffer,
                                 nbytes);
  err /= d->channels * sizeof(float);
  *buf = d->k_buffer;

  /* SRC locks up if we return zero here, return a silent frame */
  if(err==0) {
    eprintf("ambix-jplay: ringbuffer empty... zeroing data\n");
    memset(d->k_buffer, 0, (size_t)nsamples * sizeof(float));
    err = d->o.rb_request_frames;
  }

  return (long)err;
}

static jack_port_t*_jack_port_register(jack_client_t *client, int direction, const char*format, int n) {
  char name[64];
  jack_port_t*port;
  snprintf(name, 64, format, n);
  port = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, direction, 0);
  if(!port) {
    eprintf("jack_port_register() failed at %s\n", name);
    FAILURE;
  }
  return port;
}
int jackplay(const char *file_name,
             struct player_opt o)
{
  struct player d;
  d.o = o;
  observe_signals ();

  /* Open sound file. */

  ambix_info_t ambixinfo;
  memset(&ambixinfo, 0, sizeof(ambixinfo));
  ambixinfo.fileformat=AMBIX_BASIC;
  d.sound_file = ambix_open(file_name, AMBIX_READ, &ambixinfo);

  eprintf("ambixchannels=%d\textrachannels=%d\n", ambixinfo.ambichannels, ambixinfo.extrachannels);

  d.a_channels = ambixinfo.ambichannels;
  d.e_channels = ambixinfo.extrachannels;

  d.channels = d.a_channels + d.e_channels;

  /* Allocate channel based data. */

  if(d.channels < 1) {
    eprintf("ambix-jplay: illegal number of channels in file: %d\n",
            d.channels);
    FAILURE;
  }
  d.out = xmalloc(d.channels * sizeof(float *));
  d.output_port = xmalloc(d.channels * sizeof(jack_port_t *));

  /* Allocate buffers. */

  d.buffer_samples = d.o.buffer_frames * d.channels;
  d.buffer_bytes = d.buffer_samples * sizeof(float);

  d.a_buffer = xmalloc(d.o.buffer_frames * d.a_channels * sizeof(float32_t));
  d.e_buffer = xmalloc(d.o.buffer_frames * d.e_channels * sizeof(float32_t));

  eprintf("%d*%d*(%d|%d) ... %d\n", (int)d.o.buffer_frames, (int)sizeof(float32_t), (int)d.a_channels, (int)d.e_channels, (int)d.buffer_bytes);

  d.d_buffer = xmalloc(d.buffer_bytes);
  d.j_buffer = xmalloc(d.buffer_bytes);
  d.k_buffer = xmalloc(d.buffer_bytes);
  d.rb = jack_ringbuffer_create(d.buffer_bytes);

#ifdef HAVE_SAMPLERATE
  /* Setup sample rate conversion. */
  int err;
  d.src = src_callback_new (read_input_from_rb,
                            d.o.converter,
                            d.channels,
                            &err,
                            &d);
  if(!d.src) {
    eprintf("ambix-jplay: sample rate conversion setup failed: %s\n",
            src_strerror(err));
    FAILURE;
  }
#else
# warning samplerate conversion
#endif /* HAVE_SAMPLERATE */


  /* Create communication pipe. */

  xpipe(d.pipe);

  /* Become a client of the JACK server.  */

  if(d.o.unique_name) {
    d.client = jack_client_unique_store(d.o.client_name);
  } else {
    d.client = jack_client_open(d.o.client_name,JackNullOption,NULL);
  }
  if(!d.client) {
    eprintf("ambix-jplay: could not create jack client: %s", d.o.client_name);
    FAILURE;
  }


  /* Start disk thread, the priority number is a random guess.... */

  jack_client_create_thread (d.client,
                             &(d.disk_thread),
                             50,
                             true,
                             disk_proc,
                             &d);

  /* Set error, process and shutdown handlers. */

  jack_set_error_function(jack_client_minimal_error_handler);
  jack_on_shutdown(d.client, jack_client_minimal_shutdown_handler, 0);
  if(d.o.transport_aware) {
    jack_set_sync_callback(d.client, sync_handler, &d);
  }
  jack_set_process_callback(d.client, signal_proc, &d);

  /* Inform the user of sample-rate mismatch. */

  int osr = jack_get_sample_rate(d.client);
  int isr = ambixinfo.samplerate;
  if(osr != isr) {
    d.o.src_ratio *= (osr / isr);
    eprintf("ambix-jplay: resampling, sample rate of file != server, %d != %d\n",
            isr,
            osr);
  }

  /* Create output ports, connect if env variable set and activate
     client. */
  //  jack_port_make_standard(d.client, d.output_port, d.channels, true);
  do {
    int i=0, a, e;
    for(a=0; a<d.a_channels; a++) {
      d.output_port[i] = _jack_port_register(d.client, JackPortIsOutput, "ACN_%d", a+1);
      i++;
    }
    for(e=0; e<d.e_channels; e++) {
      d.output_port[i] = _jack_port_register(d.client, JackPortIsOutput, "out_%d", e+1);
      i++;
    }
  } while(0);


  if(jack_activate(d.client)) {
    eprintf("jack_activate() failed\n");
    FAILURE;
  }
#if 0
  char *dst_pattern = getenv("JACK_PLAY_CONNECT_TO");
  if (dst_pattern) {
    char src_pattern[128];
    snprintf(src_pattern,128,"%s:out_%%d",d.o.client_name);
    jack_port_connect_pattern(d.client,d.channels,src_pattern,dst_pattern);
  }
#endif
  /* Wait for disk thread to end, which it does when it reaches the
     end of the file or is interrupted. */

  pthread_join(d.disk_thread, NULL);

  /* Close sound file, free ring buffer, close JACK connection, close
     pipe, free data buffers, indicate success. */

  jack_client_close(d.client);
  ambix_close(d.sound_file);
  jack_ringbuffer_free(d.rb);
  close(d.pipe[0]);
  close(d.pipe[1]);
  free(d.d_buffer);
  free(d.j_buffer);
  free(d.k_buffer);
  free(d.out);
  free(d.output_port);
#ifdef HAVE_SAMPLERATE
  src_delete(d.src);
#endif /* HAVE_SAMPLERATE */
  return 0;
}

int main(int argc, char *argv[])
{
  struct player_opt o;
  int c;

  o.buffer_frames = 4096;
  o.minimal_frames = 32;
#ifdef HAVE_AMBIX_SEEK
  o.seek_request = -1;
#endif /* HAVE_AMBIX_SEEK */
  o.transport_aware = false;
  o.unique_name = true;
  o.src_ratio = 1.0;
  o.rb_request_frames = 64;
#ifdef HAVE_SAMPLERATE
  o.converter = SRC_SINC_FASTEST;
#endif /* HAVE_SAMPLERATE */
  strncpy(o.client_name, "ambix-jplay", 64);

  while((c = getopt(argc, argv, "b:c:hi:m:n:q:r:tu")) != -1) {
    switch(c) {
    case 'b':
      o.buffer_frames = (int)strtol(optarg, NULL, 0);
      break;
    case 'c':
      o.converter = (int)strtol(optarg, NULL, 0);
      break;
    case 'h':
      usage (argv[0]);
      break;
#ifdef HAVE_AMBIX_SEEK
    case 'i':
      o.seek_request = (int64_t)strtol(optarg, NULL, 0);
      break;
#endif /* HAVE_AMBIX_SEEK */
    case 'm':
      o.minimal_frames = (int)strtoll(optarg, NULL, 0);
      break;
    case 'n':
      strncpy(o.client_name, optarg, 64);
      eprintf("jack client name: %s\n", o.client_name);
      break;
    case 'q':
      o.rb_request_frames = strtol(optarg, NULL, 0);
      break;
#ifdef HAVE_SAMPLERATE
    case 'r':
      o.src_ratio = strtod(optarg, NULL);
      break;
#endif /* HAVE_SAMPLERATE */
    case 't':
      o.transport_aware = true;
    case 'u':
      o.unique_name = false;
      break;
    default:
      eprintf("ambix-jplay: illegal option, %c\n", c);
      usage (argv[0]);
      break;
    }
  }
  if(optind > argc - 1) {
    usage (argv[0]);
  }
  int i;
  for(i = optind; i < argc; i++) {
    printf("ambix-jplay: %s\n", argv[i]);
    jackplay(argv[i], o);
  }
  return EXIT_SUCCESS;
}
