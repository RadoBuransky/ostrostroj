if (NOT DEFINED CACHE{libs-${CMAKE_CXX_COMPILER_TARGET}})
  set(LOCAL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
  file(MAKE_DIRECTORY ${LOCAL_LIB_DIR})
  execute_process(COMMAND ssh ${REMOTE_HOSTNAME} "sudo apt install libasound2-dev libgpiod-dev" COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND scp ${REMOTE_HOSTNAME}:/usr/lib/aarch64-linux-gnu/libasound.so ${LOCAL_LIB_DIR} COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND scp ${REMOTE_HOSTNAME}:/usr/lib/aarch64-linux-gnu/libgpiod.so ${LOCAL_LIB_DIR} COMMAND_ERROR_IS_FATAL ANY)

  set(LOCAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
  file(MAKE_DIRECTORY ${LOCAL_INCLUDE_DIR})
  execute_process(COMMAND scp -r ${REMOTE_HOSTNAME}:/usr/include/alsa ${LOCAL_INCLUDE_DIR} COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND scp ${REMOTE_HOSTNAME}:/usr/include/gpiod.h ${LOCAL_INCLUDE_DIR} COMMAND_ERROR_IS_FATAL ANY)

  set("libs-${CMAKE_CXX_COMPILER_TARGET}" ON CACHE BOOL "")
endif() 