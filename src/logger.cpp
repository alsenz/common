#include "common/logger.hpp"

#include <ctime>

namespace as::common {


    std::string logger::format = "%d [%t %a] %p %c (%M) %F:%L %m%n";
    caf::timestamp logger::start = caf::make_timestamp();
    logger::verbosity_t logger::verbosity = logger::verbosity_t::info;

    std::string
    logger::format_line(const std::string &actor_id_str, const std::string &category, const std::string &class_name,
                        const std::string &filename, unsigned int line_no,
                        const std::string &msg, const std::string &func_name,
                        const logger::verbosity_t priority) {

        // Extra bits
        const auto ts = caf::make_timestamp();
        std::time_t ct = std::chrono::system_clock::to_time_t(ts);
        std::string time_str = std::ctime(&ct);
        time_str.resize(time_str.size() - 1); // Remove the newline on the end
        const auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(ts - start);
        long difference_ct = difference.count();


        std::stringstream formatted;
            enum class state_t {scan, expect};
            auto state = state_t::scan;
            for(std::size_t p = 0; p < format.size(); p++) {
                    char c= format.at(p);
                    switch(state) {
                            case state_t::scan: {
                                    if(c != '%') {
                                            formatted << c;
                                    } else {
                                            state = state_t::expect;
                                    }
                            }
                            break;
                            case state_t::expect: {
                                    switch(c) {
                                            case '%':
                                                    formatted << c;
                                            break;
                                            case 'a':
                                                    formatted << "actor-";
                                            formatted << actor_id_str;
                                            break;
                                            case 'c':
                                                    formatted << category;
                                            break;
                                            case 'C':
                                                    formatted << class_name;
                                            break;
                                            case 'd':
                                                    formatted << time_str;
                                            break;
                                            case 'F':
                                                    formatted << filename;
                                            break;
                                            case 'l':
                                                    formatted << "line-";
                                            formatted << line_no;
                                            break;
                                            case 'L':
                                                    formatted << line_no;
                                            break;
                                            case 'm':
                                                    formatted << msg;
                                            break;
                                            case 'M':
                                                    formatted << func_name;
                                            break;
                                            case 'n':
                                                    formatted << '\n';
                                            break;
                                            case 'p': {
                                                switch(priority) {
                                                    case verbosity_t::trace:
                                                        formatted << "TRACE";
                                                        break;
                                                    case verbosity_t::debug:
                                                        formatted << "DEBUG";
                                                        break;
                                                    case verbosity_t::info:
                                                        formatted << "INFO";
                                                        break;
                                                    case verbosity_t::warn:
                                                        formatted << "WARN";
                                                        break;
                                                    case verbosity_t::error:
                                                        formatted << "ERROR";
                                                        break;
                                                    default:
                                                    case verbosity_t::fatal:
                                                        formatted << "FATAL";
                                                        break;
                                                }
                                            }
                                            break;
                                            case 'r':
                                                    formatted << difference_ct;
                                            break;
                                            case 't':
                                            case 'x':
                                            case 'X':
                                                    formatted << "thread-";
                                            formatted << std::this_thread::get_id();
                                            break;
                                            default:
                                                    formatted << "<?>";
                                            break;
                                    }
                                    state = state_t::scan;
                            }
                            break;
                    }
            }
            return formatted.str();
    }

    void logger::log_a(caf::local_actor *local_actor_thiz, const std::string &category, const std::string &class_name,
                     const std::string &filename, unsigned int line_no, const std::string &msg,
                     const std::string &func_name, const logger::verbosity_t priority) {
        if(priority >= logger::verbosity) {
            std::string actor_id_str = std::to_string(local_actor_thiz->id());
            while (actor_id_str.size() < 7) {
                actor_id_str.insert(0, 1, '0'); //Fixed width it!
            }

            caf::aout(local_actor_thiz) << format_line(actor_id_str, category, class_name, filename, line_no,
                                                       msg, func_name, priority);
        }
    }

    void logger::log_s(caf::actor_system &sys, const std::string &category, const std::string &class_name,
                       const std::string &filename, unsigned int line_no, const std::string &msg,
                       const std::string &func_name, const logger::verbosity_t priority) {
        if(priority >= logger::verbosity) {
            caf::scoped_actor thiz{sys};
            caf::aout(thiz) << format_line("no-actor", category, class_name, filename, line_no,
                                           msg, func_name, priority);
        }
    }

    void logger::log_sc(std::unique_ptr<caf::scoped_actor> &scoped_actor_thiz, const std::string &category, const std::string &class_name,
                        const std::string &filename, unsigned int line_no, const std::string &msg,
                        const std::string &func_name, const logger::verbosity_t priority) {
        if(priority >= logger::verbosity) {
            caf::aout(*scoped_actor_thiz) << format_line("xxxxxxx", category, class_name, filename, line_no,
                                                         msg, func_name, priority);
        }
    }

    void logger::ostream_helper::flush() {
        if(priority >= logger::verbosity) {
            if (!flushed) {
                if (std::holds_alternative<caf::local_actor *>(thiz)) {
                    logger::log_a(std::get<caf::local_actor *>(thiz), category, class_name, filename, line_no, ss.str(),
                                  func_name, priority);
                } else if (std::holds_alternative<std::unique_ptr<caf::scoped_actor>>(thiz)) {
                    logger::log_sc(std::get<std::unique_ptr<caf::scoped_actor>>(thiz), category, class_name, filename,
                                   line_no, ss.str(), func_name, priority);
                }
                flushed = true;
            }
        }
    }

    logger::ostream_helper::~ostream_helper() {
        flush();
    }

    logger::ostream_helper::ostream_helper(caf::local_actor *local_actor_thiz, const std::string &category,
                                           const std::string &class_name, const std::string &filename,
                                           unsigned int line_no, const std::string &func_name,
                                           const logger::verbosity_t priority)
        : thiz(local_actor_thiz), category(category), class_name(class_name),
          filename(filename), line_no(line_no), func_name(func_name), priority(priority), ss() {}

    logger::ostream_helper &logger::ostream_helper::operator<<(std::ostream &(*pf)(std::ostream &)) {
        if(priority >= logger::verbosity) {
            if (pf == (std::basic_ostream<char> &(*)(std::basic_ostream<char> &)) &std::endl) {
                flush();
            } else if (pf == (std::basic_ostream<char> &(*)(std::basic_ostream<char> &)) &std::flush) {
                flush();
            } else {
                ss << pf;
            }
        }
        return *this;
    }
} //ns as::common
