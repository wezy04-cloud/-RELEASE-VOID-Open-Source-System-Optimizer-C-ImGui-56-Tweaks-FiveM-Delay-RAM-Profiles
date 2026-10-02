#pragma once
#include <string>

// License activation. The validation in license.cpp is a local DEMO —
// swap Validate() for a request to your own auth backend.
namespace license
{
    enum class Status { Idle, Checking, Valid, Invalid };

    struct Info
    {
        std::string key;
        std::string user;
        std::string plan;
        std::string expires;
        std::string hwid;
    };

    void               Activate(const std::string& key); // async
    Status             Poll();                           // call every frame
    void               Logout();
    const std::string& Error();
    const Info&        Current();

    std::string Mask(const std::string& key);

    // "Remember me" persistence (%APPDATA%\VOID\license.dat)
    std::string LoadSaved();
    void        Save(const std::string& key);
    void        ForgetSaved();
}
