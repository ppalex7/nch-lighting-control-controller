#############################################################################################################################
# file:  CMakeLists.txt
# brief: Converted from STM32CubeIDE (.cproject) — nch-lighting-control-controller
#
# usage:
#   cmake --preset Debug     &&  cmake --build --preset Debug
#   cmake --preset Release   &&  cmake --build --preset Release
#############################################################################################################################
cmake_minimum_required(VERSION 3.20)

###################### CONSTANTS ######################################
set (PROJECT_TYPE_EXECUTABLE          "exe")
set (PROJECT_TYPE_STATIC_LIBRARY      "static-lib")
set (MCPU_CORTEX_M0				      "-mcpu=cortex-m0")
set (MCPU_CORTEX_M0PLUS				  "-mcpu=cortex-m0plus")
set (MCPU_CORTEX_M3				      "-mcpu=cortex-m3")
set (MCPU_CORTEX_M4				      "-mcpu=cortex-m4")
set (MCPU_CORTEX_M7				      "-mcpu=cortex-m7")
set (MFPU_FPV4_SP_D16                 "-mfpu=fpv4-sp-d16")
set (MFPU_FPV5_D16                    "-mfpu=fpv5-d16")
set (RUNTIME_LIBRARY_REDUCED_C        "--specs=nano.specs")
set (RUNTIME_LIBRARY_STD_C            "")
set (RUNTIME_LIBRARY_SYSCALLS_MINIMAL "--specs=nosys.specs")
set (RUNTIME_LIBRARY_SYSCALLS_NONE    "")
set (MFLOAT_ABI_SOFTWARE              "-mfloat-abi=soft")
set (MFLOAT_ABI_HARDWARE              "-mfloat-abi=hard")
set (MFLOAT_ABI_MIX                   "-mfloat-abi=softfp")
#######################################################################

###################### VARIABLES ######################################
set (PROJECT_NAME             "nch-lighting-control-controller")
set (PROJECT_TYPE             "exe")
set (LINKER_SCRIPT            "../STM32F030C6TX_FLASH.ld")
set (MCPU                     "-mcpu=cortex-m0")
set (MFPU                     "")                       # Cortex-M0: FPU нет
set (MFLOAT_ABI               "")                       # soft float
set (RUNTIME_LIBRARY          "--specs=nano.specs")
set (RUNTIME_LIBRARY_SYSCALLS "--specs=nosys.specs")

# Внешний репозиторий с CMSIS-драйверами (в .cproject был зашит абсолютный путь).
# Путь машинно-специфичный — задаётся через -DCMSIS_F0_PATH=... либо
# (рекомендуется) CMakeUserPresets.json, который не коммитится в репозиторий.
set (CMSIS_F0_PATH ""
     CACHE PATH "Path to STM32CubeF0 CMSIS drivers")

if (NOT CMSIS_F0_PATH)
	message(FATAL_ERROR
		"CMSIS_F0_PATH не задан. Укажи -DCMSIS_F0_PATH=<путь к STM32CubeF0/Drivers/CMSIS> "
		"или создай CMakeUserPresets.json (см. CLAUDE.md).")
endif ()

# --- Исходники -------------------------------------------------------
# В CubeIDE компилировались папки: Src, Startup, uart_logger
# (common и EncButton/src/core были только include-путями — header-only).
file (GLOB PROJECT_SOURCES CONFIGURE_DEPENDS
	"Src/*.c"
	"Src/*.cpp"
	"uart_logger/*.c"
	"uart_logger/*.cpp"
	)
file (GLOB STARTUP_SOURCES CONFIGURE_DEPENDS "Startup/*.s")
list (APPEND PROJECT_SOURCES ${STARTUP_SOURCES})

# Файлы, исключённые из сборки в CubeIDE (sourceEntries excluding="...").
list (FILTER PROJECT_SOURCES EXCLUDE REGEX "sysmem\\.c$")

set (PROJECT_DEFINES
	BUFFER_MESSAGES_COUNT=32
	HSE_VALUE=12000000U
	STM32
	STM32F0
	STM32F030C6Tx
	)

set (PROJECT_INCLUDES
	Inc
	uart_logger
	common
	EncButton/src/core
	${CMSIS_F0_PATH}/Device/ST/STM32F0xx/Include
	${CMSIS_F0_PATH}/Core/Include
	)
#######################################################################

################## PROJECT SETUP ######################################
project(${PROJECT_NAME} C CXX ASM)

if (${PROJECT_TYPE} MATCHES ${PROJECT_TYPE_EXECUTABLE})
  add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                     COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
                     COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin)
elseif (${PROJECT_TYPE} MATCHES ${PROJECT_TYPE_STATIC_LIBRARY})
  add_library(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

add_compile_definitions (${PROJECT_DEFINES})
add_compile_definitions ($<$<CONFIG:Debug>:DEBUG>)   # DEBUG только в Debug
include_directories (${PROJECT_INCLUDES})

set (CMAKE_EXECUTABLE_SUFFIX ".elf")
set (CMAKE_STATIC_LIBRARY_SUFFIX ".a")

# Общие флаги (то, что ST раскладывает по каждому tool в .cproject)
set (COMMON_FLAGS  "${MCPU} ${MFPU} ${MFLOAT_ABI} ${RUNTIME_LIBRARY} -mthumb -ffunction-sections -fdata-sections")
set (WARN_FLAGS    "-Wall -Wextra -Wmissing-include-dirs -Wswitch-enum -Wconversion")

set (CMAKE_C_FLAGS   "${COMMON_FLAGS} -std=gnu18 ${WARN_FLAGS}")
set (CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=gnu++17 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -Wextra -Wmissing-include-dirs -Wconversion")
set (CMAKE_ASM_FLAGS "${MCPU} ${MFPU} ${MFLOAT_ABI} -mthumb -x assembler-with-cpp")

# Per-config: Debug = -Og -g3, Release = -Os -g0 (как в .cproject).
# Задаём явно, чтобы перебить дефолты CMake (-O3 -DNDEBUG в Release и т.п.).
set (CMAKE_C_FLAGS_DEBUG     "-Og -g3")
set (CMAKE_C_FLAGS_RELEASE   "-Os -g0")
set (CMAKE_CXX_FLAGS_DEBUG   "-Og -g3")
set (CMAKE_CXX_FLAGS_RELEASE "-Os -g0")
set (CMAKE_ASM_FLAGS_DEBUG   "-g3")
set (CMAKE_ASM_FLAGS_RELEASE "-g0")

# NB: nano.specs (${RUNTIME_LIBRARY}) здесь НЕ указываем — CMake сам подставляет
# CMAKE_CXX_FLAGS в строку линковки, и nano.specs уже придёт оттуда. Если добавить
# его и сюда, на линковке будет два --specs=nano.specs → fatal error 'nano_link'.
set (CMAKE_EXE_LINKER_FLAGS
     "-T${LINKER_SCRIPT} ${RUNTIME_LIBRARY_SYSCALLS} \
-Wl,-Map=${PROJECT_NAME}.map -Wl,--gc-sections -static \
-Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group")
