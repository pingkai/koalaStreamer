/*
 * Copyright (c) 2012 pingkai010@gmail.com
 *
 *
 */
#ifndef KOALA_TYPE_H
#define KOALA_TYPE_H
#include <stdint.h>
typedef enum {
    DEMUX_MODE_NORMOL,
    DEMUX_MODE_I_FRAME,
} Demux_mode_e;

typedef enum {
    STREAM_TYPE_UNKNOWN = -1,
    STREAM_TYPE_VIDEO,
    STREAM_TYPE_AUDIO,
    STREAM_TYPE_SUB,
    STREAM_TYPE_NUM,
} Stream_type;

enum KoalaCodecID{
    KOALA_CODEC_ID_NONE,

    KOALA_CODEC_ID_H264,
    KOALA_CODEC_ID_MPEG4,
    KOALA_CODEC_ID_RV30,
    KOALA_CODEC_ID_RV40,
    KOALA_CODEC_ID_MPEG2VIDEO,
    KOALA_CODEC_ID_VC1,
    KOALA_CODEC_ID_WMV3,
    KOALA_CODEC_ID_WMV1,
    KOALA_CODEC_ID_WMV2,
    KOALA_CODEC_ID_MSMPEG4V2,
    KOALA_CODEC_ID_DIV311,
    KOALA_CODEC_ID_FLV1,
    KOALA_CODEC_ID_SVQ3,
    KOALA_CODEC_ID_MPEG1VIDEO,
    KOALA_CODEC_ID_VP6,
    KOALA_CODEC_ID_VP8,
    KOALA_CODEC_ID_MJPEG,
    KOALA_CODEC_ID_H263,
    KOALA_CODEC_ID_HEVC,

    KOALA_CODEC_ID_AAC,
    KOALA_CODEC_ID_AC3,
    KOALA_CODEC_ID_EAC3,
    KOALA_CODEC_ID_DTS,
    KOALA_CODEC_ID_DTSE,
    KOALA_CODEC_ID_MP3,
    KOALA_CODEC_ID_APE,
    KOALA_CODEC_ID_COOK,
    KOALA_CODEC_ID_SIPR,
    KOALA_CODEC_ID_QDM2,
    KOALA_CODEC_ID_MP2,
    KOALA_CODEC_ID_MP1,
    KOALA_CODEC_ID_AMR_NB,
    KOALA_CODEC_ID_WMAV2,
    KOALA_CODEC_ID_WMAPRO,
    KOALA_CODEC_ID_PCM_S16LE,
    KOALA_CODEC_ID_PCM_S16BE,
    KOALA_CODEC_ID_PCM_BLURAY,
    KOALA_CODEC_ID_ADPCM,
    KOALA_CODEC_ID_PCM_S24LE,
    KOALA_CODEC_ID_PCM_U8,
    KOALA_CODEC_ID_PCM_MULAW,
    KOALA_CODEC_ID_ATRAC3,
    KOALA_CODEC_ID_VORBIS,
    KOALA_CODEC_ID_ALAC,
    KOALA_CODEC_ID_FLAC,

    KOALA_CODEC_ID_TEXT,
    KOALA_CODEC_ID_SSA,
    KOALA_CODEC_ID_SRT,
};


enum KoalaSampleFormat {
    KOALA_SAMPLE_FMT_NONE = -1,
    KOALA_SAMPLE_FMT_U8,          ///< unsigned 8 bits
    KOALA_SAMPLE_FMT_S16,         ///< signed 16 bits
    KOALA_SAMPLE_FMT_S32,         ///< signed 32 bits
    KOALA_SAMPLE_FMT_FLT,         ///< float
    KOALA_SAMPLE_FMT_DBL,         ///< double

    KOALA_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
    KOALA_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
    KOALA_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
    KOALA_SAMPLE_FMT_FLTP,        ///< float, planar
    KOALA_SAMPLE_FMT_DBLP,        ///< double, planar

    KOALA_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
};

typedef struct{
    int nChannles;
    int sample_rate;
    enum KoalaCodecID codec;
    enum KoalaSampleFormat sample_fmt;
    uint64_t channel_layout;
    int frame_size;
    uint8_t *extradata;
    int extradata_size;
    int bits_per_coded_sample;
    int nb_frame;

}audio_info;

typedef struct{
    int pix_fmt;
    int width;
    int height;

}video_info;

/**
 * The stream is stored in the file as an attached picture/"cover art" (e.g.
 * APIC frame in ID3v2). The single packet associated with it will be returned
 * among the first few packets read from the file unless seeking takes place.
 * It can also be accessed at any time in AVStream.attached_pic.
 */
#define AV_DISPOSITION_ATTACHED_PIC      0x0400

typedef struct {
    Stream_type type;
    int64_t duration;//ms
    enum KoalaCodecID codec;
    int index;
    int nb_index_entries;
    void * koala_codec_context;
    int koala_codec_context_size;

    char* title;
    char* language;
    int64_t seeked_time;

    int disposition; /**< AV_DISPOSITION_* bit field */

    // TODO:  use union
    //audio
    int channels;
    int samplerate;
    int frame_size;
    int profile;
    int bits_per_coded_sample;
    enum KoalaSampleFormat sample_fmt;

    //video only
    int width;
    int height;
    int rotate;
    int displayWidth;
    int displayHeight;
    double avg_fps;

    int pid;
    int no_program;
    int attached_pic;
    uint8_t *extradata;
    int extradata_size;
}Stream_meta;


enum pix_fmt{
   KOALA_PIX_FMT_NONE= -1,
   KOALA_PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
   KOALA_PIX_FMT_YUVJ420P = 12,  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV420P and setting color_range
   KOALA_PIX_FMT_YUVJ422P,

   KOALA_PIX_FMT_APPLE_PRIVATE = 1000,
};


#define MAX_AUDIO_FRAME_SIZE 192000

typedef int  (*decoder_buf_callback) (unsigned char *buffer[], int nb_samples,int line_size,long long pts,void *CbpHandle);
typedef int  (*decoder_buf_callback_video)(unsigned char *buffer[], int linesize[],long long pts,int type,void *CbpHandle);

enum dec_flag{
    dec_flag_dummy,
    dec_flag_hw,
    dec_flag_sw,
    dec_flag_out,
};
#define DECFLAG_DUMMY  1 << dec_flag_dummy
#define DECFLAG_HW  1 << dec_flag_hw
#define DECFLAG_SW  1 << dec_flag_sw
#define DECFLAG_OUT 1 << dec_flag_out

enum callback_cmd{
    callback_cmd_get_source_meta,
    callback_cmd_get_sink_meta,
    callback_cmd_get_frame
};

#endif
