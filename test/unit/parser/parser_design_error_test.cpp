// SPDX-FileCopyrightText: 2006-2024, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2024, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include <sharg/parser.hpp>
#include <sharg/test/expect_throw_msg.hpp>
#include <sharg/test/test_fixture.hpp>

class design_error_test : public sharg::test::test_fixture
{};

TEST_F(design_error_test, app_name_validation)
{
    char const * argv[] = {"./parser_test", "-i", "3"};
    int const argc{3};
    int option_value{};

    auto create_parser = [&](std::string app_name)
    {
        sharg::parser parser{std::move(app_name), argc, argv, sharg::update_notifications::off};
        parser.add_option(option_value, sharg::config{.short_id = 'i'});
        return parser;
    };

    EXPECT_NO_THROW(create_parser("test_parser").parse());
    EXPECT_NO_THROW(create_parser("test-parser1234_foo").parse());

    EXPECT_THROW(create_parser("test parser").parse(), sharg::design_error);
    EXPECT_THROW(create_parser("test;").parse(), sharg::design_error);
    EXPECT_THROW(create_parser(";").parse(), sharg::design_error);
    EXPECT_THROW(create_parser("test;bad script:D").parse(), sharg::design_error);
}

// -----------------------------------------------------------------------------
// option config verification
// -----------------------------------------------------------------------------

class verify_option_config_test : public sharg::test::test_fixture
{};

TEST_F(verify_option_config_test, short_option_was_used_before)
{
    int option_value{};
    auto parser = get_parser();
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.short_id = 'i'}), sharg::design_error);
}

TEST_F(verify_option_config_test, long_option_was_used_before)
{
    int option_value{};

    auto parser = get_parser();
    parser.add_option(option_value, sharg::config{.long_id = "int"});
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "int"}), sharg::design_error);
}

TEST_F(verify_option_config_test, short_and_long_id_empty)
{
    int option_value{};

    auto parser = get_parser();
    EXPECT_THROW(parser.add_option(option_value, sharg::config{}), sharg::design_error);
}

TEST_F(verify_option_config_test, special_identifiers)
{
    int option_value{};

    auto parser = get_parser();
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.short_id = 'h'}), sharg::design_error);
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "help"}), sharg::design_error);
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "hh"}), sharg::design_error);
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "advanced-help"}), sharg::design_error);
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "export-help"}), sharg::design_error);
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "version"}), sharg::design_error);
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "copyright"}), sharg::design_error);
}

TEST_F(verify_option_config_test, single_character_long_id)
{
    int option_value{};

    auto parser = get_parser();
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "z"}), sharg::design_error);
}

TEST_F(verify_option_config_test, non_printable_characters)
{
    int option_value{};

    auto parser = get_parser();
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.short_id = '\t'}), sharg::design_error);
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "no\n"}), sharg::design_error);
    EXPECT_THROW(parser.add_option(option_value, sharg::config{.long_id = "-no"}), sharg::design_error);
}

// -----------------------------------------------------------------------------
// flag config verification
// -----------------------------------------------------------------------------

class verify_flag_config_test : public sharg::test::test_fixture
{};

TEST_F(verify_flag_config_test, default_value_is_true)
{
    bool true_value{true};

    auto parser = get_parser();
    EXPECT_THROW(parser.add_flag(true_value, sharg::config{.short_id = 'i'}), sharg::design_error);
}

TEST_F(verify_flag_config_test, short_option_was_used_before)
{
    bool flag_value{false};

    auto parser = get_parser();
    parser.add_flag(flag_value, sharg::config{.short_id = 'i'});
    EXPECT_THROW(parser.add_flag(flag_value, sharg::config{.short_id = 'i'}), sharg::design_error);
}

TEST_F(verify_flag_config_test, long_option_was_used_before)
{
    bool flag_value{false};

    auto parser = get_parser();
    parser.add_flag(flag_value, sharg::config{.long_id = "int"});
    EXPECT_THROW(parser.add_flag(flag_value, sharg::config{.long_id = "int"}), sharg::design_error);
}

TEST_F(verify_flag_config_test, short_and_long_id_empty)
{
    bool flag_value{false};

    auto parser = get_parser();
    EXPECT_THROW(parser.add_flag(flag_value, sharg::config{}), sharg::design_error);
}

TEST_F(verify_flag_config_test, single_character_long_id)
{
    bool flag_value{false};

    auto parser = get_parser();
    EXPECT_THROW(parser.add_flag(flag_value, sharg::config{.long_id = "z"}), sharg::design_error);
}

// -----------------------------------------------------------------------------
// positional option verification
// -----------------------------------------------------------------------------

class verify_positional_option_config_test : public sharg::test::test_fixture
{};

TEST_F(verify_positional_option_config_test, list_option_not_the_very_last_option)
{
    int option_value{};
    std::vector<int> vec{};

    auto parser = get_parser("arg1", "arg2", "arg3");

    parser.add_positional_option(vec, sharg::config{});
    EXPECT_THROW(parser.add_positional_option(option_value, sharg::config{}), sharg::design_error);
}

TEST_F(verify_positional_option_config_test, short_id_set)
{
    int option_value{};

    auto parser = get_parser("arg1");
    EXPECT_THROW(parser.add_positional_option(option_value, sharg::config{.short_id = 'a'}), sharg::design_error);
}

TEST_F(verify_positional_option_config_test, long_id_set)
{
    int option_value{};

    auto parser = get_parser("arg1");
    EXPECT_THROW(parser.add_positional_option(option_value, sharg::config{.long_id = "abc"}), sharg::design_error);
}

TEST_F(verify_positional_option_config_test, advanced_config_set)
{
    int option_value{};

    auto parser = get_parser("arg1");
    EXPECT_THROW(parser.add_positional_option(option_value, sharg::config{.advanced = true}), sharg::design_error);
}

TEST_F(verify_positional_option_config_test, hidden_config_set)
{
    int option_value{};

    auto parser = get_parser("arg1");
    EXPECT_THROW(parser.add_positional_option(option_value, sharg::config{.hidden = true}), sharg::design_error);
}

// -----------------------------------------------------------------------------
// default_message config verification
// -----------------------------------------------------------------------------

class verify_default_message_config_test : public sharg::test::test_fixture
{};

TEST_F(verify_default_message_config_test, required_option_set)
{
    int option_value{};

    auto parser = get_parser("arg1");
    EXPECT_THROW(parser.add_option(option_value,
                                   sharg::config{.long_id = "int", .default_message = "Some number", .required = true}),
                 sharg::design_error);
}

TEST_F(verify_default_message_config_test, positional_option_set)
{
    int option_value{};

    auto parser = get_parser("arg1");
    EXPECT_THROW(parser.add_positional_option(option_value, sharg::config{.default_message = "Some number"}),
                 sharg::design_error);
}

TEST_F(verify_default_message_config_test, flag_set)
{
    bool value{};

    auto parser = get_parser();
    EXPECT_THROW(parser.add_flag(value, sharg::config{.short_id = 'i', .default_message = "false"}),
                 sharg::design_error);
}

// -----------------------------------------------------------------------------
// general
// -----------------------------------------------------------------------------

TEST_F(design_error_test, parse_called_twice)
{
    std::string option_value{};

    auto parser = get_parser("--version-check", "false", "-s", "option_string");
    parser.add_option(option_value, sharg::config{.short_id = 's'});

    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, "option_string");
    EXPECT_THROW(parser.parse(), sharg::design_error);
}

TEST_F(design_error_test, subcommand_parser_error)
{
    bool flag_value{false};

    // subcommand parsing was not enabled on construction but get_sub_parser() is called
    auto parser = get_parser("-f");
    parser.add_flag(flag_value, sharg::config{.short_id = 'f'});

    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(flag_value, true);
    EXPECT_THROW(parser.get_sub_parser(), sharg::design_error);

    flag_value = false;

    // subcommand key word must only contain alpha numeric characters
    parser = get_subcommand_parser({}, {"with space"});

    EXPECT_THROW(parser.parse(), sharg::design_error);

    // no positional options are allowed
    parser = get_subcommand_parser({"-f", "foo"}, {"foo"});

    EXPECT_THROW(parser.add_positional_option(flag_value, sharg::config{}), sharg::design_error);
    EXPECT_NO_THROW(parser.add_option(flag_value, sharg::config{.short_id = 'o'}));
    EXPECT_NO_THROW(parser.add_flag(flag_value, sharg::config{.short_id = 'f'}));
    EXPECT_THROW(parser.get_sub_parser(), sharg::design_error);
    EXPECT_EQ(flag_value, false);
    EXPECT_NO_THROW(parser.parse()); // Prints nothing, but sets sub_parser.
    EXPECT_NO_THROW(parser.get_sub_parser());
    EXPECT_EQ(flag_value, true);

    flag_value = false;

    // options are allowed
    parser = get_subcommand_parser({"-o", "true"}, {"foo"});

    EXPECT_THROW(parser.add_positional_option(flag_value, sharg::config{}), sharg::design_error);
    EXPECT_NO_THROW(parser.add_option(flag_value, sharg::config{.short_id = 'o'}));
    EXPECT_EQ(flag_value, false);
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(flag_value, true);
}

TEST_F(design_error_test, not_allowed_after_parse)
{
    int32_t value{};
    bool flag{};

    auto parser = get_parser("-i", "3");
    parser.add_option(value, sharg::config{.short_id = 'i'});
    EXPECT_NO_THROW(parser.parse());

    EXPECT_THROW_MSG(parser.add_option(value, sharg::config{.short_id = 'i'}),
                     sharg::design_error,
                     "add_option may only be used before calling parse().");
    EXPECT_THROW_MSG(parser.add_flag(flag, sharg::config{.short_id = 'i'}),
                     sharg::design_error,
                     "add_flag may only be used before calling parse().");
    EXPECT_THROW_MSG(parser.add_positional_option(value, sharg::config{}),
                     sharg::design_error,
                     "add_positional_option may only be used before calling parse().");
    EXPECT_THROW_MSG(parser.add_section(""),
                     sharg::design_error,
                     "add_section may only be used before calling parse().");
    EXPECT_THROW_MSG(parser.add_subsection(""),
                     sharg::design_error,
                     "add_subsection may only be used before calling parse().");
    EXPECT_THROW_MSG(parser.add_line(""), sharg::design_error, "add_line may only be used before calling parse().");
    EXPECT_THROW_MSG(parser.add_list_item("", ""),
                     sharg::design_error,
                     "add_list_item may only be used before calling parse().");
}
