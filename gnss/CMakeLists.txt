cmake_minimum_required(VERSION 3.6)

project(gnss-logger)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(nlohmann_json REQUIRED)

set(CMAKE_CXX_STANDARD 17)

file(GLOB PROJECT_SRC ${CMAKE_SOURCE_DIR}/src/*.cpp)
include_directories(${CMAKE_SOURCE_DIR}/inc)

add_executable(${PROJECT_NAME}
    ${PROJECT_SRC}
)

target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${nlohmann_json_LIBRARIES}
    gps
    pthread
)

set(EXECUTABLES ${PROJECT_NAME})

install(TARGETS ${EXECUTABLES} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${SUPPORT_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
