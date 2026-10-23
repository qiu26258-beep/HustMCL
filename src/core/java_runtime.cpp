#include <hustmcl/core/java_runtime.h>

namespace hustmcl::core {
namespace {

bool isCompatible(const JavaRuntime& runtime, JavaRequirement requirement)
{
    return !runtime.executable.empty()
        && runtime.majorVersion == requirement.majorVersion
        && runtime.architecture == RuntimeArchitecture::X64;
}

} // namespace

JavaSelection selectJavaRuntime(
    JavaRequirement requirement,
    std::span<const JavaRuntime> candidates,
    const std::optional<JavaRuntime>& manualRuntime)
{
    if (requirement.majorVersion <= 0) {
        return JavaSelectionError::InvalidRequirement;
    }
    if (manualRuntime) {
        if (isCompatible(*manualRuntime, requirement)) {
            return *manualRuntime;
        }
        return JavaSelectionError::InvalidManualRuntime;
    }
    for (const auto& runtime : candidates) {
        if (isCompatible(runtime, requirement)) {
            return runtime;
        }
    }
    return JavaSelectionError::NoCompatibleRuntime;
}

} // namespace hustmcl::core
