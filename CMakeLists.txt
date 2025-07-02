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
file(GLOB SHADERS "${NASHI_ROOT}/src/shaders/*.frag" "${NASHI_ROOT}/src/shaders/*.vert" "${NASHI_ROOT}/src/shaders/*.comp")

add_executable(nashi ${SOURCES} ${SHADERS} ${HEADERS})

target_include_directories(nashi PRIVATE "${NASHI_ROOT}/src/headers")

# Organize in IDE
source_group(TREE "${NASHI_ROOT}/src/shaders" PREFIX "Shaders" FILES ${SHADERS})
source_group(TREE "${NASHI_ROOT}/src/headers" PREFIX "Headers" FILES ${HEADERS})

# GLM dependency
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG master)
FetchContent_MakeAvailable(glm)

# Vulkan support on Windows
set(GLAD_INCLUDE_PATH "")
if(WIN32)
  find_package(Vulkan)
  if(Vulkan_FOUND)
    message(STATUS "Vulkan headers & loader found.")
    set(NASHI_USE_VULKAN ON)
    set(GLSLC_PATH "$ENV{VULKAN_SDK}/Bin/glslc.exe")
    if(NOT EXISTS ${GLSLC_PATH})
      message(FATAL_ERROR "glslc.exe not found at ${GLSLC_PATH}. Please check Vulkan SDK installation.")
    endif()
  else()
    message(WARNING "Vulkan loader NOT found. Disabling Vulkan support.")
    set(NASHI_USE_VULKAN OFF)
  endif()

  # find_package(OpenGL REQUIRED)
  # add_library(glad STATIC
  #   ${NASHI_ROOT}/vendor/glad/src/glad.c
  # )
  # set(NASHI_USE_OPENGL ON)
  # set(GLAD_INCLUDE_PATH "${NASHI_ROOT}/vendor/glad/include")
  # message(STATUS "Glad include path: ${NASHI_ROOT}/vendor/glad/include")
  
  # set(NASHI_USE_DIRECT3D12 ON)
  # set(DXC_PATH dxc)
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

# Link libraries and handle shaders
if(NASHI_USE_VULKAN)
  target_link_libraries(nashi PRIVATE Vulkan::Vulkan glm::glm ${MIDDLEWARE})
  target_compile_definitions(nashi PRIVATE NASHI_USE_VULKAN)

  set(SPIRV_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
  foreach(SHADER ${SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SPIRV_FILE "${SPIRV_DIR}/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${SPIRV_DIR}
      COMMAND ${GLSLC_PATH} -o ${SPIRV_FILE} ${SHADER}
      DEPENDS ${SHADER}
      COMMENT "Compiling shader ${FILE_NAME} to SPIR-V"
      VERBATIM
    )
    list(APPEND SPIRV_SHADERS ${SPIRV_FILE})
  endforeach()
  add_custom_target(shaders ALL DEPENDS ${SPIRV_SHADERS})
  add_dependencies(nashi shaders)
  add_custom_command(TARGET nashi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DSRC="${SPIRV_DIR}" -DDEST="$<TARGET_FILE_DIR:nashi>" -P "${CMAKE_CURRENT_SOURCE_DIR}/CopyShaders.cmake"
    COMMENT "Copying compiled SPIR-V shaders to binary directory"
  )

elseif(NASHI_USE_OPENGL)
  target_link_libraries(nashi PRIVATE glad glm::glm OpenGL::GL ${MIDDLEWARE})
  target_include_directories(glad PUBLIC ${GLAD_INCLUDE_PATH})
  target_compile_definitions(nashi PRIVATE NASHI_USE_OPENGL)

  add_custom_command(TARGET nashi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/app/src/shaders"
      "$<TARGET_FILE_DIR:nashi>/shaders"
    COMMENT "Copying shaders directory"
  )

  add_custom_command(TARGET nashi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/app/src/shaders"
      "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMENT "Copying shaders directory"
  )
elseif(NASHI_USE_DIRECT3D12)
  target_link_libraries(nashi PRIVATE glm::glm d3d12 dxgi dxguid d3dcompiler ${MIDDLEWARE})
  target_include_directories(nashi PRIVATE ${NASHI_ROOT}/vendor/directx)
  target_compile_definitions(nashi PRIVATE NASHI_USE_DIRECT3D12)

  set(CSO_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
  file(GLOB  PIXEL_SHADERS "${NASHI_ROOT}/src/shaders/*.frag")
  file(GLOB  VERTEX_SHADERS "${NASHI_ROOT}/src/shaders/*.vert")

  foreach(PIXEL_SHADER ${PIXEL_SHADERS})
    get_filename_component(FILE_NAME ${PIXEL_SHADER} NAME)
    set(CSO_FILE "${CSO_DIR}/${FILE_NAME}.cso")
    add_custom_command(
      OUTPUT ${CSO_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CSO_DIR}
      COMMAND ${DXC_PATH} -T ps_6_0 -E main -Fo ${CSO_FILE} ${PIXEL_SHADER}
      DEPENDS ${PIXEL_SHADER}
      COMMENT "Compiling shader ${FILE_NAME} to CSO"
      VERBATIM
    )
    list(APPEND CSO_SHADERS ${CSO_FILE})
  endforeach()

  foreach(VERTEX_SHADER ${VERTEX_SHADERS})
    get_filename_component(FILE_NAME ${VERTEX_SHADER} NAME)
    set(CSO_FILE "${CSO_DIR}/${FILE_NAME}.cso")
    add_custom_command(
      OUTPUT ${CSO_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CSO_DIR}
      COMMAND ${DXC_PATH} -T vs_6_0 -E main -Fo ${CSO_FILE} ${VERTEX_SHADER}
      DEPENDS ${VERTEX_SHADER}
      COMMENT "Compiling shader ${FILE_NAME} to CSO"
      VERBATIM
    )
    list(APPEND CSO_SHADERS ${CSO_FILE})
  endforeach()

  add_custom_target(shaders ALL DEPENDS ${CSO_SHADERS})
  add_dependencies(nashi shaders)

  add_custom_command(TARGET nashi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DSRC="${CSO_DIR}" -DDEST="$<TARGET_FILE_DIR:nashi>" -P "${CMAKE_CURRENT_SOURCE_DIR}/CopyShaders.cmake"
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


