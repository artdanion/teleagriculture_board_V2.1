#include <Arduino.h>
#include <vector>

#pragma once

#define NUM_PREDIFINED_NTP 7
#define CUSTOM_NTP_INDEX 7

namespace WiFiManagerNS
{
  namespace NTP
  {
    struct NTP_Server
    {
      const std::string name;
      const std::string addr;
    };

    extern std::vector<NTP_Server> NTP_Servers;
  };
};