# Troubleshooting

> Status: Stable | Last Updated: 2026-06-21

## Common Issues

| Issue | Solution |
|-------|----------|
| `CMAKE_CXX_COMPILE_OBJECT not set` | Add `CXX` to project languages: `project(theword C CXX)` |
| New .cpp files not compiled | Delete `build/` folder and reconfigure CMake |
| libcurl not found | Install `libcurl4-openssl-dev`, ensure `find_package(CURL)` comes after raylib in CMakeLists.txt |
| API returns "Access denied" | Use Bible ID 3034 (BSB) instead of 111 (NIV) |
| API key not working | Verify `.env` file exists and `YVP_APP_KEY` is set correctly |
| `libraylib.so: cannot open shared object file` | Run `sudo ldconfig` or set `LD_LIBRARY_PATH` |
| Raylib not found by CMake | Use `-DCMAKE_PREFIX_PATH=/usr/local` or rebuild raylib from source |
| Missing OpenGL references | Install `libgl1-mesa-dev libx11-dev` |

## CMake Generator Issues

On Linux, always use `-G "Unix Makefiles"`. The default or Ninja generators may fail with Raylib's FetchContent configuration.

## macOS

Not currently supported. Raylib works on macOS, but the build configuration has not been tested.
