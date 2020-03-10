#include <ngs/modules/player.h>

#include <cassert>

namespace ngs::player {
    std::unique_ptr<ngs::Module> VoiceDefinition::new_module() {
        return std::make_unique<Module>();
    }
    
    Module::Module() : ngs::Module(ngs::BUSS_NORMAL_PLAYER) { }

    void Module::process(const MemState &mem, Voice *voice) {
        Parameters *params = voice->get_parameters<Parameters>(mem);
        assert(false); // not sure what's supposed to happen here so I'll just have it tell me if it reaches here
    }
}