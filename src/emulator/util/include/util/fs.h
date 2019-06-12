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

#include <radical/radical.hpp>

class Root {
    Radical::Path base_path;
    Radical::Path pref_path;

public:
    void set_base_path(const Radical::Path &p) {
        base_path = p;
    }

    Radical::Path get_base_path() const {
        return base_path;
    }

    std::string get_base_path_string() const {
        return base_path.get();
    }

    void set_pref_path(const Radical::Path &p) {
        pref_path = p;
    }

    Radical::Path get_pref_path() const {
        return pref_path;
    }

    std::string get_pref_path_string() const {
        return pref_path.get();
    }
}; // class root

namespace fs_utils {

/**
  * \brief  Construct a file name (optionally with an extension) to be placed in a Vita3K directory.
  * \param  base_path   The main output path for the file.
  * \param  folder_path The sub-directory/sub-directories to output to.
  * \param  file_name   The name of the file.
  * \param  extension   The extension of the file (optional)
  * \return A complete Boost.Filesystem file path normalized.
  */
inline Radical::Path construct_file_name(const Radical::Path &base_path, const Radical::Path &folder_path, const Radical::Path &file_name, const Radical::Path &extension) {
    Radical::Path full_file_path{ base_path / folder_path / file_name };
    if (!extension.empty())
        full_file_path.setExtension(extension.get());

    return full_file_path;
}

} // namespace fs_utils
