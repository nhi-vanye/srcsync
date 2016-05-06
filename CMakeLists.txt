PROJECT( psync )
CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

INCLUDE(ExternalProject)
INCLUDE(FindOpenSSL)

INCLUDE_DIRECTORIES( 
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${psync_BINARY_DIR}/usr/local/include
    ${psync_BINARY_DIR}/usr/local/include/libssh
)

LINK_DIRECTORIES( 
    ${OPENSSL_LIB_DIR}
    ${psync_BINARY_DIR}/usr/local/lib
)

ExternalProject_Add(
    "poco"

    GIT_REPOSITORY "https://github.com/pocoproject/poco.git"
    GIT_TAG "poco-1.7.3-release"

#    CONFIGURE_COMMAND ./configure --no-samples --no-tests
    CMAKE_ARGS -Wno-dev 
    INSTALL_COMMAND make install DESTDIR=${PROJECT_BINARY_DIR}
)


ExternalProject_Add(
    "libssh2"

    GIT_REPOSITORY "https://github.com/libssh2/libssh2.git"
    GIT_TAG "libssh2-1.7.0"

    CMAKE_ARGS -Wno-dev 

    INSTALL_COMMAND make install DESTDIR=${PROJECT_BINARY_DIR}
)


ExternalProject_Add(
    "acrosync-library"

    GIT_REPOSITORY "https://github.com/gilbertchen/acrosync-library.git"

    UPDATE_COMMAND cp ${PROJECT_SOURCE_DIR}/depot/acrosync.cmake CMakeLists.txt

    CMAKE_ARGS -Wno-dev  -Dpsync_BINARY_DIR:STATIC=${PROJECT_BINARY_DIR}

    INSTALL_COMMAND make install DESTDIR=${PROJECT_BINARY_DIR}
)

ADD_EXECUTABLE( psync src/psync.cc )

TARGET_LINK_LIBRARIES( psync

    PocoNet
    PocoUtil
    PocoFoundation

    acrosync

    ssh2
)

ADD_DEPENDENCIES( psync

    poco
    libssh2
    acrosync-library
)
