#include <Server.h>
#include <boost/log/trivial.hpp>

unsigned long long Server::m_last_id = first_id_;
std::queue<std::pair<std::string, std::string>> Server::m_messages = std::queue<std::pair<std::string, std::string>>();
boost::asio::io_service Server::m_service;
std::list<std::shared_ptr<Client>> Server::m_clientsList;
std::mutex Server::m_mtx;

void Server::accept_thread(int port) {
    try {

        BOOST_LOG_TRIVIAL(debug) << "Thread accept_thread enable";
        std::string greetings = "Welcome to Simple TCP Chat\n"
                                "What is your name : ";

        boost::asio::ip::tcp::acceptor acceptor(m_service,
                                                boost::asio::ip::tcp::endpoint(
                                                        boost::asio::ip::tcp::v4(), port));
        while (true) {
            auto client = std::make_shared<Client>(m_messages, m_service, m_last_id);
            m_last_id++;

            acceptor.accept(client->sock());
            client->update_ping();
            BOOST_LOG_TRIVIAL(info) << "USER id:" << client->get_id() << " connect to server";
            client->write(greetings);
            BOOST_LOG_TRIVIAL(trace) << "greetings send";

            m_mtx.lock();
            BOOST_LOG_TRIVIAL(trace) << "m_mtx lock tread accept_thread";

            m_clientsList.push_back(client);
            BOOST_LOG_TRIVIAL(debug) << "USER id:" << client->get_id() << " add to ClientsList";
            BOOST_LOG_TRIVIAL(debug) << "ClientsList size = " << m_clientsList.size();

            m_mtx.unlock();
            BOOST_LOG_TRIVIAL(trace) << "m_mtx unlock tread accept_thread";

            boost::this_thread::sleep(boost::posix_time::millisec(1));
        }
    }catch(const std::exception & e){
        BOOST_LOG_TRIVIAL(fatal)<<"FATAL ERROR accept_thread: " << e.what();
        exit(1);
    }
}

void Server::handle_clients_thread() {
    try {
        BOOST_LOG_TRIVIAL(debug) << "Thread handle_clients_thread enable";
        while (true) {
            boost::this_thread::sleep(boost::posix_time::millisec(1));

            m_mtx.lock();
            BOOST_LOG_TRIVIAL(trace) << "m_mtx lock tread handle_clients_thread";

            if (m_clientsList.empty()) {
                m_mtx.unlock();
                BOOST_LOG_TRIVIAL(trace) << "m_mtx unlock tread handle_clients_thread";
                BOOST_LOG_TRIVIAL(debug) << "m_clientsList empty -> trace handle_clients_thread sleep "
                                         << handle_clients_thread_sleep_ << "ms";
                boost::this_thread::sleep(boost::posix_time::millisec((long long) handle_clients_thread_sleep_));
                continue;
            }

            for (const auto &x: m_clientsList) {
                x->answer_to_client();
            }

            m_clientsList.erase(
                    std::remove_if(
                            m_clientsList.begin(),
                            m_clientsList.end(),
                            [&](const std::shared_ptr<Client> &Client) -> bool {
                                if (Client->get_user_exit()) {
                                    BOOST_LOG_TRIVIAL(info) << "USER id:" << Client->get_id() << " delete";
                                    m_messages.emplace("Server", Client->get_username() + " leave the chat");
                                };
                                return Client->get_user_exit();
                            }),
                    m_clientsList.end());

            while (!m_messages.empty()) {
                std::string msg = m_messages.front().first + ": " + m_messages.front().second;
                for (const auto &x: m_clientsList) {
                    if (x->user_is_ok())
                        x->write(msg);
                }
                BOOST_LOG_TRIVIAL(trace) << "message " << msg << " send";
                m_messages.pop();
            }

            m_mtx.unlock();
            BOOST_LOG_TRIVIAL(trace) << "m_mtx unlock tread handle_clients_thread";
        }
    }catch(const std::exception & e){
        BOOST_LOG_TRIVIAL(fatal)<<"FATAL ERROR handle_clients_thread: " << e.what();
        exit(2);
    }
}
