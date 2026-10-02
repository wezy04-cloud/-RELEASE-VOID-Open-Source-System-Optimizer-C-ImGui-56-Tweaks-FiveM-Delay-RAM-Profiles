#include "license.hpp"
#include "sysinfo.hpp"
#include <windows.h>
#include <future>
#include <chrono>
#include <thread>
#include <fstream>
#include <ctime>
#include <cctype>
#include <cstdlib>

namespace license
{
    namespace
    {
        struct Result
        {
            bool        ok = false;
            std::string error;
            Info        info;
        };

        std::future<Result> g_task;
        Status              g_status = Status::Idle;
        std::string         g_error;
        Info                g_info;

        // Strips dashes/spaces and upper-cases. Returns "" on illegal characters.
        std::string Normalize(const std::string& key)
        {
            std::string s;
            for (char c : key)
            {
                if (std::isalnum((unsigned char)c))
                    s += (char)std::toupper((unsigned char)c);
                else if (c != '-' && c != ' ')
                    return {};
            }
            return s;
        }

        std::string Pretty(const std::string& n)
        {
            std::string s;
            for (size_t i = 0; i < n.size(); ++i)
            {
                if (i && i % 4 == 0)
                    s += '-';
                s += n[i];
            }
            return s;
        }

        Result Validate(std::string key, std::string hwid)
        {
            // Simulated network round-trip.
            std::this_thread::sleep_for(std::chrono::milliseconds(1400));

            // ------------------------------------------------------------------
            // DEMO VALIDATION. Replace this block with a call to your own auth
            // server (send key + hwid over HTTPS and verify a signed response).
            // Never ship real license checks that only run client-side.
            // ------------------------------------------------------------------
            Result r;
            std::string n = Normalize(key);
            if (n.empty())
            {
                r.error = "Please enter a license key";
                return r;
            }
            while (n.size() < 16) n += 'X';
            if (n.size() > 16) n = n.substr(0, 16);

            const bool lifetime = n.rfind("LIFE", 0) == 0;
            r.ok        = true;
            r.info.key  = Pretty(n);
            r.info.user = sys::UserName();
            r.info.hwid = hwid;
            r.info.plan = lifetime ? "Lifetime" : "Premium";
            if (lifetime)
            {
                r.info.expires = "Never";
            }
            else
            {
                const time_t t = time(nullptr) + 30 * 86400;
                tm tmv{};
                localtime_s(&tmv, &t);
                char buf[32];
                strftime(buf, sizeof(buf), "%d %b %Y", &tmv);
                r.info.expires = buf;
            }
            return r;
        }

        std::string StoragePath()
        {
            char*  env = nullptr;
            size_t len = 0;
            std::string dir = (_dupenv_s(&env, &len, "APPDATA") == 0 && env) ? env : ".";
            free(env);
            dir += "\\VOID";
            CreateDirectoryA(dir.c_str(), nullptr);
            return dir + "\\license.dat";
        }
    }

    void Activate(const std::string& key)
    {
        if (g_status == Status::Checking)
            return;
        g_status = Status::Checking;
        g_error.clear();
        g_task = std::async(std::launch::async, Validate, key, sys::Hwid());
    }

    Status Poll()
    {
        if (g_status == Status::Checking && g_task.valid() &&
            g_task.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            Result r = g_task.get();
            if (r.ok)
            {
                g_status = Status::Valid;
                g_info   = r.info;
            }
            else
            {
                g_status = Status::Invalid;
                g_error  = r.error;
            }
        }
        return g_status;
    }

    void Logout()
    {
        g_status = Status::Idle;
        g_info   = {};
    }

    const std::string& Error()   { return g_error; }
    const Info&        Current() { return g_info; }

    std::string Mask(const std::string& key)
    {
        if (key.size() < 19)
            return key;
        return key.substr(0, 4) + "-\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2-\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2-" + key.substr(15, 4);
    }

    std::string LoadSaved()
    {
        std::ifstream f(StoragePath());
        std::string s;
        std::getline(f, s);
        return s;
    }

    void Save(const std::string& key)
    {
        std::ofstream f(StoragePath(), std::ios::trunc);
        f << key;
    }

    void ForgetSaved() { DeleteFileA(StoragePath().c_str()); }
}
