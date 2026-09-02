set(namespace "CelEngine")

set(MAX_ENTITIES 2048 CACHE STRING "Max number of entities that can exist at once")
set(CMD_BUFFERS_PER_POOL 30 CACHE STRING "(Deprecated) Total number of cmd buffers created per pool for render passes")

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
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED)

target_sources(${PROJECT_NAME}
        PRIVATE
        "ecs/EntityManager.cpp"
        "ecs/ComponentsManager.cpp"
        "ecs/ScheduleGraph.cpp"
        "ecs/QueryManager.cpp"

        "core/World.cpp"
        "core/Time.cpp"
        "core/Transform.cpp"
        "core/CorePlugin.cpp"

        "input/Input.cpp"

        "renderer/Vma.cpp"
        "renderer/RenderPlugin.cpp"
        "renderer/CleanupRenderer.cpp"
        "renderer/Window.cpp"
        "renderer/VulkanSetup.cpp"
        "renderer/VulkanHelpers.cpp"
        "renderer/Draw.cpp"
        "renderer/VulkanUtils.cpp"
        "renderer/AssetServer.cpp"
        "renderer/Descriptors.cpp"
        "renderer/Camera.cpp"

        "renderer/render-graph/PassBuilder.cpp"
        "renderer/render-graph/RenderGraph.cpp"
        "renderer/render-graph/RenderGraphPlugin.cpp"
        "renderer/render-graph/ExecutionPlan.cpp"
        "renderer/render-graph/PassServer.cpp"

        "renderer/passes/HandleAllocator.cpp"

        "renderer/resource-management/ResourceTracker.cpp"
        "renderer/resource-management/VulkanResourceManager.cpp"
        "renderer/resource-management/PipelineBuilder.cpp"
        "renderer/resource-management/MegaBuffer.cpp"
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)


set(public_headers
        include/ecs/ComponentArray.h
        include/ecs/ComponentsManager.h
        include/core/App.h
        include/ecs/EntityManager.h
        include/ecs/Helpers.h
        include/ecs/IResource.h
        include/ecs/IQuery.h
        include/ecs/Locks.h
        include/ecs/Resource.h
        include/ecs/ResourceManager.h
        include/ecs/Schedule.h
        include/ecs/ScheduleGraph.h
        include/ecs/SystemScheduler.h
        include/ecs/System.h
        include/ecs/QueryManager.h
        include/ecs/Types.h
        include/ecs/Query.h

        include/core/Plugin.h
        include/core/World.h
        include/core/Time.h
        include/core/Transform.h
        include/core/Hierarchy.h
        include/core/CorePlugin.h
        include/core/Running.h
        include/core/ThreadManager.h

        include/common/SlotMap.h
        include/common/GrowVector.h
        include/common/Graph.h
        include/common/Handle.h
        include/common/Scheduler.h

        include/input/Input.h

        include/renderer/AssetServer.h
        include/renderer/RenderPlugin.h
        include/renderer/VulkanSetup.h
        include/renderer/Window.h
        include/renderer/VulkanHelpers.h
        include/renderer/VulkanTypes.h
        include/renderer/Draw.h
        include/renderer/VulkanUtils.h
        include/renderer/AssetTypes.h
        include/renderer/Descriptors.h
        include/renderer/Camera.h
        include/renderer/MegaBuffer.h

        include/renderer/render-graph/RenderGraph.h
        include/renderer/render-graph/RenderGraphPlugin.h
        include/renderer/render-graph/PassBuilder.h
        include/renderer/render-graph/RenderPass.h
        include/renderer/render-graph/AccessFlags.h
        include/renderer/render-graph/ExecutionPlan.h

        include/renderer/passes/HandleAllocator.h
        include/renderer/passes/Passes.h

        include/renderer/passes/DrawMeshes.h
        include/renderer/passes/DrawSkybox.h
        include/renderer/passes/UploadAssets.h


        include/renderer/resource-management/ResourceTracker.h
        include/renderer/resource-management/VulkanResourceManager.h
        include/renderer/resource-management/PipelineBuilder.h
        include/renderer/resource-management/DeletionQueue.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${public_headers}")

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
        EXPORT "${PROJECT_NAME}Targets"
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/include/core/Config.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/core
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT "${PROJECT_NAME}Targets"
        FILE "${PROJECT_NAME}Targets.cmake"
        NAMESPACE ${namespace}::
        DESTINATION cmake
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION " ${version} "
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
