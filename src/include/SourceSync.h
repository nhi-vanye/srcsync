

#include "Poco/Util/ServerApplication.h"
#include "Poco/PriorityNotificationQueue.h"
#include "Poco/NestedDiagnosticContext.h"

#include "Queue.h"

#ifndef SOURCESYNC_H
#define SOURCESYNC_H

class SourceSync : public Poco::Util::ServerApplication
{

    // methods    

    public:
        SourceSync();

        void initialize( Poco::Util::Application &self );
        
        void uninitialize() {};

        int main( const std::vector<std::string> &args );

        Poco::PriorityNotificationQueue *queue() { return queue_->queue(); };

    protected:

        void displayHelp();
        void defineOptions( Poco::Util::OptionSet &options );
        
    private:

        void handleVerbose(const std::string &name, const std::string &value);


    // data    
        
    private:

        Queue *queue_;

};

extern SourceSync *thisApp;

#define FUNCTIONTRACE Poco::NDCScope _theNdcScope( __PRETTY_FUNCTION__, __LINE__, __FILE__)

#endif // SOURCESYNC_H
