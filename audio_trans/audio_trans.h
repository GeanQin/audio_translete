#ifndef __AUDIO_TRANS_H__
#define __AUDIO_TRANS_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef void *codec_handle;

typedef enum  
{
	AENC_FORMAT_NONE = 0,
	AENC_FORMAT_PCM = AENC_FORMAT_NONE,
	AENC_FORMAT_AAC = 1,
	AENC_FORMAT_G711A,
	AENC_FORMAT_OPUS
} aenc_format_e;

typedef enum
{
    AUDIO_SAMPLERATE_8000   = 8000,
    AUDIO_SAMPLERATE_16000  = 16000,
    AUDIO_SAMPLERATE_32000  = 32000,
    AUDIO_SAMPLERATE_48000  = 48000
} audio_samplerate_e;

typedef struct
{
    audio_samplerate_e samplerate;
    int channels;
    int bit_depth;
    int fps;
    aenc_format_e format;
} audio_param_t;

#if 1   //  aac编码器 
/*
 * 初始化aac编码器
 * @param[in]
 *      audio_param     音频参数
 * @param[out]
 *      input_len       编码时需要输入的数据量
 *      output_len_max  编码后输出的最大数据量
 * @retval
 *      codec_handle    解码器句柄
 *      NULL            失败
 */
codec_handle aac_encode_init(audio_param_t audio_param, unsigned long *input_len, unsigned long *output_len_max);
/*
 * pcm编码为aac
 * @param[in]
 *      handle          解码器句柄
 *      input_buf       输入的buff
 *      input_len       aac_encode_init获取到的input_len
*       output_buf_size 输出buff的大小
 * @param[out]
 *      output_buf      编码后的buff
 * @retval
 *      >0              编码后的长度
 *      <=0             失败
 */
int aac_encode_frame(codec_handle handle, unsigned char *input_buf, unsigned long input_len, unsigned char *output_buf, int output_buf_size);
/*
 * 关闭aac编码器
 * @param[in]
 *      handle          编码器句柄
 */
void acc_encode_deinit(codec_handle handle);
#endif

#if 1   //  aac解码器
/*
 * 初始化aac解码器
 * @param[in]
 *      audio_param     音频参数
 *      frame           aac一帧数据
 *      frame_len       数据长度
 * @retval
 *      codec_handle    解码器句柄
 *      NULL            失败
 */
codec_handle aac_decode_init(audio_param_t audio_param, unsigned char *frame, unsigned long frame_len);
/*
 * aac解码为pcm
 * @param[in]
 *      handle          解码器句柄
 *      audio_param     音频参数
 *      input_buf       输入的帧信息
 *      input_len       输入的帧长度
 *      output_buf_size 解码后的buff的大小
 * @param[out]
 *      output_buf      解码后的buff
 * @retval
 *      >0              解码后的长度
 *      <=0             失败
 */
int aac_decode_frame(codec_handle handle, audio_param_t audio_param, unsigned char *input_buf, unsigned long input_len, unsigned char *output_buf, int output_buf_size);
/*
 * 关闭aac解码器
 * @param[in]
 *      handle          解码器句柄
 */
void acc_decode_deinit(codec_handle handle);

#endif

#if 1   //  g711a编码
/*
 * pcm编码为g711a
 * @param[in]
 *      pcm_data        输入的buff
 *      pcm_len         buff的长度
 *      g711a_len       输出buff的大小
 * @param[out]
 *      g711a_data      编码后的buff
 * @retval
 *      >0              编码后的长度
 *      <=0             失败
 */
int g711a_encode(char *pcm_data, int pcm_len, char *g711a_data, int g711a_len);
#endif

#if 1   //  g711a解码
/*
 * aac解码为g711a
 * @param[in]
 *      g711a_data      输入的buff
 *      g711a_len       输入的buff长度
 *      pcm_len         输出buff的大小
 * @param[out]
 *      pcm_buf         解码后的buff
 * @retval
 *      >0              解码后的长度
 *      <=0             失败
 */
int g711a_decode(char *g711a_data, int g711a_len, char *pcm_buf, int pcm_len);
#endif

#if 1   //  opus编码
/*
 * 初始化opus编码器
 * @param[in]
 *      audio_param     音频参数
 * @retval
 *      codec_handle    编码器句柄
 *      NULL            失败
 */
codec_handle opus_encode_init(audio_param_t audio_param);
/*
 * pcm编码为opus
 * @param[in]
 *      handle          解码器句柄
 *      input_buf       输入的buff
 *      input_len       输入的buff长度
 *      output_buf_size 输出buff的大小
 * @param[out]
 *      output_buf      编码后的buff
 * @retval
 *      >0              编码后的长度
 *      <=0             失败
 */
int opus_encode_frame(codec_handle handle, unsigned char *input_buf, unsigned long input_len, unsigned char *output_buf, int output_buf_size);
/*
 * 关闭opus解码器
 * @param[in]
 *      handle          解码器句柄
 */
void opus_encode_deinit(codec_handle handle);
#endif

#if 1   //  opus解码
#include "opus/opus_types.h"
/*
 * 初始化opus解码器
 * @param[in]
 *      audio_param     音频参数
 * @retval
 *      codec_handle    解码器句柄
 *      NULL            失败
 */
codec_handle opus_decode_init(audio_param_t audio_param);
/*
 * opus解码为pcm
 * @param[in]
 *      handle          解码器句柄
 *      input_buf       输入的帧信息
 *      input_len       输入的帧长度
 *      output_buf_size 解码后buff的大小
 * @param[out]
 *      output_buf      解码后的buff
 * @retval
 *      >0              解码后的长度
 *      <=0             失败
 */
int opus_decode_frame(codec_handle handle, unsigned char *input_buf, unsigned long input_len, opus_int16 *output_buf, int output_buf_size);
/*
 * 关闭opus解码器
 * @param[in]
 *      handle          解码器句柄
 */
void opus_decode_deinit(codec_handle handle);
#endif

#ifdef __cplusplus
}
#endif

#endif