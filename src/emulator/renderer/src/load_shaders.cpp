#include <renderer/functions.h>

#include "functions.h"
#include "profile.h"

#include <gxm/types.h>
#include <renderer/types.h>
#include <shader/spirv_recompiler.h>
#include <util/fs.h>
#include <util/log.h>

#include <utility>

using namespace glbinding;

namespace renderer {
static std::string load_shader(const char *hash, const char *extension, const char *base_path) {
    const auto shader_path = fs_utils::construct_file_name(base_path, "shaders", hash, extension);
    std::ifstream is(shader_path.get(), std::ifstream::binary);
    if (!is) {
        return std::string();
    }

    is.seekg(0, std::ifstream::end);
    const size_t size = is.tellg();
    is.seekg(0);

    std::string source(size, ' ');
    is.read(&source.front(), size);

    return source;
}

static void dump_missing_shader(const char *hash, const char *extension, const SceGxmProgram &program, const char *source, const char *spirv,
    const char *disasm, const char *base_path, const char *title_id) {
    const Radical::Path shader_base_dir{ Radical::Path("shaderlog") / title_id };
    if (!(Radical::Path(base_path) / shader_base_dir).exists())
        (Radical::Path(base_path) / shader_base_dir).createDirectories();

    const auto shader_base_path = fs_utils::construct_file_name(base_path, shader_base_dir, hash, extension);

    // Dump missing shader GLSL.
    std::ofstream glsl_file(shader_base_path.get());
    if (glsl_file) {
        glsl_file << source;
        glsl_file.close();
    }

    const auto write_data_with_ext = [&](const char *ext, const char *data, const std::int64_t size) {
        // Dump missing shader binary.
        Radical::Path out_path{ shader_base_path };
        out_path.setExtension(ext);

        if (size == -1) {
            std::ofstream of{ out_path.get() };
            if (!of.fail()) {
                of << data; // This is a normal string
                of.close();
            }
        } else {
            std::ofstream of{ out_path.get(), std::ofstream::binary };
            if (!of.fail()) {
                of.write(data, size);
                of.close();
            }
        }
    };

    write_data_with_ext(".gxp", reinterpret_cast<const char *>(&program), program.size);
    write_data_with_ext(".dsm", disasm, -1);
    write_data_with_ext(".spt", spirv, -1);
}

GLSLCacheEntry load_shader(GLSLCache &cache, const SceGxmProgram &program, const char *base_path, const char *title_id) {
    const Sha256Hash hash_bytes = sha256(&program, program.size);
    const GLSLCache::const_iterator cached = cache.find(hash_bytes);
    if (cached != cache.end()) {
        return { cached->first, cached->second };
    }

    auto shader_type_to_str = [](emu::SceGxmProgramType type) {
        return type == emu::Vertex ? "vert" : type == emu::Fragment ? "frag" : "unknown";
    };

    emu::SceGxmProgramType program_type = program.get_type();
    const char *shader_type_str = shader_type_to_str(program_type);

    const Sha256HashText hash_text = hex(hash_bytes);

    std::string source = load_shader(hash_text.data(), shader_type_str, base_path);
    if (source.empty()) {
        LOG_INFO("Generating {} shader {}", shader_type_str, hash_text.data());

        std::string spirv_dump;
        std::string disasm_dump;

        source = shader::convert_gxp_to_glsl(program, hash_text.data(), false, &spirv_dump, &disasm_dump);

        dump_missing_shader(hash_text.data(), shader_type_str, program, source.c_str(), spirv_dump.c_str(), disasm_dump.c_str(),
            base_path, title_id);
    }

    cache.emplace(hash_bytes, source);

    return { hash_bytes, source };
}

} // namespace renderer
