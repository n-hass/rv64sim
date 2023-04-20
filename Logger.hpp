// C++17 required for filesytem. 
// if using pre-17, include boost/filesystem and replace namespace fs=std::filesystem with fs=boost::filesystem

#ifndef CUSTOM_LOGGER_H
#define CUSTOM_LOGGER_H

#include <string>
#include <fstream>
#include <chrono>
#include <ctime>
#include <vector>

#if defined(__has_include)
    #if  __has_include(<filesystem>) && defined(__cpp_lib_filesystem)
        #include <filesystem>
        #define fs std::filesystem
    #else
        #include <boost/filesystem.hpp>
        #define fs boost::filesystem
    #endif

#else
    #include <boost/filesystem.hpp>
    #define fs boost::filesystem
#endif

// if boost undefined, #error No filesystem library available. Compile with boost library or use at least C++ 17.

// enumarotor class for shorthand of the log level
enum Level {
    Fatal,
    Error,
    Warning,
    Info,
    Debug,
    General
};

// Logger class //

class Logger {
private:
    fs::path::string_type fullPathStr; // the full path, as a string (only useful for the getDir function)
    char* fullPath; // the full path as char[], this is used for opening the file-stream
    std::ofstream file; // the file-stream

    bool set; // if the path has been set and the log set up
    bool globalised; // if a pointer (this) has been added to the logger vector
    bool paused; // if the logging is 'paused'

    std::chrono::time_point<std::chrono::system_clock> startTime;

    std::string levels[6] = {"Fatal", "Error", "Warning", "Info", "Debug", "General"};
    
public:
    // inline static vector allows session-wide access to log file
    inline static std::vector<Logger*> all;

    Logger() { // constructor
        bool set = false;
        set = 0; // silence stupid compiler warning
        paused = false;
        globalised = false;
    }
    
    /**
     * Writes a nice finish to the log.
     * If the logger was globalised, will remove from the vector of loggers
     */
    ~Logger() { // destructor

        if ( file.is_open() ) {
            file << "\n–– –– ––\nFinished log: " << timeNow() << std::endl;
            std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - startTime;
            file << "Elapsed time (seconds): " << elapsed_seconds.count() << "s\n–– –– ––" << std::endl;

            file.close();
            delete fullPath;
        } else if (set) {
            file.open(fullPath, std::fstream::app);
            file << "\n–– –– ––\nFinished log: " << timeNow() << std::endl;
            std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - startTime;
            file << "Elapsed time (seconds): " << elapsed_seconds.count() << "s\n–– –– ––" << std::endl;
            
            file.close();
            delete fullPath;
        }

        
        if (globalised) {
            all.erase(std::remove(all.begin(), all.end(), this), all.end()); // remove pointer to this object from all vector
        }
    }
    
    /**
     * Starts the logging.
     * Creates file or writes to file.
     * a string path and filename is taken in and used to create an
     * ofstream object.
     * this is written to and saved as state for the logger object.
     */
    void start (std::string pathToLogDirectory, std::string logFileName) {

        // ensures portability. path is built as string or wstring and as a char.
        std::string pathStr = pathToLogDirectory + fs::path::preferred_separator + logFileName;
        fullPath = new char[pathStr.size() +1];

        pathStr.copy(fullPath, pathStr.size() + 1);
        fullPath[pathStr.size()] = '\0';

        fs::path a (fullPath);
        fullPathStr = a.make_preferred().native();
        
        set = true;
        startTime = std::chrono::system_clock::now();

        // start logging
        file.open(fullPath,std::fstream::app);

        paused = false;

        file << "–– –– ––\nStarted log: " << timeNow() << "\n–– –– ––\n" << std::endl;
    }

    void start(std::string logFileName){
        this->start( fs::current_path().string(), logFileName);
    }

    /**
     * closes the file. 
     * Can be silenced if true value passed
     */
    void pause (bool silent) { // pause, with optional silencing
        if (silent == false) {
            file << "Logging paused." << std::endl;
        }
        file.close();
    }
    void pause () {
        file << "Logging paused." << std::endl;
        file.close();
    }

    /**
     * opens file again
     */
    void resume (bool silent) { // resume, with optional silencing
        if (file.is_open() == false) {
            file.open(fullPath,std::fstream::app);
            if (silent == false) {
                file << "\nLogging resumed at "+ timeNow() << std::endl;
            }
        }
    }
    void resume () {
        if (file.is_open() == false){
            file.open(fullPath,std::fstream::app);
            file << "\nLogging resumed at "+ timeNow() << std::endl;
        }
    }

    /**
     * returns the directory set by the input of start()
     */
    fs::path::string_type getDir() {
        return fullPathStr;
    }


    /**
     * writes a log to the log file. 
     * if a file hasn't been set, will throw const char* exception
     */
    void log (Level s, const std::string& msg) {
        if (paused == false) {
            if (set) {
                if ( file.is_open() ) {
                    file << timeNow() << ": " << levels[static_cast<int>(s)] << " – " << msg << std::endl;
                } else {
                    file.open(fullPath,std::fstream::app);
                    file << timeNow() << ": " << levels[static_cast<int>(s)] << " – " << msg << std::endl;
                }
            } else {
                throw "No log file has been set. use Logger::start(path, filename) to begin";
            }
        }
    }

    void log_fatal(const std::string& msg) {
        log(Level::Fatal, msg);
    }

    void log_error(const std::string& msg) {
        log(Level::Error, msg);
    }

    void log_warning(const std::string& msg) {
        log(Level::Warning, msg);
    }

    void log_info(const std::string& msg) {
        log(Level::Info, msg);
    }

    void log_debug(const std::string& msg) {
        log(Level::Debug, msg);
    }

    void log_general(const std::string& msg) {
        log(Level::General, msg);
    }

    void insertBreak() {
        if (file.is_open()) {
            file << "–\n";
        }
    }

    void operator<<(const std::string& msg) {
        log(Level::General, msg);
    }

    /**
     * adds a pointer to the all vector so it may
     * be accessed from ANY module/translation unit.
     * Must be careful with static scope or, for ease of use,
     * define all loggers on the heap.
     */
    void globalise () {
        if (globalised) return;
        all.push_back(this);
        globalised = true;
    }

    /**
     * gets the current date and time from system as a string
     */
    static std::string timeNow() {
        auto timeBase = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(timeBase);

        std::string out = std::ctime(&time);
        out.erase(std::remove(out.begin(), out.end(), '\n'), out.end()); // remove newline character from string
        return out;
    }

    /**
     * Running this assumes all loggers were constructed dynamically on the heap.
     * This will SEG FAULT if ran when a mix of both stack allocated and heap allocated
     * loggers exist simulatenously anywhere within a program. 
     * Scope is irrelevant – this acts 'globally'
     */
    static void deleteAll() {
        for (int i=0; i<all.size(); i++) {
            Logger* temp = all[i];
            all[i] = nullptr; // prevent movement in the vector
            delete temp;
        }
        all.clear();
    }
    
};

#endif