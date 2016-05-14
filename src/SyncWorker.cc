
#include "Poco/Util/ServerApplication.h"

#include "Poco/PriorityNotificationQueue.h"
#include "Poco/Thread.h"
#include "Poco/Logger.h"
#include "Poco/StringTokenizer.h"
#include "Poco/Glob.h"

#include <rsync/rsync_client.h>

#include <rsync/rsync_entry.h>
#include <rsync/rsync_file.h>
#include <rsync/rsync_log.h>
#include <rsync/rsync_pathutil.h>
#include <rsync/rsync_socketutil.h>
#include <rsync/rsync_sshio.h>

#include <libssh2.h>

#include <openssl/md5.h>

#include "SourceSync.h"

#include "config.h"



SyncWorker::SyncWorker ( const std::string &name, Poco::PriorityNotificationQueue *queue) : name_(name), queue_( queue ), logger_(Poco::Logger::get("SyncWorker")) 
{ 
    FUNCTIONTRACE;

    logger_.debug( "Creating " + name_ );

    remote_ = new Poco::URI( Poco::Util::Application::instance().config().getString( CONFIG_DEST ) );


    std::string ignore = Poco::Util::Application::instance().config().getString( CONFIG_IGNORE, "" );

    if ( ignore.empty() == false ) {

        Poco::StringTokenizer tok( ignore, " ", Poco::StringTokenizer::TOK_TRIM );

        for ( int i = 0; i < tok.count(); i++ ) {

            ignore_.push_back( new Poco::Glob( tok[i] ) );
        }

    }


    initialize();
}

void SyncWorker::initialize()
{

    const char * userC = NULL;
    const char * passC = NULL;
    const char * keyC = NULL;

    std::string auth = remote_->getUserInfo();

    Poco::StringTokenizer tok( auth, ":", Poco::StringTokenizer::TOK_TRIM );

    std::string user = tok[ 0 ];

    userC = user.c_str();

    std::string key = Poco::Util::Application::instance().config().getString(CONFIG_RSYNC_SSH_KEYFILE, "");

    std::string pass;

    if ( tok.count() == 2  ) {

        pass = tok[ 1 ];

        passC = pass.c_str();
    }
    else if ( key.empty() == false ) {

        keyC = key.c_str();

    }
    else {

        throw Poco::Exception( "Missing or invalid credentials for " + Poco::Util::Application::instance().config().getString( CONFIG_DEST ) );
    }

    if ( keyC ) {
        logger_.debug( Poco::format("%s: connecting as %s to %s using keyfile %s %s", name_, user, remote_->getHost(), key , pass) );
    }
    else {
        logger_.debug( Poco::format("%s: connecting as %s to %s using password %s", name_, user, remote_->getHost(), pass ) );
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

            int protocol = Poco::Util::Application::instance().config().getInt( CONFIG_RSYNC_PROTOCOL_VERSION, 32 );

            rsync::Client client(&sshio_, "rsync", protocol, &zero);

            if ( req && req->type() == MSG_FILE_SYNC ) { 

                FileSyncMessage *msg = dynamic_cast<FileSyncMessage *>( n.get() );

                if ( msg ) {

                    std::string localPath = msg->path();

                    bool ignoreThisFile = false;

                    if ( ignore_.size() > 0 ) {

                        for ( std::vector<Poco::Glob *>::iterator it = ignore_.begin(); it != ignore_.end() ; it++ ) {

                            if ( (*it)->match( localPath ) ) {

                                ignoreThisFile = true;

                                break;
                            }
                        }
                    }

                    if ( ignoreThisFile ) {
                        logger_.information( Poco::format("Ignoring %s", localPath) );
                    }
                    else {
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

            }
            else if ( req->type() == MSG_DIR_SYNC ) { 

                DirSyncMessage *msg = dynamic_cast<DirSyncMessage *>( n.get() );

                if ( msg ) {

                    std::string localPath = msg->path();

                    bool ignoreThisFile = false;

                    if ( ignore_.size() > 0 ) {

                        for ( std::vector<Poco::Glob *>::iterator it = ignore_.begin(); it != ignore_.end() ; it++ ) {

                            if ( (*it)->match( localPath ) ) {

                                ignoreThisFile = true;

                                break;
                            }
                        }
                    }

                    if ( ignoreThisFile ) {
                        logger_.information( Poco::format("Ignoring %s", localPath) );
                    }
                    else {
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

            }
            else {

                logger_.error("Unknown notification message type '" + req->type() + "'");

            }

            thisApp->jobEnd();

            logger_.information( Poco::format("%s: %d task(s) remain to processed", name_, thisApp->jobCount() ) );
        }


        n = queue_->waitDequeueNotification();
    }

}

