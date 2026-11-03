find_program(SLANGC_EXECUTABLE slangc
        HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
        REQUIRED)


# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

set(CEL_SHADER_SOURCES
        mesh.slang
        skybox.slang
        tonemap.slang
)

# -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

set(CEL_SHADER_FLAGS
        -profile spirv_1_4
        -target spirv
        -fvk-use-scalar-layout
        -matrix-layout-column-major
        -g
)

if (CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CEL_SHADER_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")
else ()
    set(CEL_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/shaders")
endif ()
set(CEL_SHADER_OUTPUT_DIR "${CEL_SHADER_OUTPUT_DIR}" PARENT_SCOPE)

set(cel_spv_outputs)

# Compile all entry points from all shaders
foreach (source IN LISTS CEL_SHADER_SOURCES)
    set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    get_filename_component(name "${source}" NAME_WE)
    get_filename_component(subdir "${source}" DIRECTORY)
    set(out_dir "${CEL_SHADER_OUTPUT_DIR}/${subdir}")

    set(output "${out_dir}/${name}.spv")
    set(depfile "${CMAKE_CURRENT_BINARY_DIR}/deps/${subdir}/${name}.d")

    add_custom_command(
            OUTPUT "${output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${out_dir}" "${CMAKE_CURRENT_BINARY_DIR}/deps/${subdir}"
            COMMAND ${SLANGC_EXECUTABLE} "${source_path}"
            ${CEL_SHADER_FLAGS}
            -o "${output}"
            -depfile "${depfile}"
            DEPENDS "${source_path}"
            DEPFILE "${depfile}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Compiling shader ${source}"
            VERBATIM
    )
    list(APPEND cel_spv_outputs "${output}")
endforeach ()

add_custom_target(CelShaders ALL DEPENDS ${cel_spv_outputs}
        SOURCES ${CEL_SHADER_SOURCES} common.slang)
