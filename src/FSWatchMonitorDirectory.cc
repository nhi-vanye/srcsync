/**
 * \file FSWatchMonitorDirectory.cc
 *
 * \brief - Monitors a Directory using libfswatch and sends notifications on changes.
 *
 */

#include <iostream>
//#include <string>
//#include <exception>
//#include <csignal>
//#include <cstdlib>
//#include <cmath>
//#include <ctime>
//#include <cerrno>
//#include <vector>
//#include <map>

#include <sstream>

#include "Poco/String.h"

#include "Poco/NestedDiagnosticContext.h"

#include "Poco/Logger.h"

#include "Poco/File.h"
#include "Poco/Path.h"

#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"

#include "libfswatch/c++/event.hpp"
#include "libfswatch/c++/monitor.hpp"

#include "FSWatchMonitorDirectory.h"

#include "Queue.h"

#include "SourceSync.h"

static void handleEventsBounce(const std::vector<fsw::event>& events, void *context);

FSWatchMonitorDirectory::FSWatchMonitorDirectory( const std::string &path) : MonitorDirectory(path, "FSWatchDir"), start(this, &FSWatchMonitorDirectory::runImpl) 
{
    FUNCTIONTRACE;

    Poco::BasicEvent<std::string> pathChanged;

    Poco::Path p(path);

    // special case for "." - otherwise we get CWD/.
    // and that breaks string substitution later.
    if ( path == "." ) {

        p = Poco::Path::current();
    }

    if ( p.isAbsolute() ) {

        base_ = p;
    }
    else {

        // uses CWD
        base_ = p.absolute();
    }

    logger_.debug( Poco::format("Monitoring %s (%s)",path, base_.toString() ) );

    std::vector<std::string> paths;

    paths.push_back( path );

    monitor_ = fsw::monitor_factory::create_monitor(fsw_monitor_type::system_default_monitor_type, // type - system specific default..
            paths,
            handleEventsBounce,
            this);

    // active_monitor->set_properties(monitor_properties);
    // active_monitor->set_allow_overflow(allow_overflow);
    monitor_->set_latency( 1.0 );
    // monitor_->set_fire_idle_event( true );
    monitor_->set_recursive( true );

    // active_monitor->set_directory_only(dflag);
    // active_monitor->set_event_type_filters(event_filters);
    // active_monitor->set_filters(filters);
    // active_monitor->set_follow_symlinks(Lflag);
    // active_monitor->set_watch_access(aflag);


    // this triggers the initial synchronization of this directory
    // make sure we create parent directories first by prioritizing those with shorter paths
    // files will get prioritized by modified date.
    logger_.debug( Poco::format("Adding %s at priority %Lu", path, (Poco::UInt64) path.length() ) );
    thisApp->queue()->enqueueNotification( new DirSyncMessage( path ), path.length() );
    logger_.information( Poco::format("%d task(s) in queue", thisApp->jobCount() ) );

    // start monitoring in a new thread
    start();
}

void FSWatchMonitorDirectory::handleEvents(const std::vector<fsw::event>& events)
{
    for ( std::vector<fsw::event>::const_iterator it = events.begin(); it != events.end(); it++ ) {

        const fsw::event &ev = *it;

        std::vector<fsw_event_flag> flags = ev.get_flags();

        for ( std::vector<fsw_event_flag>::iterator jt = flags.begin(); jt != flags.end(); jt++ ) {


            if ( *jt & IsFile ) {

                std::string pathRelativeToBase( ev.get_path() );

                Poco::replaceInPlace( pathRelativeToBase, base_.toString().c_str(), "" );

                logger_.information( Poco::format("Syncing (f) %s...", pathRelativeToBase ) );
                logger_.debug( Poco::format("Queuing file %s at priority %Lu", pathRelativeToBase, (Poco::UInt64) ev.get_time() ) );

                thisApp->queue()->enqueueNotification( 
                        new FileSyncMessage( pathRelativeToBase ), ev.get_time() );

            }
            else if ( *jt & IsDir ) {

                std::string pathRelativeToBase( ev.get_path() );

                Poco::replaceInPlace( pathRelativeToBase, base_.toString().c_str(), "" );

                logger_.information( Poco::format("Syncing (d) %s...", pathRelativeToBase ) );
                logger_.debug( Poco::format("Queuing directory %s at priority %Lu", pathRelativeToBase, (Poco::UInt64) ev.get_time() ) );

                thisApp->queue()->enqueueNotification( 
                        new DirSyncMessage( pathRelativeToBase ),  ev.get_time()  );

            }
            else {
                logger_.information( Poco::format("%d task(s) in queue", thisApp->jobCount() ) );
            }
        }
    }
}

static void handleEventsBounce(const std::vector<fsw::event>& events, void *context)
{
    FSWatchMonitorDirectory *obj = static_cast<FSWatchMonitorDirectory *> (context);

    obj->handleEvents( events );
}
