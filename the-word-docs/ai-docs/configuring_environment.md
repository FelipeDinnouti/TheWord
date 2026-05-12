
Using CMake (Recommended for Larger Projects)

Install CMake: From CMake website.

Create CMakeLists.txt:

```CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(raylib_project)

find_package(raylib REQUIRED)
add_executable(game main.cpp)
target_link_libraries(game PRIVATE raylib)
```

Build:

```bash
cmake -B build
```


```bash
cmake --build build
```


## Running the Project

### **1. Install MSYS2 and Tools**
1. **Download MSYS2**:  
   Go to [msys2.org](https://www.msys2.org/), install it to `C:\msys64` (default).  
2. **Update Packages**:  
   Open the **MSYS2 MSYS** terminal (not MINGW) and run:  
   ```bash
   pacman -Syu  # Updates core packages (close terminal when prompted)
   pacman -Su   # Updates remaining packages
   ```
3. **Install Compiler and Tools**:  
   Open the **MSYS2 MINGW64** terminal (this is critical!) and run:  
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-raylib
   ```
   This installs:
   - `g++` (C++ compiler)
   - `CMake`
   - `make` (build tool)
   - Raylib (library and headers)

---

### **2. Create a Simple Raylib Project**
1. **Create a Project Folder**:  
   Example: `C:/projects/raylib_test`.  
   Avoid spaces in the path (e.g., no `My Projects`).  
2. **Add Two Files**:  
   - `main.cpp` (your code):  
     ```cpp
     #include "raylib.h"
     
     int main() {
         InitWindow(800, 450, "Raylib + CMake");
         SetTargetFPS(60);
     
         while (!WindowShouldClose()) {
             BeginDrawing();
             ClearBackground(RAYWHITE);
             DrawText("Hello from Raylib!", 190, 200, 20, LIGHTGRAY);
             EndDrawing();
         }
     
         CloseWindow();
         return 0;
     }
     ```
   - `CMakeLists.txt` (build instructions):  
     ```cmake
     cmake_minimum_required(VERSION 3.16)
     project(raylib_project)
     
     set(CMAKE_CXX_STANDARD 11)
     
     # Find Raylib (installed via MSYS2)
     find_package(raylib REQUIRED)
     
     # Create the executable
     add_executable(game main.cpp)
     
     # Link Raylib and Windows libraries
     target_link_libraries(game PRIVATE raylib opengl32 gdi32 winmm)
     ```

---

### **3. Build the Project**
1. **Open the MINGW64 Terminal**:  
   Ensure you’re in the **MSYS2 MINGW64** terminal (title bar should say **MINGW64**).  
2. **Navigate to Your Project**:  
   ```bash
   cd /c/projects/raylib_test  # Replace with your path
   ```
3. **Run CMake**:  
   ```bash
   cmake -B build -G "MinGW Makefiles"
   ```
   - `-B build`: Creates a `build` folder for compiled files.  
   - `-G "MinGW Makefiles"`: Tells CMake to use the MinGW compiler.  
4. **Compile the Project**:  
   ```bash
   cmake --build build
   ```
   If successful, this creates `build/game.exe`.

---

### **4. Run the Executable**
1. **Run the Program**:  
   From the terminal:  
   ```bash
   ./build/game.exe
   ```
   You should see a window with "Hello from Raylib!".

---

### **Troubleshooting**
#### If CMake Fails:
- **Error: "CMake can’t find Raylib"**:  
  Manually specify Raylib’s location in `CMakeLists.txt`:  
  ```cmake
  set(raylib_DIR "C:/msys64/mingw64/lib/cmake/raylib")
  find_package(raylib REQUIRED)
  ```

#### If Compilation Fails:
- **Undefined References (e.g., `InitWindow`)**:  
  Ensure you’re in the **MINGW64** terminal and reinstalled Raylib:  
  ```bash
  pacman -S mingw-w64-x86_64-raylib
  ```

#### If `game.exe` Doesn’t Run:
- **Missing DLLs**:  
  Copy `C:\msys64\mingw64\bin\libraylib.dll` into your project’s `build` folder.

---

### **Alternative: Compile Without CMake**
If you prefer `g++` directly:  
```bash
g++ main.cpp -o game.exe -I/mingw64/include -L/mingw64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

---

### **Summary**
1. Use the **MINGW64 terminal** for everything.  
2. `CMakeLists.txt` tells CMake how to build your project.  
3. Always run `cmake -B build` and `cmake --build build` in sequence.  

Let me know if you hit a snag – we’ll debug together! 🛠️