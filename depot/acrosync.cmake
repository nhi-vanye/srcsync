PROJECT(acrosync)

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

ADD_LIBRARY(acrosync SHARED
    rsync/rsync_client.cpp
    rsync/rsync_entry.cpp
    rsync/rsync_file.cpp
    rsync/rsync_io.cpp
    rsync/rsync_log.cpp
    rsync/rsync_pathutil.cpp
    rsync/rsync_socketio.cpp
    rsync/rsync_socketutil.cpp
    rsync/rsync_sshio.cpp
    rsync/rsync_stream.cpp
    rsync/rsync_timeutil.cpp
    rsync/rsync_util.cpp
)

TARGET_LINK_LIBRARIES( acrosync 
    ${OPENSSL_LIBRARIES}
    ssh2
    iconv
)

INSTALL(
    FILES
        rsync/rsync_client.h
        rsync/rsync_entry.h
        rsync/rsync_file.h
        rsync/rsync_io.h
        rsync/rsync_log.h
        rsync/rsync_pathutil.h
        rsync/rsync_socketio.h
        rsync/rsync_socketutil.h
        rsync/rsync_sshio.h
        rsync/rsync_stream.h
        rsync/rsync_timeutil.h
        rsync/rsync_util.h

    DESTINATION include
)

INSTALL(
    TARGETS
        acrosync

    LIBRARY DESTINATION lib
)
