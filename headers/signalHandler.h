//
// Created by matvey on 17.04.23.
//

#ifndef SIMPLETCPCHAT_SIGNALHANDLER_H
#define SIMPLETCPCHAT_SIGNALHANDLER_H

#include <csignal>
#include "init_log.h"

void signalHandler(int signum) {
    BOOST_LOG_TRIVIAL(info) << "Interrupt signal (" << signum << ") received";
    exit(signum);
}

#endif //SIMPLETCPCHAT_SIGNALHANDLER_H
