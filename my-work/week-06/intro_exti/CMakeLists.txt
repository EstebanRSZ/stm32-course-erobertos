# Set the minimum version of CMake that can be used
cmake_minimum_required(VERSION 3.20)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Set target definition variables required by the toolchain file
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target.cmake)

# Set the toolchain file for cross-compilation
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake)

# Project definition
project(intro_exti)

# Enable CMake support for ASM and C/CPP languages
enable_language(C ASM)

set(PROJECT_NAME intro_exti)

#Ruta al paquete de firmeware de ST para la familia STM32F4
set(CMSIS_ROOT "/home/esteban_rs/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.0")

# Create an executable object type
add_executable(${PROJECT_NAME})

# Include additional CMake files for flags, components, and files
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/flags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/components.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/files.cmake)

# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
  # User defined sources
  ${CMSIS_ROOT}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
  # User defined include directories
  ${CMSIS_ROOT}/Drivers/CMSIS/Core/Include # Estas son las rutas de los archivos de cabecera del CMSIS, para los procesadores Cortex M4
  ${CMSIS_ROOT}/Drivers/CMSIS/Device/ST/STM32F4xx/Include # Estas son las rutas de los archivos de cabecera del CMSIS y del dispositivo STM32F4xx

)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
  # User defined compiler options
)

# Add additional defined symbols
target_compile_definitions(${PROJECT_NAME} PRIVATE
  # Add additional defined symbols here
  STM32F411xE # usa STM32F446xx si trabajas con NUCLEO-F446RE
)

# Linker options
target_link_options(${PROJECT_NAME} PRIVATE 
  # User defined linker options
)

# Add additional linker search directories
target_link_directories(${PROJECT_NAME} PRIVATE
  # Add additional linker search directories here
)

# Add additional linker libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  # Add additional linker libraries here
)