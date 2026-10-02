#pragma once
#include <string>

namespace network
{
    bool FlushDns();
    bool SetDns(int provider);
    bool ApplyTweak(int index, bool enable);
    bool ReadTweak(int index);

    std::string AdapterName();
    std::string LocalIP();
    std::string GatewayIP();
}
