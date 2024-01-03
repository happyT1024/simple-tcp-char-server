#pragma once

#include <init_log.h>

void signalHandler(int signum) {
    BOOST_LOG_TRIVIAL(info) << "Interrupt signal (" << signum << ") received";
    std::exit(signum);
}