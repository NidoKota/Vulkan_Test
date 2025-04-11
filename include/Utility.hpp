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
    
    //mutex coutMutex;
    inline void cerrMultiThread(std::string str)
    {
        //lock_guard<mutex> lock(coutMutex);
        cerr << str;
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

    std::string getIndexStr(size_t index)
    {
        std::stringstream s;
        for (size_t i = 0; i < index; i++)
        {
            s << "    ";
        }
        return s.str();
    }

#define SS2STR(x) \
([&]() -> std::string { std::stringstream ss; ss << std::boolalpha << x; return ss.str(); })()

// #define LOG_STR \
// "\n(at <a href=""{" << __FILE__ << "}"" line=""{" << __LINE__ << "}"">" << __FILE__ << ":" << __LINE__ << "</a>)"

#define FILE_LINE_STR \
__FILE__ << ":" << std::setw(3) << std::setfill('0') << __LINE__ << std::setfill(' ')

#define GET_THIRD_ARG(_1, _2, NAME, ...) NAME

#define LOG_CUSTOM_INDEX(x, index) \
do { coutMultiThread(SS2STR(getTimeStamp() << " " << FILE_LINE_STR << " " << getIndexStr(index) << x << "\n")); } while (false)

#define LOG_0_INDEX(x) \
LOG_CUSTOM_INDEX(x, 0)

#define LOG(...) GET_THIRD_ARG(__VA_ARGS__, LOG_CUSTOM_INDEX, LOG_0_INDEX)(__VA_ARGS__)

#define LOGERR_CUSTOM_INDEX(x, index) \
do { coutMultiThread(SS2STR(getTimeStamp() << " " << FILE_LINE_STR << " " << getIndexStr(index) << x << "\n")); } while (false)

#define LOGERR_0_INDEX(x) \
LOGERR_CUSTOM_INDEX(x, 0)

#define LOGERR(...) GET_THIRD_ARG(__VA_ARGS__, LOGERR_CUSTOM_INDEX, LOGERR_0_INDEX)(__VA_ARGS__)
}