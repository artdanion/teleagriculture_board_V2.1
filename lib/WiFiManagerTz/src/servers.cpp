#include <Arduino.h>
#include <vector>
#include <servers.h>

namespace WiFiManagerNS
{
    namespace NTP
    {
        std::vector<NTP_Server> NTP_Servers = {
            {"Global", "pool.ntp.org"},
            {"Africa", "africa.pool.ntp.org"},
            {"Asia", "asia.pool.ntp.org"},
            {"Europe", "europe.pool.ntp.org"},
            {"North America", "north-america.pool.ntp.org"},
            {"Oceania", "oceania.pool.ntp.org"},
            {"South America", "south-america.pool.ntp.org"},
            //{"DHCP", "0.0.0.0"}, // TODO: enable private NTP servers
        };
    };
};