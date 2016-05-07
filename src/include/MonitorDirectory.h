/**
 * \file MonitorDirectory.h
 *
 * \brief - Monitors a Directory and sends events
 *
 */

class MonitorDirectory
{
    public:

        MonitorDirectory( const std::string &path);

    private:
        void onItemAdded(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        void onItemRemoved(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        void onItemModified(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        void onItemMovedFrom(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        void onItemMovedTo(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        Poco::SharedPtr<Poco::DirectoryWatcher> watcher_;

        Poco::Logger &logger_;
};

