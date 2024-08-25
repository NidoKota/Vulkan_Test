#pragma once

#define _USE_MATH_DEFINES

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <optional>

namespace Vulkan_Test
{
    using namespace std;

    //mutex coutMutex;
    inline void coutMultiThread(std::string str)
    {
        //lock_guard<mutex> lock(coutMutex);
        cout << str;
    }

    inline string getTimeStamp()
    {
        auto now = chrono::system_clock::now();
        auto nowC = chrono::system_clock::to_time_t(now);
        auto nowMs = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
        stringstream ss;
        ss << put_time(localtime(&nowC), "%T") << '.' << setfill('0') << setw(3) << nowMs.count();
        return ss.str();
    }

#define SS2STR(x) \
([&]() -> std::string { std::stringstream ss; ss << std::boolalpha << x; return ss.str(); })()

#define LOG_STR \
"\n(at <a href=""{" << __FILE__ << "}"" line=""{" << __LINE__ << "}"">" << __FILE__ << ":" << __LINE__ << "</a>)"

#define LOG(x) \
do { coutMultiThread(SS2STR(getTimeStamp() << " " << x << "\n")); } while (false)
}