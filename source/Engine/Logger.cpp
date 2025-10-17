/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Logger.hpp"
#include <iostream>

namespace CS230
{
    Logger::Logger(Logger::Severity severity, bool use_console, std::chrono::system_clock::time_point last_tick) : min_level(severity), out_stream("Trace.log"), start_time(last_tick){
        if (use_console == true) {
            out_stream.basic_ios<char>::rdbuf(std::cout.rdbuf());
        }
    }

    Logger::~Logger() {
        out_stream.flush();
        out_stream.close();
    }
    void Logger::LogError(std::string text) {
        log(Severity::Error, text);
    }
    void Logger::LogEvent(std::string text) {
        log(Severity::Event, text);
    }
    void Logger::LogDebug(std::string text) {
        log(Severity::Debug, text);
    }
    void Logger::LogVerbose(std::string text) {
        log(Severity::Verbose, text);
    }
    double CS230::Logger::seconds_since_start() {
        
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        return std::chrono::duration<double>(now - start_time).count();
    }

    void Logger::log(CS230::Logger::Severity severity, std::string message) {
        const char* level[] = {"Verbose", "Debug", "Event", "Error"};

        if (min_level <= severity){
            out_stream.precision(4);
            out_stream << '[' << std::fixed << seconds_since_start() << "]\t";
            out_stream << level[int(severity)] << '\t' << message << std::endl;
        }
        return;
    }

}
