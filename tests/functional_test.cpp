#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/socket.h>
#include <thread>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <Client.h>
#include <ClientCfg.h>

static void init_openssl() {
    static bool done = false;
    if (!done) {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        done = true;
    }
}

static SSL_CTX *create_server_ctx() {
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) return nullptr;
    SSL_CTX_set_ecdh_auto(ctx, 1);
    if (SSL_CTX_use_certificate_file(ctx, "cert/server.crt", SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        return nullptr;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "cert/server.key", SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        return nullptr;
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        SSL_CTX_free(ctx);
        return nullptr;
    }
    return ctx;
}

static SSL_CTX *create_client_ctx() {
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (ctx) SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    return ctx;
}

class TlsSocketPairTest : public ::testing::Test {
protected:
    void SetUp() override {
        init_openssl();

        ASSERT_NE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), -1);

        server_ctx = create_server_ctx();
        ASSERT_NE(server_ctx, nullptr);

        client_ctx = create_client_ctx();
        ASSERT_NE(client_ctx, nullptr);

        std::promise<void> server_ready;
        auto server_future = server_ready.get_future();

        server_thread = std::thread([this, &server_ready]() {
            server_ssl = SSL_new(server_ctx);
            SSL_set_fd(server_ssl, fds[0]);
            server_ready.set_value();
            ASSERT_GT(SSL_accept(server_ssl), 0);
        });

        server_future.wait();

        client_ssl = SSL_new(client_ctx);
        SSL_set_fd(client_ssl, fds[1]);
        ASSERT_GT(SSL_connect(client_ssl), 0);

        server_thread.join();
    }

    void TearDown() override {
        if (server_ssl) { SSL_shutdown(server_ssl); SSL_free(server_ssl); }
        if (client_ssl) { SSL_shutdown(client_ssl); SSL_free(client_ssl); }
        if (server_ctx) SSL_CTX_free(server_ctx);
        if (client_ctx) SSL_CTX_free(client_ctx);
        if (fds[0] != -1) close(fds[0]);
        if (fds[1] != -1) close(fds[1]);
    }

    int fds[2] = {-1, -1};
    SSL_CTX *server_ctx = nullptr;
    SSL_CTX *client_ctx = nullptr;
    SSL *server_ssl = nullptr;
    SSL *client_ssl = nullptr;
    std::thread server_thread;
};

TEST_F(TlsSocketPairTest, SSLHandshakeSuccess) {
    ASSERT_NE(server_ssl, nullptr);
    ASSERT_NE(client_ssl, nullptr);
    ASSERT_TRUE(SSL_is_init_finished(server_ssl));
    ASSERT_TRUE(SSL_is_init_finished(client_ssl));
}

TEST_F(TlsSocketPairTest, SendReceiveOverTLS) {
    std::string msg = "Hello from server\n";
    ASSERT_GT(SSL_write(server_ssl, msg.data(), static_cast<int>(msg.size())), 0);

    char buf[256] = {};
    int ret = SSL_read(client_ssl, buf, sizeof(buf) - 1);
    ASSERT_GT(ret, 0);
    EXPECT_EQ(std::string(buf, static_cast<std::size_t>(ret)), msg);

    std::string reply = "Hello back\n";
    ASSERT_GT(SSL_write(client_ssl, reply.data(), static_cast<int>(reply.size())), 0);

    memset(buf, 0, sizeof(buf));
    ret = SSL_read(server_ssl, buf, sizeof(buf) - 1);
    ASSERT_GT(ret, 0);
    EXPECT_EQ(std::string(buf, static_cast<std::size_t>(ret)), reply);
}

TEST_F(TlsSocketPairTest, LargeMessageOverTLS) {
    std::string large(2000, 'A');
    ASSERT_GT(SSL_write(server_ssl, large.data(), static_cast<int>(large.size())), 0);

    char buf[4096] = {};
    int total = 0;
    while (total < 2000) {
        int ret = SSL_read(client_ssl, buf + total, static_cast<int>(sizeof(buf) - total - 1));
        ASSERT_GT(ret, 0);
        total += ret;
    }
    buf[total] = '\0';
    EXPECT_EQ(std::string(buf, static_cast<std::size_t>(total)), large);
}

TEST_F(TlsSocketPairTest, BidirectionalTraffic) {
    std::string srv_msg = "Hello from server";
    std::string cli_msg = "Hello from client";

    ASSERT_GT(SSL_write(server_ssl, srv_msg.data(), static_cast<int>(srv_msg.size())), 0);
    ASSERT_GT(SSL_write(client_ssl, cli_msg.data(), static_cast<int>(cli_msg.size())), 0);

    char buf[256] = {};
    int ret = SSL_read(client_ssl, buf, sizeof(buf) - 1);
    ASSERT_GT(ret, 0);
    EXPECT_EQ(std::string(buf, static_cast<std::size_t>(ret)), srv_msg);

    memset(buf, 0, sizeof(buf));
    ret = SSL_read(server_ssl, buf, sizeof(buf) - 1);
    ASSERT_GT(ret, 0);
    EXPECT_EQ(std::string(buf, static_cast<std::size_t>(ret)), cli_msg);
}
