//
// Created by matvey on 11.05.23.
//

#ifndef SIMPLETCPCHAT_INIT_LOG_H
#define SIMPLETCPCHAT_INIT_LOG_H

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

void init_test_log()
{
    logging::core::get()->set_filter
            (
                    logging::trivial::severity > logging::trivial::fatal
            );
}


#endif //SIMPLETCPCHAT_INIT_LOG_H
