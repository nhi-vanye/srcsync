

#include "Poco/Util/ServerApplication.h"


class SourceSync : public Poco::Util::ServerApplication
{

    // methods    

    public:
        SourceSync();

        void initialize( Poco::Util::Application &self );
        
        void uninitialize() {};

        int main( const std::vector<std::string> &args );

    protected:

        void displayHelp();
        void defineOptions( Poco::Util::OptionSet &options );
        
    private:

        void handleVerbose(const std::string &name, const std::string &value);


    // data    
        
    private:

        Poco::SharedPtr<Poco::NotificationQueue> queue_;

};
