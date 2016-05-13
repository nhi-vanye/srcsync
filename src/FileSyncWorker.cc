
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

    const char * userC = NULL;
    const char * passC = NULL;
    const char * keyC = NULL;

    if ( Poco::Util::Application::instance().config().getString( CONFIG_RSYNC_SSH_METHOD, RSYNC_SSH_METHOD_USER ) == RSYNC_SSH_METHOD_USER ) {

        std::string auth = remote_->getUserInfo();

        Poco::StringTokenizer tok( auth, ":", Poco::StringTokenizer::TOK_TRIM );

        if ( tok.count() != 2 ) {
            throw Poco::Exception( "Missing or invalid credentials in " + Poco::Util::Application::instance().config().getString( CONFIG_DEST ) );
        }

        std::string user = tok[ 0 ];
        std::string pass = tok[ 1 ];

        userC = user.c_str();
        passC = pass.c_str();
    }
    else {

        std::string key = Poco::Util::Application::instance().config().getString(CONFIG_RSYNC_SSH_KEYFILE);

        keyC = key.c_str();
    }

    sshio_.connect(
            remote_->getHost().c_str(), 
            remote_->getPort(), 
            userC, 
            passC,
            keyC, 
            NULL);
}



void SyncWorker::run()
{
    FUNCTIONTRACE;

    Poco::AutoPtr<Poco::Notification> n( queue_->waitDequeueNotification() );

    logger_.information( Poco::format("%s: dequeued", name_ ) );

    int zero = 0;

    while ( n ) {

        SyncMessage *req = dynamic_cast<SyncMessage *>( n.get() );

        if ( req ) {

            thisApp->jobStart();

            if ( sshio_.isConnected() == false ) {

                initialize();
            }

            logger_.debug( Poco::format("%s: Received message '%s' for %s", name_, req->type(), req->path()) );

            // \TODO
            // filter out un-interesting file changes, i.e. editor backups etc.

            rsync::Client client(&sshio_, "rsync", 32, &zero);

            if ( req && req->type() == MSG_FILE_SYNC ) { 

                FileSyncMessage *msg = dynamic_cast<FileSyncMessage *>( n.get() );

                if ( msg ) {

                    std::string localPath = msg->path();
                    std::string remotePath = remote_->getPath();

                    if ( remotePath.back() != '/' ) {
                        remotePath += Poco::Path::separator();
                    }

                    remotePath += msg->path();

                    logger_.debug( Poco::format("%s: syncing file %s to %s", name_, localPath, remotePath ) );
                    std::set<std::string> files;

                    files.insert( localPath );

                    client.upload( localPath.c_str(), remotePath.c_str(), &files );

                    logger_.notice( Poco::format("Updated %s", localPath) );
                    logger_.debug( Poco::format("%s: updated %s", name_, localPath) );
                }

            }
            else if ( req->type() == MSG_DIR_SYNC ) { 

                DirSyncMessage *msg = dynamic_cast<DirSyncMessage *>( n.get() );

                if ( msg ) {

                    std::string localPath = msg->path();
                    std::string remotePath = remote_->getPath();

                    if ( remotePath.back() != '/' ) {
                        remotePath += Poco::Path::separator();
                    }

                    remotePath += msg->path();
                    logger_.debug( Poco::format("%s: syncing dir %s to %s", name_, localPath, remotePath ) );

                    client.upload( localPath.c_str(), remotePath.c_str() );

                    logger_.notice( Poco::format("Updated %s", localPath) );
                    logger_.debug( Poco::format("%s: updated %s", name_, localPath) );
                }

            }
            else {

                logger_.error("Unknown notification message type '" + req->type() + "'");

            }

            thisApp->jobEnd();

            logger_.information( Poco::format("%s: %d task(s) remain to processed", name_, thisApp->jobCount() ) );
        }


        n = queue_->waitDequeueNotification();
        logger_.information( Poco::format("%s: dequeued", name_ ) );
    }

}

