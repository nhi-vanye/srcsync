
#include "Poco/PriorityNotificationQueue.h"
#include "Poco/ThreadPool.h"
#include "Poco/Thread.h"
#include "Poco/URI.h"
#include "Poco/Glob.h"

#include <rsync/rsync_client.h>
#include <rsync/rsync_sshio.h>
#include <libssh2.h>

#ifndef QUEUE_H
#define QUEUE_H

#define MSG_SYNC "SyncMessage"
#define MSG_FILE_SYNC "FileSyncMessage"
#define MSG_DIR_SYNC "DirSyncMessage"

class SyncMessage : public Poco::Notification
{

    public:
        SyncMessage( const std::string &path ) : path_(path), type_(MSG_SYNC) { };
        SyncMessage( const std::string &path, const std::string &type ) : path_(path), type_(type) { };

        const virtual std::string &path() { return path_; };
        const virtual std::string type() { return type_; };

    protected:
        std::string path_;
        std::string type_;
};

class FileSyncMessage : public SyncMessage
{

    public:
        FileSyncMessage( const std::string &path ) : SyncMessage(path, MSG_FILE_SYNC) { };
};

class DirSyncMessage : public SyncMessage
{

    public:
        DirSyncMessage( const std::string &path ) : SyncMessage(path, MSG_DIR_SYNC) { };
};

class SyncWorker : public Poco::Runnable 
{

    public:
        SyncWorker( const std::string &name, Poco::PriorityNotificationQueue *queue);

        void run();
        std::string name() { return name_; };

        void initialize();


    protected:
        std::string name_;
        Poco::PriorityNotificationQueue *queue_;

    private:
        Poco::SharedPtr<Poco::URI> remote_;

        Poco::Logger &logger_;

        rsync::SSHIO  sshio_;

        std::vector<Poco::Glob *> ignore_;

};


class Queue : public Poco::PriorityNotificationQueue
{

    public:
        Queue();

        Poco::PriorityNotificationQueue *queue() { return queue_; };
        
        // rsync library only has a single logger object so this can't be wired into the workers.
        void outputCallback(const char * path, bool isDir, int64_t size, int64_t time, const char * symlink);
        void statusCallback(const char * status);
        void logCallback(const char *id, int level, const char *message);

    private:

    // data
    private:
        std::vector<SyncWorker *> workers_;
        Poco::SharedPtr<Poco::PriorityNotificationQueue> queue_;
        Poco::SharedPtr<Poco::ThreadPool> pool_;

        Poco::Logger &logger_;
};

#endif
