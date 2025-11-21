# Cel Engine

A fully custom built game engine in C++, implementing an efficient ECS and auto parallelism.
The design of the engine is primarily based on this [blog](https://austinmorlan.com/posts/entity_component_system/) by Austin Morlan, as well as the [EnTT Framework](https://github.com/skypjack/entt), with a [Bevy](https://bevy.org/) inspired API.
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
