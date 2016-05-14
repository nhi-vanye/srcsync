
#include "Poco/ThreadPool.h"
#include "Poco/PriorityNotificationQueue.h"

#include "Poco/NestedDiagnosticContext.h"

#include "Poco/Logger.h"


#include <rsync/rsync_log.h>

#include "SourceSync.h"

#include "Queue.h"

#include "config.h"

Queue::Queue() : logger_(Poco::Logger::get("QueueMangr")) 
{

    FUNCTIONTRACE;

    queue_ = new Poco::PriorityNotificationQueue;

    for ( int i = 0; i < Poco::Util::Application::instance().config().getInt( CONFIG_QUEUE_WORKER_COUNT, 8 ); i++ ) {

        workers_.push_back( new SyncWorker( Poco::format("worker-%d", i ) , queue_ ) );   
    }

    // create a Thread Pool.
    pool_ = new Poco::ThreadPool(
            Poco::Util::Application::instance().config().getInt( CONFIG_QUEUE_MIN_THREADS, 2),
            Poco::Util::Application::instance().config().getInt( CONFIG_QUEUE_MAX_THREADS, 64),
            Poco::Util::Application::instance().config().getInt( CONFIG_QUEUE_THREAD_IDLE, 120),
            0
            );

    for ( std::vector< SyncWorker *>::iterator it = workers_.begin() ; it != workers_.end() ; it++ ) {
        SyncWorker *w = *it;
        pool_->start( *w );
    }

    // rsync logging is limited to a single Log object, so we can't push
    // this down to the workers :-(
    rsync::Log::out.connect(this, &Queue::logCallback);


    int rsyncLogLevel = Poco::Util::Application::instance().config().getInt( CONFIG_RSYNC_LOG_LEVEL, 3);

    rsync::Log::setLevel( (rsync::Log::Level) rsyncLogLevel );
}

void Queue::logCallback(const char *id, int level, const char *message)
{

    // too many of these for some reason..
    if ( strcmp(id, "ICONV_FAILURE") == 0 ) {

        return;
    }

    if ( level == rsync::Log::Debug ) {

        logger_.debug(  std::string(id) + " " + message );
    }
    else if ( level == rsync::Log::Trace ) {

        logger_.trace(  std::string(id) + " " + message );
    }
    else if ( level == rsync::Log::Info ) {

        logger_.information(  std::string(id) + " " + message );
    }
    else if ( level == rsync::Log::Warning ) {

        logger_.warning(  std::string(id) + " " + message );
    }
    else if ( level == rsync::Log::Error ) {

        logger_.error(  std::string(id) + " " + message );
    }
    else {

        logger_.fatal(  std::string(id) + " " + message );

        throw Poco::Exception("Fatal Error during synchronization - does target directory exist ?");
    }

}

void Queue::outputCallback(const char * path, bool isDir, int64_t size, int64_t time, const char * symlink)
{
    FUNCTIONTRACE;

    logger_.debug( "OUT: " + Poco::format("%Ld %s %Ld", (Poco::Int64) time, std::string(path), (Poco::Int64) size) );
}

void Queue::statusCallback(const char * msg)
{
    FUNCTIONTRACE;

    logger_.notice( msg );
}

