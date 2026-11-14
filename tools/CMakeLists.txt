cmake_minimum_required(VERSION 3.20)

project(nch-lighting-pwm-report-tool LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ClockConfig.cmake")

if(CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "The PWM report must be built with a native C++ compiler.")
endif()

add_executable(pwm-report-bin pwm-report.cpp)
target_compile_features(pwm-report-bin PRIVATE cxx_std_17)
target_compile_definitions(pwm-report-bin PRIVATE HSE_VALUE=${HSE_VALUE})
target_include_directories(pwm-report-bin PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../Inc")

add_custom_target(pwm-report
    COMMAND "$<TARGET_FILE:pwm-report-bin>"
    DEPENDS pwm-report-bin
    COMMENT "Printing configured PWM parameters and brightness table"
    USES_TERMINAL
    VERBATIM
)
