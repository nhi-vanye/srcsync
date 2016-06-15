/**
 * \file SourceSync.cc
 *
 * \brief - synchronize (source) files as they change to a remote system using rsync protocol
 *
 * \details
 * Given a directory to monitor as files are changed (edited) 
 * they will be copied to the remote system (typically over a SSH session)
 *
 * * Use case is to edit files locally but compile remotely
 * * Very specific use case is when the remote system is a docker host (i.e. has no NFS/CIFS etc)
 *
 * From an idea by docker-osx-dev - a shell script. Worked but had
 * the restriction that the local and remote directory hierarchies
 * had to be the same...
 *
 * Original Author: Richard Offer <richard@whitequeen.com>
 *
 * License: NOT YET AVAILABLE
 *
 * Requires acrosync library which requires changes to the library
 * to be made available - even if the library is only "distributed"
 * internally.
 *
 * Luckily we don't make any changes - we build straight from the git repo.
 *
 */
#include "stdio.h"
#include <string>
#include <iostream>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "Poco/NestedDiagnosticContext.h"

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/HelpFormatter.h"

#include "Poco/NumberParser.h"
#include "Poco/SharedPtr.h"

#include "Poco/Logger.h"
#include "Poco/Util/LoggingConfigurator.h"
#include "Poco/Util/PropertyFileConfiguration.h"

#include "Poco/File.h"
#include "Poco/Path.h"
#include "Poco/URI.h"

#include "Poco/RecursiveDirectoryIterator.h"
#include "Poco/DirectoryWatcher.h"

#include "Poco/Delegate.h"
#include "Poco/Observer.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"

#include <rsync/rsync_socketutil.h>
#include <libssh2.h>

#include "config.h"

#include "Queue.h"

#ifdef USE_LIB_FSWATCH
#include "FSWatchMonitorDirectory.h"
#endif
#ifdef USE_POCO_DIRECTORY_WATCHER
#include "PocoMonitorDirectory.h"
#endif

#include "SourceSync.h"

SourceSync *thisApp = NULL;

SourceSync::SourceSync() 
{
    FUNCTIONTRACE;

    thisApp = this;

    setUnixOptions( true );
    setLogger( Poco::Logger::get("SourceSync") );

    logger().setLevel("", Poco::Message::PRIO_NOTICE );

};

void SourceSync::initialize( Poco::Util::Application &self ) 
{

    FUNCTIONTRACE;

    Poco::Path appPath( config().getString("application.dir") + Poco::Path::separator() + config().getString("application.baseName") );

    std::string logConfigPath = appPath.toString() + std::string(".logger");

    Poco::File logConfigFile(logConfigPath);

    if ( logConfigFile.exists() ) {

        Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> logProperties = new Poco::Util::PropertyFileConfiguration(logConfigPath);

        Poco::Util::LoggingConfigurator configurator;
        configurator.configure(logProperties);
    }
    else {

        logger().information( "Log configuration file '" + logConfigPath + "' not found, using defaults");

        config().setString("logging.loggers.root.channel", "c1");
        config().setString("logging.formatters.f1.class", "PatternFormatter");
        config().setString("logging.formatters.f1.pattern", "%H:%M:%S [%q] %t");
        config().setString("logging.channels.c1.class", "ConsoleChannel");
        config().setString("logging.channels.c1.formatter", "f1");

    }


    // call each subsystem's initialize()
    // since Logging is a subsystem this is where we start to get fancy logging.
    // While we'd like to have nicer logging before this we need to make
    // sure all the configuration it set up before calling subsystems.
    try {
        Poco::Util::Application::initialize(self);
    } 
    catch ( Poco::FileNotFoundException &ex ) {

        logger().error( ex.displayText() );
    }
    catch ( Poco::Exception &ex ) {

        logger().error( ex.displayText() );

    }
}; 

void SourceSync::displayHelp() 
{
    FUNCTIONTRACE;

    Poco::Util::HelpFormatter helpFormatter( options() );

    helpFormatter.setUnixStyle( true );
    helpFormatter.setAutoIndent();
    helpFormatter.setCommand( "psync" );
    helpFormatter.setUsage( "OPTIONS ARGUMENTS" );
    helpFormatter.format( std::cout );

};        

void SourceSync::defineOptions( Poco::Util::OptionSet &options ) 
{
    FUNCTIONTRACE;

    Poco::Util::Application::defineOptions( options );

    options.addOption(
            Poco::Util::Option( "help", "h", "display help information on command line arguments" )
            .required( false )
            .repeatable( false )
            .binding( CONFIG_HELP ) );

    options.addOption(
            Poco::Util::Option("verbose", "v", "set verbosity of messages")
            .required(false)
            .repeatable(false)
            .argument("LEVEL", true)
            .callback(Poco::Util::OptionCallback<SourceSync>(this, &SourceSync::handleVerbose)));

    options.addOption(
            Poco::Util::Option("rsync-log-level", "", "set verbosity of rsync messages")
            .required(false)
            .repeatable(false)
            .argument("LEVEL", true)
            .binding( CONFIG_RSYNC_LOG_LEVEL ) );


    // a profile is really a config file that is specific for a remote host...
    options.addOption(
            Poco::Util::Option( "profile", "", "Load profile from FILE" )
            .required( false )
            .repeatable( false )
            .argument( "FILE" )
            .binding( CONFIG_PROFILE ) );

    options.addOption(
            Poco::Util::Option( "ignore", "", "Space separated list of file globs to ignore" )
            .required( false )
            .repeatable( false )
            .argument( "GLOBLIST" )
            .binding( CONFIG_IGNORE ) );


    options.addOption(
            Poco::Util::Option( "src", "s", "Mirror from DIR ( source )" )
            .required( false )
            .repeatable( false )
            .argument( "DIR" )
            .binding( CONFIG_SRC ) );
    options.addOption(
            Poco::Util::Option( "from", "", "From DIR ( source )" )
            .required( false )
            .repeatable( false )
            .argument( "DIR" )
            .binding( CONFIG_SRC ) );

    options.addOption(
            Poco::Util::Option( "dest", "d", "Mirror to URI ( destination )" )
            .required( false )
            .repeatable( false )
            .argument( "URI" )
            .binding( CONFIG_DEST ) );
    options.addOption(
            Poco::Util::Option( "to", "", "To URI ( destination )" )
            .required( false )
            .repeatable( false )
            .argument( "URI" )
            .binding( CONFIG_DEST ) );

    options.addOption(
            Poco::Util::Option( "private-key", "", "Use KEYFILE to authenticate SSH connection to remote host" )
            .required( false )
            .repeatable( false )
            .argument( "KEYFILE" )
            .binding( CONFIG_RSYNC_SSH_KEYFILE ) );

    options.addOption(
            Poco::Util::Option( "rsync-protocol-version", "", "Use version VER of rsync to remote host" )
            .required( false )
            .repeatable( false )
            .argument( "VER" )
            .binding( CONFIG_RSYNC_PROTOCOL_VERSION ) );

    options.addOption(
            Poco::Util::Option( "method", "", Poco::format("Use METHOD to synchronize (%s, %s)", std::string(CONFIG_SYNC_METHOD_ACROSYNC), std::string(CONFIG_SYNC_METHOD_RSYNC)  ))
            .required( false )
            .repeatable( false )
            .argument( "METHOD" )
            .binding( CONFIG_SYNC_METHOD ) );

    options.addOption(
            Poco::Util::Option( "dry-run", "-n", "Try to synchronize, but don't" )
            .required( false )
            .repeatable( false )
            .binding( CONFIG_DRYRUN ) );

    config().setString( CONFIG_HELP, "-false-");
    config().setString( CONFIG_SRC, "");
    config().setString( CONFIG_DEST, "");
    config().setString( CONFIG_SYNC_METHOD, "rsync");
    config().setString( CONFIG_DRYRUN, "false");
};

void SourceSync::handleVerbose(const std::string &name, const std::string &value)
{
    FUNCTIONTRACE;

    int v =
#ifdef _DEBUG 
        Poco::Message::PRIO_INFORMATION;
#else
    Poco::Message::PRIO_ERROR;
#endif

    try {

        config().setInt( CONFIG_VERBOSE, Poco::NumberParser::parse( value ) );

    }
    catch ( Poco::SyntaxException ) {

        if ( config().hasProperty(  CONFIG_VERBOSE ) ) {
            v = config().getInt( CONFIG_VERBOSE );
        }

        v++;

        config().setInt( CONFIG_VERBOSE, v);

    }

    logger().setLevel("", config().getInt( CONFIG_VERBOSE ));
}

int SourceSync::main( const std::vector<std::string> &args ) {

    FUNCTIONTRACE;

    setLogger(Poco::Logger::get("SourceSync"));

    if ( config().hasProperty( CONFIG_VERBOSE ) ) {

        logger().setLevel("", config().getInt( CONFIG_VERBOSE ) );
    }


    if ( config().getString( CONFIG_HELP ).empty() ) {

        displayHelp();
        return Poco::Util::Application::EXIT_OK;
    }

    int exitStatus = Poco::Util::Application::EXIT_USAGE;

    try {

        if ( args.size() > 1 ) {

            throw Poco::Exception( "Unknown argument " + args[1] );
        }

        if ( config().getString( CONFIG_SRC, "").empty() ) {

            throw Poco::Exception( "No source argument specified" );
        }
        if ( config().getString( CONFIG_DEST, "").empty() ) {

            throw Poco::Exception( "No destination argument specified" );
        }

        // ensure we always have src and destination ending with '/'
        if ( config().getString( CONFIG_SRC).back() != '/' ) {

            config().setString( CONFIG_SRC, config().getString( CONFIG_SRC) + Poco::Path::separator() );
        }
        if ( config().getString( CONFIG_DEST).back() != '/' ) {

            config().setString( CONFIG_DEST, config().getString( CONFIG_DEST) + Poco::Path::separator() );
        }

        logger().notice( Poco::format( "Syncing from %s to %s", 
                    config().getString( CONFIG_SRC ) ,
                    config().getString( CONFIG_DEST, "" )  ) );

        if ( config().getString( CONFIG_SYNC_METHOD ) == CONFIG_SYNC_METHOD_ACROSYNC ) {
            rsync::SocketUtil::startup();

            // initiate libssh2
            //
            int rc = libssh2_init(0);
            if (rc != 0) {
                throw Poco::Exception( "Failed to initiate libssh2");
            }
        }

        // create the Queue for managing workers
        queue_ = new Queue;

        logger().notice("Processing initial synchronization");

#ifdef USE_LIB_FSWATCH        
        FSWatchMonitorDirectory *d = new FSWatchMonitorDirectory( config().getString( CONFIG_SRC )  ); 
#endif

#ifdef USE_POCO_DIRECTORY_WATCHER
        PocoMonitorDirectory *d = new PocoMonitorDirectory( config().getString( CONFIG_SRC )  ); 
#endif        

        // get enough queued so that queue size isn't zero before checking...
        Poco::Thread::sleep(5000);
        while ( jobCount_.value() != 0 ) {

            Poco::Thread::sleep(15000);
        }

        logger().notice("Initial synchronization complete");

        waitForTerminationRequest();

        return Poco::Util::Application::EXIT_OK;

    }
    catch ( Poco::Exception &ex ) {

        logger().error( "D47E5398-1BAE-4C8B-B3FF-18A298858D78 : " + ex.displayText() );

        Poco::NDC::current().dump(std::cerr);

        return Poco::Util::Application::EXIT_USAGE;
    }

    return exitStatus;
};

POCO_SERVER_MAIN(SourceSync);
