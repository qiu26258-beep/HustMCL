#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv)
{
    if (argc != 3 || std::string(argv[1]) != "-XshowSettings:properties" || std::string(argv[2]) != "-version") { return 10; }
    for (const auto* key : {"JAVA_TOOL_OPTIONS", "JDK_JAVA_OPTIONS", "_JAVA_OPTIONS", "CLASSPATH"}) {
        if (std::getenv(key)) { return 11; }
    }
    const auto* environment = std::getenv("HUSTMCL_TEST_PROBE_MODE");
    const std::string mode = environment ? environment : "valid";
    if (mode == "delay") { std::this_thread::sleep_for(std::chrono::seconds(5)); }
    if (mode == "nonzero") { return 7; }
    if (mode == "noisy") { std::cout << std::string(100000, 'x') << std::flush; return 0; }
    if (mode == "malformed") { std::cerr << "not Java output\n"; return 0; }
    std::cerr << "Property settings:\n    java.specification.version = " << (mode == "legacy" ? "1.8" : "21")
              << "\n    os.arch = " << (mode == "x86" ? "x86" : "amd64") << '\n';
    if (mode == "duplicate") { std::cout << "    os.arch = aarch64\n"; }
    return 0;
}
