include_directories(Generated_Source/PSoC5)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(balloc balloc.c)

add_library(list list.c)

add_library(logging logging.c)
target_link_libraries(logging balloc list)
# target_compile_definitions(logging PUBLIC "LOG_ENABLE_TRACE=$<IF:$<CONFIG:Debug>,1,0>")
# message(${CMAKE_BUILD_TYPE})
# message("CMAKE_BUILD_TYPE = '${CMAKE_BUILD_TYPE}'")
if(${LOG_TRACE_ENABLE})
    target_compile_definitions(logging PUBLIC LOG_ENABLE_TRACE=1)
else()
    target_compile_definitions(logging PRIVATE LOG_ENABLE_TRACE=0)
endif()

add_library(serial serial.c)

add_library(audio_proc audio_proc.c)

add_library(filter filter.c)

# add_library(audio_out audio_out.c)
