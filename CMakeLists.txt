cmake_minimum_required(VERSION 4.1)
project(intersim)

set(CMAKE_CXX_STANDARD 20)

file(GLOB IMGUI_SOURCES
    third_party/imgui/imgui/imgui.cpp
    third_party/imgui/imgui/imgui.h
    third_party/imgui/imgui/imgui_draw.cpp
    third_party/imgui/imgui/imgui_demo.cpp
    third_party/imgui/imgui/imgui_tables.cpp
    third_party/imgui/imgui/imgui_widgets.cpp
    third_party/imgui/imgui/backends/imgui_impl_sdl3.cpp
    third_party/imgui/imgui/backends/imgui_impl_sdl3.h
    third_party/imgui/imgui/backends/imgui_impl_sdlrenderer3.cpp
    third_party/imgui/imgui/backends/imgui_impl_sdlrenderer3.h
    third_party/imgui/imgui/misc/cpp/imgui_stdlib.cpp
    third_party/imgui/imgui/misc/cpp/imgui_stdlib.h
)

add_executable(intersim
    ${IMGUI_SOURCES}

    src/main.cpp
    src/log.h
    src/taskScheduler.cpp
    src/taskScheduler.h
    src/pdp11/kw11.cpp
    src/pdp11/kw11.h
    src/pdp11/device.cpp
    src/pdp11/device.h
    src/pdp11/mem.cpp
    src/pdp11/mem.h
    src/pdp11/ui/kw11_window.h
    src/pdp11/dl11.cpp
    src/pdp11/dl11.h
    src/pdp11/ui/dl11_window.h
    src/pdp11/cpu.cpp
    src/pdp11/cpu.h
    src/pdp11/ui/cpu1170_window.h
    src/pdp11/rk11.cpp
    src/pdp11/rk11.h
    src/pdp11/ui/rk11_window.h
    src/pdp11/mmu.cpp
    src/pdp11/mmu.h
    src/pdp11/unibus.cpp
    src/pdp11/unibus.h
    src/pdp11/ui/mmu_window.h
)

target_include_directories(${PROJECT_NAME} PUBLIC
    third_party/imgui/imgui
    third_party/imgui/imgui/backends
    third_party/imgui/imgui/misc/cpp
)

target_link_libraries(${PROJECT_NAME}
    SDL3
    GL
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
)
