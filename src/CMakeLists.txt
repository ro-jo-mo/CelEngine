set(namespace "CelEngine")

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
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED)

target_sources(${PROJECT_NAME}
        PRIVATE
        "ecs/EntityManager.cpp"
        "ecs/ComponentsManager.cpp"
        "ecs/ScheduleGraph.cpp"
        "ecs/Scheduler.cpp"
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
        "renderer/PipelineBuilder.cpp"
        "renderer/VulkanHelpers.cpp"
        "renderer/Draw.cpp"
        "renderer/VulkanUtils.cpp"
        "renderer/AssetServer.cpp"
        "renderer/Descriptors.cpp"
        "renderer/Camera.cpp"
        "renderer/MegaBuffer.cpp"
        "renderer/render-graph/PassBuilder.cpp"
        "renderer/render-graph/RenderGraph.cpp"
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
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
        include/ecs/Scheduler.h
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

        include/common/SlotMap.h

        include/input/Input.h

        include/renderer/AssetServer.h
        include/renderer/DeletionQueue.h
        include/renderer/RenderPlugin.h
        include/renderer/VulkanSetup.h
        include/renderer/Window.h
        include/renderer/PipelineBuilder.h
        include/renderer/VulkanHelpers.h
        include/renderer/VulkanTypes.h
        include/renderer/Draw.h
        include/renderer/VulkanUtils.h
        include/renderer/AssetTypes.h
        include/renderer/Descriptors.h
        include/renderer/Camera.h
        include/renderer/MegaBuffer.h

        include/renderer/render-graph/RenderGraph.h
        include/renderer/render-graph/PassBuilder.h

)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${public_headers}")

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
        EXPORT "${PROJECT_NAME}Targets"
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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
