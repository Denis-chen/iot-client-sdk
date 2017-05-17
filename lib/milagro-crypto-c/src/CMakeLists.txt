# Create AMCL library

# Add the binary tree directory to the search path for linking and include files
link_directories( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories (${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_BINARY_DIR}/include )

set(SOURCES_AMCL_CORE
hash.c
rand.c
randapi.c
aes.c
gcm.c
oct.c
utils.c)

set(SOURCES_AMCL_CURVE
big.c
fp.c
ecp.c
rom.c
ff.c
version.c)

message(STATUS "Build libamcl_core")
add_library(amcl_core ${LIB_TYPE} ${SOURCES_AMCL_CORE})
target_link_libraries (amcl_core) 

message(STATUS "Build libamcl_curve")
add_library(amcl_curve ${LIB_TYPE} ${SOURCES_AMCL_CURVE})
target_link_libraries (amcl_curve amcl_core) 

if(AMCL_CHOICE MATCHES "BN" OR AMCL_CHOICE MATCHES "BLS")
  message(STATUS "Build libamcl_pairing")
  set(SOURCES_AMCL_PAIRING
  fp2.c
  ecp2.c
  fp4.c
  fp12.c
  pair.c)
  add_library(amcl_pairing ${LIB_TYPE} ${SOURCES_AMCL_PAIRING})
  target_link_libraries (amcl_pairing amcl_curve amcl_core) 
endif(AMCL_CHOICE MATCHES "BN" OR AMCL_CHOICE MATCHES "BLS")

if(BUILD_MPIN)
  message(STATUS "Build libamcl_mpin")
  add_library(amcl_mpin ${LIB_TYPE} mpin.c)
  target_link_libraries (amcl_mpin amcl_pairing) 
endif(BUILD_MPIN)

if(BUILD_WCC)
  message(STATUS "Build libamcl_wcc")
  add_library(amcl_wcc ${LIB_TYPE} wcc.c)
  target_link_libraries (amcl_wcc amcl_pairing) 
endif(BUILD_WCC)

message(STATUS "Build libamcl_ecc")
add_library(amcl_ecc ${LIB_TYPE} ecdh.c)
target_link_libraries (amcl_ecc amcl_curve) 

message(STATUS "Build libamcl_rsa")
add_library(amcl_rsa ${LIB_TYPE} rsa.c)
target_link_libraries (amcl_rsa amcl_curve) 

if(BUILD_X509)
  message(STATUS "Build libamcl_x509")
  add_library(amcl_x509 ${LIB_TYPE} x509.c)
  target_link_libraries (amcl_x509 amcl_rsa amcl_ecc) 
endif(BUILD_X509)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message(STATUS "Copy libs to test for testing")
    add_custom_command(
                TARGET amcl_core
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_core.dll" "${PROJECT_BINARY_DIR}/test/")

    add_custom_command(
                TARGET amcl_curve
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_curve.dll" "${PROJECT_BINARY_DIR}/test/")

    add_custom_command(
                TARGET amcl_ecc
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_ecc.dll" "${PROJECT_BINARY_DIR}/test/")

    add_custom_command(
                TARGET amcl_rsa
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_rsa.dll" "${PROJECT_BINARY_DIR}/test/")

    if(BUILD_X509)
    add_custom_command(
                TARGET amcl_x509
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_x509.dll" "${PROJECT_BINARY_DIR}/test/")
    endif(BUILD_X509)

    if(BUILD_MPIN OR BUILD_WCC)
      add_custom_command(
                  TARGET amcl_pairing
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_pairing.dll" "${PROJECT_BINARY_DIR}/test/")
    endif(BUILD_MPIN OR BUILD_WCC)

    if(BUILD_MPIN)
      add_custom_command(
                  TARGET amcl_mpin
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_mpin.dll" "${PROJECT_BINARY_DIR}/test/")
    endif(BUILD_MPIN)

    if(BUILD_WCC)
      add_custom_command(
                  TARGET amcl_wcc
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_wcc.dll" "${PROJECT_BINARY_DIR}/test/")
    endif(BUILD_WCC)

endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# Copy built libs to wrappers/python to allow testing without
# having to install the libs.
if (BUILD_PYTHON AND BUILD_SHARED_LIBS)

  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    message(STATUS "Copy libs to wrappers/python for testing")
    add_custom_command(
                TARGET amcl_core
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_core.so" "${PROJECT_BINARY_DIR}/wrappers/python/")

    add_custom_command(
                TARGET amcl_curve
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_curve.so" "${PROJECT_BINARY_DIR}/wrappers/python/")

    if(BUILD_MPIN OR BUILD_WCC)
      add_custom_command(
                  TARGET amcl_pairing
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_pairing.so" "${PROJECT_BINARY_DIR}/wrappers/python/")
    endif(BUILD_MPIN OR BUILD_WCC)

    if(BUILD_MPIN)
      add_custom_command(
                  TARGET amcl_mpin
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_mpin.so" "${PROJECT_BINARY_DIR}/wrappers/python/")
    endif(BUILD_MPIN)

    if(BUILD_WCC)
      add_custom_command(
                  TARGET amcl_wcc
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_wcc.so" "${PROJECT_BINARY_DIR}/wrappers/python/")
    endif(BUILD_WCC)  

  endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message(STATUS "Copy libs to wrappers/python for testing")
    add_custom_command(
                TARGET amcl_core
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_core.dll" "${PROJECT_BINARY_DIR}/wrappers/python/")

    add_custom_command(
                TARGET amcl_curve
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_curve.dll" "${PROJECT_BINARY_DIR}/wrappers/python/")

    if(BUILD_MPIN OR BUILD_WCC)
      add_custom_command(
                  TARGET amcl_pairing
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_pairing.dll" "${PROJECT_BINARY_DIR}/wrappers/python/")
    endif(BUILD_MPIN OR BUILD_WCC)

    if(BUILD_MPIN)
      add_custom_command(
                  TARGET amcl_mpin
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_mpin.dll" "${PROJECT_BINARY_DIR}/wrappers/python/")
    endif(BUILD_MPIN)

    if(BUILD_WCC)
      add_custom_command(
                  TARGET amcl_wcc
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_wcc.dll" "${PROJECT_BINARY_DIR}/wrappers/python/")
    endif(BUILD_WCC)

  endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    message(STATUS "Copy libs to wrappers/python for testing")
    add_custom_command(
                TARGET amcl_core
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_core.dylib" "${PROJECT_BINARY_DIR}/wrappers/python/")

    add_custom_command(
                TARGET amcl_curve
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_curve.dylib" "${PROJECT_BINARY_DIR}/wrappers/python/")

    if(BUILD_MPIN OR BUILD_WCC)
      add_custom_command(
                  TARGET amcl_curve
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_curve.dylib" "${PROJECT_BINARY_DIR}/wrappers/python/")
    endif(BUILD_MPIN OR BUILD_WCC)

    if(BUILD_MPIN)
      add_custom_command(
                  TARGET amcl_mpin
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_mpin.dylib" "${PROJECT_BINARY_DIR}/wrappers/python/")
    endif(BUILD_MPIN)

    if(BUILD_WCC)
      add_custom_command(
                  TARGET amcl_wcc
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_wcc.dylib" "${PROJECT_BINARY_DIR}/wrappers/python/")
    endif(BUILD_WCC)  


  endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


endif (BUILD_PYTHON AND BUILD_SHARED_LIBS)


# Copy built libs to wrappers/go to allow testing without
# having to install the libs.
if (BUILD_GO AND BUILD_SHARED_LIBS)

  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    message(STATUS "Copy libs to wrappers/go for testing")
    add_custom_command(
                TARGET amcl_core
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_core.so" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_rsa
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_rsa.so" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_ecc
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_ecc.so" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_curve
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_curve.so" "${PROJECT_BINARY_DIR}/wrappers/go/")

    if(BUILD_MPIN OR BUILD_WCC)
      add_custom_command(
                  TARGET amcl_pairing
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_pairing.so" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_MPIN OR BUILD_WCC)

    if(BUILD_MPIN)
      add_custom_command(
                  TARGET amcl_mpin
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_mpin.so" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_MPIN)

    if(BUILD_WCC)
      add_custom_command(
                  TARGET amcl_wcc
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_wcc.so" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_WCC)  

  endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message(STATUS "Copy libs to wrappers/go for testing")
    add_custom_command(
                TARGET amcl_core
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_core.dll" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_rsa
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_rsa.dll" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_ecc
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_ecc.dll" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_curve
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_curve.dll" "${PROJECT_BINARY_DIR}/wrappers/go/")

    if(BUILD_MPIN OR BUILD_WCC)
      add_custom_command(
                  TARGET amcl_pairing
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_pairing.dll" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_MPIN OR BUILD_WCC)

    if(BUILD_MPIN)
      add_custom_command(
                  TARGET amcl_mpin
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_mpin.dll" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_MPIN)

    if(BUILD_WCC)
      add_custom_command(
                  TARGET amcl_wcc
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_wcc.dll" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_WCC)

  endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    message(STATUS "Copy libs to wrappers/go for testing")
    add_custom_command(
                TARGET amcl_core
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_core.dylib" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_rsa
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_rsa.dylib" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_ecc
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_ecc.dylib" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_curve
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_curve.dylib" "${PROJECT_BINARY_DIR}/wrappers/go/")

    if(BUILD_MPIN OR BUILD_WCC)
        add_custom_command(
                    TARGET amcl_pairing
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_pairing.dylib" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_MPIN OR BUILD_WCC)

    if(BUILD_MPIN)
      add_custom_command(
                  TARGET amcl_mpin
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_mpin.dylib" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_MPIN)

    if(BUILD_WCC)
      add_custom_command(
                  TARGET amcl_wcc
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_wcc.dylib" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_WCC)  


  endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


endif (BUILD_GO AND BUILD_SHARED_LIBS)

# Static build
if (BUILD_GO AND NOT BUILD_SHARED_LIBS)

  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    message(STATUS "Copy libs to wrappers/go for testing")
    add_custom_command(
                TARGET amcl_core
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_core.a" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_rsa
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_rsa.a" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_ecc
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_ecc.a" "${PROJECT_BINARY_DIR}/wrappers/go/")

    add_custom_command(
                TARGET amcl_curve
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_curve.a" "${PROJECT_BINARY_DIR}/wrappers/go/")

    if(BUILD_MPIN OR BUILD_WCC)
      add_custom_command(
                  TARGET amcl_pairing
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_pairing.a" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_MPIN OR BUILD_WCC)

    if(BUILD_MPIN)
      add_custom_command(
                  TARGET amcl_mpin
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_mpin.a" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_MPIN)

    if(BUILD_WCC)
      add_custom_command(
                  TARGET amcl_wcc
                  POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/libamcl_wcc.a" "${PROJECT_BINARY_DIR}/wrappers/go/")
    endif(BUILD_WCC)  

  endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

endif (BUILD_GO AND NOT BUILD_SHARED_LIBS)

# Install libs
install(DIRECTORY DESTINATION lib DIRECTORY_PERMISSIONS
        OWNER_WRITE OWNER_READ OWNER_EXECUTE
        GROUP_WRITE GROUP_READ
        WORLD_WRITE WORLD_READ WORLD_EXECUTE)

set(INSTALL_LIBS amcl_core amcl_curve amcl_rsa amcl_ecc)

if(BUILD_X509)
  set(INSTALL_LIBS ${INSTALL_LIBS} amcl_x509)
endif(BUILD_X509)

if(BUILD_MPIN)
  set(INSTALL_LIBS ${INSTALL_LIBS} amcl_mpin amcl_pairing )
endif(BUILD_MPIN)

if(BUILD_WCC)
  set(INSTALL_LIBS ${INSTALL_LIBS} amcl_wcc amcl_pairing )
endif(BUILD_WCC)

install (TARGETS ${INSTALL_LIBS} DESTINATION lib PERMISSIONS
        OWNER_WRITE OWNER_READ OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)

