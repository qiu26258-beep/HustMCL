#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <variant>

namespace hustmcl::core {

enum class RuntimeArchitecture { Unknown, X86, X64, Arm64 };

// Describes an already-probed runtime. This module never touches disk or launches Java.
struct JavaRuntime {
    std::filesystem::path executable;
    int majorVersion;
    RuntimeArchitecture architecture;
};

// Supplied by version metadata / a verified compatibility policy, not a game-version cutoff.
struct JavaRequirement {
    int majorVersion;
};

enum class JavaSelectionError {
    InvalidRequirement,
    NoCompatibleRuntime,
    InvalidManualRuntime,
};

using JavaSelection = std::variant<JavaRuntime, JavaSelectionError>;

// Exact major match is deliberately conservative until broader compatibility is verified.
// Candidates are already in preferred order. An invalid manual choice never silently falls back.
[[nodiscard]] JavaSelection selectJavaRuntime(
    JavaRequirement requirement,
    std::span<const JavaRuntime> candidates,
    const std::optional<JavaRuntime>& manualRuntime = std::nullopt);

} // namespace hustmcl::core
