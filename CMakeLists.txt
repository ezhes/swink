cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

# System configuration (RPI3)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR BCM2837)
set(CLANG_TARGET_TRIPLE arm64-none-eabi)
set(LINKER_TARGET aarch64elf)
add_compile_definitions(PLATFORM_RPI3)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})
set(CMAKE_CXX_COMPILER clang)
set(CMAKE_CXX_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_ASM_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})

set( CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY )
add_link_options("LINKER:-nostdlib,-m,${LINKER_TARGET},-flto")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdinc")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")

project (kernel C ASM)
add_executable(kernel
    kernel/core/start/start.S
    kernel/core/start/vm_bootstrap.S
    kernel/core/start/start.c

    kernel/core/exception/exception.S
    kernel/core/exception/exception.c

    kernel/core/vm/vm_page_allocator.c

    kernel/lib/string.c
    kernel/lib/debug.c
    kernel/lib/stdio.c
    kernel/lib/list.c
    
    kernel/machine/io/mini_uart/mini_uart.c
    kernel/machine/io/console/console.c
    kernel/machine/io/pmc/pmc.c
    kernel/machine/io/vc/vc_mailbox.c
    kernel/machine/io/vc/vc_functions.c

    kernel/machine/pmap/pmap.c
    kernel/machine/pmap/pmap_init.c
    kernel/machine/pmap/pmap_pfa.c

    kernel/machine/routines/routines.S

    kernel/machine/synchronization/synchs.c
)
target_link_options(kernel PUBLIC "LINKER:-T,${CMAKE_SOURCE_DIR}/kernel/link.ld")
target_include_directories(kernel PRIVATE "${CMAKE_SOURCE_DIR}/kernel")
target_include_directories(kernel PRIVATE "${CMAKE_SOURCE_DIR}/kernel")

add_custom_command(
    TARGET kernel POST_BUILD
    COMMAND llvm-objcopy -O binary ./kernel ./kernel8.img
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building kernel image..." )
