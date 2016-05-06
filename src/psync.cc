/**
 * \file psync.cc
 *
 * \brief - synchronize files as they change to a remote system using rsync protocol
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

#include <iostream>


#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/RecursiveDirectoryIterator.h"
#include "Poco/NumberParser.h"


#define APPNAME "psync"

#define CONFIG_HELP     APPNAME ".help"     // --help
#define CONFIG_SRC      APPNAME ".src"      // --src
#define CONFIG_DEST     APPNAME ".dest"     // --dest
#define CONFIG_VERBOSE  APPNAME ".verbose"  // --verbose

class SyncDirectory
{
    public:

        SyncDirectory( const std::string &path) {

            Poco::Util::Application::instance().logger().debug( "Monitoring " + path );
        }


};

class pSync : public Poco::Util::ServerApplication
{

    public:

        pSync() {

            setUnixOptions( true );

            logger().setLevel("", Poco::Message::PRIO_NOTICE );
        };

        void initialize( Poco::Util::Application &self ) {

        }; 

        void uninitialize() {};


    protected:

        virtual void displayHelp() {

            Poco::Util::HelpFormatter helpFormatter( options() );

            helpFormatter.setUnixStyle( true );
            helpFormatter.setAutoIndent();
            helpFormatter.setCommand( "psync" );
            helpFormatter.setUsage( "OPTIONS ARGUMENTS" );
            helpFormatter.format( std::cout );

        };        

        void defineOptions( Poco::Util::OptionSet &options ) {

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
                    .callback(Poco::Util::OptionCallback<pSync>(this, &pSync::handleVerbose)));
            
            
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
                    Poco::Util::Option( "dest", "d", "Miror to DIR ( destination )" )
                    .required( false )
                    .repeatable( false )
                    .argument( "DIR" )
                    .binding( CONFIG_DEST ) );
            options.addOption(
                    Poco::Util::Option( "to", "", "To DIR ( destination )" )
                    .required( false )
                    .repeatable( false )
                    .argument( "DIR" )
                    .binding( CONFIG_DEST ) );


            config().setString( CONFIG_HELP, "-false-");
            config().setString( CONFIG_SRC, "");
            config().setString( CONFIG_DEST, "");
        };
        void handleVerbose(const std::string &name, const std::string &value)
        {

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
        


        int main( const std::vector<std::string> &args ) {

            setLogger(Poco::Logger::get(APPNAME));

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
                     
                    throw Poco::Exception( "No source/from argument specified" );
                }
                if ( config().getString( CONFIG_DEST, "").empty() ) {
                     
                    throw Poco::Exception( "No destination/to argument specified" );
                }


                // create a directory iterator for the source directory 
                // and spin up a SyncDirectory instance for each one
                Poco::SimpleRecursiveDirectoryIterator dirIt( config().getString( CONFIG_SRC ) );
                Poco::SimpleRecursiveDirectoryIterator end;

                while ( dirIt != end ) {

                    if ( dirIt->isDirectory() ) {

                        SyncDirectory *d = new SyncDirectory( dirIt->path() ); 

                    }

                    ++dirIt;
                }
                

                waitForTerminationRequest();

                return Poco::Util::Application::EXIT_OK;

            }
            catch ( Poco::Exception &ex ) {

                logger().error( "D47E5398-1BAE-4C8B-B3FF-18A298858D78 : " + ex.displayText() );

                return Poco::Util::Application::EXIT_USAGE;
            }

            return exitStatus;
        };
};

POCO_SERVER_MAIN(pSync);
