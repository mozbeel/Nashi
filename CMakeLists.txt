cmake_minimum_required(VERSION 3.10)
project(nashi)

set(CMAKE_CXX_STANDARD 17)
set(NASHI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/app")
set(NASHI_USE_VULKAN OFF)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)

file(GLOB_RECURSE SOURCES "${NASHI_ROOT}/src/*.cpp") 
file(GLOB_RECURSE HEADERS "${NASHI_ROOT}/src/headers/*.hpp" "${NASHI_ROOT}/src/headers/*.h") 
file(GLOB_RECURSE SHADERS "${NASHI_ROOT}/src/shaders/*.frag" "${NASHI_ROOT}/src/shaders/*.vert" "${NASHI_ROOT}/src/shaders/*.comp")

add_executable(nashi ${SOURCES} ${SHADERS} ${HEADERS})

source_group(TREE "${NASHI_ROOT}/src/shaders" PREFIX "Shaders" FILES ${SHADERS})
source_group(TREE "${NASHI_ROOT}/src/headers" PREFIX "Headers" FILES ${HEADERS})

find_package(glm)

if (NOT TARGET glm::glm)
  include(FetchContent)
  FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG master  # or latest stable
  )
  FetchContent_MakeAvailable(glm)
endif()

if (WIN32)
  find_package(Vulkan)

  if (Vulkan_FOUND)
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

endif()
target_include_directories(nashi PRIVATE "${NASHI_ROOT}/src/headers")

if (NOT NASHI_VR)
 include(FetchContent)

  # Declare SDL3 as an external project
  FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG "main"     # or whatever the latest tag is
  )

  # Download & add SDL3 subdirectory
  FetchContent_MakeAvailable(SDL3)

  set(MIDDLEWARE SDL3::SDL3)

endif()

if (NASHI_USE_VULKAN)
  target_link_libraries(nashi PRIVATE Vulkan::Vulkan glm::glm ${MIDDLEWARE})
  target_compile_definitions(nashi PRIVATE NASHI_USE_VULKAN)

  set(SPIRV_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")  # This includes config (Debug/Release)

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
    COMMAND ${CMAKE_COMMAND} -DSRC="${SPIRV_DIR}" -DDEST="$<TARGET_FILE_DIR:nashi>"
            -P "${CMAKE_SOURCE_DIR}/CopyShaders.cmake"
    COMMENT "Copying compiled SPIR-V shaders to binary directory"
  )

endif()
