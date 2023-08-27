include_directories(Generated_Source/PSoC5)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/FreeRTOS/Source/include)
include_directories(${CMAKE_SOURCE_DIR}/FreeRTOS/Source/portable/GCC/ARM_CM3)
include_directories(${CMAKE_SOURCE_DIR}/FreeRTOS/Source/include)

# Cast all the things
add_compile_options("-Wall" "-Wno-int-to-pointer-cast" "-Wno-pointer-to-int-cast")

add_library(list list2.c)

add_library(logging logger.c)
target_link_libraries(logging balloc list)

if(${LOG_TRACE_ENABLE})
    target_compile_definitions(logging PUBLIC LOG_ENABLE_TRACE=1)
else()
    target_compile_definitions(logging PRIVATE LOG_ENABLE_TRACE=0)
endif()

add_library(audio_tx audio_tx.c)

add_library(usb usb.c)

add_library(sync sync.c)

add_library(serial serial_tx.c serial_rx.c serial_logger.c)
add_library(audio_proc audio_proc.c)

# add_library(filter filter.c)

add_library(muter muter.c)

# For intellisense
add_library(main main.c)
target_compile_options(main PUBLIC "-Wno-int-to-pointer-cast")
target_compile_definitions(main PRIVATE TEST_BUILD=1)


