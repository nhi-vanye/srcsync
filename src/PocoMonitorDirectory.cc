/**
 * \file MonitorDirectory.cc
 *
 * \brief - Monitors a Directory and sends events
 *
 */

#include <iostream>
#include <string>
#include <exception>
#include <csignal>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cerrno>
#include <vector>
#include <map>

#include "Poco/NestedDiagnosticContext.h"

#include "Poco/Logger.h"

#include "Poco/File.h"
#include "Poco/Path.h"

#ifdef USE_POCO_DIRECTORY_WATCHER
#include "Poco/DirectoryWatcher.h"
#endif // USE_POCO_DIRECTORY_WATCHER

#include "Poco/Delegate.h"
#include "Poco/Observer.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"

#ifdef USE_LIB_FSWATCH
#include "libfswatch/c++/event.hpp"
#include "libfswatch/c++/monitor.hpp"
#endif

#include "MonitorDirectory.h"

#include "Queue.h"

#include "SourceSync.h"

#ifdef USE_LIB_FSWATCH
static fsw::FSW_EVENT_CALLBACK handleFSWatchEvents;
#endif // USE_LIB_FSWATCH

MonitorDirectory::MonitorDirectory( const std::string &path) : logger_(Poco::Logger::get("MonitorDirectory")) 
{
    FUNCTIONTRACE;

    Poco::BasicEvent<std::string> pathChanged;

    logger_.debug( "Monitoring " + path );

    // you might think that Poco's DirectoryWatcher would be good here.
    // The problem is that on on OS X * it uses kqueue() and that is limited by 
    // open file descriptors (256).
    // libfswatch is GPL - but allows for using Apple File Events
    //
    // * the primary use case is for me to edit on my MacBook and compile on
    // docker so I need a good solution for OS X
    //
#ifdef USE_POCO_DIRECTORY_WATCHER
    watcher_ = new Poco::DirectoryWatcher( path );

    watcher_->itemAdded += Poco::delegate(this, &MonitorDirectory::onItemAdded);
    watcher_->itemRemoved += Poco::delegate(this, &MonitorDirectory::onItemRemoved);
    watcher_->itemModified += Poco::delegate(this, &MonitorDirectory::onItemModified);
    watcher_->itemMovedFrom += Poco::delegate(this, &MonitorDirectory::onItemMovedFrom);
    watcher_->itemMovedTo += Poco::delegate(this, &MonitorDirectory::onItemMovedTo);
#endif // USE_POCO_DIRECTORY_WATCHER

    std::vector<std::string> paths;

    paths.push_back( path );

#ifdef USE_LIB_FSWATCH
    fsw::monitor *fswmonitor = fsw::monitor_factory::create_monitor(fsw_monitor_type::system_default_monitor_type, // type - system specific default..
            paths,
            handleFSWatchEvents);

    // active_monitor->set_properties(monitor_properties);
    // active_monitor->set_allow_overflow(allow_overflow);
    fswmonitor->set_latency( 30.0 ); // 5minutes
    fswmonitor->set_fire_idle_event( true );
    fswmonitor->set_recursive( false ); // maybe better as true with a single MonitorDirectory - but that would break using Poco...

    // active_monitor->set_directory_only(dflag);
    // active_monitor->set_event_type_filters(event_filters);
    // active_monitor->set_filters(filters);
    // active_monitor->set_follow_symlinks(Lflag);
    // active_monitor->set_watch_access(aflag);

    fswmonitor->start();

#endif //  USE_LIB_FSWATCH


    // this triggers the initial synchronization of this directory
    // make sure we create parent directories first by prioritizing those with shorter paths
    // files will get prioritized by modified date.
    thisApp->queue()->enqueueNotification( new DirSyncMessage( path ), path.length() );
    logger_.information( Poco::format("%d messages in queue", thisApp->queue()->size() ) );
    logger_.debug( Poco::format("Added %s at priority %Lu", path, (Poco::UInt64) path.length() ) );
}

#ifdef USE_POCO_DIRECTORY_WATCHER
void MonitorDirectory::onItemAdded(const Poco::DirectoryWatcher::DirectoryEvent& ev) 
{
    FUNCTIONTRACE;

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Added " + ev.item.path() );

        MonitorDirectory *d = new MonitorDirectory( ev.item.path() ); 

    }
    else if ( ev.item.isFile() ) {

        logger_.debug("File Added " + ev.item.path() );

        Poco::File f( ev.item.path() );

        Poco::Timestamp t = f.getLastModified();

        // 1293840000 is midnight Jan 1 2011 GMT
        // 
        Poco::UInt64 tdiff = t.epochTime() - 1293840000;

        int priority = (int) tdiff;

        thisApp->queue()->enqueueNotification( new FileSyncMessage( ev.item.path() ), priority );
        logger_.debug( Poco::format("Added %s at priority %d", ev.item.path(), priority ) );
        logger_.information( Poco::format("%d messages in queue", thisApp->queue()->size() ) );
    }
}

void MonitorDirectory::onItemRemoved(const Poco::DirectoryWatcher::DirectoryEvent& ev) 
{

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Removed " + ev.item.path() );

    }
    else if ( ev.item.isFile() ) {

        logger_.debug("File Removed " + ev.item.path() );
    }
}

void MonitorDirectory::onItemModified(const Poco::DirectoryWatcher::DirectoryEvent& ev) {

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Changed " + ev.item.path() );

    }
    else if ( ev.item.isFile() ) {

        logger_.debug("File Changed " + ev.item.path() );

        Poco::File f( ev.item.path() );

        Poco::Timestamp t = f.getLastModified();

        // 1293840000 is midnight Jan 1 2011 GMT
        // 
        Poco::UInt64 tdiff = t.epochTime() - 1293840000;

        int priority = (int) tdiff;

        thisApp->queue()->enqueueNotification( new FileSyncMessage( ev.item.path() ), priority );
        logger_.debug( Poco::format("Added %s at priority %d", ev.item.path(), priority ) );
        logger_.information( Poco::format("%d messages in queue", thisApp->queue()->size() ) );
    }
}

void MonitorDirectory::onItemMovedFrom(const Poco::DirectoryWatcher::DirectoryEvent& ev) {

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Moved From " + ev.item.path() );

    }
    else if ( ev.item.isFile() ) {

        logger_.debug("File Moved From " + ev.item.path() );
    }
}

void MonitorDirectory::onItemMovedTo(const Poco::DirectoryWatcher::DirectoryEvent& ev) {

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Moved To " + ev.item.path() );

        MonitorDirectory *d = new MonitorDirectory( ev.item.path() ); 

    }
    else if ( ev.item.isFile() ) {

        logger_.debug("File Moved To " + ev.item.path() );
    }
}
#endif // USE_POCO_DIRECTORY_WATCHER


#ifdef USE_LIB_FSWATCH
static void handleFSWatchEvents(const std::vector<fsw::event>& events, void *context)
{
    for ( std::vector<fsw::event>::const_iterator it = events.begin(); it != events.end(); it++ ) {
        std::cout << " GOT FSWatch Event for " << (*it).get_path() << std::endl;
        
    }

    std::cout << "libfswatch " << std::endl;

}
#endif // USE_LIB_FSWATCH
