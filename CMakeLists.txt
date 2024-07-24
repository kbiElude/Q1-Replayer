cmake_minimum_required (VERSION 2.8)
project (Launcher)

option (APIINTERCEPTOR_DUMP_API_CALLS "No need for API dump support" OFF)


file(GLOB LauncherSources  "${Launcher_SOURCE_DIR}/Launcher/*.cpp")
file(GLOB ReplayerIncludes "${Launcher_SOURCE_DIR}/Replayer/include/*.h")
file(GLOB ReplayerSources  "${Launcher_SOURCE_DIR}/Replayer/src/*.cpp")

add_subdirectory(deps/APIInterceptor)
add_subdirectory(deps/glfw)
add_subdirectory(deps/imgui-cmake)

add_executable(Launcher        ${LauncherIncludes}
                               ${LauncherSources})
add_library   (Replayer SHARED ${ReplayerIncludes}
                               ${ReplayerSources})

include_directories  ("${APIInterceptor_SOURCE_DIR}")
include_directories  ("${APIInterceptor_SOURCE_DIR}/include")
include_directories  ("${APIInterceptor_SOURCE_DIR}/include/Khronos")
include_directories  ("${APIInterceptor_BINARY_DIR}")
include_directories  ("${Launcher_SOURCE_DIR}/deps/")
include_directories  ("${Launcher_SOURCE_DIR}/deps/imgui")
include_directories  ("${Launcher_SOURCE_DIR}/Replayer/include/")

target_link_libraries(Launcher Detours)
target_link_libraries(Replayer APIInterceptor glfw imgui)

add_dependencies     (Launcher Replayer)

source_group ("Launcher include files"   FILES ${LauncherIncludes})
source_group ("Launcher source files"    FILES ${LauncherSources})
source_group ("Replayer include files"   FILES ${ReplayerIncludes})
source_group ("Replayer source files"    FILES ${ReplayerSources})

#SET_TARGET_PROPERTIES(${Replayer} PROPERTIES LINK_FLAGS_DEBUG "/WHOLEARCHIVE")
#SET_TARGET_PROPERTIES(${Replayer} PROPERTIES LINK_FLAGS_RELEASE "/WHOLEARCHIVE")