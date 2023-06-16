//
// Created by matvey on 09.04.23.
//

#include "Server.h"

unsigned long long Server::last_id_ = first_id_;
std::queue<std::pair<std::string, std::string>> Server::messages_ = std::queue<std::pair<std::string, std::string>>();
boost::asio::io_service Server::service_;
std::list<std::shared_ptr<Client>> Server::clientsList_;
std::mutex Server::mtx;

void Server::accept_thread(int port) {
    try {
        BOOST_LOG_TRIVIAL(debug) << "Thread accept_thread enable";
        std::string greetings = "Welcome to Simple TCP Chat\n"
                                "What is your name : ";

        boost::asio::ip::tcp::acceptor acceptor(service_,
                                                boost::asio::ip::tcp::endpoint(
                                                        boost::asio::ip::tcp::v4(), port));
        while (true) {
            std::shared_ptr<Client> client(new Client(messages_, service_, last_id_));
            last_id_++;

            acceptor.accept(client->sock());
            client->update_ping();
            BOOST_LOG_TRIVIAL(info) << "USER id:" << client->get_id() << " connect to server";
            client->write(greetings);
            BOOST_LOG_TRIVIAL(trace) << "greetings send";

            mtx.lock();
            BOOST_LOG_TRIVIAL(trace) << "mtx lock tread accept_thread";

            clientsList_.push_back(client);
            BOOST_LOG_TRIVIAL(debug) << "USER id:" << client->get_id() << " add to ClientsList";
            BOOST_LOG_TRIVIAL(debug) << "ClientsList size = " << clientsList_.size();

            mtx.unlock();
            BOOST_LOG_TRIVIAL(trace) << "mtx unlock tread accept_thread";

            boost::this_thread::sleep(boost::posix_time::millisec(1));
        }
    }catch(std::exception e){
        BOOST_LOG_TRIVIAL(fatal)<<"FATAL ERROR accept_thread: " << e.what();
        exit(1);
    }
}

void Server::handle_clients_thread() {
    try {
        BOOST_LOG_TRIVIAL(debug) << "Thread handle_clients_thread enable";
        while (true) {
            boost::this_thread::sleep(boost::posix_time::millisec(1));

            mtx.lock();
            BOOST_LOG_TRIVIAL(trace) << "mtx lock tread handle_clients_thread";

            if (clientsList_.empty()) {
                mtx.unlock();
                BOOST_LOG_TRIVIAL(trace) << "mtx unlock tread handle_clients_thread";
                BOOST_LOG_TRIVIAL(debug) << "clientsList_ empty -> trace handle_clients_thread sleep "
                                         << handle_clients_thread_sleep_ << "ms";
                boost::this_thread::sleep(boost::posix_time::millisec((long long) handle_clients_thread_sleep_));
                continue;
            }

            for (const auto &x: clientsList_) {
                x->answer_to_client();
            }

            clientsList_.erase(
                    std::remove_if(
                            clientsList_.begin(),
                            clientsList_.end(),
                            [&](const std::shared_ptr<Client> &Client) -> bool {
                                if (Client->get_user_exit()) {
                                    BOOST_LOG_TRIVIAL(info) << "USER id:" << Client->get_id() << " delete";
                                    messages_.emplace("Server", Client->get_username() + " leave the chat");
                                };
                                return Client->get_user_exit();
                            }),
                    clientsList_.end());

            while (!messages_.empty()) {
                std::string msg = messages_.front().first + ": " + messages_.front().second;
                for (const auto &x: clientsList_) {
                    if (x->user_is_ok())
                        x->write(msg);
                }
                BOOST_LOG_TRIVIAL(trace) << "message " << msg << " send";
                messages_.pop();
            }

            mtx.unlock();
            BOOST_LOG_TRIVIAL(trace) << "mtx unlock tread handle_clients_thread";
        }
    }catch(std::exception e){
        BOOST_LOG_TRIVIAL(fatal)<<"FATAL ERROR handle_clients_thread: " << e.what();
        exit(2);
    }
}
