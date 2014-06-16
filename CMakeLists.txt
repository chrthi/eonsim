cmake_minimum_required(VERSION 2.8)

project(eonsim)

find_package(Boost 1.49.0 REQUIRED COMPONENTS program_options)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

AUX_SOURCE_DIRECTORY(. MAINSRC)
AUX_SOURCE_DIRECTORY(./provisioning_schemes PSSRC)
add_executable(eonsim ${MAINSRC} ${PSSRC})

TARGET_LINK_LIBRARIES(eonsim ${Boost_LIBRARIES})

if(CMAKE_COMPILER_IS_GNUCXX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -pthread -march=native -pipe -fmessage-length=0")
endif()
