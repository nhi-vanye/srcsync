/**
 * \file MonitorDirectory.h
 *
 * \brief - Base class for directory monitor implementations
 *
 */


class MonitorDirectory
{
    public:

        MonitorDirectory( const std::string &path, const std::string &loggerName = "MonitorDirectory") : logger_(Poco::Logger::get( loggerName )) {};

   protected:

        Poco::Logger &logger_;
};
 
