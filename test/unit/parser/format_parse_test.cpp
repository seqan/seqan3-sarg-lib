// SPDX-FileCopyrightText: 2006-2024, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2024, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include <ranges>

#include <sharg/parser.hpp>
#include <sharg/test/expect_throw_msg.hpp>
#include <sharg/test/test_fixture.hpp>

class format_parse_test : public sharg::test::test_fixture
{};

TEST_F(format_parse_test, add_option_short_id)
{
    std::string option_value;

    // argument separated by space
    auto parser = get_parser("-s", "option_string1");
    parser.add_option(option_value, sharg::config{.short_id = 's'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, "option_string1");

    // argument separated by no space
    parser = get_parser("-Soption_string2");
    parser.add_option(option_value, sharg::config{.short_id = 'S'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, "option_string2");

    // argument separated by =
    parser = get_parser("-s=option_string3");
    parser.add_option(option_value, sharg::config{.short_id = 's'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, "option_string3");
}

TEST_F(format_parse_test, add_option_long_id)
{
    std::string option_value;

    // argument separated by space
    auto parser = get_parser("--string-option", "option_string1");
    parser.add_option(option_value, sharg::config{.long_id = "string-option"});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, "option_string1");

    // argument separated by no space
    parser = get_parser("--string-optionoption_string2");
    parser.add_option(option_value, sharg::config{.long_id = "string-option"});
    EXPECT_THROW(parser.parse(), sharg::parser_error);
    EXPECT_EQ(option_value, "option_string1");

    // argument separated by =
    parser = get_parser("--string-option=option_string3");
    parser.add_option(option_value, sharg::config{.long_id = "string-option"});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, "option_string3");
}

TEST_F(format_parse_test, add_flag_short_id_single)
{
    bool option_value1{false};
    bool option_value2{false};

    auto parser = get_parser("-f");
    parser.add_flag(option_value1, sharg::config{.short_id = 'f'});
    parser.add_flag(option_value2, sharg::config{.short_id = 'a'});

    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value1, true);
    EXPECT_EQ(option_value2, false);
}

TEST_F(format_parse_test, add_flag_short_id_multiple)
{
    bool option_value1{false};
    bool option_value2{false};
    bool option_value3{false};
    bool option_value4{false};

    auto parser = get_parser("-fbc");
    parser.add_flag(option_value1, sharg::config{.short_id = 'f'});
    parser.add_flag(option_value2, sharg::config{.short_id = 'a'});
    parser.add_flag(option_value3, sharg::config{.short_id = 'b'});
    parser.add_flag(option_value4, sharg::config{.short_id = 'c'});

    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value1, true);
    EXPECT_EQ(option_value2, false);
    EXPECT_EQ(option_value3, true);
    EXPECT_EQ(option_value4, true);
}

TEST_F(format_parse_test, add_flag_long_id)
{
    bool option_value1{false};
    bool option_value2{false};

    auto parser = get_parser("--another-flag");
    parser.add_flag(option_value1, sharg::config{.long_id = "flag"});
    parser.add_flag(option_value2, sharg::config{.long_id = "another-flag"});

    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value1, false);
    EXPECT_EQ(option_value2, true);
}

TEST_F(format_parse_test, add_positional_option)
{
    std::string positional_value;

    auto parser = get_parser("positional_string");
    parser.add_positional_option(positional_value, sharg::config{});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(positional_value, "positional_string");
}

TEST_F(format_parse_test, independent_add_order)
{
    // testing same command line input different add_* order
    std::string positional_value{};
    bool flag_value{false};
    int option_value{};
    auto parser = get_parser("-i", "2", "-b", "arg");

    auto check_values_and_reset = [&]()
    {
        EXPECT_EQ(positional_value, "arg");
        EXPECT_EQ(option_value, 2);
        EXPECT_EQ(flag_value, true);

        positional_value.clear();
        flag_value = false;
        option_value = 0;
        parser = get_parser("-i", "2", "-b", "arg");
    };

    auto add_option = [&]()
    {
        parser.add_option(option_value, sharg::config{.short_id = 'i'});
    };

    auto add_flag = [&]()
    {
        parser.add_flag(flag_value, sharg::config{.short_id = 'b'});
    };

    auto add_positional_option = [&]()
    {
        parser.add_positional_option(positional_value, sharg::config{});
    };

    // Order 1: option, flag, positional
    add_option();
    add_flag();
    add_positional_option();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 2: flag, option, positional
    add_flag();
    add_option();
    add_positional_option();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 3: option, positional, flag
    add_option();
    add_positional_option();
    add_flag();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 4: flag, positional, option
    add_flag();
    add_positional_option();
    add_option();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 5: positional, flag, option
    add_positional_option();
    add_flag();
    add_option();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 6: positional, option, flag
    add_positional_option();
    add_option();
    add_flag();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();
}

TEST_F(format_parse_test, independent_cmd_order)
{
    // testing different command line order
    std::string positional_value{};
    bool flag_value{false};
    int option_value{};

    auto check_values_and_reset = [&]()
    {
        EXPECT_EQ(positional_value, "arg");
        EXPECT_EQ(option_value, 2);
        EXPECT_EQ(flag_value, true);

        positional_value.clear();
        flag_value = false;
        option_value = 0;
    };

    auto parser = get_parser();

    auto setup_parser = [&]()
    {
        parser.add_option(option_value, sharg::config{.short_id = 'i'});
        parser.add_flag(flag_value, sharg::config{.short_id = 'b'});
        parser.add_positional_option(positional_value, sharg::config{});
    };

    // Order 1: option, flag, positional (POSIX conform)
    parser = get_parser("-i", "2", "-b", "arg");
    setup_parser();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 2: flag, option, positional (POSIX conform)
    parser = get_parser("-b", "-i", "2", "arg");
    setup_parser();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 3: option, positional, flag
    parser = get_parser("-i", "2", "arg", "-b");
    setup_parser();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 4: flag, positional, option
    parser = get_parser("-b", "arg", "-i", "2");
    setup_parser();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 5: positional, flag, option
    parser = get_parser("arg", "-b", "-i", "2");
    setup_parser();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // Order 6: positional, option, flag
    parser = get_parser("arg", "-i", "2", "-b");
    setup_parser();
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();
}

TEST_F(format_parse_test, double_dash_separation_success)
{
    // string option with dash
    std::string option_value{};
    auto parser = get_parser("--", "-strange");
    parser.add_positional_option(option_value, sharg::config{});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, "-strange");

    // negative integer option
    int option_value_int{};
    parser = get_parser("--", "-120");
    parser.add_positional_option(option_value_int, sharg::config{});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value_int, -120);
}

TEST_F(format_parse_test, special_characters_as_value_success)
{
    std::string option_value{};

    // weird option value. But since r/regex option is parsed and with its
    // value should work correct
    auto parser = get_parser("--regex", "-i=/45*&//--");
    parser.add_option(option_value, sharg::config{.long_id = "regex"});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, "-i=/45*&//--");
}

TEST_F(format_parse_test, empty_value_error)
{
    int option_value{};

    // short option
    auto parser = get_parser("-i");
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    EXPECT_THROW(parser.parse(), sharg::parser_error);

    // long option
    parser = get_parser("--long");
    parser.add_option(option_value, sharg::config{.long_id = "long"});
    EXPECT_THROW(parser.parse(), sharg::parser_error);

    // short option
    parser = get_parser("-i=");
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    EXPECT_THROW(parser.parse(), sharg::parser_error);

    // long option
    parser = get_parser("--long=");
    parser.add_option(option_value, sharg::config{.long_id = "long"});
    EXPECT_THROW(parser.parse(), sharg::parser_error);
}

TEST_F(format_parse_test, parse_success_bool_option)
{
    bool option_value{false};
    bool positional_value{false};

    auto check_values_and_reset = [&]()
    {
        EXPECT_EQ(option_value, true);
        EXPECT_EQ(positional_value, false);

        option_value = false;
        positional_value = false;
    };

    // numbers 0 and 1
    auto parser = get_parser("-b", "1", "0");
    parser.add_option(option_value, sharg::config{.short_id = 'b'});
    parser.add_positional_option(positional_value, sharg::config{});
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();

    // true and false
    parser = get_parser("-b", "true", "false");
    parser.add_option(option_value, sharg::config{.short_id = 'b'});
    parser.add_positional_option(positional_value, sharg::config{});
    EXPECT_NO_THROW(parser.parse());
    check_values_and_reset();
}

TEST_F(format_parse_test, parse_success_int_option)
{
    int option_value{};
    size_t positional_value{};

    auto parser = get_parser("-i", "-2", "278");
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    parser.add_positional_option(positional_value, sharg::config{});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, -2);
    EXPECT_EQ(positional_value, 278u);
}

TEST_F(format_parse_test, parse_success_double_option)
{
    double option_value{};
    double positional_value{};

    // double expression with e
    auto parser = get_parser("-d", "6.0221418e23");
    parser.add_option(option_value, sharg::config{.short_id = 'd'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_FLOAT_EQ(option_value, 6.0221418e23);
    EXPECT_FLOAT_EQ(positional_value, 0.0);

    // double expression with dot
    parser = get_parser("-d", "12.457", "0.123");
    parser.add_option(option_value, sharg::config{.short_id = 'd'});
    parser.add_positional_option(positional_value, sharg::config{});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_FLOAT_EQ(option_value, 12.457);
    EXPECT_FLOAT_EQ(positional_value, 0.123);
}

TEST_F(format_parse_test, parse_error_bool_option)
{
    bool option_value{false};

    // fail on character input
    auto parser = get_parser("-b", "a");
    parser.add_option(option_value, sharg::config{.short_id = 'b'});
    EXPECT_THROW(parser.parse(), sharg::parser_error);
    EXPECT_EQ(option_value, false);

    // fail on number input except 0 and 1
    parser = get_parser("-b", "124");
    parser.add_option(option_value, sharg::config{.short_id = 'b'});
    EXPECT_THROW(parser.parse(), sharg::parser_error);
    EXPECT_EQ(option_value, false);
}

TEST_F(format_parse_test, parse_error_int_option)
{
    int signed_int{};
    unsigned unsigned_int{};
    int8_t signed_int8{};
    uint8_t unsigned_uint8{};

    auto parser = get_parser();

    auto check_for_failure = [&]<typename t>(t & option_value, std::string_view value, t expected = 0)
    {
        parser = get_parser("-i", value.data());
        parser.add_option(option_value, sharg::config{.short_id = 'i'});
        EXPECT_THROW(parser.parse(), sharg::parser_error);
        EXPECT_EQ(option_value, expected);
    };

    // fail on character
    check_for_failure(signed_int, "abc");

    // fail on number followed by character
    check_for_failure(signed_int, "2abc", 2); // Todo: Expected?

    // fail on double
    check_for_failure(signed_int, "3.12", 3); // Todo: Expected?

    // fail on negative number for unsigned
    check_for_failure(unsigned_int, "-1");

    // fail on overflow
    check_for_failure(signed_int8, "129");
    check_for_failure(unsigned_uint8, "267");
}

TEST_F(format_parse_test, parse_error_double_option)
{
    double option_value{};

    // fail on character
    auto parser = get_parser("-i", "abc");
    parser.add_option(option_value, sharg::config{.short_id = 'd'});
    EXPECT_THROW(parser.parse(), sharg::parser_error);
    EXPECT_FLOAT_EQ(option_value, 0.0);

    // fail on number followed by character
    parser = get_parser("-d", "12.457a");
    parser.add_option(option_value, sharg::config{.short_id = 'd'});
    EXPECT_THROW(parser.parse(), sharg::parser_error);
    EXPECT_FLOAT_EQ(option_value, 12.457); // Todo: Expected?
}

TEST_F(format_parse_test, too_many_arguments_error)
{
    int option_value{};

    auto parser = get_parser("5", "15");
    parser.add_positional_option(option_value, sharg::config{});
    EXPECT_THROW(parser.parse(), sharg::too_many_arguments);
    EXPECT_EQ(option_value, 5); // Todo: Expected?

    // since -- indicates -i as a positional argument, this causes a too many args error
    parser = get_parser("2", "--", "-i");
    parser.add_positional_option(option_value, sharg::config{});
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    EXPECT_THROW(parser.parse(), sharg::too_many_arguments);
    EXPECT_EQ(option_value, 2); // Todo: Expected?
}

TEST_F(format_parse_test, too_few_arguments_error)
{
    int option_value{};

    auto parser = get_parser("15");
    parser.add_positional_option(option_value, sharg::config{});
    parser.add_positional_option(option_value, sharg::config{});
    EXPECT_THROW(parser.parse(), sharg::too_few_arguments);
    EXPECT_EQ(option_value, 15); // Todo: Expected?

    // since -- indicates -i as a positional argument, this causes a too many args error
    parser = get_parser("-i", "2");
    parser.add_positional_option(option_value, sharg::config{});
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    EXPECT_THROW(parser.parse(), sharg::too_few_arguments);
    EXPECT_EQ(option_value, 2); // Todo: Expected?
}

TEST_F(format_parse_test, unknown_option_error)
{
    auto parser = get_parser();

    auto check_for_failure = [&](auto... arguments)
    {
        parser = get_parser(arguments...);
        EXPECT_THROW(parser.parse(), sharg::unknown_option);
    };

    // unknown short option
    check_for_failure("-i", "15");

    // unknown long option
    check_for_failure("--arg", "8");

    // unknown short flag
    check_for_failure("-a");

    // unknown long flag
    check_for_failure("--arg");

    // negative numbers are seen as options
    check_for_failure("-5");

    // unknown short option in a more complex command line
    int option_value_i{};
    std::string option_value_a{};
    std::string positional_option{};
    parser = get_parser("-i", "129", "arg1", "-b", "bcd", "-a", "abc");
    parser.add_option(option_value_i, sharg::config{.short_id = 'i'});
    parser.add_option(option_value_a, sharg::config{.short_id = 'a'});
    parser.add_positional_option(positional_option, sharg::config{});
    EXPECT_THROW(parser.parse(), sharg::unknown_option);
    EXPECT_EQ(option_value_i, 129);   // Todo: Expected?
    EXPECT_EQ(option_value_a, "abc"); // Todo: Expected?
    EXPECT_EQ(positional_option, "");
}

TEST_F(format_parse_test, option_declared_multiple_times_error)
{
    int option_value{};

    // short option
    auto parser = get_parser("-i", "15", "-i", "3");
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    EXPECT_THROW(parser.parse(), sharg::option_declared_multiple_times);
    EXPECT_EQ(option_value, 15); // Todo: Expected?

    // since -- indicates -i as a positional argument, this causes a too many args error
    parser = get_parser("--long", "5", "--long", "6");
    parser.add_option(option_value, sharg::config{.long_id = "long"});
    EXPECT_THROW(parser.parse(), sharg::option_declared_multiple_times);
    EXPECT_EQ(option_value, 5); // Todo: Expected?

    // since -- indicates -i as a positional argument, this causes a too many args error
    parser = get_parser("-i", "5", "--long", "6");
    parser.add_option(option_value, sharg::config{.short_id = 'i', .long_id = "long"});
    EXPECT_THROW(parser.parse(), sharg::option_declared_multiple_times);
    EXPECT_EQ(option_value, 6); // Todo: Expected?
}

TEST_F(format_parse_test, required_option_missing)
{
    int option_value{};

    // option is required
    auto parser = get_parser("5", "-i", "15");
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    parser.add_option(option_value, sharg::config{.short_id = 'a', .required = true});
    parser.add_positional_option(option_value, sharg::config{});
    EXPECT_THROW(parser.parse(), sharg::required_option_missing);
    EXPECT_EQ(option_value, 15); // Todo: Expected?
}

TEST_F(format_parse_test, argv_const_combinations)
{
    bool flag_value{false};

    auto parser = get_parser();

    auto check_and_reset = [&]()
    {
        parser.add_flag(flag_value, sharg::config{.short_id = 'f'});
        EXPECT_NO_THROW(parser.parse());
        EXPECT_EQ(flag_value, true);
        flag_value = false;
    };

    char arg1[]{"./parser"};
    char arg2[]{"-f"};
    char * argv[] = {arg1, arg2};

    // none const
    parser = sharg::parser{"test_parser", 2, argv, sharg::update_notifications::off};
    check_and_reset();

    // all const*
    char const * const * const argv_all_const{argv};
    parser = sharg::parser{"test_parser", 2, argv_all_const, sharg::update_notifications::off};
    check_and_reset();

    // const 1
    char const * argv_const1[] = {"./parser_test", "-f"};
    parser = sharg::parser{"test_parser", 2, argv_const1, sharg::update_notifications::off};
    check_and_reset();

    // const 2
    char * const argv_const2[] = {arg1, arg2};
    parser = sharg::parser{"test_parser", 2, argv_const2, sharg::update_notifications::off};
    check_and_reset();

    // const 12
    char const * const argv_const12[] = {arg1, arg2};
    parser = sharg::parser{"test_parser", 2, argv_const12, sharg::update_notifications::off};
    check_and_reset();
}

TEST_F(format_parse_test, multiple_empty_options)
{
    int option_value{};

    auto parser = get_parser("-s=1");
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    parser.add_option(option_value, sharg::config{.short_id = 's'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, 1);
    option_value = 0;

    parser = get_parser("-s=1", "--unknown");
    parser.add_option(option_value, sharg::config{.short_id = 'i'});
    parser.add_option(option_value, sharg::config{.short_id = 's'});
    EXPECT_THROW(parser.parse(), sharg::unknown_option);
    EXPECT_EQ(option_value, 1); // Todo: Expected?

    parser = get_parser("--long=2");
    parser.add_option(option_value, sharg::config{.long_id = "longi"});
    parser.add_option(option_value, sharg::config{.long_id = "long"});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, 2);
}

TEST_F(format_parse_test, version_check_option_error)
{
    // version-check must be followed by a value
    auto parser = get_parser("--version-check");
    EXPECT_THROW(parser.parse(), sharg::parser_error);

    // version-check value must be 0 or 1
    parser = get_parser("--version-check", "foo");
    EXPECT_THROW(parser.parse(), sharg::parser_error);
}

TEST_F(format_parse_test, subcommand_parser_success)
{
    bool flag_value{false};
    std::string option_value{};

    auto check_and_reset_options = [&](bool flag, std::string option)
    {
        EXPECT_EQ(flag_value, flag);
        EXPECT_EQ(option_value, option);
        flag_value = false;
        option_value.clear();
    };

    // parsing
    auto top_level_parser = get_subcommand_parser({"-f", "sub1", "foo"}, {"sub1", "sub2"});
    top_level_parser.add_flag(flag_value, sharg::config{.short_id = 'f'});
    EXPECT_NO_THROW(top_level_parser.parse());
    check_and_reset_options(true, "");

    sharg::parser & sub_parser = top_level_parser.get_sub_parser();
    EXPECT_EQ(sub_parser.info.app_name, "test_parser-sub1");
    sub_parser.add_positional_option(option_value, sharg::config{});
    EXPECT_NO_THROW(sub_parser.parse());
    check_and_reset_options(false, "foo");

    // top-level help page
    top_level_parser = get_subcommand_parser({"-h", "-f", "sub1", "foo"}, {"sub1", "sub2"});
    top_level_parser.add_flag(flag_value, sharg::config{.short_id = 'f'});
    EXPECT_FALSE(get_parse_cout_on_exit(top_level_parser).empty());
    check_and_reset_options(false, "");

    // sub-parser help page
    top_level_parser = get_subcommand_parser({"-f", "sub1", "foo", "-h"}, {"sub1", "sub2"});
    top_level_parser.add_flag(flag_value, sharg::config{.short_id = 'f'});
    EXPECT_NO_THROW(top_level_parser.parse());
    check_and_reset_options(true, "");

    sharg::parser & sub_parser2 = top_level_parser.get_sub_parser();
    EXPECT_EQ(sub_parser2.info.app_name, "test_parser-sub1");
    sub_parser2.add_positional_option(option_value, sharg::config{});
    EXPECT_FALSE(get_parse_cout_on_exit(sub_parser2).empty());
    check_and_reset_options(false, "");

    // sub command may contain dash, see https://github.com/seqan/product_backlog/issues/234
    EXPECT_NO_THROW((sharg::parser{"top", {"./top", "-dash"}, sharg::update_notifications::off, {"-dash"}}));
}

TEST_F(format_parse_test, subcommand_parser_error)
{
    constexpr std::string_view expected_message = "You specified an unknown subcommand! Available subcommands are: "
                                                  "[sub1]. Use -h/--help for more information.";

    // incorrect sub command regardless of following arguments, https://github.com/seqan/seqan3/issues/2172
    auto parser = get_subcommand_parser({"subiddysub", "-f"}, {"sub1"});
    EXPECT_THROW_MSG(parser.parse(), sharg::user_input_error, expected_message);

    // incorrect sub command with no other arguments
    parser = get_subcommand_parser({"subiddysub"}, {"sub1"});
    EXPECT_THROW_MSG(parser.parse(), sharg::user_input_error, expected_message);

    // incorrect sub command with trailing special option, https://github.com/seqan/sharg-parser/issues/171
    parser = get_subcommand_parser({"subiddysub", "-h"}, {"sub1"});
    EXPECT_THROW_MSG(parser.parse(), sharg::user_input_error, expected_message);
}

TEST_F(format_parse_test, issue1544)
{
    std::string option_value{};
    std::string option_value2{};

    // wrong separation of long value:
    auto parser = get_parser("--foohallo");
    parser.add_option(option_value, sharg::config{.long_id = "foo"});
    EXPECT_THROW(parser.parse(), sharg::unknown_option);
    EXPECT_EQ(option_value, "");

    // unknown option (`--foo-bar`) that has a prefix of a known option (`--foo`)
    parser = get_parser("--foo", "hallo", "--foo-bar", "ballo");
    parser.add_option(option_value, sharg::config{.long_id = "foo"});
    EXPECT_THROW(parser.parse(), sharg::unknown_option);
    EXPECT_EQ(option_value, "hallo"); // Todo: Expected?

    // known option (`--foo-bar`) that has a prefix of a unknown option (`--foo`)
    parser = get_parser("--foo", "hallo", "--foo-bar", "ballo");
    parser.add_option(option_value, sharg::config{.long_id = "foo-bar"});
    EXPECT_THROW(parser.parse(), sharg::unknown_option);
    EXPECT_EQ(option_value, "ballo"); // Todo: Expected?

    // known option (`--foo`) is a prefix of another known option (`--foo-bar`)
    parser = get_parser("--foo", "hallo", "--foo-bar", "ballo");
    parser.add_option(option_value, sharg::config{.long_id = "foo"});
    parser.add_option(option_value2, sharg::config{.long_id = "foo-bar"});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(option_value, "hallo");
    EXPECT_EQ(option_value2, "ballo");
}

TEST_F(format_parse_test, is_option_set)
{
    std::string option_value{};
    std::vector<std::string> positional_options{};

    auto parser = get_parser("-l", "hallo", "--foobar", "ballo", "--", "--loo", "--bar");
    // `--` signals the end of options!
    parser.add_positional_option(positional_options, sharg::config{});
    parser.add_option(option_value, sharg::config{.short_id = 'b', .long_id = "bar"});
    parser.add_option(option_value, sharg::config{.short_id = 'l', .long_id = "loo"});
    parser.add_option(option_value, sharg::config{.short_id = 'f', .long_id = "foobar"});

    // you cannot call option_is_set before parse()
    EXPECT_THROW(parser.is_option_set("foo"), sharg::design_error);

    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(positional_options.size(), 2u);
    EXPECT_EQ(positional_options[0], "--loo");
    EXPECT_EQ(positional_options[1], "--bar");

    EXPECT_TRUE(parser.is_option_set('l'));
    EXPECT_TRUE(parser.is_option_set("loo")); // --loo is behind the `--`, but -l is before
    EXPECT_TRUE(parser.is_option_set('f'));
    EXPECT_TRUE(parser.is_option_set("foobar"));

    EXPECT_FALSE(parser.is_option_set('b'));
    EXPECT_FALSE(parser.is_option_set("bar")); // --bar is behind the `--`

    // errors:
    EXPECT_THROW(parser.is_option_set("l"), sharg::design_error); // short identifiers are passed as chars not strings
    EXPECT_THROW(parser.is_option_set("f"), sharg::design_error); // short identifiers are passed as chars not strings

    EXPECT_THROW(parser.is_option_set("foo"), sharg::design_error);
    EXPECT_THROW(parser.is_option_set("--"), sharg::design_error);
    EXPECT_THROW(parser.is_option_set(""), sharg::design_error);

    EXPECT_THROW(parser.is_option_set('!'), sharg::design_error);
    EXPECT_THROW(parser.is_option_set('-'), sharg::design_error);
    EXPECT_THROW(parser.is_option_set('_'), sharg::design_error);
    EXPECT_THROW(parser.is_option_set('\0'), sharg::design_error);
}

// https://github.com/seqan/seqan3/issues/2835
TEST_F(format_parse_test, error_message_parsing)
{
    uint64_t option_value{};

    auto parser = get_parser("--value", "-30");
    parser.add_option(option_value, sharg::config{.long_id = "value"});
    EXPECT_THROW_MSG(parser.parse(),
                     sharg::user_input_error,
                     "Value parse failed for --value: Argument -30 could "
                     "not be parsed as type unsigned 64 bit integer.");
}

// https://github.com/seqan/seqan3/pull/2381
TEST_F(format_parse_test, container_options)
{
    std::vector<int> integer_options{};
    std::vector<bool> bool_options{};

    auto parser = get_parser("-i", "2", "-i", "1", "-i", "3");
    parser.add_option(integer_options, sharg::config{.short_id = 'i'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_TRUE(integer_options == (std::vector<int>{2, 1, 3}));

    parser = get_parser("-b", "true", "-b", "false", "-b", "true");
    parser.add_option(bool_options, sharg::config{.short_id = 'b'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_TRUE(bool_options == (std::vector<bool>{true, false, true}));
}

// https://github.com/seqan/seqan3/issues/2393
TEST_F(format_parse_test, container_default)
{
    std::vector<int> option_values{1, 2, 3};
    bool option{false};

    auto reset_options = [&]()
    {
        option_values = {1, 2, 3};
        option = false;
    };

    // overwrite default
    auto parser = get_parser("-i", "2", "-i", "1", "-i", "3");
    parser.add_option(option_values, sharg::config{.short_id = 'i'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_TRUE(option_values == (std::vector<int>{2, 1, 3}));
    reset_options();

    // overwrite default, parameters are not consecutive
    parser = get_parser("-i", "2", "-b", "true", "-i", "1", "-i", "3");
    parser.add_option(option_values, sharg::config{.short_id = 'i'});
    parser.add_option(option, sharg::config{.short_id = 'b'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_TRUE(option_values == (std::vector<int>{2, 1, 3}));
    EXPECT_TRUE(option);
    reset_options();

    // use default
    parser = get_parser("-b", "true");
    parser.add_option(option_values, sharg::config{.short_id = 'i'});
    parser.add_option(option, sharg::config{.short_id = 'b'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_TRUE(option_values == (std::vector<int>{1, 2, 3}));
    EXPECT_TRUE(option);
    reset_options();

    // overwrite default for positional options
    parser = get_parser("2", "1", "3");
    parser.add_positional_option(option_values, sharg::config{});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_TRUE(option_values == (std::vector<int>{2, 1, 3}));
}

TEST_F(format_parse_test, executable_name)
{
    bool flag{false};
    auto parser = get_parser();

    auto check = [&](std::string_view expected)
    {
        parser = sharg::parser{"test_parser", {expected.data(), "-t"}, sharg::update_notifications::off};
        parser.add_flag(flag, sharg::config{.short_id = 't'});
        EXPECT_NO_THROW(parser.parse());
        auto & executable_name = sharg::detail::test_accessor::executable_name(parser);
        ASSERT_EQ(executable_name.size(), 1);
        EXPECT_EQ(executable_name[0], expected);
        flag = false;
    };

    check("parser_test");
    check("./parser_test");
    check("./bin/parser_test");

    parser = sharg::parser{"test_parser", {"parser_test", "build", "-t"}, sharg::update_notifications::off, {"build"}};
    EXPECT_NO_THROW(parser.parse());

    auto & sub_parser = parser.get_sub_parser();
    sub_parser.add_flag(flag, sharg::config{.short_id = 't'});
    EXPECT_NO_THROW(sub_parser.parse());

    auto & executable_name = sharg::detail::test_accessor::executable_name(sub_parser);
    ASSERT_EQ(executable_name.size(), 2);
    EXPECT_EQ(executable_name[0], "parser_test");
    EXPECT_EQ(executable_name[1], "build");
}

TEST_F(format_parse_test, flag_independence)
{
    bool flag_value{false};

    auto parser = get_parser("-a", "-z");
    parser.add_flag(flag_value, sharg::config{.short_id = 'a'});
    parser.add_flag(flag_value, sharg::config{.short_id = 'z'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(flag_value, true);

    flag_value = false;

    parser = get_parser("-a");
    parser.add_flag(flag_value, sharg::config{.short_id = 'a'});
    parser.add_flag(flag_value, sharg::config{.short_id = 'z'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(flag_value, true);

    flag_value = false;

    parser = get_parser("-z");
    parser.add_flag(flag_value, sharg::config{.short_id = 'a'});
    parser.add_flag(flag_value, sharg::config{.short_id = 'z'});
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(flag_value, true);
}

TEST_F(format_parse_test, parse_order_example)
{
    bool flag_value{false};

    // Flags are evaluated after options
    auto parser = get_parser("-f", "-o", "false");

    EXPECT_NO_THROW(parser.add_option(flag_value, sharg::config{.short_id = 'o'}));
    EXPECT_NO_THROW(parser.add_flag(flag_value, sharg::config{.short_id = 'f'}));
    EXPECT_EQ(flag_value, false);
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(flag_value, true); // option sets to false, but flag sets to true

    flag_value = false;

    // Because this option syntax is also allowed
    parser = get_parser("-otrue");

    EXPECT_NO_THROW(parser.add_option(flag_value, sharg::config{.short_id = 'o'}));
    EXPECT_EQ(flag_value, false);
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(flag_value, true);

    flag_value = false;

    // And this flag syntax is allowed
    parser = get_parser("-otrue");
    std::array<bool, 5> flag_values{false, false, false, false, false};

    EXPECT_NO_THROW(parser.add_flag(flag_values[0], sharg::config{.short_id = 'o'}));
    EXPECT_NO_THROW(parser.add_flag(flag_values[1], sharg::config{.short_id = 't'}));
    EXPECT_NO_THROW(parser.add_flag(flag_values[2], sharg::config{.short_id = 'r'}));
    EXPECT_NO_THROW(parser.add_flag(flag_values[3], sharg::config{.short_id = 'u'}));
    EXPECT_NO_THROW(parser.add_flag(flag_values[4], sharg::config{.short_id = 'e'}));
    EXPECT_TRUE(std::ranges::none_of(flag_values, std::identity{})); // All false
    EXPECT_NO_THROW(parser.parse());
    EXPECT_TRUE(std::ranges::all_of(flag_values, std::identity{})); // All true

    // Positional options are evaluated last
    parser = get_parser("false", "-o", "true", "-f");

    EXPECT_NO_THROW(parser.add_option(flag_value, sharg::config{.short_id = 'o'}));
    EXPECT_NO_THROW(parser.add_flag(flag_value, sharg::config{.short_id = 'f'}));
    EXPECT_NO_THROW(parser.add_positional_option(flag_value, sharg::config{}));
    EXPECT_EQ(flag_value, false);
    EXPECT_NO_THROW(parser.parse());
    EXPECT_EQ(flag_value, false);
}
