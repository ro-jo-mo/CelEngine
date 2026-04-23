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
## Requirements
Requires an installation of:
- Vulkan SDK
- glfw
- glm

# Example 
```cpp 
#include ...

using namespace Cel;  

// Systems represent individual pieces of game logic to run 
// They state the relevant data to retrieve for their operation through the type 

// Explicitly stating the required data allows us to run systems in 
// parallel automatically, based on the data modified

// The query system used here allows systems to filter for just the 
// specific entities it requires
// Resources represent singleton instances of an object to be used 
// across the program

class MySystem : public System<
	Query<With<Player, Health>,Without<Ai>>,
	Query<With<const Enemy>>,
	Resource<Time>> {  
public:  
    void Run(Query<With<Player, Health>,Without<Ai>> playerQuery,
		     Query<With<const Enemy>> enemyQuery,
		     Resource<Time> time) override { 
		      
        for (auto [player,hp] : playerQuery) {  
            hp.hp += ...  
        }  
    }  
}  
// Plugins represent complete units of game logic
// For example a physics plugin might handle all logic regarding physics objects
// This makes reusing plugins across projects very simple.

class MyPlugin : public Plugin {  
public:  
    void Build(Scheduler scheduler, ResourceManager &resourceManager) override {  
        resourceManager.InsertResource<MyResource>();  
        
        // Systems can be assigned to run after different stages of the game.
        // For example, once at startup (Schedule::Startup) 
        // or each frame (Schedule::Update) 
        // Systems can additionally be deliberately scheduled 
        // to run before / after other systems within their assigned stage.
        
        scheduler.AddSystem<MySystem>(Schedule::Update).After<MyOtherSystem>;  
		scheduler.AddSystem<MyOtherSystem>(Schedule::Update);
		...
    }  
};

// Running the game is then simple enough

int main() {  
    Ecs ecs;  
    ecs.AddPlugin<MyPlugin>();  
    ecs.Run();  
    return 0;  
}
```
