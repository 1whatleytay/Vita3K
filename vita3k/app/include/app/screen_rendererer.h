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

#include <glutil/shader.h>

struct HostState;

namespace app {
class screen_renderer {
public:
    virtual ~screen_renderer() = default;

    virtual bool init(const std::string &base_path) = 0;
    virtual void render(const HostState &state) = 0;

    virtual void begin_render() = 0;
    virtual void end_render() = 0;

protected:
    struct screen_vertex {
        GLfloat pos[3];
        GLfloat uv[2];
    };

    static constexpr size_t screen_vertex_size = sizeof(screen_vertex);
    static constexpr uint32_t screen_vertex_count = 4;

    using screen_vertices_t = screen_vertex[screen_vertex_count];
};
} // namespace app
