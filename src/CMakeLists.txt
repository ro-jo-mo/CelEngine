set(namespace "CelEngine")

# Must come before anything that uses CMAKE_INSTALL_INCLUDEDIR
include(GNUInstallDirs)

set(MAX_ENTITIES 2048 CACHE STRING "Max number of entities that can exist at once")
set(MAX_COMPONENTS 128 CACHE STRING "Max number of components that be registered into the ECS")
set(MAX_QUERY_RETRIEVE 15 CACHE STRING "Max number of components a query can retrieve. (Ideally 2^n - 1 for alignment")
set(CMD_BUFFERS_PER_POOL 30 CACHE STRING "(Deprecated) Total number of cmd buffers created per pool for render passes")
set(MAX_VARIABLE_DESCRIPTOR_ARRAY 4000 CACHE STRING "The number of descriptors assigned when creating a variable descriptor array for a shader")
set(MAX_SAMPLERS 64 CACHE STRING "Size of the global sampler descriptor array (unique sampler settings). Also passed to shaders")
set(MATERIAL_BUFFER_SIZE 8192 CACHE STRING "Amount of GPU memory allocated for the material buffer")
set(VERTEX_BUFFER_SIZE 134217728 CACHE STRING "Amount of GPU memory allocated for the vertex buffer")
set(INDEX_BUFFER_SIZE 8388608 CACHE STRING "Amount of GPU memory allocated for the index buffer")
set(LIGHT_BUFFER_SIZE 1024 CACHE STRING "Amount of GPU memory allocated for the lights buffer")

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/core/Config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/include/core/Config.h
        @ONLY
)


find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glm REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(VulkanUtilityLibraries REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(SPIRV-Headers CONFIG REQUIRED)
find_package(mikktspace CONFIG REQUIRED)

add_library(${PROJECT_NAME} STATIC)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        fmt::fmt
        glm::glm
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        SDL3::SDL3
        vk-bootstrap::vk-bootstrap
        Vulkan::UtilityHeaders
        fastgltf::fastgltf
        KTX::ktx
        unofficial::spirv-reflect
        SPIRV-Headers::SPIRV-Headers
        mikktspace::mikktspace
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED SPIRV_REFLECT_USE_SYSTEM_SPIRV_H)

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

target_sources(${PROJECT_NAME}
        PRIVATE
        "ecs/EntityManager.cpp"
        "ecs/ScheduleGraph.cpp"
        "ecs/QueryManager.cpp"
        "ecs/Archetype.cpp"
        "ecs/ArchetypeManager.cpp"

        "core/World.cpp"
        "core/Time.cpp"
        "core/Transform.cpp"
        "core/CorePlugin.cpp"
        "core/ThreadManager.cpp"
        "core/Arena.cpp"


        "input/Input.cpp"

        "renderer/Vma.cpp"
        "renderer/RenderPlugin.cpp"
        "renderer/CleanupRenderer.cpp"
        "renderer/Window.cpp"
        "renderer/VulkanSetup.cpp"
        "renderer/VulkanHelpers.cpp"
        "renderer/VulkanUtils.cpp"
        "renderer/asset/AssetServer.cpp"
        "renderer/asset/GltfLoader.cpp"
        "renderer/asset/MikkTSpaceImplementation.cpp"
        "renderer/asset/ImageLoader.cpp"
        "renderer/asset/ProceduralLoader.cpp"
        "renderer/Descriptors.cpp"
        "renderer/Camera.cpp"
        "renderer/SceneData.cpp"


        "renderer/render-graph/PassBuilder.cpp"
        "renderer/render-graph/RenderGraph.cpp"
        "renderer/render-graph/RenderGraphPlugin.cpp"
        "renderer/render-graph/ExecutionPlan.cpp"
        "renderer/render-graph/PassServer.cpp"
        "renderer/render-graph/SubmitSplitter.cpp"


        "renderer/passes/HandleAllocator.cpp"
        "renderer/passes/BasePasses.cpp"
        "renderer/passes/DrawMeshes.cpp"
        "renderer/passes/DrawSkybox.cpp"
        "renderer/passes/PresentPass.cpp"
        "renderer/passes/PassPlugin.cpp"
        "renderer/passes/StagingPool.cpp"
        "renderer/passes/Tonemap.cpp"

        "renderer/resource-management/ResourceTracker.cpp"
        "renderer/resource-management/VulkanResourceManager.cpp"
        "renderer/resource-management/PipelineBuilder.cpp"
        "renderer/resource-management/MegaBuffer.cpp"
)

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

# Installs the include tree with its folder layout (core/, ecs/, renderer/...)
# so the same #include paths work against the build tree and an install.
install(TARGETS ${PROJECT_NAME}
        EXPORT "${PROJECT_NAME}Targets"
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/include/core/Config.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/core
)

install(EXPORT "${PROJECT_NAME}Targets"
        FILE "${PROJECT_NAME}Targets.cmake"
        NAMESPACE ${namespace}::
        DESTINATION cmake
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION cmake
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION cmake
)
