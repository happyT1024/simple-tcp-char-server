#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>
#include <cstring>

#include <boost/asio.hpp>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define ANSI_RESET   "\033[0m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_GRAY    "\033[90m"
#define ANSI_BOLD    "\033[1m"

static std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running = false;
}

static void print_banner() {
    std::cout << ANSI_CYAN ANSI_BOLD
              << "╔══════════════════════════════════╗\n"
              << "║     Simple TCP Chat  v0.2        ║\n"
              << "║     TLS 1.3  encrypted           ║\n"
              << "╚══════════════════════════════════╝"
              << ANSI_RESET "\n\n";
}

/// Вывести строку с ANSI-цветом: Server — синим, остальные — зелёным.
static void print_colored(const std::string &line) {
    if (line.empty()) {
        std::cout << '\n';
        return;
    }

    auto sep = line.find(": ");
    if (sep == std::string::npos) {
        std::cout << line;
        return;
    }

    std::string name = line.substr(0, sep);
    std::string msg  = line.substr(sep + 2);

    if (name == "Server") {
        std::cout << ANSI_CYAN << name << ANSI_RESET ": " << msg;
    } else {
        std::cout << ANSI_GREEN << name << ANSI_RESET ": " << msg;
    }
}

/// Фоновый поток: читает данные из TLS-сокета, разбивает по '\n' и выводит цветом.
void read_thread(SSL *ssl) {
    std::string buf(4096, '\0');
    while (g_running) {
        int ret = SSL_read(ssl, buf.data(), static_cast<int>(buf.size()));
        if (ret <= 0) {
            int err = SSL_get_error(ssl, ret);
            if (err != SSL_ERROR_SYSCALL || errno != 0) {
                std::cerr << "SSL read error: " << err << std::endl;
            }
            g_running = false;
            break;
        }

        std::size_t start = 0;
        for (std::size_t i = 0; i < static_cast<std::size_t>(ret); ++i) {
            if (buf[i] == '\n') {
                print_colored(buf.substr(start, i - start));
                std::cout << '\n';
                start = i + 1;
            }
        }
        if (start < static_cast<std::size_t>(ret)) {
            print_colored(buf.substr(start, ret - start));
        }
        std::cout.flush();
    }
}

struct ClientConfig {
    std::string host = "127.0.0.1";
    int port = 8001;
};

/// Разобрать аргументы -h HOST и -p PORT.
static ClientConfig parse_args(int argc, char *argv[]) {
    ClientConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-h" && i + 1 < argc) {
            cfg.host = argv[++i];
        } else if (arg == "-p" && i + 1 < argc) {
            cfg.port = std::atoi(argv[++i]);
        }
    }
    return cfg;
}

/// Инициализировать OpenSSL и создать TLS-контекст клиента (без верификации сертификата).
static SSL_CTX* init_openssl_client() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        std::cerr << "Unable to create SSL context" << std::endl;
        return nullptr;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    return ctx;
}

static boost::asio::ip::tcp::socket connect_to_server(boost::asio::io_service & service,
                                                       const std::string & host, int port) {
    boost::asio::ip::tcp::resolver resolver(service);
    boost::asio::ip::tcp::socket sock(service);
    boost::asio::connect(sock, resolver.resolve(host, std::to_string(port)));
    return sock;
}

/// Выполнить TLS handshake. При ошибке — вывести в cerr и вернуть nullptr.
static SSL* do_ssl_connect(SSL_CTX *ctx, boost::asio::ip::tcp::socket & sock) {
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock.native_handle());
    if (SSL_connect(ssl) <= 0) {
        std::cerr << "SSL handshake failed" << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return nullptr;
    }
    return ssl;
}

/// Цикл ввода: читает строки из stdin и отправляет через TLS.
static void run_input_loop(SSL *ssl) {
    std::string input;
    while (g_running && std::getline(std::cin, input)) {
        input += '\n';
        SSL_write(ssl, input.data(), static_cast<int>(input.size()));
        std::cout << ANSI_YELLOW "> " ANSI_RESET;
    }
}

int main(int argc, char *argv[]) {
    ClientConfig cfg = parse_args(argc, argv);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    SSL_CTX *ctx = init_openssl_client();
    if (!ctx) return 1;

    try {
        boost::asio::io_service service;
        boost::asio::ip::tcp::socket sock = connect_to_server(service, cfg.host, cfg.port);

        SSL *ssl = do_ssl_connect(ctx, sock);
        if (!ssl) {
            SSL_CTX_free(ctx);
            return 1;
        }

        print_banner();

        std::thread reader(read_thread, ssl);
        run_input_loop(ssl);

        g_running = false;
        reader.join();

        SSL_shutdown(ssl);
        SSL_free(ssl);
        sock.close();
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        SSL_CTX_free(ctx);
        return 1;
    }

    SSL_CTX_free(ctx);
    return 0;
}
