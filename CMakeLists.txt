cmake_minimum_required(VERSION 3.10)
project(nashi)
include(FetchContent)

# Default to Debug if not specified (for single-config generators)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(NASHI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/app")
set(NASHI_USE_VULKAN OFF)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)

# Define release flags for different compilers
if(MSVC)
  set(RELEASE_COMPILE_FLAGS "/O1" "/Gy" "/GL")
  set(RELEASE_LINK_FLAGS "/LTCG" "/OPT:REF" "/OPT:ICF")
else()
  set(RELEASE_COMPILE_FLAGS "-Os" "-ffunction-sections" "-fdata-sections" "-flto" "-fno-exceptions" "-fno-rtti" "-fvisibility=hidden")
  set(RELEASE_LINK_FLAGS "-Wl,--gc-sections" "-s" "-flto")
endif()

# Sources, headers, shaders
file(GLOB_RECURSE SOURCES "${NASHI_ROOT}/src/*.cpp")
file(GLOB_RECURSE HEADERS "${NASHI_ROOT}/src/headers/*.hpp" "${NASHI_ROOT}/src/headers/*.h")
file(GLOB_RECURSE SHADERS "${NASHI_ROOT}/src/shaders/*.frag" "${NASHI_ROOT}/src/shaders/*.vert" "${NASHI_ROOT}/src/shaders/*.comp")

add_executable(nashi ${SOURCES} ${SHADERS} ${HEADERS})

target_include_directories(nashi PRIVATE "${NASHI_ROOT}/src/headers")

# Organize in IDE
source_group(TREE "${NASHI_ROOT}/src/shaders" PREFIX "Shaders" FILES ${SHADERS})
source_group(TREE "${NASHI_ROOT}/src/headers" PREFIX "Headers" FILES ${HEADERS})

# GLM dependency
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG master)
FetchContent_MakeAvailable(glm)

# Vulkan support on Windows
set(GLSLC_PATH "$ENV{VULKAN_SDK}/Bin/glslc.exe")
set(SPIRV_CROSS_PATH spirv-cross)
set(GLAD_INCLUDE_PATH "")
if(WIN32)
  # find_package(Vulkan)
  # if(Vulkan_FOUND)
  #   message(STATUS "Vulkan headers & loader found.")
  #   set(NASHI_USE_VULKAN ON)
  #   set(GLSLC_PATH "$ENV{VULKAN_SDK}/Bin/glslc.exe")
  #   if(NOT EXISTS ${GLSLC_PATH})
  #     message(FATAL_ERROR "glslc.exe not found at ${GLSLC_PATH}. Please check Vulkan SDK installation.")
  #   endif()
  # else()
  #   message(WARNING "Vulkan loader NOT found. Disabling Vulkan support.")
  #   set(NASHI_USE_VULKAN OFF)
  # endif()


  # find_package(OpenGL REQUIRED)
  # add_library(glad STATIC
  #   ${NASHI_ROOT}/vendor/glad/src/glad.c
  # )
  # set(NASHI_USE_OPENGL ON)
  # set(GLAD_INCLUDE_PATH "${NASHI_ROOT}/vendor/glad/include")
  # message(STATUS "Glad include path: ${NASHI_ROOT}/vendor/glad/include")
  
  set(NASHI_USE_DIRECT3D12 ON)
  set(DXC_PATH dxc)
endif()

# Handle Release flags and definitions for multi-config and single-config
if(CMAKE_CONFIGURATION_TYPES)
  # Multi-config generators (Visual Studio, Xcode)
  target_compile_definitions(nashi PRIVATE
    $<$<CONFIG:Release>:NASHI_RELEASE_BUILD>
  )
  target_compile_options(nashi PRIVATE
    $<$<CONFIG:Release>:${RELEASE_COMPILE_FLAGS}>
  )
  target_link_options(nashi PRIVATE
    $<$<CONFIG:Release>:${RELEASE_LINK_FLAGS}>
  )
  
  # Windows-specific release linker flags and properties
  if(WIN32)
    target_link_options(nashi PRIVATE
      $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>
    )
    set_property(TARGET nashi PROPERTY
      WIN32_EXECUTABLE $<$<CONFIG:Release>:TRUE>
    )
  endif()

else()
  # Single-config generators (Makefile, Ninja)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(nashi PRIVATE NASHI_RELEASE_BUILD)
    target_compile_options(nashi PRIVATE ${RELEASE_COMPILE_FLAGS})
    target_link_options(nashi PRIVATE ${RELEASE_LINK_FLAGS})

    if(WIN32)
      set_target_properties(nashi PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
        WIN32_EXECUTABLE TRUE
      )
    endif()
  endif()
endif()

# SDL3 (if VR not enabled)
if(NOT NASHI_VR)
  include(FetchContent)
  FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG main)
  FetchContent_MakeAvailable(SDL3)
  set(MIDDLEWARE SDL3::SDL3)
endif()


function(compile_shaders)
  cmake_parse_arguments(SHADER_ARGS "" "TARGET;OUTPUT_DIR;GLSLC" "SHADERS" ${ARGN})

  set(SPIRV_SHADERS "")
  foreach(SHADER ${SHADER_ARGS_SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME_WE)
    set(SPIRV_FILE "${SHADER_ARGS_OUTPUT_DIR}/${FILE_NAME}.spv")

    add_custom_command(
      OUTPUT ${SPIRV_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_ARGS_OUTPUT_DIR}
      COMMAND ${GLSLC_PATH} -o ${SPIRV_FILE} ${SHADER}
      DEPENDS ${SHADER}
      COMMENT "Compiling shader ${FILE_NAME} to SPIR-V"
      VERBATIM
    )
    list(APPEND SPIRV_SHADERS ${SPIRV_FILE})
  endforeach()

  add_custom_target(${SHADER_ARGS_TARGET}_shaders ALL DEPENDS ${SPIRV_SHADERS})
  add_dependencies(${SHADER_ARGS_TARGET} ${SHADER_ARGS_TARGET}_shaders)
endfunction()


# Link libraries and handle shaders
if(NASHI_USE_VULKAN)
  target_link_libraries(nashi PRIVATE Vulkan::Vulkan glm::glm ${MIDDLEWARE})
  target_compile_definitions(nashi PRIVATE NASHI_USE_VULKAN)

  set(SPIRV_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders") 

  compile_shaders(
    TARGET nashi
    OUTPUT_DIR "${SPIRV_DIR}"
    GLSLC "${GLSLC_PATH}"
    SHADERS ${SHADERS}
  )

  add_custom_command(TARGET nashi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DSRC="${SPIRV_DIR}" -DDEST="$<TARGET_FILE_DIR:nashi>" -P "${CMAKE_CURRENT_SOURCE_DIR}/CopyShaders.cmake"
    COMMENT "Copying compiled SPIR-V shaders to binary directory"
  )

elseif(NASHI_USE_DIRECT3D12)
  # link / include…
  target_link_libraries(nashi PRIVATE glm::glm d3d12 dxgi dxguid d3dcompiler ${MIDDLEWARE})
  target_include_directories(nashi PRIVATE ${NASHI_ROOT}/vendor/directx)
  target_compile_definitions(nashi PRIVATE NASHI_USE_DIRECT3D12)

  # 1) Define your output dirs
  set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
  set(SPIRV_DIR   "${SHADER_OUTPUT_DIR}")
  set(HLSL_DIR    "${SHADER_OUTPUT_DIR}")
  set(CSO_DIR     "${SHADER_OUTPUT_DIR}")
  file(MAKE_DIRECTORY ${SPIRV_DIR} ${HLSL_DIR} ${CSO_DIR})

  # 2) GLSL → SPIR‑V
  set(SPIRV_FILES "")
  foreach(src IN LISTS SHADERS)
    get_filename_component(fn ${src} NAME)
    set(spv "${SPIRV_DIR}/${fn}.spv")
    add_custom_command(
      OUTPUT ${spv}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${SPIRV_DIR}
      COMMAND ${GLSLC_PATH} -c ${src} -o ${spv}
      DEPENDS ${src}
      COMMENT "GLSL → SPIR‑V: ${fn}"
      VERBATIM
    )
    list(APPEND SPIRV_FILES ${spv})
  endforeach()
  add_custom_target(compile_spv ALL DEPENDS ${SPIRV_FILES})
  add_dependencies(nashi compile_spv)

  # 3) SPIR‑V → HLSL
  set(HLSL_VS "")
  set(HLSL_PS "")
  foreach(spv IN LISTS SPIRV_FILES)
    get_filename_component(fn ${spv} NAME)
    set(hlsl "${HLSL_DIR}/${fn}.hlsl")
    add_custom_command(
      OUTPUT ${hlsl}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${HLSL_DIR}
      COMMAND ${SPIRV_CROSS_PATH} ${spv} --hlsl --shader-model 60 --output ${hlsl} 
      DEPENDS ${spv}
      COMMENT "SPIR‑V → HLSL: ${fn}"
      VERBATIM
    )
    if(fn MATCHES "\\.vert\\.spv$")
      list(APPEND HLSL_VS ${hlsl})
    else()
      list(APPEND HLSL_PS ${hlsl})
    endif()
  endforeach()
  add_custom_target(spirv_to_hlsl ALL DEPENDS ${HLSL_VS} ${HLSL_PS})
  add_dependencies(nashi spirv_to_hlsl)

  # 4) HLSL → CSO
# Vertex shaders → vs_6_0
  foreach(hlsl IN LISTS HLSL_VS)
    # hlsl might be ".../basic.vert.spv.hlsl"
    get_filename_component(fn_full ${hlsl} NAME)      # fn_full="basic.vert.spv.hlsl"

    # remove the trailing ".spv.hlsl"
    string(REGEX REPLACE "\\.spv\\.hlsl$" "" fn_base ${fn_full})
    # fn_base="basic.vert"

    set(cso "${CSO_DIR}/${fn_base}.cso")

    add_custom_command(
      OUTPUT ${cso}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CSO_DIR}
      COMMAND ${DXC_PATH} -T vs_6_0 -E main -Fo ${cso} ${hlsl}
      DEPENDS ${hlsl}
      COMMENT "HLSL → CSO (vs): ${fn_base}"
      VERBATIM
    )
    list(APPEND CSO_FILES ${cso})
  endforeach()

  # Pixel shaders → ps_6_0
  foreach(hlsl IN LISTS HLSL_PS)
    # hlsl might be ".../basic.frag.spv.hlsl"
    get_filename_component(fn_full ${hlsl} NAME)      # fn_full="basic.frag.spv.hlsl"

    # remove the trailing ".spv.hlsl"
    string(REGEX REPLACE "\\.spv\\.hlsl$" "" fn_base ${fn_full})
    # fn_base="basic.frag"

    set(cso "${CSO_DIR}/${fn_base}.cso")

    add_custom_command(
      OUTPUT ${cso}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CSO_DIR}
      COMMAND ${DXC_PATH} -T ps_6_0 -E main -Fo ${cso} ${hlsl}
      DEPENDS ${hlsl}
      COMMENT "HLSL → CSO (ps): ${fn_base}"
      VERBATIM
    )
    list(APPEND CSO_FILES ${cso})
  endforeach()

  add_custom_target(hlsl_to_cso ALL DEPENDS ${CSO_FILES})
  add_dependencies(nashi hlsl_to_cso)
  
  # 5) copy final .cso’s alongside your exe
  add_custom_command(TARGET nashi POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -DSRC="${CSO_DIR}"
            -DDEST="$<TARGET_FILE_DIR:nashi>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/CopyShaders.cmake"
    COMMENT "Copying compiled CSO shaders to binary directory"
  )

endif()

# Optional: Strip binary on release builds for non-MSVC
if(NOT MSVC)
  if(CMAKE_CONFIGURATION_TYPES)
    add_custom_command(TARGET nashi POST_BUILD
      COMMAND $<$<CONFIG:Release>:${CMAKE_STRIP} $<TARGET_FILE:nashi>>
      COMMENT "Stripping symbols from release binary"
      VERBATIM
    )
  else()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
      add_custom_command(TARGET nashi POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:nashi>
        COMMENT "Stripping symbols from release binary"
        VERBATIM
      )
    endif()
  endif()
endif()


