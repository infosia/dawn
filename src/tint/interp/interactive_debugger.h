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

#ifndef SRC_TINT_INTERP_INTERACTIVE_DEBUGGER_H_
#define SRC_TINT_INTERP_INTERACTIVE_DEBUGGER_H_

#include <istream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "tint/tint.h"
#include "tint/utils/text/styled_text_printer.h"

// Forward Declarations
namespace tint::ast {
class Statement;
}  // namespace tint::ast
namespace tint::interp {
class ShaderExecutor;
}  // namespace tint::interp

namespace tint::interp {

/// The InteractiveDebugger class implements a command-line interactive WGSL shader debugger.
class InteractiveDebugger {
  public:
    /// Construct an interactive debugger for a given shader executor.
    /// @param executor the shader executor to attach to
    /// @param input the command input stream
    explicit InteractiveDebugger(ShaderExecutor& executor, std::istream& input);

  private:
    /// Handle errors during shader execution.
    /// @param error the error diagnostics
    void Error(diag::List&& error);

    /// Provide an interaction point for the user.
    /// @param force_break if `true`, will always break to a user prompt
    void Interact(bool force_break);

    /// Set up a breakpoint.
    /// @param tokens the command tokens entered by the user
    void Backtrace(const std::vector<std::string>& tokens);

    /// Set up a breakpoint.
    /// @param tokens the command tokens entered by the user
    void Break(const std::vector<std::string>& tokens);

    /// List or modify existing breakpoints.
    /// @param tokens the command tokens entered by the user
    void Breakpoint(const std::vector<std::string>& tokens);

    /// Switch to a different invocation.
    /// @param tokens the command tokens entered by the user
    void Invocation(const std::vector<std::string>& tokens);

    /// Switch to a different workgroup.
    /// @param tokens the command tokens entered by the user
    void Workgroup(const std::vector<std::string>& tokens);

    /// Show the backtrace for the current invocation.
    /// @param max_depth the number of frames to show
    void ShowBacktrace(uint32_t max_depth) const;

    /// Show the current execution context.
    void ShowContext() const;

    /// Output a range of source lines, optionally highlighting a particular source range.
    /// @param first the first line number to output
    /// @param count the number of lines to output
    /// @param highlight the optional source range to highlight
    void ShowLines(int64_t first, int64_t count, Source::Range highlight = {}) const;

    const Source::File& source_;
    ShaderExecutor& executor_;
    std::istream& input_;
    std::unique_ptr<StyledTextPrinter> diag_printer_;

    bool interactive_;
    bool use_rich_text_;

    std::string last_command;
    bool continuing_ = false;
    const ast::Statement* last_stmt = nullptr;
    std::unordered_set<const ast::Node*> breakpoints_;
    std::unordered_map<uint64_t, const ast::Node*> possible_breakpoints_;
};

}  // namespace tint::interp

#endif  // SRC_TINT_INTERP_INTERACTIVE_DEBUGGER_H_
