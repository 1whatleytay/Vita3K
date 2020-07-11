#include <codec/state.h>

#define DEBUG

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/log.h>
#include <libswresample/swresample.h>
}

#include <util/log.h>

uint32_t AacDecoderState::get(DecoderQuery query) {
    switch (query) {
    case DecoderQuery::CHANNELS: return context->channels;
    case DecoderQuery::BIT_RATE: return context->bit_rate;
    case DecoderQuery::SAMPLE_RATE: return context->sample_rate;
    default:
        return 0;
    }
}

bool AacDecoderState::send(const uint8_t *data, uint32_t size) {
    AVPacket *packet = av_packet_alloc();

    std::vector<uint8_t> temporary(size + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(temporary.data(), data, size);

    packet->size = size;
    packet->data = temporary.data();

    int err = avcodec_send_packet(context, packet);
    av_packet_free(&packet);
    if (err < 0) {
        LOG_WARN("Error sending Aac packet: {}.", codec_error_name(err));
        return false;
    }

    return true;
}

bool AacDecoderState::receive(uint8_t *data, DecoderSize *size) {
    AVFrame *frame = av_frame_alloc();

    int err = avcodec_receive_frame(context, frame);
    if (err < 0) {
        LOG_WARN("Error receiving Aac frame: {}.", codec_error_name(err));
        av_frame_free(&frame);
        return false;
    }

    assert(frame->format == AV_SAMPLE_FMT_FLTP);

    if (data) {
        convert_f32_to_s16(
            reinterpret_cast<float *>(frame->extended_data),
            reinterpret_cast<int16_t *>(data),
            frame->channels, frame->channels,
            frame->nb_samples, frame->sample_rate);
    }

    if (size) {
        size->samples = frame->nb_samples;
    }

    av_frame_free(&frame);
    return true;
}

AacDecoderState::AacDecoderState(uint32_t sample_rate, uint32_t channels) {
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    assert(codec);

    context = avcodec_alloc_context3(codec);
    assert(context);

    context->channels = channels;
    context->sample_rate = sample_rate;

    int err = avcodec_open2(context, codec, nullptr);
    assert(err == 0);
}
