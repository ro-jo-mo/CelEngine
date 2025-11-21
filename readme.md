# Cel Engine

A fully custom built game engine in C++, implementing an efficient ECS and auto parallelism.

# Build & Install

```cmd
mkdir build
cmake CMakeLists.txt
cmake --build build/
cmake --install build/ --prefix install
```

Then in a cmake file:

```cmake
find_package(CelEngine CONFIG REQUIRED)

add_executable(MyProject ...)

target_link_libraries(${PROJECT_NAME} PRIVATE CelEngine::CelEngine)
```
