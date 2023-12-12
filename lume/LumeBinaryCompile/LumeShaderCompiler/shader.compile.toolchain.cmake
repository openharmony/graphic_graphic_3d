cmake_minimum_required(VERSION 3.6.0)

# Inhibit all of CMake's own NDK handling code.
set(CMAKE_SYSTEM_VERSION 1) #系统版本

set(CMAKE_SYSTEM_NAME Linux)#系统版本

list(APPEND CMAKE_FIND_ROOT_PATH "${OHOS_NDK}")#指定当前目录为优先目录
#set(CMAKE_FIND_ROOT_PATH, "${OHOS_NDK}")
message("zwq123 ${OHOS_NDK}")
if(NOT CMAKE_FIND_ROOT_PATH_MODE_PROGRAM)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)#不在制定目录下查找（CMAKE_FIND_ROOT_PATH）查找文件
  #set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
endif()

if(NOT CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)#只在制定目录下查找文件
endif()

if(NOT CMAKE_FIND_ROOT_PATH_MODE_INCLUDE)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

if(NOT CMAKE_FIND_ROOT_PATH_MODE_PACKAGE)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

#zwq
set (CMAKE_LINKER "${OHOS_NDK}/bin/lld")
#end

set(CMAKE_C_COMPILER_ID_RUN TRUE)
set(CMAKE_CXX_COMPILER_ID_RUN TRUE)
set(CMAKE_C_COMPILER_ID Clang)
set(CMAKE_CXX_COMPILER_ID Clang)
set(CMAKE_C_COMPILER_VERSION 9.0)
set(CMAKE_CXX_COMPILER_VERSION 9.0)
set(CMAKE_C_STANDARD_COMPUTED_DEFAULT 11)
set(CMAKE_CXX_STANDARD_COMPUTED_DEFAULT 15)
set(CMAKE_C_COMPILER_FRONTEND_VARIANT "GNU")
set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT "GNU")

set(CMAKE_C_STANDARD_LIBRARIES_INIT "-latomic -lm")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "${CMAKE_C_STANDARD_LIBRARIES_INIT}")

#set(CMAKE_CXX_FLAGS           "-isystem ${OHOS_NDK}/../sysroot/usr/include/c++/v1 ${CMAKE_CXX_FLAGS}")#isystem查找头文件的路径
set(CMAKE_CXX_FLAGS           "-fuse-ld=lld -isystem ${OHOS_NDK}/include/c++/v1 ${CMAKE_CXX_FLAGS}")
#set(CMAKE_CXX_FLAGS           "-fuse-ld=lld -isystem  Y:/workspace/1113_sdk/out/generic_generic_arm_64only/general_all_tablet_standard/obj/third_party/musl/usr/include/aarch64-linux-ohos ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_COMPILER        "${OHOS_NDK}/bin/clang")
set(CMAKE_CXX_COMPILER      "${OHOS_NDK}/bin/clang++")
#set(STD_LINKER_FLAGS "-L ${OHOS_NDK}/../sysroot/usr/lib -Wl,-rpath,${OHOS_NDK}/../sysroot/usr/lib -lc++")#-L 库目录查找路径，-Wl告诉编译器将后面的参数传递给链接器，-lc++标准的c++库
set(STD_LINKER_FLAGS "-L ${OHOS_NDK}/lib/x86_64-unknown-linux-gnu -Wl,-rpath,${OHOS_NDK}/lib/x86_64-unknown-linux-gnu -lc++")
#set(STD_LINKER_FLAGS "-L Y:/workspace/1113_sdk/out/generic_generic_arm_64only/general_all_tablet_standard/obj/third_party/musl/usr/lib/aarch64-linux-ohos -Wl,-rpath,Y:/workspace/1113_sdk/out/generic_generic_arm_64only/general_all_tablet_standard/obj/third_party/musl/usr/lib/aarch64-linux-ohos -lc++")
set(CMAKE_SHARED_LINKER_FLAGS "${STD_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${STD_LINKER_FLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS    "${STD_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
