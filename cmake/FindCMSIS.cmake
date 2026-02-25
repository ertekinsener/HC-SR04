# For information about why and how of this file: https://cmake.org/cmake/help/latest/command/find_package.html

# Define list of supported RTOS components
set(CMSIS_RTOS RTOS RTOS_V2)

# If no components specified, use all supported STM32 families
if(NOT CMSIS_FIND_COMPONENTS)
    set(CMSIS_FIND_COMPONENTS ${STM32_SUPPORTED_FAMILIES_LONG_NAME})
endif()

# Expand STM32H7 family into specific cores
if(STM32H7 IN_LIST CMSIS_FIND_COMPONENTS)
    list(REMOVE_ITEM CMSIS_FIND_COMPONENTS STM32H7)
    list(APPEND CMSIS_FIND_COMPONENTS STM32H7_M7 STM32H7_M4)
endif()

# Expand STM32WB family into specific cores
if(STM32WB IN_LIST CMSIS_FIND_COMPONENTS)
    list(REMOVE_ITEM CMSIS_FIND_COMPONENTS STM32WB)
    list(APPEND CMSIS_FIND_COMPONENTS STM32WB_M4)
endif()

# Expand STM32WL family into specific cores
if(STM32WL IN_LIST CMSIS_FIND_COMPONENTS)
    list(REMOVE_ITEM CMSIS_FIND_COMPONENTS STM32WL)
    list(APPEND CMSIS_FIND_COMPONENTS STM32WL_M4 STM32WL_M0PLUS)
endif()

# Expand STM32MP1 family into specific cores
if(STM32MP1 IN_LIST CMSIS_FIND_COMPONENTS)
    list(REMOVE_ITEM CMSIS_FIND_COMPONENTS STM32MP1)
    list(APPEND CMSIS_FIND_COMPONENTS STM32MP1_M4)
endif()

# Remove duplicate entries from components list
list(REMOVE_DUPLICATES CMSIS_FIND_COMPONENTS)

# Iterate through all components and categorize them as RTOS or family components
foreach(COMP ${CMSIS_FIND_COMPONENTS})
    # Convert component name to lowercase
    string(TOLOWER ${COMP} COMP_L)
    # Convert component name to uppercase
    string(TOUPPER ${COMP} COMP)

    # Check if component is an RTOS component
    if(${COMP} IN_LIST CMSIS_RTOS)
        # Add to RTOS components list
        list(APPEND CMSIS_FIND_COMPONENTS_RTOS ${COMP})
        # Skip to next iteration
        continue()
    endif()

    # Component is not RTOS, so check if it's a valid STM32 family component using regex
    string(REGEX MATCH "^STM32([CFGHLMUW]P?[0-9BL])([0-9A-Z][0-9M][A-Z][0-9A-Z])?_?(M0PLUS|M4|M7)?.*$" COMP ${COMP})
    # If regex matched, add to family components list
    if(CMAKE_MATCH_1)
        list(APPEND CMSIS_FIND_COMPONENTS_FAMILIES ${COMP})
    endif()
endforeach()

# If no family components found, use all supported families as default
if(NOT CMSIS_FIND_COMPONENTS_FAMILIES)
    set(CMSIS_FIND_COMPONENTS_FAMILIES ${STM32_SUPPORTED_FAMILIES_LONG_NAME})
endif()

# If no RTOS components found, use all supported RTOS types as default
if(NOT CMSIS_FIND_COMPONENTS_RTOS)
    set(CMSIS_FIND_COMPONENTS_RTOS ${CMSIS_RTOS})
endif()

# Log the families being searched
message(STATUS "Search for CMSIS families: ${CMSIS_FIND_COMPONENTS_FAMILIES}")
# Log the RTOS versions being searched
message(STATUS "Search for CMSIS RTOS: ${CMSIS_FIND_COMPONENTS_RTOS}")

# Include device information from stm32 module
include(stm32/devices)

# Function to generate default linker script for a specific device and core
function(cmsis_generate_default_linker_script FAMILY DEVICE CORE)
    # If core is specified, add core suffix to variable names
    if(CORE)
        set(CORE_C "::${CORE}")
        set(CORE_U "_${CORE}")
    endif()
    
    # Set output linker script filename
    set(OUTPUT_LD_FILE "${CMAKE_CURRENT_BINARY_DIR}/${DEVICE}${CORE_U}.ld")
    
    # Handle MP1 family differently (uses pre-made linker script)
    if(${FAMILY} STREQUAL MP1)
        # Convert family name to lowercase
        string(TOLOWER ${FAMILY} FAMILY_L)
        # Search for existing MP1 linker script
        find_file(CMSIS_${FAMILY}${CORE_U}_LD_SCRIPT
            NAMES stm32mp15xx_m4.ld
            PATHS "${CMSIS_${FAMILY}${CORE_U}_PATH}/Source/Templates/gcc/linker"
            NO_DEFAULT_PATH
        )
        # Copy the found linker script to output location
        add_custom_command(OUTPUT "${OUTPUT_LD_FILE}"
            COMMAND ${CMAKE_COMMAND}
                -E copy ${CMSIS_${FAMILY}${CORE_U}_LD_SCRIPT} ${OUTPUT_LD_FILE})
    else()    
        # Get FLASH memory information for the device
        stm32_get_memory_info(FAMILY ${FAMILY} DEVICE ${DEVICE} CORE ${CORE} FLASH SIZE FLASH_SIZE ORIGIN FLASH_ORIGIN)
        # Get RAM memory information for the device
        stm32_get_memory_info(FAMILY ${FAMILY} DEVICE ${DEVICE} CORE ${CORE} RAM SIZE RAM_SIZE ORIGIN RAM_ORIGIN)
        # Get CCRAM memory information for the device
        stm32_get_memory_info(FAMILY ${FAMILY} DEVICE ${DEVICE} CORE ${CORE} CCRAM SIZE CCRAM_SIZE ORIGIN CCRAM_ORIGIN)
        # Get shared RAM memory information for the device
        stm32_get_memory_info(FAMILY ${FAMILY} DEVICE ${DEVICE} CORE ${CORE} RAM_SHARE SIZE RAM_SHARE_SIZE ORIGIN RAM_SHARE_ORIGIN)
        # Get HEAP size information for the device
        stm32_get_memory_info(FAMILY ${FAMILY} DEVICE ${DEVICE} CORE ${CORE} HEAP SIZE HEAP_SIZE)
        # Get STACK size information for the device
        stm32_get_memory_info(FAMILY ${FAMILY} DEVICE ${DEVICE} CORE ${CORE} STACK SIZE STACK_SIZE)

        # Generate linker script using memory information
        add_custom_command(OUTPUT "${OUTPUT_LD_FILE}"
            COMMAND ${CMAKE_COMMAND} 
                -DFLASH_ORIGIN="${FLASH_ORIGIN}" 
                -DRAM_ORIGIN="${RAM_ORIGIN}" 
                -DCCRAM_ORIGIN="${CCRAM_ORIGIN}" 
                -DRAM_SHARE_ORIGIN="${RAM_SHARE_ORIGIN}" 
                -DFLASH_SIZE="${FLASH_SIZE}" 
                -DRAM_SIZE="${RAM_SIZE}" 
                -DCCRAM_SIZE="${CCRAM_SIZE}"
                -DRAM_SHARE_SIZE="${RAM_SHARE_SIZE}" 
                -DSTACK_SIZE="${STACK_SIZE}" 
                -DHEAP_SIZE="${HEAP_SIZE}" 
                -DLINKER_SCRIPT="${OUTPUT_LD_FILE}"
                -P "${STM32_CMAKE_DIR}/stm32/linker_ld.cmake"
        )
    endif()
    # Create custom target for linker script generation
    add_custom_target(CMSIS_LD_${DEVICE}${CORE_U} DEPENDS "${OUTPUT_LD_FILE}")
    # Add linker script target as dependency to device library
    add_dependencies(CMSIS::STM32::${DEVICE}${CORE_C} CMSIS_LD_${DEVICE}${CORE_U})
    # Add generated linker script to device library
    stm32_add_linker_script(CMSIS::STM32::${DEVICE}${CORE_C} INTERFACE "${OUTPUT_LD_FILE}")
endfunction() 

# Process each family component
foreach(COMP ${CMSIS_FIND_COMPONENTS_FAMILIES})
    # Convert component name to lowercase
    string(TOLOWER ${COMP} COMP_L)
    # Convert component name to uppercase
    string(TOUPPER ${COMP} COMP)
    
    # Parse component name using regex to extract family, device, and core
    string(REGEX MATCH "^STM32([CFGHLMUW]P?[0-9BL])([0-9A-Z][0-9M][A-Z][0-9A-Z])?_?(M0PLUS|M4|M7)?.*$" COMP ${COMP})
    # CMAKE_MATCH_<n> contains n'th subexpression
    # CMAKE_MATCH_0 contains full match

    # Check if regex parsing was successful
    if((NOT CMAKE_MATCH_1) AND (NOT CMAKE_MATCH_2))
        # Exit with error if component format is invalid
        message(FATAL_ERROR "Unknown CMSIS component: ${COMP}")
    endif()
    
    # If full device name is specified (family + specific device)
    if(CMAKE_MATCH_2)
        # Extract family from regex match
        set(FAMILY ${CMAKE_MATCH_1})
        # Extract specific device from regex match
        set(STM_DEVICES "${CMAKE_MATCH_1}${CMAKE_MATCH_2}")
        # Log full device name match
        message(TRACE "FindCMSIS: full device name match for COMP ${COMP}, STM_DEVICES is ${STM_DEVICES}")
    else()
        # Extract family from regex match
        set(FAMILY ${CMAKE_MATCH_1})
        # Get all devices in the family
        stm32_get_devices_by_family(STM_DEVICES FAMILY ${FAMILY})
        # Log family-only match
        message(TRACE "FindCMSIS: family only match for COMP ${COMP}, STM_DEVICES is ${STM_DEVICES}")
    endif()
    
    # If core is specified in component name
    if(CMAKE_MATCH_3)
        # Extract core from regex match
        set(CORE ${CMAKE_MATCH_3})
        # Set core with C++ namespace separator
        set(CORE_C "::${CORE}")
        # Set core with underscore separator
        set(CORE_U "_${CORE}")
        # Set core with 'c' prefix and lowercase for startup file names
        set(CORE_Ucm "_c${CORE}")
        # Convert core underscore version to lowercase
        string(TOLOWER ${CORE_Ucm} CORE_Ucm)
        # Log core match in component name
        message(TRACE "FindCMSIS: core match in component name for COMP ${COMP}. CORE is ${CORE}")
    else()
        # Unset core variables if no core specified
        unset(CORE)
        unset(CORE_C)
        unset(CORE_U)
        unset(CORE_Ucm)
    endif()
    
    # Convert family name to lowercase
    string(TOLOWER ${FAMILY} FAMILY_L)
    
    # If CMSIS path not set, try to get from environment variable
    if((NOT STM32_CMSIS_${FAMILY}_PATH) AND (NOT STM32_CUBE_${FAMILY}_PATH) AND (DEFINED ENV{STM32_CUBE_${FAMILY}_PATH}))
        # Set path from environment variable
        set(STM32_CUBE_${FAMILY}_PATH $ENV{STM32_CUBE_${FAMILY}_PATH} CACHE PATH "Path to STM32Cube${FAMILY}")
        # Log that environment variable was used
        message(STATUS "ENV STM32_CUBE_${FAMILY}_PATH specified, using STM32_CUBE_${FAMILY}_PATH: ${STM32_CUBE_${FAMILY}_PATH}")
    endif()

    # If no path found, use default location
    if((NOT STM32_CMSIS_${FAMILY}_PATH) AND (NOT STM32_CUBE_${FAMILY}_PATH))
        # Set default path
        set(STM32_CUBE_${FAMILY}_PATH /opt/STM32Cube${FAMILY} CACHE PATH "Path to STM32Cube${FAMILY}")
        # Log that default path is being used
        message(STATUS "Neither STM32_CUBE_${FAMILY}_PATH nor STM32_CMSIS_${FAMILY}_PATH specified using default STM32_CUBE_${FAMILY}_PATH: ${STM32_CUBE_${FAMILY}_PATH}")
    endif()
     
    # Search for CMSIS core header file (cmsis_gcc.h)
    find_path(CMSIS_${FAMILY}${CORE_U}_CORE_PATH
        NAMES Include/cmsis_gcc.h
        PATHS "${STM32_CMSIS_PATH}" "${STM32_CUBE_${FAMILY}_PATH}/Drivers/CMSIS"
        NO_DEFAULT_PATH
    )
    # If core header not found, skip this family
    if (NOT CMSIS_${FAMILY}${CORE_U}_CORE_PATH)
        # Log verbose message about missing core header
        message(VERBOSE "FindCMSIS: cmsis_gcc.h for ${FAMILY}${CORE_U} has not been found")
        # Continue to next family
        continue()
    endif()
    
    # Search for STM32 family header file (stm32xxxx.h)
    find_path(CMSIS_${FAMILY}${CORE_U}_PATH
        NAMES Include/stm32${FAMILY_L}xx.h
        PATHS "${STM32_CMSIS_${FAMILY}_PATH}" "${STM32_CUBE_${FAMILY}_PATH}/Drivers/CMSIS/Device/ST/STM32${FAMILY}xx"
        NO_DEFAULT_PATH
    )
    # If family header not found, skip this family
    if (NOT CMSIS_${FAMILY}${CORE_U}_PATH)
        # Log verbose message about missing family header
        message(VERBOSE "FindCMSIS: stm32${FAMILY_L}xx.h for ${FAMILY}${CORE_U} has not been found")
        # Continue to next family
        continue()
    endif()
    # Add include directories to list
    list(APPEND CMSIS_INCLUDE_DIRS "${CMSIS_${FAMILY}${CORE_U}_CORE_PATH}/Include" "${CMSIS_${FAMILY}${CORE_U}_PATH}/Include")

    # If version not already set, try to find it from PDSC file
    if(NOT CMSIS_${FAMILY}${CORE_U}_VERSION)
        # Search for ARM.CMSIS.pdsc file
        find_file(CMSIS_${FAMILY}${CORE_U}_PDSC
            NAMES ARM.CMSIS.pdsc
            PATHS "${CMSIS_${FAMILY}${CORE_U}_CORE_PATH}"
            NO_DEFAULT_PATH
        )
        # If PDSC file not found, set default version
        if (NOT CMSIS_${FAMILY}${CORE_U}_PDSC)
            # Set version to 0.0.0 as default
            set(CMSIS_${FAMILY}${CORE_U}_VERSION "0.0.0")
        else()
            # Extract version string from PDSC file
            file(STRINGS "${CMSIS_${FAMILY}${CORE_U}_PDSC}" VERSION_STRINGS REGEX "<release version=\"([0-9]*\\.[0-9]*\\.[0-9]*)\" date=\"[0-9]+\\-[0-9]+\\-[0-9]+\">")
            # Get first matching version string
            list(GET VERSION_STRINGS 0 STR)
            # Parse version numbers using regex
            string(REGEX MATCH "<release version=\"([0-9]*)\\.([0-9]*)\\.([0-9]*)\" date=\"[0-9]+\\-[0-9]+\\-[0-9]+\">" MATCHED ${STR})
            # Set version from parsed matches
            set(CMSIS_${FAMILY}${CORE_U}_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" CACHE INTERNAL "CMSIS STM32${FAMILY}${CORE_U} version")
        endif()
    endif()
    
    # Set version for this component
    set(CMSIS_${COMP}_VERSION ${CMSIS_${FAMILY}${CORE_U}_VERSION})
    # Set overall CMSIS version
    set(CMSIS_VERSION ${CMSIS_${COMP}_VERSION})

    # Create CMSIS library target for family if it doesn't exist
    if(NOT (TARGET CMSIS::STM32::${FAMILY}${CORE_C}))
        # Log creation of new library
        message(TRACE "FindCMSIS: creating library CMSIS::STM32::${FAMILY}${CORE_C}")
        # Add interface library for CMSIS
        add_library(CMSIS::STM32::${FAMILY}${CORE_C} INTERFACE IMPORTED)
        # Link to STM32 family target which contains compile options
        target_link_libraries(CMSIS::STM32::${FAMILY}${CORE_C} INTERFACE STM32::${FAMILY}${CORE_C})
        # Add CMSIS core include directory
        target_include_directories(CMSIS::STM32::${FAMILY}${CORE_C} INTERFACE "${CMSIS_${FAMILY}${CORE_U}_CORE_PATH}/Include")
        # Add STM32 family include directory
        target_include_directories(CMSIS::STM32::${FAMILY}${CORE_C} INTERFACE "${CMSIS_${FAMILY}${CORE_U}_PATH}/Include")
    endif()

    # Search for system initialization source file (system_stm32xxxx.c)
    find_file(CMSIS_${FAMILY}${CORE_U}_SYSTEM
        NAMES system_stm32${FAMILY_L}xx.c
        PATHS "${CMSIS_${FAMILY}${CORE_U}_PATH}/Source/Templates"
        NO_DEFAULT_PATH
    )
    # Add system source file to list
    list(APPEND CMSIS_SOURCES "${CMSIS_${FAMILY}${CORE_U}_SYSTEM}")
    
    # If system file not found, skip this family
    if(NOT CMSIS_${FAMILY}${CORE_U}_SYSTEM)
        # Log verbose message about missing system file
        message(VERBOSE "FindCMSIS: system_stm32${FAMILY_L}xx.c for ${FAMILY}${CORE_U} has not been found")
        # Continue to next family
        continue()
    endif()
    
    # Flag to track if devices were found
    set(STM_DEVICES_FOUND TRUE)
    # Process each device in the family
    foreach(DEVICE ${STM_DEVICES})
        # Log current device being processed
        message(TRACE "FindCMSIS: Iterating DEVICE ${DEVICE}")
        
        # Get cores supported by this device
        stm32_get_cores(DEV_CORES FAMILY ${FAMILY} DEVICE ${DEVICE})
        # If specific core requested, check if device supports it
        if(CORE AND (NOT ${CORE} IN_LIST DEV_CORES))
            # Log skipping device due to core mismatch
            message(TRACE "FindCMSIS: skip device because CORE ${CORE} provided doesn't correspond to FAMILY ${FAMILY} DEVICE ${DEVICE}")
            # Skip to next device
            continue()
        endif()
                
        # Get chip type for this device
        stm32_get_chip_type(${FAMILY} ${DEVICE} TYPE)
        # Convert device name to lowercase
        string(TOLOWER ${DEVICE} DEVICE_L)
        # Convert type name to lowercase
        string(TOLOWER ${TYPE} TYPE_L)
        
        # Get list of enabled languages in project
        get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
        # Check if ASM language is enabled
        if(NOT "ASM" IN_LIST languages)
            # Log message about skipping due to missing ASM language
            message(STATUS "FindCMSIS: Not generating target for startup file and linker script because ASM language is not enabled")
            # Skip processing startup files
            continue()
        endif()
        
        # Search for startup assembly file
        find_file(CMSIS_${FAMILY}${CORE_U}_${TYPE}_STARTUP
            NAMES startup_stm32${TYPE_L}.s 
                  startup_stm32${TYPE_L}${CORE_Ucm}.s
            PATHS "${CMSIS_${FAMILY}${CORE_U}_PATH}/Source/Templates/gcc"
            NO_DEFAULT_PATH
        )
        # Add startup file to sources list
        list(APPEND CMSIS_SOURCES "${CMSIS_${FAMILY}${CORE_U}_${TYPE}_STARTUP}")
        # If startup file not found, mark as not found
        if(NOT CMSIS_${FAMILY}${CORE_U}_${TYPE}_STARTUP)
            # Set flag indicating device not found
            set(STM_DEVICES_FOUND FALSE)
            # Log verbose message about missing startup file
            message(VERBOSE "FindCMSIS: did not find file: startup_stm32${TYPE_L}.s or startup_stm32${TYPE_L}${CORE_Ucm}.s")
            # Exit loop
            break()
        endif()
        
        # Create CMSIS library target for chip type if it doesn't exist
        if(NOT (TARGET CMSIS::STM32::${TYPE}${CORE_C}))
            # Log creation of new library
            message(TRACE "FindCMSIS: creating library CMSIS::STM32::${TYPE}${CORE_C}")
            # Add interface library for CMSIS chip type
            add_library(CMSIS::STM32::${TYPE}${CORE_C} INTERFACE IMPORTED)
            # Link to family CMSIS target
            target_link_libraries(CMSIS::STM32::${TYPE}${CORE_C} INTERFACE CMSIS::STM32::${FAMILY}${CORE_C} STM32::${TYPE}${CORE_C})
            # Add startup file as source
            target_sources(CMSIS::STM32::${TYPE}${CORE_C} INTERFACE "${CMSIS_${FAMILY}${CORE_U}_${TYPE}_STARTUP}")
            # Add system file as source
            target_sources(CMSIS::STM32::${TYPE}${CORE_C} INTERFACE "${CMSIS_${FAMILY}${CORE_U}_SYSTEM}")
        endif()
        
        # Create CMSIS library target for specific device
        add_library(CMSIS::STM32::${DEVICE}${CORE_C} INTERFACE IMPORTED)
        # Link to chip type CMSIS target
        target_link_libraries(CMSIS::STM32::${DEVICE}${CORE_C} INTERFACE CMSIS::STM32::${TYPE}${CORE_C})
        # Generate default linker script for device
        cmsis_generate_default_linker_script(${FAMILY} ${DEVICE} "${CORE}")
    endforeach()

    # Set found status for this component based on device search result
    if(STM_DEVICES_FOUND)
       # Mark component as found
       set(CMSIS_${COMP}_FOUND TRUE)
       # Log debug message
       message(DEBUG "CMSIS_${COMP}_FOUND TRUE")
    else()
       # Mark component as not found
       set(CMSIS_${COMP}_FOUND FALSE)
       # Log debug message
       message(DEBUG "CMSIS_${COMP}_FOUND FALSE")
    endif()

    # Process each RTOS component for this family
    foreach(RTOS_COMP ${CMSIS_FIND_COMPONENTS_RTOS})
        # Check if RTOS_V2 is requested and set version accordingly
        if (${RTOS_COMP} STREQUAL "RTOS_V2")
            # Set RTOS version to 2
            set(RTOS_COMP_VERSION "2")
        else()
            # Unset version for other RTOS types
            unset(RTOS_COMP_VERSION)
        endif()

        # Search for RTOS header file
        find_path(CMSIS_${FAMILY}${CORE_U}_${RTOS_COMP}_PATH
            NAMES "cmsis_os${RTOS_COMP_VERSION}.h"
            PATHS "${STM32_CUBE_${FAMILY}_PATH}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_${RTOS_COMP}"
            NO_DEFAULT_PATH
        )
        # If RTOS header not found, skip this RTOS component
        if (NOT CMSIS_${FAMILY}${CORE_U}_${RTOS_COMP}_PATH)
            # Continue to next RTOS component
            continue()
        endif()

        # Search for RTOS source file
        find_file(CMSIS_${FAMILY}${CORE_U}_${RTOS_COMP}_SOURCE
            NAMES "cmsis_os${RTOS_COMP_VERSION}.c"
            PATHS "${STM32_CUBE_${FAMILY}_PATH}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_${RTOS_COMP}"
            NO_DEFAULT_PATH
        )
        # If RTOS source not found, skip this RTOS component
        if (NOT CMSIS_${FAMILY}${CORE_U}_${RTOS_COMP}_SOURCE)
            # Continue to next RTOS component
            continue()
        endif()

        # Create RTOS library target if it doesn't exist
        if(NOT (TARGET CMSIS::STM32::${FAMILY}${CORE_C}::${RTOS_COMP}))
            # Add interface library for RTOS
            add_library(CMSIS::STM32::${FAMILY}${CORE_C}::${RTOS_COMP} INTERFACE IMPORTED)
            # Link to family CMSIS target
            target_link_libraries(CMSIS::STM32::${FAMILY}${CORE_C}::${RTOS_COMP} INTERFACE CMSIS::STM32::${FAMILY}${CORE_C})
            # Add RTOS include directory
            target_include_directories(CMSIS::STM32::${FAMILY}${CORE_C}::${RTOS_COMP} INTERFACE "${CMSIS_${FAMILY}${CORE_U}_${RTOS_COMP}_PATH}")
            # Add RTOS source file
            target_sources(CMSIS::STM32::${FAMILY}${CORE_C}::${RTOS_COMP} INTERFACE "${CMSIS_${FAMILY}${CORE_U}_${RTOS_COMP}_SOURCE}")
        endif()

        # Add RTOS source to global sources list
        list(APPEND CMSIS_SOURCES "${CMSIS_${FAMILY}${CORE_U}_${RTOS_COMP}_SOURCE}")
        # Add RTOS include directory to global include list
        list(APPEND CMSIS_INCLUDE_DIRS "${CMSIS_${FAMILY}${CORE_U}_${RTOS_COMP}_PATH}")
        # Mark RTOS component as found
        set(CMSIS_${RTOS_COMP}_FOUND TRUE)
    endforeach()

    # Remove duplicate entries from include directories list
    list(REMOVE_DUPLICATES CMSIS_INCLUDE_DIRS)
    # Remove duplicate entries from sources list
    list(REMOVE_DUPLICATES CMSIS_SOURCES)
endforeach()

# Include CMake function for standard package handling
include(FindPackageHandleStandardArgs)
# Handle standard find_package arguments and set CMSIS_FOUND
find_package_handle_standard_args(CMSIS
    REQUIRED_VARS CMSIS_INCLUDE_DIRS CMSIS_SOURCES
    FOUND_VAR CMSIS_FOUND
    VERSION_VAR CMSIS_VERSION
    HANDLE_COMPONENTS
)
