PROJECT( srcsync )
CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

INCLUDE(ExternalProject)
INCLUDE(FindOpenSSL)


IF ( "$ENV{GITLAB_CI}" STREQUAL "" )

  SET( VER_MAJOR "0" )
  SET( VER_MINOR "0" )
  SET( VER_PATCH "0" )
  EXECUTE_PROCESS(COMMAND date +%Y%m%dT%H%M OUTPUT_VARIABLE BUILD_ID OUTPUT_STRIP_TRAILING_WHITESPACE)

ELSE ( "$ENV{GITLAB_CI}" STREQUAL "" )

  SET( VER_MAJOR $ENV{VERSION_MAJOR} )
  SET( VER_MINOR $ENV{VERSION_MINOR} )
  SET( VER_PATCH $ENV{VERSION_PATCH} )
  SET( BUILD_ID $ENV{CI_BUILD_ID} )

ENDIF ( "$ENV{GITLAB_CI}" STREQUAL "" )


INCLUDE_DIRECTORIES( 
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${srcsync_BINARY_DIR}/usr/local/include
    ${srcsync_BINARY_DIR}/usr/local/include/libssh
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    /usr/local/opt/gettext/include
)

LINK_DIRECTORIES( 
    ${srcsync_BINARY_DIR}/usr/local/lib
    /opt/local/lib/
    /usr/local/opt/gettext/lib
    ${OPENSSL_LIB_DIR}
)

ExternalProject_Add(
    "poco-git"

    GIT_REPOSITORY "https://github.com/pocoproject/poco.git"
    GIT_TAG "poco-1.7.3-release"

    # disable lots of features, we only need foundation, util
    CMAKE_ARGS  -Wno-dev 
                -DENABLE_CRYPTO=OFF 
                -DENABLE_XML=OFF 
                -DENABLE_JSON=OFF 
                -DENABLE_MONGODB=OFF  
                -DENABLE_PAGECOMPILER=OFF 
                -DENABLE_PAGECOMPILER_FILE2PAGE=OFF 
                -DENABLE_DATA=OFF 
                -DENABLE_ZIP=OFF
                -DENABLE_NETSSL=OFF

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

    PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/depot/acrosync-cmake.patch
        COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/depot/acrosync-publicKeyFile.patch

    CMAKE_ARGS -Wno-dev  -Dsrcsync_BINARY_DIR:STATIC=${PROJECT_BINARY_DIR}

    INSTALL_COMMAND make install DESTDIR=${PROJECT_BINARY_DIR}

    DEPENDS "libssh2-git"
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

SET_TARGET_PROPERTIES( srcsync 
    PROPERTIES INSTALL_RPATH "@loader_path/../lib"
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

INSTALL( 
    TARGETS srcsync

    DESTINATION bin
)

INSTALL( FILES

    ${srcsync_BINARY_DIR}/usr/local/lib/libPocoNet.43.dylib
    ${srcsync_BINARY_DIR}/usr/local/lib/libPocoUtil.43.dylib
    ${srcsync_BINARY_DIR}/usr/local/lib/libPocoFoundation.43.dylib
    ${srcsync_BINARY_DIR}/usr/local/lib/libfswatch.6.dylib 
    ${srcsync_BINARY_DIR}/usr/local/lib/libacrosync.dylib

    DESTINATION lib
)

SET( CPACK_PACKAGE_NAME srcsync)


SET(CPACK_GENERATOR "STGZ")

SET( CPACK_PACKAGE_VERSION_MAJOR ${VER_MAJOR} )
SET( CPACK_PACKAGE_VERSION_MINOR ${VER_MINOR} )
SET( CPACK_PACKAGE_VERSION_PATCH ${VER_PATCH} )
SET( CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}-${BUILD_ID}-${CMAKE_SYSTEM_NAME}" )

SET( CPACK_PACKAGE_DESCRIPTION_SUMMARY "SourceSync : a programe for sychronizing source to remote compile hosts (i.e. docker)")
SET( CPACK_PACKAGE_VENDOR "Richard Offer")
SET( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
SET( CPACK_PACKAGE_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}" )
SET( CPACK_PACKAGE_INSTALL_DIRECTORY "/usr/local/srcsync")
SET( CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)

SET( CPACK_BUNDLE_NAME srcsync )

SET(CPACK_PACKAGE_EXECUTABLES "srcsync" "srcsync")

INCLUDE(CPack)

