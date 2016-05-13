/**
 * \file MonitorDirectory.h
 *
 * \brief - Monitors a Directory and sends events
 *
 */
#include "Poco/ActiveMethod.h"
#include "Poco/Void.h"
#include "Poco/Path.h"

#include "MonitorDirectory.h"

#include "libfswatch/c++/event.hpp"
#include "libfswatch/c++/monitor.hpp"

class FSWatchMonitorDirectory : public MonitorDirectory
{
    public:

        FSWatchMonitorDirectory( const std::string &path);


        void handleEvents(const std::vector<fsw::event>& events);

        Poco::ActiveMethod<void, void, FSWatchMonitorDirectory> start;

    protected:

        void runImpl( void ) {

            monitor_->start();
        }

    private:

        // fswatch delivers changed paths as absolute
        Poco::Path base_;

        fsw::monitor *monitor_;

};

