﻿#include "sephi/net/tcp/server/server.h"

#include "sephi/net/tcp/server/session.h"


using namespace std::chrono_literals;
using namespace std::placeholders;
using std::error_code;
using std::make_shared;
using std::make_unique;
using std::move;
using std::this_thread::sleep_for;
using std::thread;

using sephi::net::tcp::SessionId;


inline SessionId sephi::net::tcp::session_ptr_to_id(
    SessionPtr const& session_ptr)
{
    return reinterpret_cast<SessionId>(session_ptr.get());
}


sephi::net::tcp::Server::Server(
    uint16_t const port,
    ServerConnectionHandler connection_handler,
    ServerPacketHandler packet_handler)
    : socket_{io_context_},
      connection_handler_{move(connection_handler)},
      packet_handler_{move(packet_handler)}
{
    asio::ip::tcp::endpoint endpoint{asio::ip::tcp::v4(), port};
    acceptor_ =
        make_unique<asio::ip::tcp::acceptor>(io_context_, endpoint);

    do_accept();
    runner_ = thread{[this] {io_context_.run(); }};
}

sephi::net::tcp::Server::~Server()
{
    close();
    io_context_.stop();
    runner_.join();
}

void sephi::net::tcp::Server::write_to_all(Chunk const& chunk)
{
    for (auto const& session : sessions_)
        session.second->write(chunk);
}

bool sephi::net::tcp::Server::write_to(
    SessionId const session_id, Chunk const& chunk)
{
    auto session{sessions_.find(session_id)};
    if (sessions_.end() == session)
        return false;

    session->second->write(chunk);
    return true;
}

void sephi::net::tcp::Server::write_to_all_except(
    SessionId const session_id, Chunk const& chunk)
{
    for (auto const& session : sessions_)
        if (session_id != session.first)
            session.second->write(chunk);
}

void sephi::net::tcp::Server::leave(SessionPtr session, error_code const& ec)
{
    auto const session_id{session_ptr_to_id(session)};
    sessions_.erase(session_id);
    connection_handler_(session_id, ConnectionState::disconnected, ec);
}

void sephi::net::tcp::Server::close()
{
    auto const ec{error_code{}};
    for (auto iter{sessions_.begin()}; sessions_.end() != iter;) {
        iter->second->close();
        leave((iter++)->second, ec);
        sleep_for(1ms);
    }
}


void sephi::net::tcp::Server::do_accept()
{
    acceptor_->async_accept(socket_, bind(&Server::handle_accept, this, _1));
}

void sephi::net::tcp::Server::handle_accept(error_code const& ec)
{
    if (ec) {
        do_accept();
        return;
    }

    auto const session_ptr{make_shared<Session>(
        move(socket_), *this, packet_handler_)};
    auto const session_id{session_ptr_to_id(session_ptr)};

    connection_handler_(session_id, ConnectionState::connected, ec);
    sessions_.emplace(make_pair(session_id, session_ptr));
    session_ptr->start();

    do_accept();
}
