//
// Created by matvey on 16.04.23.
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

void init_production_log()
{
    logging::add_file_log
            (
                    keywords::file_name = "logs/SimpleTCPChat_%N.log", // паттерн имени файла
                    /**
                     * ТАКОЙ НЕБОЛЬШОЙ РАЗМЕР НУЖЕН ДЛЯ ОТЛАДКИ, ТАК КАК BOOST ПИШЕТ ДАННЫЕ В ФАЙЛ КАК ТОЛЬКО ЗАКОНЧИТСЯ МЕСТО
                     */
                    keywords::rotation_size = 10*1024, // rotate каждые 10 KiB
                    keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0), // rotate каждую полночь
                    keywords::format = "[%TimeStamp%]: %Message%" // формат логов
            );

    logging::core::get()->set_filter
            (
                    logging::trivial::severity >= logging::trivial::info
            );
}

void init_debug_log()
{
    logging::core::get()->set_filter
            (
                    logging::trivial::severity >= logging::trivial::debug
            );
}

void init_deep_debug_log()
{
    logging::core::get()->set_filter
            (
                    logging::trivial::severity >= logging::trivial::trace
            );
}


#endif //SIMPLETCPCHAT_INIT_LOG_H
