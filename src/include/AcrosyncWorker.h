
#ifndef ACROSYNCWORKER_H
#define ACROSYNCWORKER_H

#include "Poco/PriorityNotificationQueue.h"

#include <rsync/rsync_client.h>
#include <rsync/rsync_sshio.h>
#include <libssh2.h>

#include "Queue.h"

class AcrosyncWorker : public SyncWorker
{

    public:
        AcrosyncWorker( const std::string &name, Poco::PriorityNotificationQueue *queue);

        virtual void run();

        virtual void initialize();

    private:

        rsync::SSHIO  sshio_;

        Poco::Logger &logger_;

};

#endif // ACROSYNCWORKER_H
