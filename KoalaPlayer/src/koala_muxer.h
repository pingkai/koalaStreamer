
#ifndef KOALA_MUXER_H
#define KOALA_MUXER_H

typedef struct koala_muxer_t koala_muxer;

typedef int (*audio_data_callback)(void *arg,int cmd,void **data,int *size);
typedef int (*video_data_callback)(void *arg,int cmd,void **data,int *size);

koala_muxer * koala_get_muxer_handle();
void koala_muxer_reg_data_callback(koala_muxer * pHandle,
                                       audio_data_callback acb,void *acb_arg,
                                       video_data_callback vcb,void *vcb_arg);

int koala_muxer_init_open(koala_muxer *pHandle, const char* outName,const char * outFormat);

char *koala_muxer_get_hostip(koala_muxer *pHandle);

int koala_muxer_start(koala_muxer *pHandle);

int koala_muxer_end(koala_muxer *pHandle);

void koala_muxer_close(koala_muxer *pHandle);

int koala_muxer_setopt(koala_muxer *pHandle, const char* key,const char* value);

typedef void (*koala_muxer_listener)(void* arg, int event);
int koala_muxer_set_listener(koala_muxer *pHandle, koala_muxer_listener listener, void *arg);
#endif
