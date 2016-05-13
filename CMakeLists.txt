PROJECT( psync )
CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

INCLUDE(ExternalProject)
INCLUDE(FindOpenSSL)

INCLUDE_DIRECTORIES( 
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${psync_BINARY_DIR}/usr/local/include
    ${psync_BINARY_DIR}/usr/local/include/libssh
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    /usr/local/opt/gettext/include
)

LINK_DIRECTORIES( 
    /opt/local/lib/
    /usr/local/opt/gettext/lib
    ${OPENSSL_LIB_DIR}
    ${psync_BINARY_DIR}/usr/local/lib
)

ExternalProject_Add(
    "poco-git"

    GIT_REPOSITORY "https://github.com/pocoproject/poco.git"
    GIT_TAG "poco-1.7.3-release"

#    CONFIGURE_COMMAND ./configure --no-samples --no-tests
    CMAKE_ARGS -Wno-dev 
    INSTALL_COMMAND make install DESTDIR=${PROJECT_BINARY_DIR}
)

ExternalProject_Add(
    "fswatch-src"

# can't use git because of issues on MacOS getting autotools to build config.h required by CMake
    URL "https://github.com/emcrisostomo/fswatch/releases/download/1.9.2/fswatch-1.9.2.tar.gz"

    CONFIGURE_COMMAND ../fswatch-src/configure 

    INSTALL_COMMAND make install DESTDIR=${PROJECT_BINARY_DIR}
)


ExternalProject_Add(
    "libssh2-git"

    GIT_REPOSITORY "https://github.com/libssh2/libssh2.git"
    GIT_TAG "libssh2-1.7.0"

    CMAKE_ARGS -Wno-dev 

    INSTALL_COMMAND make install DESTDIR=${PROJECT_BINARY_DIR}
)


ExternalProject_Add(
    "acrosync-git"

    GIT_REPOSITORY "https://github.com/gilbertchen/acrosync-library.git"

    UPDATE_COMMAND cp ${PROJECT_SOURCE_DIR}/depot/acrosync.cmake CMakeLists.txt

    CMAKE_ARGS -Wno-dev  -Dpsync_BINARY_DIR:STATIC=${PROJECT_BINARY_DIR}

    INSTALL_COMMAND make install DESTDIR=${PROJECT_BINARY_DIR}
)

ADD_EXECUTABLE( srcsync 

    src/SourceSync.cc 
    src/FSWatchMonitorDirectory.cc 
    src/Queue.cc 
    src/SyncWorker.cc 
)

# use libfswatch (GPL) or Poco::DirectoryWatcher ?
# Poco has scalability issues on Mac OS.
SET_TARGET_PROPERTIES( srcsync 
    PROPERTIES COMPILE_DEFINITIONS "USE_LIB_FSWATCH;HAVE_CXX_ATOMIC;HAVE_CXX_MUTEX;HAVE_CORESERVICES_CORESERVICES_H;HAVE_CSTDLIB;HAVE_CXX11;HAVE_FSEVENTS_FILE_EVENTS" 
)
SET_TARGET_PROPERTIES( srcsync 
    PROPERTIES COMPILE_FLAGS "-std=gnu++11"
)


TARGET_LINK_LIBRARIES( srcsync

    PocoNet
    PocoUtil
    PocoFoundation

    fswatch

    acrosync

    ssh2

    crypto iconv z
)

ADD_DEPENDENCIES( srcsync

    poco-git
    libssh2-git
    fswatch-src
    acrosync-git
)
