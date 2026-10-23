#include <hustmcl/core/java_runtime.h>

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace hustmcl::core;

namespace {

void require(bool condition, std::string_view scenario)
{
    if (!condition) {
        throw std::runtime_error(std::string(scenario));
    }
}

void expectError(const JavaSelection& selection, JavaSelectionError error, std::string_view scenario)
{
    require(std::holds_alternative<JavaSelectionError>(selection)
        && std::get<JavaSelectionError>(selection) == error, scenario);
}

void runTests()
{
    const JavaRuntime java8{L"C:\\Java 8\\bin\\java.exe", 8, RuntimeArchitecture::X64};
    const JavaRuntime java17{L"D:\\游戏运行环境\\Java 17\\bin\\javaw.exe", 17, RuntimeArchitecture::X64};
    const JavaRuntime java21{L"C:\\Java 21\\bin\\javaw.exe", 21, RuntimeArchitecture::X64};
    const std::array candidates{java8, java17, java21};

    const auto selected = selectJavaRuntime({17}, candidates);
    require(std::holds_alternative<JavaRuntime>(selected)
        && std::get<JavaRuntime>(selected).executable == java17.executable,
        "select exact requested major, preserving Chinese and spaces in the executable path");
    expectError(selectJavaRuntime({25}, candidates), JavaSelectionError::NoCompatibleRuntime,
        "do not select a different Java major");
    expectError(selectJavaRuntime({17}, {}), JavaSelectionError::NoCompatibleRuntime,
        "no runtime installed");
    for (const auto architecture : {RuntimeArchitecture::X86, RuntimeArchitecture::Arm64, RuntimeArchitecture::Unknown}) {
        const std::array wrongArchitecture{JavaRuntime{L"C:\\Java\\bin\\java.exe", 17, architecture}};
        expectError(selectJavaRuntime({17}, wrongArchitecture), JavaSelectionError::NoCompatibleRuntime,
            "reject unsupported / unknown architectures");
    }
    const std::array emptyPath{JavaRuntime{{}, 17, RuntimeArchitecture::X64}};
    expectError(selectJavaRuntime({17}, emptyPath), JavaSelectionError::NoCompatibleRuntime,
        "reject empty executable path");
    for (const int major : {0, -1}) {
        expectError(selectJavaRuntime({major}, candidates, java17), JavaSelectionError::InvalidRequirement,
            "invalid requirement wins over manual or automatic selection");
    }
    const JavaRuntime preferred{L"D:\\Managed Java\\bin\\java.exe", 17, RuntimeArchitecture::X64};
    const std::array ordered{preferred, java17};
    require(std::get<JavaRuntime>(selectJavaRuntime({17}, ordered)).executable == preferred.executable,
        "candidate ordering is explicit and deterministic");
    require(std::get<JavaRuntime>(selectJavaRuntime({17}, ordered, java17)).executable == java17.executable,
        "a valid manual runtime wins over automatic selection");
    expectError(selectJavaRuntime({17}, candidates, java21), JavaSelectionError::InvalidManualRuntime,
        "a wrong manual major does not silently fall back");
    expectError(selectJavaRuntime({17}, candidates, emptyPath.front()), JavaSelectionError::InvalidManualRuntime,
        "an invalid manual path does not silently fall back");
    const JavaRuntime manualX86{L"C:\\Java x86\\bin\\java.exe", 17, RuntimeArchitecture::X86};
    expectError(selectJavaRuntime({17}, candidates, manualX86), JavaSelectionError::InvalidManualRuntime,
        "reject manually selected 32-bit Java");
}

} // namespace

int main()
{
    try {
        runTests();
        std::cout << "PASS Java runtime selection scenarios\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
