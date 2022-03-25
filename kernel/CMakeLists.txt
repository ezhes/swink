set(
    KERNEL_VARIANT
    "DEBUG"
    CACHE STRING
    "The kernel variant to build. Options: DEBUG, RELEASE, TESTING"
)
add_executable(kernel
    core/start/start.S
    core/start/vm_bootstrap.S
    core/start/start.c

    core/exception/exception.S
    core/exception/exception.c

    core/vm/vm_page_allocator.c

    lib/string.c
    lib/debug.c
    lib/stdio.c
    lib/list.c
    
    machine/io/mini_uart/mini_uart.c
    machine/io/console/console.c
    machine/io/pmc/pmc.c
    machine/io/vc/vc_mailbox.c
    machine/io/vc/vc_functions.c

    machine/pmap/pmap.c
    machine/pmap/pmap_init.c
    machine/pmap/pmap_pfa.c

    machine/routines/routines.S

    machine/synchronization/synchs.c
)

if (${KERNEL_VARIANT} STREQUAL "DEBUG")
    add_compile_definitions(CONFIG_DEBUG)
elseif(${KERNEL_VARIANT} STREQUAL "RELEASE")

elseif(${KERNEL_VARIANT} STREQUAL "TESTING")
    add_compile_definitions(CONFIG_DEBUG)
    add_compile_definitions(CONFIG_TESTING)
    add_subdirectory(testing)
else()
    message(FATAL_ERROR "Invalid KERNEL_VARIANT \"${KERNEL_VARIANT}\"")
endif()

target_link_options(kernel PUBLIC "LINKER:-T,${CMAKE_SOURCE_DIR}/kernel/link.ld")
target_include_directories(kernel PRIVATE "./")

add_custom_command(
    TARGET kernel POST_BUILD
    COMMAND llvm-objcopy -O binary ./kernel ./kernel8.img
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/kernel
    COMMENT "Building kernel image..." 
)