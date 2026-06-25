#include "Network/Socket/Socket.hpp"
#include <criterion/criterion.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Test(Socket, default_constructed_socket_has_no_fd)
{
    zappy::Socket socket;
    cr_assert_eq(socket.getFd(), -1);
}

Test(Socket, adopting_constructor_wraps_existing_fd)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    cr_assert_geq(fd, 0);

    zappy::Socket wrapped(fd);
    cr_assert_eq(wrapped.getFd(), fd);
}

Test(Socket, const_getFd_overload_returns_the_same_descriptor)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    cr_assert_geq(fd, 0);

    zappy::Socket wrapped(fd);
    const zappy::Socket &constWrapped = wrapped;
    cr_assert_eq(constWrapped.getFd(), fd);
}

Test(Socket, create_succeeds_and_yields_a_valid_fd)
{
    zappy::Socket socket;
    bool ok = socket.create(AF_INET, SOCK_STREAM, 0);

    cr_assert(ok);
    cr_assert_geq(socket.getFd(), 0);
}

Test(Socket, create_with_invalid_domain_fails)
{
    zappy::Socket socket;
    bool ok = socket.create(-1, SOCK_STREAM, 0);

    cr_assert_not(ok);
}

Test(Socket, setSocket_replaces_the_owned_descriptor)
{
    zappy::Socket socket;
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    cr_assert_geq(fd, 0);

    socket.setSocket(fd);
    cr_assert_eq(socket.getFd(), fd);
}

Test(Socket, bind_and_listen_succeed_on_a_free_port)
{
    zappy::Socket socket;
    cr_assert(socket.create(AF_INET, SOCK_STREAM, 0));
    cr_assert(socket.bind(0)); // port 0: let the OS pick a free ephemeral port
    cr_assert(socket.listen());
}

Test(Socket, bind_without_a_created_socket_fails)
{
    zappy::Socket socket;
    cr_assert_not(socket.bind(4242));
}

Test(Socket, binding_twice_to_the_same_explicit_port_fails)
{
    zappy::Socket first;
    cr_assert(first.create(AF_INET, SOCK_STREAM, 0));
    cr_assert(first.bind(4343));
    cr_assert(first.listen());

    zappy::Socket second;
    cr_assert(second.create(AF_INET, SOCK_STREAM, 0));
    cr_assert_not(second.bind(4343));
}

Test(Socket, accept_with_no_pending_connection_on_nonblocking_socket_returns_negative)
{
    zappy::Socket socket;
    cr_assert(socket.create(AF_INET, SOCK_STREAM, 0));
    cr_assert(socket.bind(0));
    cr_assert(socket.listen());

    int flags = fcntl(socket.getFd(), F_GETFL, 0);
    fcntl(socket.getFd(), F_SETFL, flags | O_NONBLOCK);

    cr_assert_eq(socket.accept(), -1);
}

Test(Socket, accept_returns_a_valid_fd_for_a_pending_connection)
{
    zappy::Socket server;
    cr_assert(server.create(AF_INET, SOCK_STREAM, 0));
    cr_assert(server.bind(0));
    cr_assert(server.listen());

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    cr_assert_eq(getsockname(server.getFd(), (struct sockaddr *)&addr, &len), 0);

    int client = ::socket(AF_INET, SOCK_STREAM, 0);
    cr_assert_geq(client, 0);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    cr_assert_eq(connect(client, (struct sockaddr *)&addr, sizeof(addr)), 0);

    int accepted = server.accept();
    cr_assert_geq(accepted, 0);

    close(client);
    close(accepted);
}
