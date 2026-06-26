#include "Network/Client/Client.hpp"
#include "Network/Client/ClientType.hpp"
#include <criterion/criterion.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

/// @brief Creates a connected socket pair, returns the two ends.
static void makeSocketPair(int &a, int &b)
{
    int sv[2];
    cr_assert_eq(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
    a = sv[0];
    b = sv[1];
}

Test(Client, default_type_is_unknown)
{
    int a, b;
    makeSocketPair(a, b);
    zappy::Client client(a);

    cr_assert_eq(client.getType(), zappy::ClientType::UNKNOWN);
    close(b);
}

Test(Client, constructor_can_set_an_explicit_type)
{
    int a, b;
    makeSocketPair(a, b);
    zappy::Client client(a, zappy::ClientType::AI);

    cr_assert_eq(client.getType(), zappy::ClientType::AI);
    close(b);
}

Test(Client, getFd_returns_the_wrapped_descriptor)
{
    int a, b;
    makeSocketPair(a, b);
    zappy::Client client(a);

    cr_assert_eq(client.getFd(), a);
    close(b);
}

Test(Client, setType_changes_the_role)
{
    int a, b;
    makeSocketPair(a, b);
    zappy::Client client(a);

    client.setType(zappy::ClientType::GRAPHIC);
    cr_assert_eq(client.getType(), zappy::ClientType::GRAPHIC);

    client.setType(zappy::ClientType::DEAD);
    cr_assert_eq(client.getType(), zappy::ClientType::DEAD);
    close(b);
}

Test(Client, buffer_starts_empty)
{
    int a, b;
    makeSocketPair(a, b);
    zappy::Client client(a);

    cr_assert(client.getBuffer().empty());
    close(b);
}

Test(Client, const_getBuffer_overload_reflects_the_same_value)
{
    int a, b;
    makeSocketPair(a, b);
    zappy::Client client(a);
    client.setBuffer("Look\n");

    const zappy::Client &constClient = client;
    cr_assert_str_eq(constClient.getBuffer().c_str(), "Look\n");
    close(b);
}

Test(Client, getType_via_const_reference)
{
    int a, b;
    makeSocketPair(a, b);
    zappy::Client client(a, zappy::ClientType::GRAPHIC);

    const zappy::Client &constClient = client;
    cr_assert_eq(constClient.getType(), zappy::ClientType::GRAPHIC);
    close(b);
}

Test(Client, setBuffer_replaces_pending_buffer)
{
    int a, b;
    makeSocketPair(a, b);
    zappy::Client client(a);

    client.setBuffer("Forward\n");
    cr_assert_str_eq(client.getBuffer().c_str(), "Forward\n");

    client.setBuffer("");
    cr_assert(client.getBuffer().empty());
    close(b);
}

Test(Client, mutable_buffer_reference_can_be_appended_to)
{
    int a, b;
    makeSocketPair(a, b);
    zappy::Client client(a);

    client.getBuffer() += "Right\n";
    cr_assert_str_eq(client.getBuffer().c_str(), "Right\n");
    close(b);
}

Test(Client, destructor_closes_the_owned_fd)
{
    int a, b;
    makeSocketPair(a, b);

    {
        zappy::Client client(a);
        (void)client;
    }
    // The fd was closed by the destructor: writing from the peer should now fail
    // to be read back successfully (the peer gets EOF/ECONNRESET on its end).
    char buf[1] = {0};
    ssize_t res = write(a, buf, 1);
    cr_assert_eq(res, -1);
    cr_assert_eq(errno, EBADF);
    close(b);
}
