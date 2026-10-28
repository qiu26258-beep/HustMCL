# HustMCL public preview

Windows 10/11 x64 launcher for Minecraft Java. Sign in with a Microsoft
account that owns the game to install official versions and launch online,
or pick a local offline identity for single-player worlds; the choice is
kept in local settings. Modrinth-format ZIP/MRPACK packs (including PCL
wrappers) import through the same flow. Modern Vanilla and Fabric are
supported; Forge/NeoForge/Quilt and other pack formats are not. Minecraft
and matching Java are downloaded on demand, and the game is always started
by the user.

This release source snapshot excludes private character-derived artwork,
Minecraft files, Java, local caches, logs, account data and user files.
Original campus geometric decoration and icon are included. Third-party
licenses are retained. No general project code license has been selected
yet.

Build with Qt 6.10.3, MinGW 13.1 x64, CMake, Ninja and Python 3.10+:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DHUSTMCL_BUILD_DESKTOP=ON -DHUSTMCL_PUBLIC_RELEASE=ON -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.10.3/mingw_64
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

The public switch is required: the local-only artwork resource is
intentionally not shipped. The installer includes dynamically replaceable
Qt DLLs and their notices. Matching Qt source is a separate asset on the
same release.
