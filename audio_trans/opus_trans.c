#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio_trans.h"

#include "opus/opus.h"

#define FEC_ENABLE 0

#if 1   //  opus编码
codec_handle opus_encode_init(audio_param_t audio_param)
{
    int err = 0;
    int frame_size = 0;
    unsigned int variable_duration = 0;
    OpusEncoder *encoder = NULL;

    if (audio_param.fps <= 0 || audio_param.samplerate % 8000 != 0)
    {
        fprintf(stderr, "fps=%d???\nsamplerate=%d???\n", audio_param.fps, audio_param.samplerate);
        return NULL;
    }

    frame_size = audio_param.samplerate / audio_param.fps;
    if (frame_size == audio_param.samplerate / 400)
        variable_duration = OPUS_FRAMESIZE_2_5_MS;
    else if (frame_size == audio_param.samplerate / 200)
        variable_duration = OPUS_FRAMESIZE_5_MS;
    else if (frame_size == audio_param.samplerate / 100)
        variable_duration = OPUS_FRAMESIZE_10_MS;
    else if (frame_size == audio_param.samplerate / 50)
        variable_duration = OPUS_FRAMESIZE_20_MS;
    else if (frame_size == audio_param.samplerate / 25)
        variable_duration = OPUS_FRAMESIZE_40_MS;
    else if (frame_size == 3 * audio_param.samplerate / 50)
        variable_duration = OPUS_FRAMESIZE_60_MS;
    else if (frame_size == 4 * audio_param.samplerate / 50)
        variable_duration = OPUS_FRAMESIZE_80_MS;
    else if (frame_size == 5 * audio_param.samplerate / 50)
        variable_duration = OPUS_FRAMESIZE_100_MS;
    else
        variable_duration = OPUS_FRAMESIZE_120_MS;

    /*
     * OPUS_APPLICATION_VOIP 视频会议
     * OPUS_APPLICATION_AUDIO 高保真
     * OPUS_APPLICATION_RESTRICTED_LOWDELAY 低延迟，但是效果差
     */
    encoder = opus_encoder_create(audio_param.samplerate, audio_param.channels, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK)
    {
        fprintf(stderr, "Cannot create encoder: %s\n", opus_strerror(err));
        return NULL;
    }

    /*
     * OPUS_SIGNAL_VOICE 语音
     * OPUS_SIGNAL_MUSIC 音乐
     */
    opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(encoder, OPUS_SET_BITRATE(OPUS_AUTO));                       // 控制最大比特率，AUTO在不说话时减少带宽。
    opus_encoder_ctl(encoder, OPUS_SET_BANDWIDTH(OPUS_AUTO));
    opus_encoder_ctl(encoder, OPUS_SET_VBR(1));                                   // 0固定码率，1动态码率
    opus_encoder_ctl(encoder, OPUS_SET_VBR_CONSTRAINT(1));                        // 0不受约束，1受约束（默认）
    opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(0));                            // 编码复杂度0~10
    opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(FEC_ENABLE));                   // 算法修复丢失的数据包，0关，1开
    opus_encoder_ctl(encoder, OPUS_SET_FORCE_CHANNELS(audio_param.channels));     // 声道数
    opus_encoder_ctl(encoder, OPUS_SET_DTX(0));                                   // 不连续传输，0关，1开
    opus_encoder_ctl(encoder, OPUS_SET_LSB_DEPTH(audio_param.bit_depth));         // 位深
    opus_encoder_ctl(encoder, OPUS_SET_EXPERT_FRAME_DURATION(variable_duration)); // 帧持续时间
    opus_encoder_ctl(encoder, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));       //同编码器创建参数

    return encoder;
}

int opus_encode_frame(codec_handle handle, unsigned char *input_buf, unsigned long input_len, unsigned char *output_buf, int output_buf_size)
{
    opus_int32 opus_data_len = 0;
    OpusEncoder *encoder = NULL;

    encoder = (OpusEncoder *)handle;

    if ((encoder == NULL) || input_buf == NULL || input_len == 0)
    {
        fprintf(stderr, "opus encode param err\n");
        return -1;
    }

    opus_data_len = opus_encode(encoder, (opus_int16 *)input_buf, input_len, output_buf, output_buf_size);
    if (opus_data_len < 0)
    {
        fprintf(stderr, "opus encode return len=%d, err!!\n", opus_data_len);
        return -1;
    }

    // printf("pcm_len:%d, opus_len:%d\n", input_len, opus_data_len);

    return opus_data_len;
}

void opus_encode_deinit(codec_handle handle)
{
    OpusEncoder *encoder = NULL;

    encoder = (OpusEncoder *)handle;
    if (encoder)
    {
        opus_encoder_destroy(encoder);
    }
}
#endif

#if 1   // opus解码
codec_handle opus_decode_init(audio_param_t audio_param)
{
    int err = 0;
    OpusDecoder *decoder = NULL;

    decoder = opus_decoder_create(audio_param.samplerate, audio_param.channels, &err);
    if (err != OPUS_OK)
    {
        fprintf(stderr, "Cannot create decoder: %s\n", opus_strerror(err));
        return NULL;
    }

    return decoder;
}

int opus_decode_frame(codec_handle handle, unsigned char *input_buf, unsigned long input_len, opus_int16 *output_buf, int output_buf_size)
{
    opus_int32 pcm_data_len = 0;
    OpusDecoder *decoder = NULL;

    decoder = (OpusDecoder *)handle;

    if ((decoder == NULL) || input_buf == NULL || input_len == 0)
    {
        fprintf(stderr, "opus decode param err\n");
        return -1;
    }

    pcm_data_len = opus_decode(decoder, input_buf, input_len, output_buf, output_buf_size, FEC_ENABLE);

    return pcm_data_len;
}

void opus_decode_deinit(codec_handle handle)
{
    OpusDecoder *decoder = NULL;

    decoder = (OpusDecoder *)handle;
    if (decoder)
    {
        opus_decoder_destroy(decoder);
    }
}
#endif