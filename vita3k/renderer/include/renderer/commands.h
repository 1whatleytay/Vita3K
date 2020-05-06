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

#include <renderer/types.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <queue>

struct GxmContextState;

namespace renderer {
#define REPORT_MISSING(backend) LOG_ERROR("Unimplemented graphics API handler with backend {}", (int)backend)
#define REPORT_STUBBED() LOG_INFO("Stubbed")

struct Renderer;
struct Context;

enum class CommandOpcode : std::uint16_t {
    // These two functions are sync, and taking pointer as parameter.
    CreateContext = 0,
    CreateRenderTarget = 1,

    /// Do draw.
    Draw = 2,

    /// This is like a NOP. It only signals back to client.
    Nop = 3,

    /// Set a GXM state.
    SetState = 4,

    SetContext = 5,
    SyncSurfaceData = 6,

    /// Jump to another command pointer, save current command pointer on a stack
    JumpWithLink = 7,

    /// Pop the stack, and jump the command popped
    JumpBack = 8,

    /// Signal sync object that fragment has been done.
    SignalSyncObject = 9,

    DestroyRenderTarget = 10
};

enum CommandErrorCode {
    CommandErrorCodeNone = 0,
    CommandErrorCodePending = -1,
    CommandErrorArgumentsTooLarge = -2
};

constexpr std::size_t MAX_COMMAND_DATA_SIZE = 0x40;

struct Command {
    CommandOpcode opcode;
    std::array<uint8_t, MAX_COMMAND_DATA_SIZE> data = { };
    int *status;

    // temporary solution, I want to use std::stack or std::queue
    uint32_t pushIndex = 0;
    uint32_t popIndex = 0;

    Command *next{};

    template <typename T>
    bool push(T &val) {
        if (pushIndex + sizeof(T) > MAX_COMMAND_DATA_SIZE) {
            return false;
        }

        *(T *)&data[pushIndex] = val;
        pushIndex += sizeof(T);

        return true;
    }

    template <typename T>
    T pop() {
        if (popIndex + sizeof(T) > pushIndex) {
            // popped too much
            assert(false);
        }

        T *item = (T *)&data[popIndex];
        popIndex += sizeof(T);

        return *item;
    }

    template <typename Head, typename... Args>
    bool push_arguments(Head arg1, Args... args2) {
        if (!push(arg1)) {
            return false;
        }

        if constexpr (sizeof...(args2) > 0) {
            return push_arguments(args2...);
        }

        return true;
    }

    template <typename... Args>
    Command(CommandOpcode opcode, int *status, Args... arguments) : opcode(opcode), status(status) {
        if constexpr (sizeof...(arguments) > 0) {
            if (!push_arguments(arguments...)) {
                assert(false);
            }
        }
    }

    void complete(int code) const;
};

// This somehow looks like vulkan
// It's to split a command list easier when ExecuteCommandList is used.
struct CommandList {
    std::queue<Command> commands;
    Context *context{ }; ///< The HLE context that try to execute this buffer.
    GxmContextState *gxm_context{ }; ///< The GXM context associated.

    // do we need this?
    template <typename... Args>
    void add(CommandOpcode opcode, const int *status, Args... arguments) {
        commands.push(Command(opcode, status, arguments...));
    }

    template <typename... Args>
    void add_set_state(Context *ctx, GXMState state, Args... arguments) {
        add(CommandOpcode::SetState, nullptr, state, arguments...);
    }
};
} // namespace renderer
