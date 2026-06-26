#include "ServerException/ServerException.hpp"
#include <criterion/criterion.h>
#include <exception>
#include <source_location>
#include <string>

// ── construction & what() content ────────────────────────────────────────────

Test(ServerException, what_contains_message)
{
    zappy::ServerException ex("something went wrong");
    std::string result(ex.what());
    cr_assert(result.find("something went wrong") != std::string::npos, "what() must contain the original message");
}

Test(ServerException, what_contains_file_name)
{
    auto loc = std::source_location::current();
    zappy::ServerException ex("file check", loc);
    std::string result(ex.what());
    // The file_name stored in loc must appear somewhere in the output.
    cr_assert(result.find(loc.file_name()) != std::string::npos, "what() must contain the source file name");
}

Test(ServerException, what_contains_line_number)
{
    auto loc = std::source_location::current();
    zappy::ServerException ex("line check", loc);
    std::string result(ex.what());
    cr_assert(result.find(std::to_string(loc.line())) != std::string::npos, "what() must contain the source line number");
}

Test(ServerException, what_contains_column_number)
{
    auto loc = std::source_location::current();
    zappy::ServerException ex("col check", loc);
    std::string result(ex.what());
    cr_assert(result.find(std::to_string(loc.column())) != std::string::npos, "what() must contain the source column number");
}

Test(ServerException, what_is_noexcept)
{
    // Verify the noexcept promise at compile time.
    static_assert(noexcept(std::declval<zappy::ServerException>().what()), "what() must be noexcept");
}

// ── inheritance ───────────────────────────────────────────────────────────────

Test(ServerException, is_catchable_as_std_exception)
{
    bool caught = false;
    try
    {
        throw zappy::ServerException("std::exception catch");
    }
    catch (const std::exception &e)
    {
        caught = true;
        std::string msg(e.what());
        cr_assert(msg.find("std::exception catch") != std::string::npos);
    }
    cr_assert(caught, "ServerException must be catchable as std::exception");
}

Test(ServerException, is_catchable_as_server_exception)
{
    bool caught = false;
    try
    {
        throw zappy::ServerException("direct catch");
    }
    catch (const zappy::ServerException &e)
    {
        caught = true;
        std::string msg(e.what());
        cr_assert(msg.find("direct catch") != std::string::npos);
    }
    cr_assert(caught);
}

// ── edge cases ────────────────────────────────────────────────────────────────

Test(ServerException, empty_message)
{
    // Empty string must not crash; what() must still return a valid C-string.
    zappy::ServerException ex("");
    const char *result = ex.what();
    cr_assert_not_null(result, "what() must never return nullptr");
}

Test(ServerException, message_with_special_characters)
{
    const std::string special = "error: file \"foo/bar.zp\" line 42 <col=7>";
    zappy::ServerException ex(special);
    std::string result(ex.what());
    cr_assert(result.find(special) != std::string::npos, "what() must preserve special characters in the message");
}

Test(ServerException, explicit_location_overrides_call_site)
{
    // Build a synthetic location with known values.
    // std::source_location::current() is a consteval call; we capture it here
    // to have a stable, inspectable value to pass to the constructor.
    constexpr std::uint_least32_t expected_line = 99;
    // We cannot set line() arbitrarily at runtime; instead, verify that an
    // explicitly supplied location is reflected in what() rather than the
    // constructor's own call site.
    auto explicit_loc = std::source_location::current(); // line ~99-ish
    zappy::ServerException ex("explicit loc", explicit_loc);
    std::string result(ex.what());
    cr_assert(result.find(std::to_string(explicit_loc.line())) != std::string::npos, "what() must use the explicitly supplied source_location");
    (void)expected_line;
}

// ── static-result note (known implementation behaviour) ──────────────────────

// The current implementation uses a `static std::string` inside what(), so
// every call to what() across ALL instances returns the string built during
// the very first call.  The tests above each run in a separate Criterion
// process, so they are not affected.  This test documents the behaviour
// explicitly so a future refactor can verify the fix.
Test(ServerException, static_result_returns_first_call_value)
{
    // First exception constructed and queried in this process.
    zappy::ServerException first("FIRST_MESSAGE");
    const char *first_result = first.what();
    cr_assert_not_null(first_result);

    // A second exception with a different message.
    zappy::ServerException second("SECOND_MESSAGE");
    const char *second_result = second.what();

    // Due to the static variable, both calls return the same pointer/content.
    // This test documents the current (arguably buggy) behaviour.
    cr_assert_str_eq(first_result, second_result, "known behaviour: static result means what() always returns the first-seen value");
}
