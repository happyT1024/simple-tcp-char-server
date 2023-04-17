#include <boost/thread.hpp>
#include "Server.h"
#include "init_log.h"
#include "signalHandler.h"

/**
 * Параметры запуска программы
 * -p port -l logs
 * Например: ./SimpleTCPChat -p 8001 -l 0
 *
 * -p: порт
 * По умолчанию 8001
 *
 * -l: логи
 * По умолчанию 2
 * 0 - все логи
 * 1 - дебаг
 * 2 - продакшен
 */

int main(int argc, char *argv[]) {

    int port = 8001;
    int logs = 2;
    for(int i=1;i<argc;++i){
        if(std::string(argv[i])=="-p"){
            i++;
            port = atoi(argv[i]);
            continue;
        }
        if(std::string(argv[i])=="-l"){
            i++;
            logs = atoi(argv[i]);
            continue;
        }
    }


    switch (logs) {
        case 0:
            init_deep_debug_log;
            break;
        case 1:
            init_debug_log();
            break;
        case 2:
            init_production_log();
            break;
        default:
            std::cout<<"\n";
            return 1;
    }

    logging::add_common_attributes();

    BOOST_LOG_TRIVIAL(info)<<"Server started at port: "<<port<<" logs type: "<<logs;

    signal(SIGTERM, signalHandler); // обработка стандартного завершения работы программы

    boost::thread_group threads;

    /**
     * В этом потоке добавляются новые клиенты
     */
    threads.create_thread(boost::bind( Server::accept_thread, port));

    /**
     * В этом потоке обрабатывается список клиентов
     */
    threads.create_thread(Server::handle_clients_thread);

    threads.join_all();

    return 0;
}
