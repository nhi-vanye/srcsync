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
#ifdef USE_POCO_DIRECTORY_WATCHER
        void onItemAdded(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        void onItemRemoved(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        void onItemModified(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        void onItemMovedFrom(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        void onItemMovedTo(const Poco::DirectoryWatcher::DirectoryEvent& ev);

        Poco::SharedPtr<Poco::DirectoryWatcher> watcher_;
#endif // USE_POCO_DIRECTORY_WATCHER

        Poco::Logger &logger_;
};

