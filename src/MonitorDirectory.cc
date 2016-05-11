/**
 * \file MonitorDirectory.cc
 *
 * \brief - Monitors a Directory and sends events
 *
 */

#include "Poco/NestedDiagnosticContext.h"

#include "Poco/Logger.h"

#include "Poco/File.h"
#include "Poco/Path.h"

#include "Poco/RecursiveDirectoryIterator.h"
#include "Poco/DirectoryWatcher.h"

#include "Poco/Delegate.h"
#include "Poco/Observer.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"

#include "MonitorDirectory.h"

#include "Queue.h"

#include "SourceSync.h"


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


