/*
 * Centralize all configuration keys
 *
 *   * makes documentation easier
 *   * removes typo's in key names
 */

#define LOCAL_LOGGER_LINE logger_.trace( Poco::format("%s:%d", std::string(__FILE__),__LINE__))
#define APP_LOGGER_LINE logger().trace( Poco::format("%s:%d", std::string(__FILE__),__LINE__))
#define COUT_LINE std::cerr<< __FILE__ << ":" << __LINE__ << std::endl;

#define APPNAME "srcsync"

// main program
//
#define CONFIG_HELP     APPNAME ".help"     // --help
#define CONFIG_SRC      APPNAME ".src"      // --src
#define CONFIG_DEST     APPNAME ".dest"     // --dest
#define CONFIG_VERBOSE  APPNAME ".verbose"  // --verbose
#define CONFIG_PROFILE  APPNAME ".profile"  // --profile
#define CONFIG_IGNORE   APPNAME ".ignore"   // --ignore

// Queue management
//
#define CONFIG_QUEUE_WORKER_COUNT   APPNAME ".queue.worker.count"
#define CONFIG_QUEUE_MIN_THREADS    APPNAME ".queue.thread-min"
#define CONFIG_QUEUE_MAX_THREADS    APPNAME ".queue.thread-max"
#define CONFIG_QUEUE_THREAD_IDLE    APPNAME ".queue.thread-idle"

// rsync library
#define CONFIG_RSYNC_LOG_LEVEL          APPNAME ".rsync.log.level"
#define CONFIG_RSYNC_SSH_KEYFILE        APPNAME ".rsync.ssh.keyfile"
#define CONFIG_RSYNC_PROTOCOL_VERSION   APPNAME ".rsync.protocol.version"


// libfswatch

