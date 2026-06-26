#include "ArgParser/ArgParser.hpp"
#include "ServerException/ServerException.hpp"
#include <criterion/criterion.h>
#include <string>
#include <vector>

/// @brief Builds a fake argv array from a list of tokens (token[0] is the program name).
static std::vector<char *> makeArgv(const std::vector<std::string> &tokens, std::vector<std::string> &storage)
{
    storage = tokens;
    std::vector<char *> argv;
    for (auto &s : storage)
        argv.push_back(s.data());
    return argv;
}

Test(ArgParser, parses_int_string_and_list_flags)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-p", "4242", "-c", "foo", "-n", "team1", "team2"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::INT, true, "port");
    parser.registerFlag("-c", zappy::FlagType::STRING, false, "client");
    parser.registerFlag("-n", zappy::FlagType::LIST, true, "teams");

    parser.parse();

    cr_assert_eq(parser.getInt("-p"), 4242);
    cr_assert_str_eq(parser.getString("-c").c_str(), "foo");
    auto teams = parser.getList("-n");
    cr_assert_eq(teams.size(), 2u);
    cr_assert_str_eq(teams[0].c_str(), "team1");
    cr_assert_str_eq(teams[1].c_str(), "team2");
}

Test(ArgParser, hasFlag_true_when_present_false_when_absent)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-p", "4242"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::INT, true, "port");
    parser.registerFlag("-c", zappy::FlagType::INT, false, "clients");
    parser.parse();

    cr_assert(parser.hasFlag("-p"));
    cr_assert_not(parser.hasFlag("-c"));
}

Test(ArgParser, missing_required_flag_throws)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::INT, true, "port");

    cr_assert_throw(parser.parse(), zappy::ServerException);
}

Test(ArgParser, optional_flag_absent_does_not_throw)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-p", "4242"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::INT, true, "port");
    parser.registerFlag("-c", zappy::FlagType::INT, false, "clients");

    cr_assert_no_throw(parser.parse());
}

Test(ArgParser, unknown_flag_throws)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-z", "value"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::INT, false, "port");

    cr_assert_throw(parser.parse(), zappy::ServerException);
}

Test(ArgParser, value_without_preceding_flag_throws)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "4242"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    cr_assert_throw(parser.parse(), zappy::ServerException);
}

Test(ArgParser, int_flag_missing_value_throws)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-p"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::INT, true, "port");

    cr_assert_throw(parser.parse(), zappy::ServerException);
}

Test(ArgParser, int_flag_followed_by_another_flag_throws)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-p", "-c"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::INT, true, "port");
    parser.registerFlag("-c", zappy::FlagType::INT, false, "clients");

    cr_assert_throw(parser.parse(), zappy::ServerException);
}

Test(ArgParser, list_flag_with_no_values_throws)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-n", "-c", "5"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-n", zappy::FlagType::LIST, true, "teams");
    parser.registerFlag("-c", zappy::FlagType::INT, false, "clients");

    cr_assert_throw(parser.parse(), zappy::ServerException);
}

Test(ArgParser, getInt_on_non_numeric_value_throws)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-p", "notanumber"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::INT, true, "port");
    parser.parse();

    cr_assert_throw(parser.getInt("-p"), zappy::ServerException);
}

Test(ArgParser, getter_on_unparsed_flag_throws)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::INT, false, "port");
    parser.parse();

    cr_assert_throw(parser.getInt("-p"), zappy::ServerException);
    cr_assert_throw(parser.getString("-p"), zappy::ServerException);
    cr_assert_throw(parser.getList("-p"), zappy::ServerException);
}

Test(ArgParser, empty_token_throws)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", ""}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    cr_assert_throw(parser.parse(), zappy::ServerException);
}

Test(ArgParser, registering_the_same_flag_twice_keeps_the_latest_configuration)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-p", "4242"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-p", zappy::FlagType::STRING, false, "first");
    parser.registerFlag("-p", zappy::FlagType::INT, true, "second");
    parser.parse();

    cr_assert_eq(parser.getInt("-p"), 4242);
}

Test(ArgParser, single_value_list_flag_is_accepted)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-n", "team1"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-n", zappy::FlagType::LIST, true, "teams");
    parser.parse();

    auto teams = parser.getList("-n");
    cr_assert_eq(teams.size(), 1u);
    cr_assert_str_eq(teams[0].c_str(), "team1");
}

Test(ArgParser, no_flags_at_all_and_none_required_parses_cleanly)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    cr_assert_no_throw(parser.parse());
    cr_assert_not(parser.hasFlag("-p"));
}

Test(ArgParser, list_flag_stops_at_next_flag)
{
    std::vector<std::string> storage;
    auto argv = makeArgv({"zappy_server", "-n", "team1", "team2", "-p", "4242"}, storage);
    zappy::ArgParser parser(static_cast<int>(argv.size()), argv.data());

    parser.registerFlag("-n", zappy::FlagType::LIST, true, "teams");
    parser.registerFlag("-p", zappy::FlagType::INT, true, "port");
    parser.parse();

    auto teams = parser.getList("-n");
    cr_assert_eq(teams.size(), 2u);
    cr_assert_eq(parser.getInt("-p"), 4242);
}
