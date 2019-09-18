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

#pragma once

#include <app/screen_rendererer.h>

struct SDL_Window;

namespace app {
class gl_screen_renderer : screen_renderer {
public:
    explicit gl_screen_renderer(SDL_Window *window);
    ~gl_screen_renderer() override;

    bool init(const std::string &base_path) override;
    void render(const HostState &state) override;

    void begin_render() override;
    void end_render() override;

private:
    SDL_Window *window{};

    GLuint m_vao{0};
    GLuint m_vbo{0};
    SharedGLObject m_render_shader;
    GLuint m_screen_texture{0};

    GLint posAttrib{0};
    GLint uvAttrib{0};
};
}
