
// obtain configuration information. 
#include "Poco/Util/Application.h"
#include "Poco/FileStream.h"

#include "Poco/File.h"
#include "Poco/Timestamp.h"

#include "gtest/gtest.h"

#define po_config Poco::Util::Application::instance().config

class NewFile : public ::testing::Test {

  protected:
    NewFile() {
      // You can do set-up work for each test here.
    };

    virtual ~NewFile() {
      // You can do clean-up work that doesn't throw exceptions here.
    };

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    virtual void SetUp() {
      // Code here will be called immediately after the constructor (right
      // before each test).
    };

    virtual void TearDown() { 
    };

    // Add local data needed for this test here.

};


TEST_F(NewFile,newZeroLengthFile)
{

    // create a new file
    Poco::Timestamp now;

    Poco::Path srcPath( Poco::format("%s/%Lu", po_config().getString( "test.srcdir"), (Poco::UInt64) now.epochMicroseconds() ) );
    Poco::File srcFile( srcPath );

    srcFile.createFile(); // zero length

    // does it exist
    EXPECT_TRUE( srcFile.exists() ) << "Filename: " << srcPath.toString();
    EXPECT_EQ( srcFile.getSize(),  0 );

    // sleep
    Poco::Thread::sleep( po_config().getInt( "test.sleep" ) * 1000 );

    // does the file exist in destination ?
    Poco::Path destPath( Poco::format("%s/%s/%Lu", po_config().getString( "test.destdir"), po_config().getString( "test.srcdir"), (Poco::UInt64) now.epochMicroseconds() ) );
    Poco::File destFile( destPath );

    // does it exist
    EXPECT_TRUE( destFile.exists() ) << "Filename: " << destPath.toString() ;
    EXPECT_EQ( destFile.getSize(),  0 );
}

TEST_F(NewFile,newNonZeroLengthFile)
{

    // create a new file
    Poco::Timestamp now;

    Poco::Path srcPath( Poco::format("%s/%Lu", po_config().getString( "test.srcdir"), (Poco::UInt64) now.epochMicroseconds() ) );
    Poco::File srcFile( srcPath );

    srcFile.createFile(); // zero length

    Poco::FileOutputStream out( srcFile.path() );

    out << srcPath.toString();
    out.close();

    // does it exist
    EXPECT_TRUE( srcFile.exists() ) << "Filename: " + srcPath.toString();
    EXPECT_EQ( srcFile.getSize(),  srcPath.toString().size() );

    // sleep
    Poco::Thread::sleep( po_config().getInt( "test.sleep" ) * 1000 );

    // does the file exist in destination ?
    Poco::Path destPath( Poco::format("%s/%s/%Lu", po_config().getString( "test.destdir"), po_config().getString( "test.srcdir"), (Poco::UInt64) now.epochMicroseconds() ) );
    Poco::File destFile( destPath );

    // does it exist
    EXPECT_TRUE( destFile.exists() ) << "Filename: " + destPath.toString() ;
    EXPECT_EQ( destFile.getSize(),  srcPath.toString().size() );
}



TEST_F(NewFile,newIgnoredZeroLengthFile)
{

    // create a new file
    Poco::Timestamp now;

    Poco::Path srcPath( Poco::format("%s/%Lu.IGNORE", po_config().getString( "test.srcdir"), (Poco::UInt64) now.epochMicroseconds() ) );
    Poco::File srcFile( srcPath );

    srcFile.createFile(); // zero length

    // does it exist
    EXPECT_TRUE( srcFile.exists() ) << "Filename: " + srcFile.path();

    // sleep
    Poco::Thread::sleep( po_config().getInt( "test.sleep" ) * 1000 );

    // does the file exist in destination ?
    Poco::Path destPath( Poco::format("%s/%s/%Lu.IGNORE", po_config().getString( "test.destdir"), po_config().getString( "test.srcdir"), (Poco::UInt64) now.epochMicroseconds() ) );
    Poco::File destFile( destPath );

    // does it exist
    EXPECT_FALSE( destFile.exists() ) << "Filename: " + destPath.toString() ;
}



