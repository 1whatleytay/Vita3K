#pragma once

#include <renderer/commands.h>
#include <renderer/types.h>

struct GxmContextState;
struct MemState;
struct FeatureState;
struct Config;

namespace renderer {
struct Renderer;
struct Context;
struct FragmentProgram;
struct RenderTarget;
struct VertexProgram;

/**
 * \brief Wait for all subjects to be done with the given sync object.
 */
void wishlist(SceGxmSyncObject *sync_object, SyncObjectSubject subjects);

/**
 * \brief Set list of subject with sync object to done.
 * 
 * This will also signals wishlists that are waiting.
 */
void subject_done(SceGxmSyncObject *sync_object, SyncObjectSubject subjects);

/**
 * Set some subjects to be in progress.
 */
void subject_in_progress(SceGxmSyncObject *sync_object, SyncObjectSubject subjects);

std::unique_ptr<Renderer> init(WindowPtr &window, Backend backend);

/**
 * \brief Copy uniform data and queue it to available command list.
 * 
 * Later when the uniform commands are processed, resources will be automatically freed.
 * 
 * For backend that support command list/buffer, this will be queued directly to API's command list/buffer.
 * 
 * \param state   The renderer state.
 * \param ctx     The context to queue uniform command in.
 * \param program Target program to get uniforms from.
 * \param buffers Set of all uniform buffers.
 * 
 */

struct TextureCacheState;

namespace texture {

// Paletted textures.
void palette_texture_to_rgba_4(uint32_t *dst, const uint8_t *src, size_t width, size_t height, const uint32_t *palette);
void palette_texture_to_rgba_8(uint32_t *dst, const uint8_t *src, size_t width, size_t height, const uint32_t *palette);
void yuv420_texture_to_rgb(uint8_t *dst, const uint8_t *src, size_t width, size_t height);
const uint32_t *get_texture_palette(const SceGxmTexture &texture, const MemState &mem);

/**
 * \brief Decompresses all the blocks of a DXT compressed texture and stores the resulting pixels in 'image'.
 * 
 * Output results is in format RGBA, with each channel being 8 bits.
 * 
 * \param width            Texture width.
 * \param height           Texture height.
 * \param block_storage    Pointer to compressed DXT1 blocks.
 * \param image            Pointer to the image where the decompressed pixels will be stored.
 * \param bc_type          Block compressed type. BC1 (DXT1), BC2 (DXT2) or BC3 (DXT3).
 */
void decompress_bc_swizz_image(std::uint32_t width, std::uint32_t height, const std::uint8_t *block_storage, std::uint32_t *image, const std::uint8_t bc_type);

void swizzled_texture_to_linear_texture(uint8_t *dest, const uint8_t *src, uint16_t width, uint16_t height, uint8_t bits_per_pixel);
void tiled_texture_to_linear_texture(uint8_t *dest, const uint8_t *src, uint16_t width, uint16_t height, uint8_t bits_per_pixel);

void cache_and_bind_texture(TextureCacheState &cache, const SceGxmTexture &gxm_texture, const MemState &mem);
size_t bits_per_pixel(SceGxmTextureBaseFormat base_format);
bool is_compressed_format(SceGxmTextureBaseFormat base_format, std::uint32_t width, std::uint32_t height, size_t &source_size);

} // namespace texture

} // namespace renderer
