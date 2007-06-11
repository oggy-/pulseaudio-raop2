#ifndef foopulsesinkinputhfoo
#define foopulsesinkinputhfoo

/* $Id$ */

/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2006 Pierre Ossman <ossman@cendio.se> for Cendio AB

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <inttypes.h>

typedef struct pa_sink_input pa_sink_input;

#include <pulse/sample.h>
#include <pulsecore/hook-list.h>
#include <pulsecore/memblockq.h>
#include <pulsecore/resampler.h>
#include <pulsecore/module.h>
#include <pulsecore/client.h>
#include <pulsecore/sink.h>
#include <pulsecore/core.h>

typedef enum pa_sink_input_state {
    PA_SINK_INPUT_RUNNING,      /*< The stream is alive and kicking */
    PA_SINK_INPUT_DRAINED,      /*< The stream stopped playing because there was no data to play */
    PA_SINK_INPUT_CORKED,       /*< The stream was corked on user request */
    PA_SINK_INPUT_DISCONNECTED  /*< The stream is dead */
} pa_sink_input_state_t;

typedef enum pa_sink_input_flags {
    PA_SINK_INPUT_VARIABLE_RATE = 1,
    PA_SINK_INPUT_NO_HOOKS = 2
} pa_sink_input_flags_t;

struct pa_sink_input {
    pa_msgobject parent;

    uint32_t index;
    pa_core *core;
    pa_atomic_t state;
    pa_sink_input_flags_t flags;

    char *name, *driver;                /* may be NULL */
    pa_module *module;                  /* may be NULL */
    pa_client *client;                  /* may be NULL */

    pa_sink *sink;

    pa_sample_spec sample_spec;
    pa_channel_map channel_map;

    pa_cvolume volume;
    int muted;

    int (*process_msg)(pa_sink_input *i, int code, void *userdata);
    int (*peek) (pa_sink_input *i, pa_memchunk *chunk);
    void (*drop) (pa_sink_input *i, const pa_memchunk *chunk, size_t length);
    void (*kill) (pa_sink_input *i);             /* may be NULL */
    pa_usec_t (*get_latency) (pa_sink_input *i); /* may be NULL */
    void (*underrun) (pa_sink_input *i);         /* may be NULL */

    pa_resample_method_t resample_method;

    struct {
        pa_sample_spec sample_spec;

        pa_memchunk resampled_chunk;
        pa_resampler *resampler;                     /* may be NULL */

        /* Some silence to play before the actual data. This is used to
         * compensate for latency differences when moving a sink input
         * "hot" between sinks. */
        /*         size_t move_silence; */
        pa_memblock *silence_memblock;               /* may be NULL */

        pa_cvolume volume;
        int muted;
    } thread_info;

    void *userdata;
};

PA_DECLARE_CLASS(pa_sink_input);
#define PA_SINK_INPUT(o) ((pa_sink_input*) (o))

enum {
    PA_SINK_INPUT_MESSAGE_SET_VOLUME,
    PA_SINK_INPUT_MESSAGE_SET_MUTE,
    PA_SINK_INPUT_MESSAGE_GET_LATENCY,
    PA_SINK_INPUT_MESSAGE_SET_RATE,
    PA_SINK_INPUT_MESSAGE_MAX
};

typedef struct pa_sink_input_new_data {
    const char *name, *driver;
    pa_module *module;
    pa_client *client;

    pa_sink *sink;

    pa_sample_spec sample_spec;
    int sample_spec_is_set;
    pa_channel_map channel_map;
    int channel_map_is_set;
    pa_cvolume volume;
    int volume_is_set;
    int muted;
    int muted_is_set;

    pa_resample_method_t resample_method;
} pa_sink_input_new_data;

pa_sink_input_new_data* pa_sink_input_new_data_init(pa_sink_input_new_data *data);
void pa_sink_input_new_data_set_sample_spec(pa_sink_input_new_data *data, const pa_sample_spec *spec);
void pa_sink_input_new_data_set_channel_map(pa_sink_input_new_data *data, const pa_channel_map *map);
void pa_sink_input_new_data_set_volume(pa_sink_input_new_data *data, const pa_cvolume *volume);
void pa_sink_input_new_data_set_muted(pa_sink_input_new_data *data, int mute);

/* To be called by the implementing module only */

pa_sink_input* pa_sink_input_new(
        pa_core *core,
        pa_sink_input_new_data *data,
        pa_sink_input_flags_t flags);

void pa_sink_input_put(pa_sink_input *i);
void pa_sink_input_disconnect(pa_sink_input* i);

void pa_sink_input_set_name(pa_sink_input *i, const char *name);

/* Callable by everyone */

/* External code may request disconnection with this function */
void pa_sink_input_kill(pa_sink_input*i);

pa_usec_t pa_sink_input_get_latency(pa_sink_input *i);

void pa_sink_input_set_volume(pa_sink_input *i, const pa_cvolume *volume);
const pa_cvolume *pa_sink_input_get_volume(pa_sink_input *i);
void pa_sink_input_set_mute(pa_sink_input *i, int mute);
int pa_sink_input_get_mute(pa_sink_input *i);

void pa_sink_input_cork(pa_sink_input *i, int b);

void pa_sink_input_set_rate(pa_sink_input *i, uint32_t rate);

pa_resample_method_t pa_sink_input_get_resample_method(pa_sink_input *i);

int pa_sink_input_move_to(pa_sink_input *i, pa_sink *dest, int immediately);

#define pa_sink_input_get_state(i) ((pa_sink_input_state_t) pa_atomic_load(&i->state))

/* To be used exclusively by the sink driver thread */

int pa_sink_input_peek(pa_sink_input *i, pa_memchunk *chunk, pa_cvolume *volume);
void pa_sink_input_drop(pa_sink_input *i, const pa_memchunk *chunk, size_t length);
int pa_sink_input_process_msg(pa_msgobject *o, int code, void *userdata, pa_memchunk *chunk);

#endif
