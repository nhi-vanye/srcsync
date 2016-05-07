/**
 * \file MonitorDirectory.cc
 *
 * \brief - Monitors a Directory and sends events
 *
 */

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

#include "SourceSync.h"


MonitorDirectory::MonitorDirectory( const std::string &path) : logger_(Poco::Logger::get("MonitorDirectory")) 
{

    Poco::BasicEvent<std::string> pathChanged;

    logger_.debug( "Monitoring " + path );

    watcher_ = new Poco::DirectoryWatcher( path );

    watcher_->itemAdded += Poco::delegate(this, &MonitorDirectory::onItemAdded);
    watcher_->itemRemoved += Poco::delegate(this, &MonitorDirectory::onItemRemoved);
    watcher_->itemModified += Poco::delegate(this, &MonitorDirectory::onItemModified);
    watcher_->itemMovedFrom += Poco::delegate(this, &MonitorDirectory::onItemMovedFrom);
    watcher_->itemMovedTo += Poco::delegate(this, &MonitorDirectory::onItemMovedTo);

}

void MonitorDirectory::onItemAdded(const Poco::DirectoryWatcher::DirectoryEvent& ev) 
{

    if ( ev.item.isDirectory() ) {

        logger_.debug("Directory Added " + ev.item.path() );

        MonitorDirectory *d = new MonitorDirectory( ev.item.path() ); 

    }
    else if ( ev.item.isFile() ) {

        logger_.debug("File Added " + ev.item.path() );

        thisApp->syncFile( ev.item.path() );
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


