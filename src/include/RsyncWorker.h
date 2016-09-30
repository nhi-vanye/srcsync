
#ifndef RSYNCWORKER_H
#define RSYNCWORKER_H

#include "Poco/PriorityNotificationQueue.h"
#include "Poco/Process.h"
#include "Poco/ActiveMethod.h"
#include "Poco/PipeStream.h"

#if USE_GROWL
#include "growl.hpp"
#endif

#include "Queue.h"

class RsyncWorker : public SyncWorker
{

    public:
        RsyncWorker( const std::string &name, Poco::PriorityNotificationQueue *queue);

        virtual void run();

        virtual void initialize();

        Poco::ActiveMethod<bool, Poco::PipeInputStream &, RsyncWorker> readStdOut;
        Poco::ActiveMethod<bool, Poco::PipeInputStream &, RsyncWorker> readStdErr;

    private:

        Poco::Logger &logger_;

        std::string user_;
        std::string password_;
        std::string private_key_;

        Poco::Path local_;

#if USE_GROWL
        Poco::SharedPtr<Growl> growl_;
#endif

    protected:
        
        bool runRsync( const std::string & src, const std::string & dest );

        bool readOutPipe( Poco::PipeInputStream & );
        bool readErrPipe( Poco::PipeInputStream & );

};

#endif // RSYNCWORKER_H
