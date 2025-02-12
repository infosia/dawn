// Copyright 2025 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef SRC_TINT_LANG_SPIRV_READER_PARSER_NAMER_H_
#define SRC_TINT_LANG_SPIRV_READER_PARSER_NAMER_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace tint::spirv::reader {

/// A Namer maps SPIR-V IDs to strings.
///
/// Sanitization:
/// Some names are user-suggested, but "sanitized" in the sense that an
/// unusual character (e.g. invalid for use in WGSL identifiers) is remapped
/// to a safer character such as an underscore.  Also, sanitized names
/// never start with an underscore.
class Namer {
  public:
    /// Sanitizes the given string, to replace unusual characters with
    /// obviously-valid idenfier characters. An empy string yields "empty".
    /// A sanitized name never starts with an underscore.
    /// @param suggested_name input string
    /// @returns sanitized name, suitable for use as an identifier
    static std::string Sanitize(const std::string& suggested_name);

    /// Creates a new namer
    Namer();
    /// Destructor
    ~Namer();

    /// @param id the SPIR-V ID
    /// @returns true if we the given ID already has a registered name.
    bool HasName(uint32_t id) { return id_to_name_.find(id) != id_to_name_.end(); }

    /// Gets a name for the ID.
    /// @param id the SPIR-V ID
    /// @returns a name for the given ID. Generates a name if non exists.
    const std::string& GetName(uint32_t id) { return id_to_name_.find(id)->second; }

    /// Gets the registered name for a struct member. If no name has
    /// been registered for this member, then returns the empty string.
    /// member index is in bounds.
    /// @param id the SPIR-V ID of the struct type
    /// @param member_index the index of the member, counting from 0
    /// @returns the registered name for the ID, or an empty string if
    /// nothing has been registered.
    std::string GetMemberName(uint32_t id, uint32_t member_index) const;

    /// Saves a sanitized name for the given ID, if that ID does not yet
    /// have a registered name, and if the sanitized name has not already
    /// been registered to a different ID.
    /// @param id the SPIR-V ID
    /// @param suggested_name the suggested name
    void RegisterSanitizedName(uint32_t id, const std::string& suggested_name);

    /// Saves a sanitized name for a member of a struct, if that member
    /// does not yet have a registered name.
    /// @param struct_id the SPIR-V ID for the struct
    /// @param member_index the index of the member inside the struct
    /// @param suggested_name the suggested name
    void RegisterSanitizedMemberName(uint32_t struct_id,
                                     uint32_t member_index,
                                     const std::string& suggested_name);

  private:
    /// @param name a string
    /// @returns true if the string has been registered as a name.
    bool IsRegistered(const std::string& name) const {
        return name_to_id_.find(name) != name_to_id_.end();
    }

    /// Records a mapping from the given ID to a name.
    /// @param id the SPIR-V ID
    /// @param name the name to map to the ID
    void Register(uint32_t id, const std::string& name);

    /// Returns an unregistered name based on a given base name.
    /// @param base_name the base name
    /// @returns a new name
    std::string FindUnusedDerivedName(const std::string& base_name);

    // Maps an ID to its registered name.
    std::unordered_map<uint32_t, std::string> id_to_name_;
    // Maps a name to a SPIR-V ID, or 0 (the case for derived names).
    std::unordered_map<std::string, uint32_t> name_to_id_;

    // Maps a struct id and member index to a suggested sanitized name.
    // If entry k in the vector is an empty string, then a suggestion
    // was recorded for a higher-numbered index, but not for index k.
    std::unordered_map<uint32_t, std::vector<std::string>> struct_member_names_;

    // Saved search id suffix for a given base name. Used by
    // FindUnusedDerivedName().
    std::unordered_map<std::string, uint32_t> next_unusued_derived_name_id_;
};

}  // namespace tint::spirv::reader

#endif  // SRC_TINT_LANG_SPIRV_READER_PARSER_NAMER_H_
