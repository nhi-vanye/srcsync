/**
 * \file PocoMonitorDirectory.cc
 *
 * \brief - Monitors a Directory and sends events using Poco's DirectoryWatcher
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

#include "PocoMonitorDirectory.h"

#include "Queue.h"

#include "SourceSync.h"

PocoMonitorDirectory::PocoMonitorDirectory( const std::string &path) : MonitorDirectory(path, "PocoMonDir") 
{
    FUNCTIONTRACE;

    Poco::BasicEvent<std::string> pathChanged;

    logger_.debug( "Monitoring " + path );

    watcher_ = new Poco::DirectoryWatcher( path );

    watcher_->itemAdded += Poco::delegate(this, &PocoMonitorDirectory::onItemAdded);
    watcher_->itemRemoved += Poco::delegate(this, &PocoMonitorDirectory::onItemRemoved);
    watcher_->itemModified += Poco::delegate(this, &PocoMonitorDirectory::onItemModified);
    watcher_->itemMovedFrom += Poco::delegate(this, &PocoMonitorDirectory::onItemMovedFrom);
    watcher_->itemMovedTo += Poco::delegate(this, &PocoMonitorDirectory::onItemMovedTo);

    std::vector<std::string> paths;

    paths.push_back( path );

    // this triggers the initial synchronization of this directory
    // make sure we create parent directories first by prioritizing those with shorter paths
    // files will get prioritized by modified date.
    thisApp->queue()->enqueueNotification( new DirSyncMessage( path ), path.length() );
    logger_.information( Poco::format("%d messages in queue", thisApp->queue()->size() ) );
    logger_.debug( Poco::format("Added %s at priority %Lu", path, (Poco::UInt64) path.length() ) );
}

void PocoMonitorDirectory::onItemAdded(const Poco::DirectoryWatcher::DirectoryEvent& ev) 
{
    FUNCTIONTRACE;

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Added " + ev.item.path() );

        PocoMonitorDirectory *d = new PocoMonitorDirectory( ev.item.path() ); 

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

void PocoMonitorDirectory::onItemRemoved(const Poco::DirectoryWatcher::DirectoryEvent& ev) 
{

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Removed " + ev.item.path() );

    }
    else if ( ev.item.isFile() ) {

        logger_.debug("File Removed " + ev.item.path() );
    }
}

void PocoMonitorDirectory::onItemModified(const Poco::DirectoryWatcher::DirectoryEvent& ev) {

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

void PocoMonitorDirectory::onItemMovedFrom(const Poco::DirectoryWatcher::DirectoryEvent& ev) {

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Moved From " + ev.item.path() );

    }
    else if ( ev.item.isFile() ) {

        logger_.debug("File Moved From " + ev.item.path() );
    }
}

void PocoMonitorDirectory::onItemMovedTo(const Poco::DirectoryWatcher::DirectoryEvent& ev) {

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Moved To " + ev.item.path() );

        PocoMonitorDirectory *d = new PocoMonitorDirectory( ev.item.path() ); 

    }
    else if ( ev.item.isFile() ) {

        logger_.debug("File Moved To " + ev.item.path() );
    }
}
