cmake_minimum_required(VERSION 4.1)
project(BetterRenderDragon)

set(CMAKE_CXX_STANDARD 23)
if(MSVC)
    add_compile_options(/MT #[[/MP]])
#    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

include(FetchContent)

macro(download NAME REPO TAG)
    FetchContent_Declare(
            ${NAME}
            GIT_REPOSITORY ${REPO}
            GIT_TAG ${TAG}
    )
    FetchContent_MakeAvailable(${NAME})
endmacro()

download(libhat https://github.com/BasedInc/libhat.git master)
download(minhook https://github.com/TsudaKageyu/minhook.git master)
download(nlohmann_json https://github.com/nlohmann/json.git v3.11.3)
download(glm https://github.com/g-truc/glm.git 1.0.1)
download(gsl https://github.com/microsoft/GSL.git v4.0.0)
download(entt https://github.com/skypjack/entt.git v3.14.0)
download(rapidjson https://github.com/Tencent/rapidjson.git master)
download(type_safe https://github.com/foonathan/type_safe.git v0.2.4)
download(imgui https://github.com/ocornut/imgui.git 93e396ffb75834b4fa79004a4f0fccb600a8e164)
download(expected-lite https://github.com/nonstd-lite/expected-lite.git v0.8.0)
set(KIERO_INCLUDE_D3D11 ON)
set(KIERO_INCLUDE_D3D12 ON)
set(KIERO_BUILD_EXAMPLES OFF)
download(kiero https://github.com/BasedInc/kiero.git master)
download(safetyhook https://github.com/cursey/safetyhook.git main)

set(IMGUI_SOURCE_FILES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

file(GLOB_RECURSE SOURCE_FILES
        src/*.cpp
        src/*.h
        include/*.cpp
        resources/Resources.rc
)
add_library(BetterRenderDragon SHARED src/dllmain.cpp ${SOURCE_FILES} ${IMGUI_SOURCE_FILES})

target_include_directories(BetterRenderDragon PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_include_directories(BetterRenderDragon PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(BetterRenderDragon PRIVATE "${rapidjson_SOURCE_DIR}/include")
target_include_directories(BetterRenderDragon PRIVATE "${imgui_SOURCE_DIR}")
target_include_directories(BetterRenderDragon PRIVATE "${imgui_SOURCE_DIR}/backends")

target_link_libraries(BetterRenderDragon PRIVATE minhook libhat nlohmann_json::nlohmann_json glm::glm Microsoft.GSL::GSL EnTT::EnTT type_safe d3d11 d3d12 dxgi d2d1 dwrite d3dcompiler expected-lite kiero safetyhook windowsapp)
