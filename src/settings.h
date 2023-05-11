//
// Created by matvey on 08.05.23.
//

#ifndef SIMPLETCPCHAT_SETTINGS_H
#define SIMPLETCPCHAT_SETTINGS_H

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/log/trivial.hpp>

namespace pt = boost::property_tree;

class settings {
private:
    std::string l_filename;
    int l_level;

    struct Server {
        int port;
        std::string name;
        int max_msg;
        int timeout;
        int max_username;

        void setName(const std::string &name);

        void setPort(int port);

        void setMaxMsg(int maxMsg);

        void setTimeout(int timeout);

        void setMaxUsername(int maxUsername);
    };

    std::vector<Server> servers;
public:
    void load(const std::string &filename) {
        pt::ptree tree;
        try {
            pt::read_xml(filename, tree);
        } catch (boost::property_tree::xml_parser_error &e) {
            BOOST_LOG_TRIVIAL(fatal) << "Конфигурационный файл " << filename << " не найден";
            exit(3);
        }

        l_level = tree.get("settings.l_level", 2);
        l_filename = tree.get<std::string>("settings.l_filename", "debug.log");


        try {
            BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("settings.servers")) {
                            Server srv;
                            try {
                                srv.setName(v.second.get<std::string>("<xmlattr>.name"));
                                srv.setPort(v.second.get<int>("port"));
                            } catch (boost::property_tree::xml_parser_error &e) {
                                BOOST_LOG_TRIVIAL(fatal)
                                    << "У сервера в конфигурационном файле не определено имя или порт";
                                exit(3);
                            }

                            srv.setMaxMsg(v.second.get<int>("max_msg", 1024));
                            srv.setMaxUsername(v.second.get<int>("max_username", 12));
                            srv.setTimeout(v.second.get<int>("timeout", 60000));

                            servers.push_back(srv);
                        }
        } catch (boost::property_tree::xml_parser_error &e) {
            BOOST_LOG_TRIVIAL(fatal) << "При попытке распарсить settings.servers произошла ошибка";
            exit(3);
        }
        if(!settingsIsCorrect()){
            BOOST_LOG_TRIVIAL(fatal) << "Попытка запустить с невозможными настройками в конфигурационном файле";
            exit(3);
        }
    }

private:
    bool settingsIsCorrect() const {
        std::unordered_set<int> ports;
        for (const auto &srv: servers) {
            if (ports.find(srv.port) == ports.end()) {
                ports.insert(srv.port);
            } else {
                BOOST_LOG_TRIVIAL(fatal) << "Два одинаковых порта в конфигурационном файле";
                return false;
            }
            if(srv.port<1000){
                return false;
            }
            if(srv.max_username<1){
                return false;
            }
            if(srv.max_msg<2){
                return false;
            }
            if(srv.timeout<1){
                return false;
            }
            if(srv.name.empty()){
                return false;
            }
        }
        return true;
    }
};


#endif //SIMPLETCPCHAT_SETTINGS_H
