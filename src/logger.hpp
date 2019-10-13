#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <sstream>
#include <variant>
#include <memory>

#include "caf/all.hpp"

#define LOG_TRACE_A(cat, msg) as::common::logger::trace_a(this, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_DEBUG_A(cat, msg) as::common::logger::debug_a(this, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_INFO_A(cat, msg) as::common::logger::info_a(this, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_WARN_A(cat, msg) as::common::logger::warn_a(this, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_ERROR_A(cat, msg) as::common::logger::error_a(this, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_FATAL_A(cat, msg) as::common::logger::fatal_a(this, msg, cat, __FUNCTION__, __LINE__, __FILE__)

#define LOG_TRACE_S(sys, cat, msg) as::common::logger::trace_s<decltype(this)>(sys, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_DEBUG_S(sys, cat, msg) as::common::logger::debug_s<decltype(this)>(sys, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_INFO_S(sys, cat, msg) as::common::logger::info_s<decltype(this)>(sys, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_WARN_S(sys, cat, msg) as::common::logger::warn_s<decltype(this)>(sys, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_ERROR_S(sys, cat, msg) as::common::logger::error_s<decltype(this)>(sys, msg, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_FATAL_S(sys, cat, msg) as::common::logger::fatal_s<decltype(this)>(sys, msg, cat, __FUNCTION__, __LINE__, __FILE__)

#define LOG_TRACE_AOUT(cat) as::common::logger::trace_aout(this, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_DEBUG_AOUT(cat) as::common::logger::debug_aout(this, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_INFO_AOUT(cat) as::common::logger::info_aout(this, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_WARN_AOUT(cat) as::common::logger::warn_aout(this, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_ERROR_AOUT(cat) as::common::logger::error_aout(this, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_FATAL_AOUT(cat) as::common::logger::fatal_aout(this, cat, __FUNCTION__, __LINE__, __FILE__)

#define LOG_TRACE_SOUT(sys, cat) as::common::logger::trace_sout<decltype(this)>(sys, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_DEBUG_SOUT(sys, cat) as::common::logger::debug_sout<decltype(this)>(sys, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_INFO_SOUT(sys, cat) as::common::logger::info_sout<decltype(this)>(sys, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_WARN_SOUT(sys, cat) as::common::logger::warn_sout<decltype(this)>(sys, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_ERROR_SOUT(sys, cat) as::common::logger::error_sout<decltype(this)>(sys, cat, __FUNCTION__, __LINE__, __FILE__)
#define LOG_FATAL_SOUT(sys, cat) as::common::logger::fatal_sout<decltype(this)>(sys, cat, __FUNCTION__, __LINE__, __FILE__)

// Useful macro for logging and propagating request error handlers inside actor classes.
#define LOG_PGATE_A(rp) [this, rp] (const caf::error &err) mutable { std::string err_str = "Propagating error: "; err_str.append(this->system().render(err)); LOG_DEBUG_A("error", err_str); rp.deliver(err); }

namespace as::common {

    // A logger for use within actors.
    struct logger {

        enum class verbosity_t {trace = 0, debug, info, warn, error, fatal};
        static verbosity_t verbosity;

        //This is just manually set from the main function usually.
        static void set_verbosity(caf::atom_value atm_v) {
            switch (static_cast<uint64_t>(atm_v)) {
                case static_cast<uint64_t>(caf::atom("trace")):
                    logger::verbosity = verbosity_t::trace;
                    break;
                case static_cast<uint64_t>(caf::atom("debug")):
                    logger::verbosity = verbosity_t::debug;
                    break;
                case static_cast<uint64_t>(caf::atom("info")):
                    logger::verbosity = verbosity_t::info;
                    break;
                case static_cast<uint64_t>(caf::atom("warn")):
                    logger::verbosity = verbosity_t::warn;
                    break;
                case static_cast<uint64_t>(caf::atom("error")):
                    logger::verbosity = verbosity_t::error;
                    break;
                default:
                case static_cast<uint64_t>(caf::atom("fatal")):
                    logger::verbosity = verbosity_t::fatal;
            }
        }

        // %a: the actor id or empty
        // %c: category to precision (precision ignored)
        // %C: class name
        // %d: date
        // %F: filename
        // %l: location (just line number for now)
        // %L: line number
        // %m: message
        // %M: current function name
        // %n: \n!
        // %p: priority (debug, error etc.)
        // %r: number of milliseconds
        // %t: thread name
        // %x: nested thread diagnostic (just thread name for now)
        // %X: ditto
        // %%: %

        static std::string format; //= "%d %p %a %t %C %M %F:%L %m%n";
        static caf::timestamp start; // the timestamp when caf started!
        //TODO add settable verbosity!!! (on startup...)

        // Temporary object used to create ostream-style logging operator
        struct ostream_helper final {

            //TODO we need to change priority to verbosity_t... I think.

            ostream_helper(caf::local_actor *local_actor_thiz, const std::string &category, const std::string &class_name,
                           const std::string &filename, unsigned int line_no, const std::string &func_name, const logger::verbosity_t priority);

            ostream_helper(std::unique_ptr<caf::scoped_actor> &&scoped_actor_thiz, const std::string &category, const std::string &class_name,
                           const std::string &filename, unsigned int line_no, const std::string &func_name, const logger::verbosity_t priority)
                : thiz(std::move(scoped_actor_thiz)), category(category), class_name(class_name),
                  filename(filename), line_no(line_no), func_name(func_name), priority(priority), ss() {}

            ostream_helper(const ostream_helper &other) = delete;

            ostream_helper(ostream_helper &&other) = delete;

            ostream_helper() = delete;

            template<typename T>
            ostream_helper &operator<<(T val) {
                if(priority >= logger::verbosity) {
                    flushed = false;
                    ss << val;
                }
                return *this;
            }

            ostream_helper &operator<<(std::ostream& (*pf)(std::ostream&));

            void flush();

            ~ostream_helper();

            std::variant<caf::local_actor *, std::unique_ptr<caf::scoped_actor>> thiz;
            const std::string &category;
            const std::string &class_name;
            const std::string &filename;
            const unsigned int line_no;
            const std::string &func_name;
            const logger::verbosity_t priority;
            bool flushed = false;
            std::stringstream ss; //The actual msg

        };

        // Actor logging
        static void log_a(caf::local_actor *local_actor_thiz, const std::string &category, const std::string &class_name,
            const std::string &filename, unsigned int line_no, const std::string &msg, const std::string &func_name,
            const logger::verbosity_t priority);

        // Logging with just a system
        static void log_s(caf::actor_system &sys, const std::string &category, const std::string &class_name,
                          const std::string &filename, unsigned int line_no, const std::string &msg, const std::string &func_name,
                          const logger::verbosity_t priority);

        // Logging with a scoped actor (mainly just for ostream_helper)
        static void log_sc(std::unique_ptr<caf::scoped_actor> &scoped_actor_thiz, const std::string &category, const std::string &class_name,
                          const std::string &filename, unsigned int line_no, const std::string &msg, const std::string &func_name,
                          const logger::verbosity_t priority);

        static std::string format_line(const std::string &actor_id_str, const std::string &category, const std::string &class_name,
            const std::string &filename, unsigned int line_no, const std::string &msg,
            const std::string &func_name, const logger::verbosity_t priority);

        // Override for logging when an actor is around.
        template<typename T>
        static void trace_a(T *thiz, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                           unsigned int line_no = 0, const std::string &filename = "?") {
            log_a(thiz, category, typeid(T).name(), filename, line_no, msg, func_name, verbosity_t::trace);
        }

        template<typename Clz>
        static void trace_s(caf::actor_system &sys, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                            unsigned int line_no = 0, const std::string &filename = "?") {
            log_s(sys, category, typeid(Clz).name(), filename, line_no, msg, func_name, verbosity_t::trace);
        }

        template<typename T>
        static ostream_helper trace_aout(T *thiz, const std::string &category, const std::string &func_name = "?",
                            unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(thiz, category, typeid(T).name(), filename, line_no, func_name, verbosity_t::trace);
        }

        template<typename Clz>
        static ostream_helper trace_sout(caf::actor_system &sys, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(std::make_unique<caf::scoped_actor>(sys), category, typeid(Clz).name(), filename, line_no, func_name, verbosity_t::trace);
        }

        // Override for logging when an actor is around.
        template<typename T>
        static void debug_a(T *thiz, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                            unsigned int line_no = 0, const std::string &filename = "?") {
            log_a(thiz, category, typeid(T).name(), filename, line_no, msg, func_name, verbosity_t::debug);
        }

        template<typename Clz>
        static void debug_s(caf::actor_system &sys, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                            unsigned int line_no = 0, const std::string &filename = "?") {
            log_s(sys, category, typeid(Clz).name(), filename, line_no, msg, func_name, verbosity_t::debug);
        }

        template<typename T>
        static ostream_helper debug_aout(T *thiz, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(thiz, category, typeid(T).name(), filename, line_no, func_name, verbosity_t::debug);
        }

        template<typename Clz>
        static ostream_helper debug_sout(caf::actor_system &sys, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(std::make_unique<caf::scoped_actor>(sys), category, typeid(Clz).name(), filename, line_no, func_name, verbosity_t::debug);
        }

        // Override for logging when an actor is around.
        template<typename T>
        static void info_a(T *thiz, const std::string &msg, const std::string &category, const std::string &func_name = "?",
            unsigned int line_no = 0, const std::string &filename = "?") {
            log_a(thiz, category, typeid(T).name(), filename, line_no, msg, func_name, verbosity_t::info);
        }

        template<typename Clz>
        static void info_s(caf::actor_system &sys, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                            unsigned int line_no = 0, const std::string &filename= "?") {
            log_s(sys, category, typeid(Clz).name(), filename, line_no, msg, func_name, verbosity_t::info);
        }

        template<typename T>
        static ostream_helper info_aout(T *thiz, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(thiz, category, typeid(T).name(), filename, line_no, func_name, verbosity_t::info);
        }

        template<typename Clz>
        static ostream_helper info_sout(caf::actor_system &sys, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(std::make_unique<caf::scoped_actor>(sys), category, typeid(Clz).name(), filename, line_no, func_name, verbosity_t::info);
        }

        // Override for logging when an actor is around.
        template<typename T>
        static void warn_a(T *thiz, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                            unsigned int line_no = 0, const std::string &filename = "?") {
            log_a(thiz, category, typeid(T).name(), filename, line_no, msg, func_name, verbosity_t::warn);
        }

        template<typename Clz>
        static void warn_s(caf::actor_system &sys, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                            unsigned int line_no = 0, const std::string &filename = "?") {
            log_s(sys, category, typeid(Clz).name(), filename, line_no, msg, func_name, verbosity_t::warn);
        }

        template<typename T>
        static ostream_helper warn_aout(T *thiz, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(thiz, category, typeid(T).name(), filename, line_no, func_name, verbosity_t::warn);
        }

        template<typename Clz>
        static ostream_helper warn_sout(caf::actor_system &sys, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(std::make_unique<caf::scoped_actor>(sys), category, typeid(Clz).name(), filename, line_no, func_name, verbosity_t::warn);
        }

        // Override for logging when an actor is around.
        template<typename T>
        static void error_a(T *thiz, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                              unsigned int line_no = 0, const std::string &filename = "?") {
            log_a(thiz, category, typeid(T).name(), filename, line_no, msg, func_name, verbosity_t::error);
        }

        template<typename Clz>
        static void error_s(caf::actor_system &sys, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                            unsigned int line_no = 0, const std::string &filename = "?") {
            log_s(sys, category, typeid(Clz).name(), filename, line_no, msg, func_name, verbosity_t::error);
        }

        template<typename T>
        static ostream_helper error_aout(T *thiz, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(thiz, category, typeid(T).name(), filename, line_no, func_name, verbosity_t::error);
        }

        template<typename Clz>
        static ostream_helper error_sout(caf::actor_system &sys, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(std::make_unique<caf::scoped_actor>(sys), category, typeid(Clz).name(), filename, line_no, func_name, verbosity_t::error);
        }

        // Override for logging when an actor is around.
        template<typename T>
        static void fatal_a(T *thiz, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                              unsigned int line_no = 0, const std::string &filename = "?") {
            log_a(thiz, category, typeid(T).name(), filename, line_no, msg, func_name, verbosity_t::fatal);
        }

        template<typename Clz>
        static void fatal_s(caf::actor_system &sys, const std::string &msg, const std::string &category, const std::string &func_name = "?",
                            unsigned int line_no = 0, const std::string &filename = "?") {
            log_s(sys, category, typeid(Clz).name(), filename, line_no, msg, func_name, verbosity_t::fatal);
        }

        template<typename T>
        static ostream_helper fatal_aout(T *thiz, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(thiz, category, typeid(T).name(), filename, line_no, func_name, verbosity_t::fatal);
        }

        template<typename Clz>
        static ostream_helper fatal_sout(caf::actor_system &sys, const std::string &category, const std::string &func_name = "?",
                                         unsigned int line_no = 0, const std::string &filename = "?") {
            return ostream_helper(std::make_unique<caf::scoped_actor>(sys), category, typeid(Clz).name(), filename, line_no, func_name, verbosity_t::fatal);
        }


    };


} //ns as::common
