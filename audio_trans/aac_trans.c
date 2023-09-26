#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "faac.h"
#include "neaacdec.h"

#include "audio_trans.h"

#if 1 //  aac编码器
codec_handle aac_encode_init(audio_param_t audio_param, unsigned long *input_len, unsigned long *output_len_max)
{
    faacEncHandle enc_handle = NULL;
    faacEncConfigurationPtr conf = NULL;

    enc_handle = faacEncOpen(audio_param.samplerate, audio_param.channels, input_len, output_len_max);
    if (enc_handle == NULL)
    {
        fprintf(stderr, "[%s] Cannot open aac encoder.\n", __func__);
        goto ERR;
    }

    conf = faacEncGetCurrentConfiguration(enc_handle);
    if (conf == NULL)
    {
        fprintf(stderr, "[%s] Get aac encoder info err.\n", __func__);
        goto ERR;
    }
    switch (audio_param.bit_depth)
    {
    case 16:
        conf->inputFormat = FAAC_INPUT_16BIT;
        break;
    case 24:
        conf->inputFormat = FAAC_INPUT_24BIT;
        break;
    case 32:
        conf->inputFormat = FAAC_INPUT_32BIT;
        break;

    default:
        goto ERR;
    }
    //  Audio Data Transport Stream 音频数据传输流。这种格式的特征是用同步字节进行将AAC音频截断
    conf->outputFormat = ADTS_STREAM;
    //  Low Complexity，意味着该编码器使用较少的计算资源来实现高质量的音频压缩
    conf->aacObjectType = LOW;
    //  中/侧录音, 需多麦克风，0关闭
    conf->allowMidside = 0;
    //  low－frequency effects, 用于音乐录制和播放中的低频声音段
    conf->useLfe = 0;
    //  更好的控制网络传输时候的带宽
    conf->bitRate = 48000;
    //  频宽
    conf->bandWidth = 32000;
    faacEncSetConfiguration(enc_handle, conf);

    return enc_handle;

ERR:
    if (enc_handle != NULL)
    {
        faacEncClose(enc_handle);
    }
    return NULL;
}

int aac_encode_frame(codec_handle handle, unsigned char *input_buf, unsigned long input_len, unsigned char *output_buf, int output_buf_size)
{
    int ret_len = 0;

    ret_len = faacEncEncode(handle, (int *)input_buf, input_len, output_buf, output_buf_size);

    return ret_len;
}

void acc_encode_deinit(codec_handle handle)
{
    if (handle != NULL)
    {
        faacEncClose(handle);
    }
}
#endif

#if 1 //  aac解码器
codec_handle aac_decode_init(audio_param_t audio_param, unsigned char *frame, unsigned long frame_len)
{
    int ret = 0;
    NeAACDecHandle dec_handle = NULL;
    unsigned long samplerate = 0;
    unsigned char channels = 0;
    NeAACDecConfigurationPtr conf = NULL;

    dec_handle = NeAACDecOpen();
    if (dec_handle == NULL)
    {
        fprintf(stderr, "[%s] Cannot open aac decoder.\n", __func__);
        goto ERR;
    }

    conf = NeAACDecGetCurrentConfiguration(dec_handle);
    if (conf == NULL)
    {
        fprintf(stderr, "[%s] Get aac decoder info err.\n", __func__);
        goto ERR;
    }

    conf->defObjectType = LC;
    conf->defSampleRate = audio_param.samplerate;
    conf->dontUpSampleImplicitSBR = 1;
    switch (audio_param.bit_depth)
    {
    case 16:
        conf->outputFormat = FAAD_FMT_16BIT;
        break;
    case 24:
        conf->outputFormat = FAAD_FMT_24BIT;
        break;
    case 32:
        conf->outputFormat = FAAD_FMT_32BIT;
        break;

    default:
        goto ERR;
    }
    NeAACDecSetConfiguration(dec_handle, conf);

    ret = NeAACDecInit(dec_handle, frame, frame_len, &samplerate, &channels);
    if (ret < 0)
    {
        fprintf(stderr, "[%s] Cannot init aac decoder.\n", __func__);
        goto ERR;
    }
    // printf("[%s]samplerate=%lu, channels=%d\n", __func__, samplerate, channels);
    // if (audio_param.samplerate != samplerate || audio_param.channels != channels)
    // {
    //     fprintf(stderr, "[%s]aac decoder param is err\n", __func__);
    //     goto ERR;
    // }

    return dec_handle;

ERR:
    if (dec_handle != NULL)
    {
        NeAACDecClose(dec_handle);
    }
    return NULL;
}

int aac_decode_frame(codec_handle handle, audio_param_t audio_param, unsigned char *input_buf, unsigned long input_len, unsigned char *output_buf, int output_buf_size)
{
    int i = 0;
    unsigned char *pcm_data = NULL;
    unsigned long pcm_len = 0;
    NeAACDecFrameInfo frame_info;

    pcm_data = (unsigned char *)NeAACDecDecode(handle, &frame_info, input_buf, input_len);

    if (frame_info.error > 0)
    {
        fprintf(stderr, "%s\n", NeAACDecGetErrorMessage(frame_info.error));
        return -1;
    }
    
    pcm_len = frame_info.samples * frame_info.channels;
    if (audio_param.channels != 1)
    {
        if (pcm_len >= output_buf_size)
        {
            fprintf(stderr, "[%s]output_buf len is not enough\n", __func__);
            return -1;
        }
        memcpy(output_buf, pcm_data, pcm_len);
        return pcm_len;
    }

    for (i = 0; i < pcm_len / 4; i++)
    {
        if (i * 2 >= output_buf_size)
        {
            fprintf(stderr, "[%s]output_buf len is not enough\n", __func__);
            return -1;
        }
        memcpy(output_buf + i * 2, pcm_data + i * 4, audio_param.bit_depth / sizeof(unsigned char));
        /* code */
    }

    return pcm_len / 2;
}

void acc_decode_deinit(codec_handle handle)
{
    if (handle != NULL)
    {
        NeAACDecClose(handle);
    }
}

#endif