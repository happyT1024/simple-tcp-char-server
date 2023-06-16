#include <boost/thread.hpp>
#include "Server.h"
#include "init_log.h"
#include "signalHandler.h"
#include "settings.h"

int main(int argc, char *argv[]) {
    signal(SIGTERM, signalHandler); // обработка стандартного завершения работы программы
    signal(SIGINT, signalHandler);

    std::string config_filename("config/debug.json");

    int port = 8001;
    int logs = 1;
//    fint port = 8001;
//    int logs = 1;
    for(int i=1;i<argc;++i){
        if(std::string(argv[i])=="-p"){
            i++;
            port = std::atoi(argv[i]);
            continue;
        }
        if(std::string(argv[i])=="-l"){
            i++;
            logs = std::atoi(argv[i]);
            continue;
        }
    }


    switch (logs) {
        case 0:
            init_deep_debug_log();
            break;
        case 1:
            init_debug_log();
            break;
        case 2:
            init_production_log();
            break;
        default:
            std::cout<<"Неправильные параметры запуска\n";
            return 1;
    }

    logging::add_common_attributes();

    BOOST_LOG_TRIVIAL(info)<<"Server started at port: "<<port<<" logs type: "<<logs;

    boost::thread_group threads;
    threads.create_thread(boost::bind( Server::accept_thread, port));
    threads.create_thread(Server::handle_clients_thread);
    threads.join_all();

    return 0;
}
