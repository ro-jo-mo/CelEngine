cmake_minimum_required(VERSION 3.30)

project("CelEngine"
        VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(namespace "CelEngine")

find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glm REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)

add_library(${PROJECT_NAME} STATIC)
add_subdirectory(examples)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        glm::glm
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        SDL3::SDL3
        vk-bootstrap::vk-bootstrap
)


target_sources(${PROJECT_NAME}
        PRIVATE
        "src/ecs/EntityManager.cpp"
        "src/ecs/Ecs.cpp"
        "src/ecs/ComponentsManager.cpp"
        "src/ecs/ScheduleGraph.cpp"
        "src/ecs/QueryManager.cpp"
        "src/ecs/World.cpp"
        "src/ecs/Time.cpp"
        "src/core/Transform.cpp"
        "src/core/CorePlugin.cpp"
        "src/renderer/Vma.cpp"
        "src/renderer/RenderPlugin.cpp"
        "src/renderer/CleanupRenderer.cpp"
        "src/renderer/Window.cpp"
        "src/renderer/VulkanInitialiser.cpp"
        "src/renderer/DeletionQueue.cpp"
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src

)


set(public_headers
        include/ecs/ComponentArray.h
        include/ecs/ComponentsManager.h
        include/ecs/Ecs.h
        include/ecs/EntityManager.h
        include/ecs/Helpers.h
        include/ecs/IResource.h
        include/ecs/IQuery.h
        include/ecs/Locks.h
        include/ecs/Plugin.h
        include/ecs/Resource.h
        include/ecs/ResourceManager.h
        include/ecs/Schedule.h
        include/ecs/ScheduleGraph.h
        include/ecs/Scheduler.h
        include/ecs/System.h
        include/ecs/SystemAllocator.h
        include/ecs/QueryManager.h
        include/ecs/Types.h
        include/ecs/Query.h
        include/ecs/World.h
        include/ecs/Time.h

        include/core/Transform.h
        include/core/Hierarchy.h
        include/core/CorePlugin.h
        include/ecs/Running.h

        include/renderer/AssetServer.h
        include/renderer/DeletionQueue.h
        include/renderer/RenderPlugin.h
        include/renderer/VulkanInitialiser.h
        include/renderer/VulkanTypes.h
        include/renderer/Window.h
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
