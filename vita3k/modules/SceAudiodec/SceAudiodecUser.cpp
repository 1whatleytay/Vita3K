// Vita3K emulator project
// Copyright (C) 2018 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include "SceAudiodecUser.h"

#include <util/lock_and_find.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

#define SCE_AUDIO_MIN_LEN 64

enum SceAudiodecType {
    SCE_AUDIODEC_TYPE_AT9  = 0x1003U,
    SCE_AUDIODEC_TYPE_MP3  = 0x1004U,
    SCE_AUDIODEC_TYPE_AAC  = 0x1005U,
    SCE_AUDIODEC_TYPE_CELP = 0x1006U
};

struct SceAudiodecInfoAt9 {
    SceUInt32 size;
    SceUInt8  configData[4];
    SceUInt32 ch;
    SceUInt32 bitRate;
    SceUInt32 samplingRate;
    SceUInt32 superFrameSize;
    SceUInt32 framesInSuperFrame;
};

struct SceAudiodecInfoMp3 {
    SceUInt32 size;      //!< sizeof(SceAudiodecInfoMp3)
    SceUInt32 ch;        //!< number of channels (mono: 1, stereo/joint stereo/two mono: 2)
    SceUInt32 version;   //!< MPEG version (MPEG1: 3, MPEG2: 2, MPEG2.5: 0)
};

struct SceAudiodecInfoAac {
    SceUInt32 size;
    SceUInt32 isAdts;
    SceUInt32 ch;
    SceUInt32 samplingRate;
    SceUInt32 isSbr;
};

/** Information structure for CELP */
struct SceAudiodecInfoCelp {
    SceUInt32 size;                  //!< sizeof(SceAudiodecInfoCelp)
    SceUInt32 excitationMode;        //!< Excitation mode
    SceUInt32 samplingRate;          //!< Sampling rate
    SceUInt32 bitRate;               //!< Bit rate (one of ::SceAudiodecCelpBitrate)
    SceUInt32 lostCount;
};

union SceAudiodecInfo {
    SceUInt32           size;
    SceAudiodecInfoAt9  at9;
    SceAudiodecInfoMp3  mp3;
    SceAudiodecInfoAac  aac;
    SceAudiodecInfoCelp celp;
};

struct SceAudiodecCtrl {
    SceUInt32 size;
    SceInt32 handle;
    Ptr<SceUInt8> pEs;          //! pointer to elementary stream
    SceUInt32 inputEsSize;	    //! size of elementary stream used actually (in byte)
    SceUInt32 maxEsSize;        //! maximum size of elementary stream used (in byte)
    Ptr<void> pPcm;			    //! pointer to PCM
    SceUInt32 outputPcmSize;    //! size of PCM output actually (in byte)
    SceUInt32 maxPcmSize;       //! maximum size of PCM output (in byte)
    SceUInt32 wordLength;       //! PCM bit depth
    Ptr<SceAudiodecInfo> pInfo; //! pointer to SceAudiodecInfo
};

struct At9Info {
    uint32_t version = 2;
    uint8_t config[4];
    uint32_t padding = 0;
};

EXPORT(int, sceAudiodecClearContext) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecCreateDecoder, SceAudiodecCtrl *ctrl, SceAudiodecType type) {
    // only supports AT9 decoding atm
    assert(type == SCE_AUDIODEC_TYPE_AT9);

    SceUID handle;
    DecoderPtr decoder_info = std::make_shared<DecoderState>();

    {
        std::lock_guard<std::mutex> guard(host.kernel.mutex);
        handle = host.kernel.get_next_uid();
        host.kernel.decoders[handle] = decoder_info;
    }
    ctrl->handle = handle;

    SceAudiodecInfoAt9 *at9_info = ctrl->pInfo.cast<SceAudiodecInfoAt9>().get(host.mem);

    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_ATRAC9);

    At9Info info;
    std::memcpy(&info.config, at9_info->configData, sizeof(info.config));

    decoder_info->context = avcodec_alloc_context3(codec);
    decoder_info->context->extradata = reinterpret_cast<uint8_t *>(&info);
    decoder_info->context->extradata_size = sizeof(info);
    decoder_info->context->block_align = ctrl->wordLength / 8;
    int err = avcodec_open2(decoder_info->context, codec, nullptr);
    assert(err == 0);

    at9_info->ch = decoder_info->context->channels;
    at9_info->bitRate = decoder_info->context->bit_rate;
    at9_info->samplingRate = decoder_info->context->sample_rate;
    // must be a multiple of SCE_AUDIO_MIN_LEN
    ctrl->maxPcmSize = SCE_AUDIO_MIN_LEN * decoder_info->context->channels * sizeof(uint16_t) * 8; // 512 samples
    // set this to some value as an experiment, no idea how this is supposed to work
    ctrl->maxEsSize = SCE_AUDIO_MIN_LEN * decoder_info->context->channels * sizeof(uint16_t) * 4;

    return 0;
}

EXPORT(int, sceAudiodecCreateDecoderExternal) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecCreateDecoderResident) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecDecode, SceAudiodecCtrl *ctrl) {
    const DecoderPtr &decoder_info = lock_and_find(ctrl->handle, host.kernel.decoders, host.kernel.mutex);

    int err;

    AVPacket *packet = av_packet_alloc();
    packet->data = ctrl->pEs.get(host.mem);
    packet->size = ctrl->maxEsSize;

    LOG_INFO("Decoding: {:0>8X}", ctrl->pEs.address());

    err = avcodec_send_packet(decoder_info->context, packet);

    switch (err) {
        case AVERROR(EAGAIN):
            break;
        case AVERROR_EOF:
            break;
        case AVERROR(EINVAL):
            break;
        case AVERROR(ENOMEM):
            break;
        case AVERROR_INVALIDDATA:
            break;
        default:
            break;
    }
    assert(err == 0);
    av_packet_free(&packet);

    AVFrame *frame = av_frame_alloc();

    err = avcodec_receive_frame(decoder_info->context, frame);
    assert(err == 0);

    ctrl->inputEsSize = ctrl->maxEsSize;
    ctrl->outputPcmSize = frame->linesize[0] / sizeof(float) * frame->channels;
    assert(ctrl->outputPcmSize <= ctrl->maxPcmSize);
    int16_t *audio_data = ctrl->pPcm.cast<int16_t>().get(host.mem);

    // Rinne says pcm data is interleaved, I will assume it is float pcm for now...
    for (uint32_t a = 0; a < frame->nb_samples; a++) {
        for (uint32_t b = 0; b < frame->channels; b++) {
            auto *frame_data = reinterpret_cast<float *>(frame->data[b]);
            float current_sample = frame_data[a];
            int16_t pcm_sample = current_sample * INT16_MAX;

            audio_data[a * frame->channels + b] = pcm_sample;
        }
    }
//    std::memcpy(ctrl->pPcm.get(host.mem), frame->data[0], frame->linesize[0]);

    av_frame_free(&frame);

    return 0;
}

EXPORT(int, sceAudiodecDecodeNFrames) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecDecodeNStreams) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecDeleteDecoder) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecDeleteDecoderExternal) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecDeleteDecoderResident) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecGetContextSize) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecGetInternalError) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecInitLibrary) {
    return STUBBED("EMPTY");
}

EXPORT(int, sceAudiodecPartlyDecode) {
    return UNIMPLEMENTED();
}

EXPORT(int, sceAudiodecTermLibrary) {
    return UNIMPLEMENTED();
}

BRIDGE_IMPL(sceAudiodecClearContext)
BRIDGE_IMPL(sceAudiodecCreateDecoder)
BRIDGE_IMPL(sceAudiodecCreateDecoderExternal)
BRIDGE_IMPL(sceAudiodecCreateDecoderResident)
BRIDGE_IMPL(sceAudiodecDecode)
BRIDGE_IMPL(sceAudiodecDecodeNFrames)
BRIDGE_IMPL(sceAudiodecDecodeNStreams)
BRIDGE_IMPL(sceAudiodecDeleteDecoder)
BRIDGE_IMPL(sceAudiodecDeleteDecoderExternal)
BRIDGE_IMPL(sceAudiodecDeleteDecoderResident)
BRIDGE_IMPL(sceAudiodecGetContextSize)
BRIDGE_IMPL(sceAudiodecGetInternalError)
BRIDGE_IMPL(sceAudiodecInitLibrary)
BRIDGE_IMPL(sceAudiodecPartlyDecode)
BRIDGE_IMPL(sceAudiodecTermLibrary)
