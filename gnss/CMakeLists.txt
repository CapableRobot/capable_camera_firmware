cmake_minimum_required(VERSION 3.6)

project(gnss-serial)

find_package(Boost REQUIRED COMPONENTS program_options)

#add_subdirectory(include)

add_executable(gnss-serial app.cpp gnss_serial.cpp serial_options.cpp)
#target_link_libraries(gnss-serial)

target_link_libraries(gnss-serial ${Boost_LIBRARIES})

set(EXECUTABLES gnss-serial)

install(TARGETS ${EXECUTABLES} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${SUPPORT_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
