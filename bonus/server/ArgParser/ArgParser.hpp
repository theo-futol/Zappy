#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "../ServerException/ServerException.hpp"

namespace zappy
{
    /// @brief Describes the value shape expected for a registered flag.
    enum class FlagType
    {
        INT,    ///< Single integer value (e.g. `-p 4242`)
        STRING, ///< Single string value  (e.g. `-c foo`)
        LIST,   ///< One or more string values until the next flag (e.g. `-n team1 team2`)
        FLAG    ///< No value, presence is the signal (e.g. `-oldgen`)
    };

    /// @brief Metadata attached to a registered flag.
    struct FlagConfig
    {
        FlagType    type;        ///< Expected value shape
        bool        required;    ///< Whether the flag must be present for parse() to succeed
        std::string description; ///< Human-readable label used in error messages
    };

    /// @brief Command-line argument parser with runtime flag registration.
    ///
    /// Flags must be registered with registerFlag() before calling parse().
    /// Values are then retrieved through the typed getters (getInt, getString, getList).
    /// Example:
    /// @code
    /// zappy::ArgParser parser(argc, argv);
    /// parser.registerFlag("-p", zappy::FlagType::INT,  true,  "port");
    /// parser.registerFlag("-n", zappy::FlagType::LIST, true,  "team names");
    /// parser.registerFlag("-c", zappy::FlagType::INT,  false, "client nb");
    /// parser.parse();
    ///
    /// int  port  = parser.getInt("-p");
    /// auto teams = parser.getList("-n");
    /// @endcode
    class ArgParser
    {
    public:
        /// @brief Stores argc/argv for later parsing. Does not parse immediately.
        /// @param argc Argument count from main().
        /// @param argv Argument vector from main().
        ArgParser(int argc, char **argv);

        /// @brief Registers a flag so that parse() knows how to handle it.
        /// @param flag        The flag token (e.g. `"-p"`).
        /// @param type        Value shape: INT, STRING, or LIST.
        /// @param required    If true, parse() throws when the flag is absent.
        /// @param description Human-readable label used in missing-flag error messages.
        void registerFlag(const std::string &flag, FlagType type, bool required,
                          const std::string &description = "");

        /// @brief Parses argv against all registered flags.
        ///
        /// Iterates argv, matches each `-flag` against registered configs, consumes
        /// its value(s), then verifies all required flags are present.
        ///
        /// @throws zappy::Exception on unknown flag, missing value, or missing required flag.
        void parse();

        /// @brief Returns true if the flag was present in argv after parse().
        /// @param flag The flag token (e.g. `"-c"`).
        bool hasFlag(const std::string &flag) const;

        /// @brief Returns the parsed value of an INT flag as an integer.
        /// @param flag The flag token (e.g. `"-p"`).
        /// @throws zappy::Exception if the flag was not parsed or the value is not a valid integer.
        int getInt(const std::string &flag) const;

        /// @brief Returns the parsed value of a STRING flag.
        /// @param flag The flag token (e.g. `"-c"`).
        /// @throws zappy::Exception if the flag was not parsed.
        std::string getString(const std::string &flag) const;

        /// @brief Returns all values collected for a LIST flag.
        /// @param flag The flag token (e.g. `"-n"`).
        /// @throws zappy::Exception if the flag was not parsed.
        std::vector<std::string> getList(const std::string &flag) const;

    private:
        int    _argc;
        char **_argv;

        /// @brief Maps each registered flag token to its configuration.
        std::unordered_map<std::string, FlagConfig> _flagConfigs;

        /// @brief Maps each parsed flag token to its collected value(s).
        std::unordered_map<std::string, std::vector<std::string>> _parsedValues;
    };
} // namespace zappy
