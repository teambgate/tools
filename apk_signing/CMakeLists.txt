#
# Copyright (C) 2017 Manh Tran
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
cmake_minimum_required(VERSION 2.8.9)
project (apk_signing C)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(WINDOWS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(MACOSX TRUE)
elseif (CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
        add_definitions(-Dlinux)
        set(LINUX TRUE)
endif()

if(${APP_DEBUG} MATCHES "true")
        add_definitions(-DAPP_DEBUG=1)
else()
        add_definitions(-DAPP_DEBUG=0)
endif()

add_subdirectory(../../cherry/kernel ../${OUT}/kernel)
add_subdirectory(../../cherry/xml ../${OUT}/xml)
add_subdirectory(../../cherry/smartfox ../${OUT}/smartfox)

include_directories(../../cherry/include)
include_directories(../../cherry/smartfox/include)
include_directories(include)

if(MACOSX)
        file(GLOB_RECURSE SOURCES "src/*.c" "src/*.m")
else()
        file(GLOB_RECURSE SOURCES "src/*.c")
endif()

set(PROJECT_LINK_LIBS kernel xml smartfox)

add_executable(apk_signing ${SOURCES})
add_dependencies(apk_signing ${PROJECT_LINK_LIBS})
target_link_libraries(apk_signing ${PROJECT_LINK_LIBS})
