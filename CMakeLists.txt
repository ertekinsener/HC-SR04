cmake_minimum_required(VERSION 3.16)

# set(STM32_TOOLCHAIN_PATH /home/ertekin/Workspace/STM32/Tools/Toolchains/arm-gnu-toolchain-14.3.rel1)
set(STM32_TOOLCHAIN_PATH /usr/bin)
set(STM32_TARGET_TRIPLET arm-none-eabi)
set(STM32_CUBE_F4_PATH /home/mergen/Workspace/Embedded/Tools/ST/CubeRepos/STM32CubeF4)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32_gcc.cmake)
# set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

project(distance-meter C CXX ASM)
set(CMAKE_MESSAGE_LOG_LEVEL TRACE)
set(HAL_COMP_LIST STM32F401RE LL_RCC LL_PWR LL_GPIO LL_UTILS LL_TIM)
set(CMSIS_COMP_LIST STM32F401RE)

find_package(CMSIS COMPONENTS "${CMSIS_COMP_LIST}" REQUIRED)
find_package(HAL COMPONENTS "${HAL_COMP_LIST}" REQUIRED)
find_package(FreeRTOS COMPONENTS ARM_CM4F STM32F401RE REQUIRED)
add_executable(${PROJECT_NAME})
stm32_add_linker_script(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/linker_scripts/STM32F401XX_FLASH.ld)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    USE_FULL_LL_DRIVER
    HSE_VALUE=25000000
    HSE_STARTUP_TIMEOUT=100
    LSE_STARTUP_TIMEOUT=5000
    LSE_VALUE=32768
    EXTERNAL_CLOCK_VALUE=12288000
    HSI_VALUE=16000000
    LSI_VALUE=32000
    VDD_VALUE=3300
    PREFETCH_ENABLE=1
    INSTRUCTION_CACHE_ENABLE=1
    DATA_CACHE_ENABLE=1
    STM32F401xE
)

# file(GLOB CORE_SRC_FILES "${PROJECT_SOURCE_DIR}/Core/Src/*.c" "${PROJECT_SOURCE_DIR}/Core/Src/*.cc" "${PROJECT_SOURCE_DIR}/Core/Src/*.cpp")
# file(GLOB SEGGER_SYSTEMVIEW_SRC_FILES "${PROJECT_SOURCE_DIR}/segger/SystemView/Sample/FreeRTOSV10/Config/Cortex-M/*.c" "${PROJECT_SOURCE_DIR}/segger/SystemView/Sample/FreeRTOSV10/*.c" "${PROJECT_SOURCE_DIR}/segger/SystemView/SYSVIEW/*.c")
# file(GLOB SEGGER_RTT_SRC_FILES "${PROJECT_SOURCE_DIR}/segger/RTT/RTT/*.c")
# target_sources(${PROJECT_NAME} PRIVATE
#     ${CORE_SRC_FILES}
#     ${SEGGER_SYSTEMVIEW_SRC_FILES}
#     ${SEGGER_RTT_SRC_FILES}
# )

# Enable link-time optimization and size optimization
# target_compile_options(${PROJECT_NAME} PRIVATE -Og -g3)
# target_link_options(${PROJECT_NAME} PRIVATE -Og -g3)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:
    -fno-exceptions # Removes exception handling logic (huge savings)
    -fno-rtti # Removes typeinfo metadata
    -fno-use-cxa-atexit # Reduces overhead for static object destruction
    -fno-threadsafe-statics # Removes mutex overhead for static local variables
    >
)
# target_include_directories(${PROJECT_NAME} PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
#     ${CMAKE_CURRENT_SOURCE_DIR}/segger/SystemView/Config
#     ${CMAKE_CURRENT_SOURCE_DIR}/segger/RTT/Config
#     ${CMAKE_CURRENT_SOURCE_DIR}/segger/SystemView/Sample/FreeRTOSV10
#     ${CMAKE_CURRENT_SOURCE_DIR}/segger/SystemView/SEGGER
#     ${CMAKE_CURRENT_SOURCE_DIR}/segger/SystemView/SEGGER
#     ${CMAKE_CURRENT_SOURCE_DIR}/segger/RTT/RTT
#     ${CMAKE_CURRENT_SOURCE_DIR}/segger/SystemView/SYSVIEW

# )

# set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map")

target_link_libraries(${PROJECT_NAME} PRIVATE
    # HAL::STM32::F0
    HAL::STM32::F4::LL_RCC
    HAL::STM32::F4::LL_PWR
    HAL::STM32::F4::LL_GPIO
    HAL::STM32::F4::LL_UTILS
    HAL::STM32::F4::LL_TIM
    CMSIS::STM32::F401RE
    # FreeRTOS::STM32::F4::ARM_CM4F
    # FreeRTOS::STM32::F4::Heap::4
    STM32::Nano
)

target_link_options(${PROJECT_NAME} PRIVATE -Wl,-Map=${PROJECT_NAME}.map)
target_link_options(${PROJECT_NAME} PRIVATE
    "-Wl,--print-memory-usage"
)
stm32_print_size_of_target(${PROJECT_NAME})

