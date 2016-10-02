
#include <sstream>

#include "Poco/Util/ServerApplication.h"

#include "Poco/PriorityNotificationQueue.h"
#include "Poco/Thread.h"
#include "Poco/Logger.h"
#include "Poco/StringTokenizer.h"
#include "Poco/Glob.h"
#include "Poco/Process.h"
#include "Poco/Pipe.h"
#include "Poco/PipeStream.h"
#include "Poco/StreamCopier.h"
#include "Poco/StringTokenizer.h"

#include "SourceSync.h"
#include "RsyncWorker.h"

#if USE_GROWL
#include "growl.hpp"
#endif

#include "config.h"

#if USE_GROWL
static const char *notifications[] = {
    "sync",
};
#endif

#define COUNT(a) sizeof(a)/sizeof(*a)

RsyncWorker::RsyncWorker ( const std::string &name, Poco::PriorityNotificationQueue *queue) : SyncWorker(name, queue), logger_(Poco::Logger::get("RsyncWorker")), readStdOut(this, &RsyncWorker::readOutPipe), readStdErr(this, &RsyncWorker::readErrPipe)
{ 
    FUNCTIONTRACE;

    logger_.debug( "Creating " + name_ );

    local_ = Poco::Path( Poco::Util::Application::instance().config().getString( CONFIG_SRC ) );

    local_.makeAbsolute();

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

void RsyncWorker::initialize()
{

    std::string auth = remote_->getUserInfo();

    Poco::StringTokenizer tok( auth, ":", Poco::StringTokenizer::TOK_TRIM );

    user_ = tok[ 0 ];

    private_key_ = Poco::Util::Application::instance().config().getString(CONFIG_RSYNC_SSH_KEYFILE, "");

    if ( tok.count() == 2  ) {

        password_ = tok[ 1 ];

    }

    if ( private_key_.empty() == false  ) {
        logger_.debug( Poco::format("%s: connecting as %s to %s using keyfile %s", name_, user_, remote_->getHost(), private_key_ ) );
    }
    else {
        logger_.debug( Poco::format("%s: connecting as %s to %s using password %s", name_, user_, remote_->getHost(), password_ ) );
    }

}

bool RsyncWorker::runRsync( const std::string & from, const std::string &to )
{

    bool success = false;

    Poco::Process::Args args;

    args.push_back( "--archive" ); // permissions, times etc..
    args.push_back( "--delete" ); 
    args.push_back( "--verbose" ); 

    if ( Poco::Util::Application::instance().config().getInt( CONFIG_VERBOSE) > Poco::Message::PRIO_INFORMATION ) {

        args.push_back( "--verbose" ); 
    }

    if ( remote_->getScheme() == "ssh" ) {

        std::string ssh("ssh");

        if ( private_key_.empty() == false ) {
            ssh += " -i " + private_key_;
        }

        if ( user_.empty() == false ) {
            ssh += " -l " + user_;
        }

        args.push_back( Poco::format("--rsh=%s", ssh )  );
    }

    args.push_back( from );
    args.push_back( to );

    std::string launchCmd( "rsync" );

    for ( Poco::Process::Args::iterator it = args.begin(); it != args.end(); it++ ) {

        launchCmd += " ";
        launchCmd += *it;
    }


    logger_.debug( Poco::format("%s: %s", name_, launchCmd ) );

    if ( Poco::Util::Application::instance().config().getString( CONFIG_DRYRUN).empty() == false ) {

        Poco::Pipe outPipe;
        Poco::Pipe errPipe;

        Poco::ProcessHandle handle = Poco::Process::launch( "rsync", args, 
                NULL, // no stdin needed
                &outPipe, 
                &errPipe);

        Poco::PipeInputStream outStream(outPipe);
        Poco::PipeInputStream errStream(errPipe);

        // these run in an active method so we don't block
        readOutPipe( outStream );
        readErrPipe( errStream );

        int ret = handle.wait();

        if ( ret == 0 ) {

            success = true;
        }
        else {

            success = false;
        }
    }

    return success;
}

bool RsyncWorker::readOutPipe( Poco::PipeInputStream & stream )
{

    std::stringstream ss;

    Poco::StreamCopier::copyStream( stream, ss);

    Poco::StringTokenizer tok( ss.str(), "\n");

    for ( std::vector<std::string>::const_iterator it = tok.begin(); it != tok.end(); it++ ) {

        logger_.information( Poco::format("%s: rsync: %s", name_, *it ) );
    }

    return true;

}

bool RsyncWorker::readErrPipe( Poco::PipeInputStream & stream )
{

    std::stringstream ss;

    Poco::StreamCopier::copyStream( stream, ss);

    Poco::StringTokenizer tok( ss.str(), "\n");

    for ( std::vector<std::string>::const_iterator it = tok.begin(); it != tok.end(); it++ ) {

        logger_.error( Poco::format("%s: rsync: %s", name_, *it ) );
    }

    return true;
}

void RsyncWorker::run()
{
    FUNCTIONTRACE;

    Poco::AutoPtr<Poco::Notification> n( queue_->waitDequeueNotification() );

    int zero = 0;

    while ( n ) {

        SyncMessage *req = dynamic_cast<SyncMessage *>( n.get() );

        if ( req ) {

            thisApp->jobStart();

            logger_.debug( Poco::format("%s: Received message '%s' for %s", name_, req->type(), req->path()) );

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
                        logger_.notice( Poco::format("%s: Ignoring %s", name_, localPath) );
                    }
                    else {
                        std::string remotePath = remote_->getHost() + ":" + remote_->getPath();

                        remotePath += msg->path();

                        localPath = local_.toString() + msg->path();

                        logger_.debug( Poco::format("%s: syncing file %s to %s", name_, localPath, remotePath ) );

                        bool st = runRsync ( localPath, remotePath );

                        if ( st ) {
                            logger_.notice( Poco::format("%s: Updated %s", name_, localPath) );

#if USE_GROWL
                            std::auto_ptr<Growl> growl( new Growl(GROWL_TCP,0,APPNAME,(const char **const)notifications,COUNT(notifications)));

                            if ( Poco::Util::Application::instance().config().getString( CONFIG_GROWL_ICON, "").empty() == false ) {

                                Poco::Path iconPath( Poco::Util::Application::instance().config().getString("application.dir") + Poco::Path::separator() );

                                iconPath.append( Poco::Util::Application::instance().config().getString( CONFIG_GROWL_ICON) );

                                growl->Notify("sync",
                                        msg->path().c_str(),
                                        Poco::format("Updated File\n  %s", msg->path()).c_str(), 
                                        "http://whitequeen.gitlab.io/srcsync",
                                        iconPath.toString().c_str() );
                            }
                            else {
                                growl->Notify("sync",
                                        msg->path().c_str(),
                                        Poco::format("Updated File\n  %s", msg->path()).c_str() );
                            }
#endif
                        }
                        else {
                            logger_.error( Poco::format("%s: Failed updating %s", name_, localPath) );
#if USE_GROWL
                            std::auto_ptr<Growl> growl( new Growl(GROWL_TCP,0,APPNAME,(const char **const)notifications,COUNT(notifications)));

                            if ( Poco::Util::Application::instance().config().getString( CONFIG_GROWL_ICON, "").empty() == false ) {

                                Poco::Path iconPath( Poco::Util::Application::instance().config().getString("application.dir") + Poco::Path::separator() );

                                iconPath.append( Poco::Util::Application::instance().config().getString( CONFIG_GROWL_ICON) );

                                growl->Notify("sync",
                                        msg->path().c_str(),
                                        Poco::format("Failed to update\n  %s", msg->path()).c_str(),
                                        "http://whitequeen.gitlab.io/srcsync",
                                        iconPath.toString().c_str() );
                            }
                            else {
                                growl->Notify("sync",
                                        msg->path().c_str(),
                                        Poco::format("Failed to update\n  %s", msg->path()).c_str());
                            }
#endif
                        }
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
                        logger_.notice( Poco::format("%s: Ignoring %s", name_, localPath) );
                    }
                    else {
                        std::string remotePath = remote_->getHost() + ":" + remote_->getPath();

                        remotePath += msg->path();

                        localPath = local_.toString() + msg->path();
                        
                        logger_.debug( Poco::format("%s: syncing dir %s to %s", name_, localPath, remotePath ) );

                        bool st = runRsync ( localPath, remotePath );

                        if ( st ) {
                            logger_.notice( Poco::format("%s: Updated %s", name_, localPath) );
#if USE_GROWL
                            if ( Poco::Util::Application::instance().config().getBool( CONFIG_GROWL_UPDATE_DIR , false ) ) {

                                std::auto_ptr<Growl> growl( new Growl(GROWL_TCP,0,APPNAME,(const char **const)notifications,COUNT(notifications)));

                                if ( Poco::Util::Application::instance().config().getString( CONFIG_GROWL_ICON, "").empty() == false ) {

                                    Poco::Path iconPath( Poco::Util::Application::instance().config().getString("application.dir") + Poco::Path::separator() );

                                    iconPath.append( Poco::Util::Application::instance().config().getString( CONFIG_GROWL_ICON) );

                                    growl->Notify("sync",
                                            msg->path().c_str(),
                                            Poco::format("Updated Directory\n  %s", msg->path()).c_str(), 
                                            "http://whitequeen.gitlab.io/srcsync",
                                            iconPath.toString().c_str() );
                                }
                                else {
                                    growl->Notify("sync",
                                            localPath.c_str(),
                                            Poco::format("Updated Directory\n  %s", localPath).c_str());
                                }
                            }
#endif
                        }
                        else {
                            logger_.error( Poco::format("%s: Failed updating %s", name_, localPath) );

#if USE_GROWL
                            std::auto_ptr<Growl> growl( new Growl(GROWL_TCP,0, APPNAME,(const char **const)notifications,COUNT(notifications)));

                            if ( Poco::Util::Application::instance().config().getBool( CONFIG_GROWL_UPDATE_DIR , false ) ) {

                                if ( Poco::Util::Application::instance().config().getString( CONFIG_GROWL_ICON, "").empty() == false ) {

                                    Poco::Path iconPath( Poco::Util::Application::instance().config().getString("application.dir") + Poco::Path::separator() );

                                    iconPath.append( Poco::Util::Application::instance().config().getString( CONFIG_GROWL_ICON) );

                                    growl->Notify("sync",
                                            msg->path().c_str(), // "Failed", 
                                            Poco::format("Failed to update directory\n  %s", msg->path()).c_str(),
                                            "http://whitequeen.gitlab.io/srcsync",
                                            iconPath.toString().c_str() );
                                }
                                else {
                                    growl->Notify("sync",
                                            localPath.c_str(), // "Failed", 
                                            Poco::format("Failed to update\n  %s", localPath).c_str());
                                }
                            }
#endif
                        }
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

