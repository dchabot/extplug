#include <stdio.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <alsa/pcm_external.h>

struct myplug_info {
    snd_pcm_extplug_t ext;
    int my_own_data;
};

void myplug_free(struct myplug_info* mp) {}

static int init_callback(snd_pcm_extplug_t *ext) {

    return 0;
}

static int close_callback(snd_pcm_extplug_t *ext) {

    return 0;
}

/* copied from pcm-plugins/speex/pcm-speex.c */
static inline void*
area_addr(const snd_pcm_channel_area_t *area, snd_pcm_uframes_t offset)
{
	unsigned int bitofs = area->first + area->step * offset;
	return (char *) area->addr + bitofs / 8;
}

static snd_pcm_sframes_t
transfer_callback(snd_pcm_extplug_t *ext,
                  const snd_pcm_channel_area_t *dst_areas,
                  snd_pcm_uframes_t dst_offset,
                  const snd_pcm_channel_area_t *src_areas,
                  snd_pcm_uframes_t src_offset,
                  snd_pcm_uframes_t size)
{
    int C = ext->channels;
    short *src, *dst;

    for(int i = 0; i < C; i++) {
        src = area_addr(&src_areas[i], src_offset);
        dst = area_addr(&dst_areas[i], dst_offset);
        memcpy(dst, src, size);
    }

    return size;
}

static snd_pcm_extplug_callback_t myplug_cb = {
    .init = init_callback,
    .transfer = transfer_callback,
    .close = close_callback,
    /* optional, but should be present for pass-through experimentation
    .hw_params = NULL,
    .hw_free = NULL,
    .dump = NULL,
    .query_chmaps = NULL,
    .set_chmap = NULL,
    .get_chmap = NULL,
    */
};

SND_PCM_PLUGIN_DEFINE_FUNC(myplug)
{
    snd_config_iterator_t i, next;
    snd_config_t *slave = NULL;
    struct myplug_info *myplug;
    int err;
    snd_config_for_each(i, next, conf) {
        snd_config_t *n = snd_config_iterator_entry(i);
        const char *id;
        if (snd_config_get_id(n, &id) < 0)
                continue;
        if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0) {
                printf("comment || type = %s\n", id);
                continue;
        }
        if (strcmp(id, "slave") == 0) {
            slave = n;
            continue;
        }
        if (strcmp(id, "my_own_parameter") == 0) {
            char *param = NULL;
            snd_config_get_ascii(n, &param);
            if(param != NULL) {
                printf("my_own_parameter = %s\n", param);
                free(param);
            }
            continue;
        }
        SNDERR("myplug, unknown field: %s", id);
        return -EINVAL;
    }

    if (! slave) {
        SNDERR("No slave defined for myplug");
        return -EINVAL;
    }

    myplug = calloc(1, sizeof(*myplug));
    if (myplug == NULL)
        return -ENOMEM;
    myplug->ext.version = SND_PCM_EXTPLUG_VERSION;
    myplug->ext.name = "My Own Plugin";
    myplug->ext.callback = &myplug_cb;
    myplug->ext.private_data = myplug;

    err = snd_pcm_extplug_create(&myplug->ext, name, root, slave, stream, mode);
    if (err < 0) {
        myplug_free(myplug);
        return err;
    }

    /* Set PCM constraints */
	snd_pcm_extplug_set_param(&myplug->ext, SND_PCM_EXTPLUG_HW_CHANNELS, 2);
	snd_pcm_extplug_set_slave_param(&myplug->ext,
					SND_PCM_EXTPLUG_HW_CHANNELS, 2);
	snd_pcm_extplug_set_param(&myplug->ext, SND_PCM_EXTPLUG_HW_FORMAT,
				  SND_PCM_FORMAT_S16);
	snd_pcm_extplug_set_slave_param(&myplug->ext, SND_PCM_EXTPLUG_HW_FORMAT,
					SND_PCM_FORMAT_S16);

    *pcmp = myplug->ext.pcm;
    return 0;
}

SND_PCM_PLUGIN_SYMBOL(myplug);

