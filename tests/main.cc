/* Driver for testing

 * Does test setup and teardown.
*/

#include <iostream>
#include <string>
#include <vector>
#include "gtest/gtest.h"

#include "Poco/Util/Application.h"
#include "Poco/File.h"
#include "Poco/NumberParser.h"
#include "Poco/Process.h"


namespace testing {

    GTEST_DECLARE_string_(filter);

}


class SourceSyncTest : public Poco::Util::Application
{

    public:

        SourceSyncTest() {

        }

        void defineOptions( Poco::Util::OptionSet &options) {

            Poco::Util::Application::defineOptions(options);

            options.addOption(
                    Poco::Util::Option("verbose", "v", "set verbosity of messages")
                    .required(false)
                    .repeatable(true)
                    .argument("LEVEL", false)
                    .callback(Poco::Util::OptionCallback<SourceSyncTest>(this, &SourceSyncTest::handleVerbose)));

            options.addOption(
                    Poco::Util::Option("src-dir", "", "Use SRCDIR as source for tests")
                    .required(false)
                    .repeatable(true)
                    .argument("SRCDIR", false)
                    .binding( "test.srcdir" ));

            options.addOption(
                    Poco::Util::Option("dest-dir", "", "Use DESTDIR as source for tests")
                    .required(false)
                    .repeatable(true)
                    .argument("DESTDIR", false)
                    .binding( "test.destdir" ));

            options.addOption(
                    Poco::Util::Option("sleep", "", "Sleep for SLEEP seconds before checking destination")
                    .required(false)
                    .repeatable(true)
                    .argument("SLEEP", false)
                    .binding( "test.sleep" ));

            options.addOption(
                    Poco::Util::Option("auth", "", "Connect using AUTH")
                    .required(false)
                    .repeatable(true)
                    .argument("AUTH", false)
                    .binding( "test.auth" ));

            options.addOption(
                    Poco::Util::Option("private-key", "", "Use KEYFILE as --private-key argument to srcsync")
                    .required(false)
                    .repeatable(true)
                    .argument("KEYFILE", false)
                    .binding( "test.keyfile" ));

            config().setString( "test.srcdir", "./TEST-SRC-DIR/");
            config().setString( "test.destdir", "./TEST-DST-DIR/");
            config().setString( "test.sleep", "5");

        }
        void handleVerbose(const std::string &name, const std::string &value)
        {

            int v =
#ifdef _DEBUG 
                Poco::Message::PRIO_INFORMATION;
#else
            Poco::Message::PRIO_ERROR;
#endif

            try {

                config().setInt( "verbose", Poco::NumberParser::parse( value ) );

            }
            catch ( Poco::SyntaxException ) {

                if ( config().hasProperty( "verbose") ) {
                    v = config().getInt( "verbose");
                }

                v++;

                config().setInt( "verbose", v);

            }

            logger().setLevel("", config().getInt( "verbose"));
        };


        int main(const std::vector<std::string> & args ) {

            // before doing anything make sure that src and dest dirs
            // exist and are empty

            Poco::Path srcPath( config().getString( "test.srcdir"));
            Poco::Path dstPath( config().getString( "test.destdir"));

            Poco::File srcdir( srcPath );
            Poco::File dstdir( dstPath );

            // removes test directories
            if ( srcdir.exists() ) {
                srcdir.remove(true);
            }
            if ( dstdir.exists() ) {
                dstdir.remove(true);
            }

            // creates test directories
            srcdir.createDirectories();
            dstdir.createDirectories();


            Poco::Process::Args launchArgs;

            std::string auth( config().getString( "test.auth" ) );

            launchArgs.push_back( Poco::format( "--from=%s", srcPath.toString() ) );
            launchArgs.push_back( Poco::format( "--to=ssh://%s@localhost%s", auth, dstPath.absolute().toString() ) );
            launchArgs.push_back( "--ignore=*.IGNORE" );
            launchArgs.push_back( "-v10" );
            launchArgs.push_back( "--rsync-log-level=1" );

            if ( config().getString( "test.keyfile", "").empty() == false ) {

                launchArgs.push_back( Poco::format( "--private-key=%s", config().getString( "test.keyfile") ) );
            }

            std::string launchCmd( "./srcsync" );

            for ( Poco::Process::Args::iterator it = launchArgs.begin(); it != launchArgs.end(); it++ ) {

                launchCmd += " ";
                launchCmd += *it;
            }
            

            logger().information( launchCmd );

            Poco::ProcessHandle handle = Poco::Process::launch( "./srcsync", launchArgs, ".");

            handle_ = new Poco::ProcessHandle( handle );

            Poco::Thread::sleep( 10000 ); // lets things initialize before running any tests...

            return RUN_ALL_TESTS();

        }

        Poco::SharedPtr<Poco::ProcessHandle> getHandle() {

            return handle_;
        }

    private:

        Poco::SharedPtr<Poco::ProcessHandle> handle_;
};


/// modified from POCO_MAIN_APP
int main(int argc, char **argv)
{

    testing::InitGoogleTest(&argc, argv);

    int ret = -1;

    try {
        Poco::AutoPtr<SourceSyncTest> test = new SourceSyncTest ;

        test->init(argc, argv);

        ret = test->run();

        // terminate srcsync started in main
        std::cout << "cleanup" << std::endl;
        Poco::Process::kill( * test->getHandle() );

    }
    catch ( Poco::Exception &ex ) {
        ; // ignore
    }

    return ret;
}

