#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "audio_trans.h"

#define AUDIO_CODEC_PCM "pcm"
#define AUDIO_CODEC_G711A "g711a"
#define AUDIO_CODEC_AAC "aac"
#define AUDIO_CODEC_OPUS "opus"

#define OUT_FILE_PREFIX "out"
#define OUT_FILE_PCM OUT_FILE_PREFIX ".pcm"
#define OUT_FILE_G711A OUT_FILE_PREFIX ".g711a"
#define OUT_FILE_AAC OUT_FILE_PREFIX ".aac"
#define OUT_FILE_OPUS OUT_FILE_PREFIX ".opus"

#define FRAME_SIZE_MAX 10240

#define SUPPORT_IMI 1

#ifdef SUPPORT_IMI
static opus_uint32
char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0] << 24) | ((opus_uint32)ch[1] << 16) | ((opus_uint32)ch[2] << 8) | (opus_uint32)ch[3];
}
static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i >> 24;
    ch[1] = (i >> 16) & 0xFF;
    ch[2] = (i >> 8) & 0xFF;
    ch[3] = i & 0xFF;
}
#endif

ssize_t get_file_content(char *file_name, uint8_t **out_buf)
{
    int fd;
    size_t file_size;
    ssize_t read_size;

    fd = open(file_name, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Cannot open %s!\n", file_name);
        return -1;
    }

    file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    *out_buf = (uint8_t *)malloc(sizeof(uint8_t) * file_size);

    read_size = read(fd, *out_buf, file_size);
    if (read_size != file_size)
    {
        fprintf(stderr, "Have not get all connect from %s!\n", file_name);
        return -1;
    }

    close(fd);
    return read_size;
}

int get_one_ADTS_frame(unsigned char *buffer, size_t buf_size, unsigned char *data, size_t *data_size)
{
    size_t size = 0;

    if (!buffer || !data || !data_size)
    {
        return -1;
    }

    while (1)
    {
        if (buf_size < 7)
        {
            return -2;
        }

        if ((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0))
        {
            size |= ((buffer[3] & 0x03) << 11); // high 2 bit
            size |= buffer[4] << 3;             // middle 8 bit
            size |= ((buffer[5] & 0xe0) >> 5);  // low 3bit
            break;
        }
        --buf_size;
        ++buffer;
    }

    if (buf_size < size)
    {
        return -3;
    }

    memcpy(data, buffer, size);
    *data_size = size;

    return 0;
}

int pcm2aac(audio_param_t audio_param, char *src_filename)
{
    codec_handle aenc_handle = NULL;
    unsigned long input_len = 0;
    unsigned long output_len_max = 0;
    FILE *fp_read = NULL;
    FILE *fp_write = NULL;
    short *read_buf = NULL; // bit_depth=16, so use short
    unsigned char *aac_buf = NULL;
    int aac_out_len = 0;

    aenc_handle = aac_encode_init(audio_param, &input_len, &output_len_max);
    // printf("aenc input len=%lu, max out len=%lu\n", input_len, output_len_max);

    fp_read = fopen(src_filename, "r");
    if (fp_read == NULL)
    {
        fprintf(stderr, "cannot open %s\n", src_filename);
        return -1;
    }
    fp_write = fopen(OUT_FILE_AAC, "w");
    if (fp_write == NULL)
    {
        fprintf(stderr, "cannot open %s\n", OUT_FILE_AAC);
        return -1;
    }

    aac_buf = (unsigned char *)malloc(output_len_max);
    read_buf = (short *)malloc(input_len * sizeof(short));
    memset(read_buf, 0, input_len * sizeof(short));
    while (fread(read_buf, sizeof(short), input_len, fp_read) == input_len)
    {
        aac_out_len = aac_encode_frame(aenc_handle, (unsigned char *)read_buf, input_len, aac_buf, output_len_max);
        // printf("aac_out_len=%d\n", aac_out_len);
        if (aac_out_len <= 0)
        {
            continue;
        }
        fwrite(aac_buf, 1, aac_out_len, fp_write);
        memset(read_buf, 0, input_len * sizeof(short));
    }
    fp_read = NULL;
    // printf("aenc ok!!!!\n");

    free(aac_buf);
    free(read_buf);
    if (fp_write != NULL)
    {
        fclose(fp_write);
    }
    if (fp_read != NULL)
    {
        fclose(fp_read);
    }

    acc_encode_deinit(aenc_handle);

    return 0;
}

int aac2pcm(audio_param_t audio_param, char *src_filename)
{
    int ret = 0;
    codec_handle adec_handle = NULL;
    uint8_t *aac_buf = NULL;
    ssize_t aac_buf_len = 0;
    unsigned char frame[FRAME_SIZE_MAX] = {0};
    unsigned long frame_len = 0;
    unsigned char *input_data = NULL;
    size_t input_data_len = 0;
    unsigned char pcm_buf[FRAME_SIZE_MAX] = {0};
    int pcm_len = 0;
    FILE *fp_write = NULL;

    fp_write = fopen(OUT_FILE_PCM, "w");
    if (fp_write == NULL)
    {
        fprintf(stderr, "cannot open %s\n", OUT_FILE_PCM);
        return -1;
    }

    aac_buf_len = get_file_content(src_filename, &aac_buf);
    if (aac_buf_len <= 0)
    {
        fprintf(stderr, "cannot read %s\n", src_filename);
        return -1;
    }

    ret = get_one_ADTS_frame(aac_buf, aac_buf_len, frame, &frame_len);
    if (ret < 0)
    {
        fprintf(stderr, "cannot find adts frame. ret=%d\n", ret);
        return -1;
    }

    adec_handle = aac_decode_init(audio_param, frame, frame_len);
    if (adec_handle == NULL)
    {
        fprintf(stderr, "aac_decode_init err\n");
        return -1;
    }

    input_data = aac_buf;
    input_data_len = aac_buf_len;
    while (get_one_ADTS_frame(input_data, input_data_len, frame, &frame_len) == 0)
    {
        pcm_len = aac_decode_frame(adec_handle, audio_param, frame, frame_len, pcm_buf, sizeof(pcm_buf));
        if (pcm_len > 0)
        {
            // printf("pcm len=%d\n", pcm_len);
            fwrite(pcm_buf, 1, pcm_len, fp_write);
        }
        input_data_len -= frame_len;
        input_data += frame_len;
    }
    // printf("decode aac ok!!!\n");
    fclose(fp_write);
    free(aac_buf);

    acc_decode_deinit(adec_handle);

    return 0;
}

int pcm2g711a(char *src_filename)
{
    int ret = 0;
    int pcm_len = 0;
    unsigned char *pcm_buf = NULL;
    int g711a_len = 0;
    unsigned char *g711a_buf = NULL;
    FILE *fp = NULL;

    pcm_len = get_file_content(src_filename, &pcm_buf);

    g711a_len = (pcm_len + 1) / 2;
    g711a_buf = (unsigned char *)malloc(g711a_len);
    ret = g711a_encode(pcm_buf, pcm_len, g711a_buf, g711a_len);
    if (ret < 0)
    {
        return ret;
    }

    fp = fopen(OUT_FILE_G711A, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "open %s failed!!!\n", OUT_FILE_G711A);
        return -1;
    }
    fwrite(g711a_buf, sizeof(unsigned char), g711a_len, fp);
    fclose(fp);

    free(g711a_buf);
    free(pcm_buf);

    return 0;
}

int g711a2pcm(char *src_filename)
{
    int ret = 0;
    int pcm_len = 0;
    unsigned char *pcm_buf = NULL;
    int g711a_len = 0;
    unsigned char *g711a_buf = NULL;
    FILE *fp = NULL;

    g711a_len = get_file_content(src_filename, &g711a_buf);

    pcm_len = g711a_len * 2;
    pcm_buf = (unsigned char *)malloc(pcm_len);
    ret = g711a_decode(g711a_buf, g711a_len, pcm_buf, pcm_len);
    if (ret < 0)
    {
        return ret;
    }

    fp = fopen(OUT_FILE_PCM, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Open %s failed!!!\n", OUT_FILE_PCM);
        return -1;
    }
    fwrite(pcm_buf, sizeof(unsigned char), pcm_len, fp);
    free(pcm_buf);
    free(g711a_buf);
    fclose(fp);

    return 0;
}

int pcm2opus(audio_param_t audio_param, char *src_filename)
{
    FILE *fp = NULL;
    uint8_t *opus_buf = NULL;
    int opus_buf_len = 0;
    int decode_offset = 0;
    int pcm_len = 0;
    unsigned char *pcm_buf = NULL;
    codec_handle handle = NULL;

    pcm_len = get_file_content(src_filename, &pcm_buf);

    fp = fopen(OUT_FILE_OPUS, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Open %s failed!!!\n", OUT_FILE_OPUS);
        return -1;
    }

    handle = opus_encode_init(audio_param);
    if (handle == NULL)
    {
        fprintf(stderr, "opus encode init failed!!!\n");
        return -1;
    }

    opus_buf = (uint8_t *)malloc(sizeof(uint8_t) * FRAME_SIZE_MAX);
    while (1)
    {
        opus_buf_len = opus_encode_frame(handle, pcm_buf + decode_offset,
                                         audio_param.samplerate / audio_param.fps * sizeof(opus_int16),
                                         opus_buf, FRAME_SIZE_MAX);
#ifdef SUPPORT_IMI
        unsigned char ch[4];
        int_to_char(opus_buf_len, ch);
        fwrite(ch, sizeof(unsigned char), 4, fp);
        memset(ch, 0, 4);
        fwrite(ch, sizeof(unsigned char), 4, fp);
#else
        fwrite(&opus_buf_len, sizeof(int), 1, fp);
#endif
        fwrite(opus_buf, sizeof(uint8_t), opus_buf_len, fp);
        decode_offset += audio_param.samplerate / audio_param.fps * sizeof(opus_int16);

        if (decode_offset >= pcm_len)
        {
            break;
        }
    }

    fclose(fp);
    free(opus_buf);
    free(pcm_buf);
    opus_encode_deinit(handle);

    return 0;
}

int opus2pcm(audio_param_t audio_param, char *src_filename)
{

    FILE *fp = NULL;
    codec_handle handle = NULL;
    opus_int16 *pcm_buf = NULL;
    int pcm_buf_len = 0;
    int decode_offset = 0;
    int frame_len = 0;
    int opus_len = 0;
    unsigned char *opus_buf = NULL;

    opus_len = get_file_content(src_filename, &opus_buf);

    fp = fopen(OUT_FILE_PCM, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Open %s failed!!!\n", OUT_FILE_PCM);
        return -1;
    }

    handle = opus_decode_init(audio_param);
    if (handle == NULL)
    {
        fprintf(stderr, "opus decode init failed!!!\n");
        return -1;
    }

    pcm_buf = (opus_int16 *)malloc(sizeof(opus_int16) * FRAME_SIZE_MAX);
    while (1)
    {
#ifdef SUPPORT_IMI
        unsigned char ch[4];
        memcpy(ch, opus_buf + decode_offset, 4);
        frame_len = char_to_int(ch);
        decode_offset += 8;
#else
        memcpy(&frame_len, opus_buf + decode_offset, sizeof(int));
        decode_offset += sizeof(int);
#endif
        pcm_buf_len = opus_decode_frame(handle, opus_buf + decode_offset, frame_len, pcm_buf, FRAME_SIZE_MAX);
        fwrite(pcm_buf, sizeof(opus_int16), pcm_buf_len, fp);
        decode_offset += frame_len;

        if (decode_offset >= opus_len)
        {
            break;
        }
    }

    fclose(fp);
    free(pcm_buf);
    free(opus_buf);
    opus_decode_deinit(handle);
}

void printf_usage(char *cmd)
{
    printf("usage: %s [src_audio_file] [to_format]\n", cmd);
    printf("\t src_audio_file: which file you want to codec?\n");
    printf("\t to_format: pcm g711a aac opus\n");
}

aenc_format_e find_audio_format(char *format)
{
    // printf("[%s] format=%s, len=%lu\n", __func__, format, strlen(format));
    if (strncmp(format, AUDIO_CODEC_PCM, strlen(AUDIO_CODEC_PCM) + 1) == 0)
    {
        return AENC_FORMAT_PCM;
    }
    else if (strncmp(format, AUDIO_CODEC_G711A, strlen(AUDIO_CODEC_G711A) + 1) == 0)
    {
        return AENC_FORMAT_G711A;
    }
    else if (strncmp(format, AUDIO_CODEC_AAC, strlen(AUDIO_CODEC_AAC) + 1) == 0)
    {
        return AENC_FORMAT_AAC;
    }
    else if (strncmp(format, AUDIO_CODEC_OPUS, strlen(AUDIO_CODEC_OPUS) + 1) == 0)
    {
        return AENC_FORMAT_OPUS;
    }
    else
    {
        fprintf(stderr, "%s: Do not support this format!!!\n", format);
        return -1;
    }
}

int pcm2other(char *src_filename, audio_param_t audio_param, aenc_format_e to_format)
{
    int ret = 0;
    switch (to_format)
    {
    case AENC_FORMAT_PCM:
        fprintf(stderr, "pcm no need translete to pcm\n");
        return -1;
    case AENC_FORMAT_G711A:
        ret = pcm2g711a(src_filename);
        break;
    case AENC_FORMAT_AAC:
        ret = pcm2aac(audio_param, src_filename);
        break;
    case AENC_FORMAT_OPUS:
        ret = pcm2opus(audio_param, src_filename);
        break;

    default:
        break;
    }

    return ret;
}

int other2pcm(char *src_filename, audio_param_t audio_param, aenc_format_e from_format)
{
    int ret = 0;
    switch (from_format)
    {
    case AENC_FORMAT_PCM:
        fprintf(stderr, "pcm no need translete to pcm\n");
        return -1;
    case AENC_FORMAT_G711A:
        ret = g711a2pcm(src_filename);
        break;
    case AENC_FORMAT_AAC:
        ret = aac2pcm(audio_param, src_filename);
        break;
    case AENC_FORMAT_OPUS:
        ret = opus2pcm(audio_param, src_filename);
        break;

    default:
        break;
    }

    return ret;
}

int main(int argc, char **argv)
{
    int ret = 0;
    audio_param_t audio_param;
    aenc_format_e to_format = AENC_FORMAT_NONE;
    char stdin_get[512] = {0};
    char src_format[16] = {0};

    if (argc < 3)
    {
        fprintf(stderr, "param err!!!\n");
        printf_usage(argv[0]);
        return -1;
    }

    if (access(argv[1], F_OK) != 0)
    {
        fprintf(stderr, "%s: No such file!!!\n", argv[1]);
        printf_usage(argv[0]);
        return -2;
    }

    to_format = find_audio_format(argv[2]);
    if (to_format < 0)
    {
        fprintf(stderr, "%s: Do not support this format!!!\n", argv[2]);
        printf_usage(argv[0]);
        return -3;
    }

    memset(&audio_param, 0, sizeof(audio_param_t));
    audio_param.format = AENC_FORMAT_PCM;
    audio_param.bit_depth = 16;
    audio_param.channels = 1;
    audio_param.samplerate = AUDIO_SAMPLERATE_16000;
    audio_param.fps = 50;
    printf("Please enter some info about your source audio\n");
    printf("format(default pcm): ");
    if (fgets(stdin_get, sizeof(stdin_get), stdin) != NULL)
    {
        if (stdin_get[0] != '\n')
        {
            stdin_get[strlen(stdin_get) - 1] = 0;
            audio_param.format = find_audio_format(stdin_get);
            if (audio_param.format < 0)
            {
                fprintf(stderr, "format=%d, err param!!!\n", audio_param.format);
                return -4;
            }
            memcpy(src_format, stdin_get, strlen(stdin_get) + 1);
        }
    }
    printf("bit_depth(default 16): ");
    memset(stdin_get, 0, sizeof(stdin_get));
    if (fgets(stdin_get, sizeof(stdin_get), stdin) != NULL)
    {
        if (stdin_get[0] != '\n')
        {
            audio_param.bit_depth = atoi(stdin_get);
            if (audio_param.bit_depth % sizeof(char) != 0)
            {
                fprintf(stderr, "bit_depth=%d, err param!!!\n", audio_param.bit_depth);
                return -5;
            }
        }
    }
    printf("channels(default 1): ");
    memset(stdin_get, 0, sizeof(stdin_get));
    if (fgets(stdin_get, sizeof(stdin_get), stdin) != NULL)
    {
        if (stdin_get[0] != '\n')
        {
            audio_param.channels = atoi(stdin_get);
            if (audio_param.channels != 1 && audio_param.channels != 2)
            {
                fprintf(stderr, "channels=%d, err param!!!\n", audio_param.channels);
                return -6;
            }
        }
    }
    printf("samplerate(default 16000): ");
    memset(stdin_get, 0, sizeof(stdin_get));
    if (fgets(stdin_get, sizeof(stdin_get), stdin) != NULL)
    {
        if (stdin_get[0] != '\n')
        {
            audio_param.samplerate = atoi(stdin_get);
            if (audio_param.samplerate % 8000 != 0)
            {
                fprintf(stderr, "samplerate=%d, err param!!!\n", audio_param.samplerate);
                return -7;
            }
        }
    }
    printf("fps(default 50): ");
    memset(stdin_get, 0, sizeof(stdin_get));
    if (fgets(stdin_get, sizeof(stdin_get), stdin) != NULL)
    {
        if (stdin_get[0] != '\n')
        {
            audio_param.fps = atoi(stdin_get);
            if (audio_param.fps % 25 != 0)
            {
                fprintf(stderr, "fps=%d, err param!!!\n", audio_param.fps);
                return -8;
            }
        }
    }

    printf("Got param: format=%d, bit_depth=%d, channels=%d, samplerate=%d, fps=%d\n",
           audio_param.format, audio_param.bit_depth, audio_param.channels, audio_param.samplerate, audio_param.fps);

    if (audio_param.format == AENC_FORMAT_PCM)
    {
        ret = pcm2other(argv[1], audio_param, to_format);
    }
    else
    {
        ret = other2pcm(argv[1], audio_param, audio_param.format);
        if (ret != 0)
        {
            goto END;
        }
        if (to_format != AENC_FORMAT_PCM)
        {
            ret = pcm2other(OUT_FILE_PCM, audio_param, to_format);
        }
    }

END:
    if (ret != 0)
    {
        fprintf(stderr, "cannot translate %s, ret=%d\n", argv[1], ret);
        return -9;
    }

    return ret;
}