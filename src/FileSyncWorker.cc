
#include "Poco/Util/ServerApplication.h"

#include "Poco/PriorityNotificationQueue.h"
#include "Poco/Thread.h"
#include "Poco/Logger.h"
#include "Poco/StringTokenizer.h"

#include <rsync_client.h>

#include <rsync_entry.h>
#include <rsync_file.h>
#include <rsync_log.h>
#include <rsync_pathutil.h>
#include <rsync_socketutil.h>
#include <rsync_sshio.h>

#include <libssh2.h>

#include <openssl/md5.h>

#include "SourceSync.h"

#include "config.h"



SyncWorker::SyncWorker ( const std::string &name, Poco::PriorityNotificationQueue *queue) : name_(name), queue_( queue ), logger_(Poco::Logger::get("SyncWorker")) 
{ 
    FUNCTIONTRACE;

    logger_.debug( "Creating " + name_ );

    remote_ = new Poco::URI( Poco::Util::Application::instance().config().getString( CONFIG_DEST ) );


    initialize();


}

void SyncWorker::initialize()
{

    std::string auth = remote_->getUserInfo();

    Poco::StringTokenizer tok( auth, ":", Poco::StringTokenizer::TOK_TRIM );

    if ( tok.count() != 2 ) {
        throw Poco::Exception( "Missing or invalid credentials in " + Poco::Util::Application::instance().config().getString( CONFIG_DEST ) );
    }

    std::string user = tok[ 0 ];
    std::string pass = tok[ 1 ];
    

    sshio_.connect(
            remote_->getHost().c_str(), 
            remote_->getPort(), 
            user.c_str(), 
            pass.c_str(),
            NULL, 
            NULL);



    // list the remote directory (not recursive)
    // client_->list( remotePath.c_str() );

}



void SyncWorker::run()
{
    FUNCTIONTRACE;

    Poco::Notification::Ptr n( queue_->waitDequeueNotification() );

    int zero = 0;

    //client.entryOut.connect(thisApp->queue(), &Queue::outputCallback);
    //client.statusOut.connect(thisApp->queue(), &Queue::statusCallback);

    while ( n ) {

        Poco::AutoPtr<SyncMessage> req = dynamic_cast<SyncMessage *>( n.get() );

        if ( sshio_.isConnected() == false ) {

            initialize();
        }
/*
        rsync::Client client(&sshio_, "rsync", 32, &zero);

        if ( req->type() == MSG_FILE_SYNC ) { 

            Poco::AutoPtr<FileSyncMessage> msg = dynamic_cast<FileSyncMessage *>( n.get() );

            std::string localPath = msg->path();
            std::string remotePath = remote_->getPath() + Poco::Path::separator() + msg->path();

            logger_.debug( Poco::format("%s: syncing file %s to %s", name_, localPath, remotePath ) );
            std::set<std::string> files;

            files.insert( localPath );

            client.upload( localPath.c_str(), remotePath.c_str(), &files );

            logger_.debug( Poco::format("%s: updated %s", name_, localPath) );

        }
        else if ( req->type() == MSG_DIR_SYNC ) { 

            Poco::AutoPtr<DirSyncMessage> msg = dynamic_cast<DirSyncMessage *>( n.get() );

            std::string localPath = msg->path();
            std::string remotePath = remote_->getPath() + Poco::Path::separator() + msg->path();

            logger_.debug( Poco::format("%s: syncing dir %s to %s", name_, localPath, remotePath ) );

            client.upload( localPath.c_str(), remotePath.c_str() );

            logger_.debug( Poco::format("%s: updated %s", name_, localPath) );

        }
        else {

            logger_.error("Unknown notification message type '" + req->type() + "'");

        }
        client.stop();
*/

        Poco::Thread::sleep(100);

        logger_.information( Poco::format("%d messages remain to processed", thisApp->queue()->size() ) );

        n = queue_->waitDequeueNotification();
    }

}

