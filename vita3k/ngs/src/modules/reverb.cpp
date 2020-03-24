#include <ngs/modules/reverb.h>

#include <util/log.h>

namespace ngs::reverb {
    std::size_t VoiceDefinition::get_buffer_parameter_size() const {
        return sizeof(Parameters);
    }

    std::unique_ptr<ngs::Module> VoiceDefinition::new_module() {
        return std::make_unique<Module>();
    }

    Module::Module() : ngs::Module(ngs::BussType::BUSS_REVERB) { }

    void Module::process(const MemState &mem, Voice *voice) {
        const std::lock_guard<std::mutex> guard(*voice->voice_lock);

        auto *params = voice->get_parameters<Parameters>(mem);

        LOG_INFO(":thinking:");

        // TODO: actually implement reverb, plan is to pass through for now
    }

    void Module::get_expectation(AudioDataType *expect_audio_type, std::int16_t *expect_channel_count) { }
}