// SPDX-FileCopyrightText: 2006-2024, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2024, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause

/*!\file
 * \author Svenja Mehringer <svenja.mehringer AT fu-berlin.de>
 * \brief Provides sharg::parser class.
 */

#pragma once

#include <unordered_set>
#include <variant>

#include <sharg/config.hpp>
#include <sharg/detail/format_help.hpp>
#include <sharg/detail/format_html.hpp>
#include <sharg/detail/format_man.hpp>
#include <sharg/detail/format_parse.hpp>
#include <sharg/detail/format_tdl.hpp>
#include <sharg/detail/poison_config.hpp>
#include <sharg/detail/version_check.hpp>

namespace sharg
{

/*!\brief The Sharg command line parser.
 * \ingroup parser
 *
 * \details
 *
 * The sharg::parser is a general purpose argument parser that provides
 * convenient access to the command line arguments passed to the program.
 * It automatically generates a help page and can export manual-pages as well
 * as HTML documentation.
 *
 * ### Terminology
 *
 * Since the terms option and arguments are frequently used in different contexts
 * we want to first clarify our usage:
 *
 * - **options** [e.g. `-i myfile`, `--infile myfile`] refer to key-value pairs.
 *               The key is either a short indentifier, restricted to a single
 *               character `-i`, or a long identifier `--infile`.
 *
 * - **positional options** [e.g. `arg1`] refers to command line arguments that
 *                          are specified without an identifier/key, are always
 *                          required and are identified by their position.
 *
 * - **flags** [e.g. `-b`] refers to identifiers that are not followed by a
 *                         value (booleans) and therefore only indicate whether
 *                         they are set or not.
 *
 * ### Add/get options, flags or positional Options
 *
 * Adding an option is done in a single call. You simply
 * need to provide a predeclared variable and some additional information like
 * the identifier, description or advanced restrictions. To actually retrieve
 * the value from the command line and enable every other mechanism you need
 * to call the sharg::parser::parse function in the end.
 *
 * \include test/snippet/parser_1.cpp
 *
 * Now you can call your application via the command line:
 *
 * ```console
 * MaxMuster% ./grade_avg_app -n Peter --bonus 1.0 2.0 1.7
 * Peter has an average grade of 1.425
 * ```
 * You can also display the help page automatically:
 *
 * ```console
 * MaxMuster% ./grade_avg_app --help
 * Grade-Average
 * =============
 *
 * POSITIONAL ARGUMENTS
 *     ARGUMENT-1 List of DOUBLE's
 *           Please specify your grades.
 *
 * OPTIONS
 *     -n, --name STRING
 *           Please specify your name.
 *     -b, --bonus
 *           Please specify if you got the bonus.
 *
 * VERSION
 *     Last update:
 *     Grade-Average version:
 *     Sharg version: 0.1.0
 *
 * ```
 *
 * ###  The POSIX conventions
 *
 * The sharg::parser follows the
 * [POSIX conventions](https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html). Note that this means
 * among others:
 *
 * 1. Options without arguments can use one hyphen, for example `-a -b` is equivalent to `-ab`.
 * 2. Whitespaces between a short option and its argument are optional. For example, `-c foo` is equivalent to `-cfoo`.
 * 3. `--` terminates the options and signals that only positional options follow. This enables the user to
 * use a positional option beginning with `-` without it being misinterpreted as an option identifier.
 *
 * \attention Currently, the sharg::parser is in disagreement with one of the
 * [POSIX conventions](https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html). It does not
 * interpret a single hyphen character as an ordinary non-option argument that may be used for in-/output from
 * standard streams.
 *
 * ### Errors that are caught by the parser
 *
 * There are two different kinds of errors: Developer errors and user errors.
 *
 * Developer errors are those that violate the sharg::parser design
 * (e.g. calling the sharg::parser::parse function twice or specifying
 * two different options with the same identifier.)
 * In this case, a sharg::design_error is thrown.
 *
 * The second kind are user errors, due to invalid command line calls. In this
 * case a sharg::parser_error is thrown.
 *
 * For example:
 *
 * ```console
 * MaxMuster% ./grade_avg_app -n Peter 2.0 abc 1.7
 * [PARSER ERROR] Value cast failed for positional option 2: Argument abc could not be casted to type DOUBLE.
 * ```
 *
 * See the sharg::parser::parse documentation for a detailed list of
 * which exceptions are caught.
 *
 * ### Update Notifications
 *
 * SeqAn applications that are using the sharg::parser can check SeqAn servers for version updates.
 * The functionality helps getting new versions out to users faster.
 * It is also used to inform application developers of new versions of the SeqAn library
 * which means that applications ship with less bugs.
 * For privacy implications, please see: https://docs.seqan.de/sharg/main_user/about_update_notifications.html.
 *
 * Developers that wish to disable this feature permanently can pass an extra constructor argument:
 *
 * \include doc/tutorial/parser/disable_version_check.cpp
 *
 * Users of applications that have this feature activated can opt-out, by either:
 *
 *  * disabling it for a specific application simply by setting the option `--version-check false/0` or
 *  * disabling it for all applications by setting the `SHARG_NO_VERSION_CHECK` environment variable.
 *
 * Note that in case there is no `--version-check` option (display available options with `-h/--help)`,
 * then the developer already disabled the version check functionality.
 *
 * \stableapi{Since version 1.0.}
 */
class parser
{
public:
    /*!\name Constructors, destructor and assignment
     * \{
     */
    parser() = delete;                           //!< Deleted.
    parser(parser const &) = delete;             //!< Deleted.
    parser & operator=(parser const &) = delete; //!< Deleted.
    parser(parser &&) = default;                 //!< Defaulted.
    parser & operator=(parser &&) = default;     //!< Defaulted.

    /*!\brief Initializes an sharg::parser object from the command line arguments.
     *
     * \param[in] app_name The name of the app that is displayed on the help page.
     * \param[in] arguments The command line arguments to parse.
     * \param[in] version_updates Notify users about version updates (default sharg::update_notifications::on).
     * \param[in] subcommands A list of subcommands (see \link subcommand_parse subcommand parsing \endlink).
     *
     * The application name must only contain alpha-numeric characters, `_` or `-` ,
     * i.e. the following regex must evaluate to true: `"^[a-zA-Z0-9_-]+$"` .
     *
     * See the [parser tutorial](https://docs.seqan.de/sharg/main_user/tutorial_parser.html)
     * for more information about the version check functionality.
     *
     * \details
     * \stableapi{Since version 1.0.}
     */
    parser(std::string const & app_name,
           std::vector<std::string> const & arguments,
           update_notifications version_updates = update_notifications::on,
           std::vector<std::string> subcommands = {}) :
        version_check_dev_decision{version_updates},
        arguments{arguments}
    {
        add_subcommands(subcommands);
        info.app_name = app_name;
    }

    //!\overload
    parser(std::string const & app_name,
           int const argc,
           char const * const * const argv,
           update_notifications version_updates = update_notifications::on,
           std::vector<std::string> subcommands = {}) :
        parser{app_name, std::vector<std::string>{argv, argv + argc}, version_updates, std::move(subcommands)}
    {}

    //!\brief The destructor.
    ~parser()
    {
        // wait for another 3 seconds
        if (version_check_future.valid())
            version_check_future.wait_for(std::chrono::seconds(3));
    }
    //!\}

    /*!\name Adding options
     * \brief Add (positional) options and flags to the parser.
     * \{
     */
    /*!\brief Adds an option to the sharg::parser.
     *
     * \tparam option_type Must have a formatted input function (stream >> value).
     *                     If option_type is a container, its value type must have the
     *                     formatted input function (exception: std::string is not
     *                     regarded as a container).
     *                     See <a href="https://en.cppreference.com/w/cpp/named_req/FormattedInputFunction">
     *                     FormattedInputFunction </a>.
     * \tparam validator_type The type of validator to be applied to the option
     *                        value. Must model sharg::validator.
     *
     * \param[in, out] value The variable in which to store the given command line argument.
     * \param[in] config A configuration object to customise the sharg::parser behaviour. See sharg::config.
     *
     * The `config.validator` must be applicable to the given output variable (\p value).
     *
     * \throws sharg::design_error if sharg::parser::parse was already called.
     * \throws sharg::design_error if the option is required and has a default_message.
     * \throws sharg::design_error if the option identifier was already used.
     * \throws sharg::design_error if the option identifier is not a valid identifier.
     *
     * \details
     * \stableapi{Since version 1.0.}
     */
    template <typename option_type, typename validator_type>
        requires (parsable<option_type> || parsable<std::ranges::range_value_t<option_type>>)
              && std::invocable<validator_type, option_type>
    void add_option(option_type & value, config<validator_type> const & config)
    {
        check_parse_not_called("add_option");
        verify_option_config(config);

        auto operation = [this, &value, config]()
        {
            auto visit_fn = [&value, &config](auto & f)
            {
                f.add_option(value, config);
            };

            std::visit(std::move(visit_fn), format);
        };

        operations.push_back(std::move(operation));
    }

    //!\cond DEV
    //!\brief A poison overload that catches calls to add_option without explicitly passing a sharg::config.
    template <typename option_type>
    void add_option(option_type &, detail::poison_config const &)
    {
        static_assert(detail::dependent_false_v<option_type>, "Forgot sharg::config?");
    }
    //!\endcond

    /*!\brief Adds a flag to the sharg::parser.
     *
     * \param[in, out] value     The variable which shows if the flag is turned off (default) or on.
     * \param[in] config A configuration object to customise the sharg::parser behaviour. See sharg::config.
     *
     * \throws sharg::design_error if sharg::parser::parse was already called.
     * \throws sharg::design_error if `value` is true.
     * \throws sharg::design_error if the option identifier was already used.
     * \throws sharg::design_error if the option identifier is not a valid identifier.
     *
     * \details
     * \stableapi{Since version 1.0.}
     */
    template <typename validator_type>
        requires std::invocable<validator_type, bool>
    void add_flag(bool & value, config<validator_type> const & config)
    {
        check_parse_not_called("add_flag");
        verify_flag_config(config);

        if (value)
            throw design_error("A flag's default value must be false.");

        auto operation = [this, &value, config]()
        {
            auto visit_fn = [&value, &config](auto & f)
            {
                f.add_flag(value, config);
            };

            std::visit(std::move(visit_fn), format);
        };

        operations.push_back(std::move(operation));
    }

    //!\cond DEV
    //!\brief A poison overload that catches calls to add_flag without explicitly passing a sharg::config.
    template <typename option_type> // Template needed to prevent instantiation of this function if unused.
    void add_flag(option_type &, detail::poison_config const &)
    {
        static_assert(detail::dependent_false_v<option_type>, "Forgot sharg::config?");
    }
    //!\endcond

    /*!\brief Adds a positional option to the sharg::parser.
     *
     * \tparam option_type Must have a formatted input function (stream >> value).
     *                     If option_type is a container, its value type must have the
     *                     formatted input function (exception: std::string is not
     *                     regarded as a container).
     *                     See <a href="https://en.cppreference.com/w/cpp/named_req/FormattedInputFunction">
     *                     FormattedInputFunction </a>.
     * \tparam validator_type The type of validator to be applied to the option
     *                        value. Must model sharg::validator.
     *
     * \param[in, out] value The variable in which to store the given command line argument.
     * \param[in] config Customise the sharg::parser behaviour. See sharg::positional_config.
     *
     * \throws sharg::design_error if sharg::parser::parse was already called.
     * \throws sharg::design_error if the option has a short or long identifier.
     * \throws sharg::design_error if the option is advanced or hidden.
     * \throws sharg::design_error if the option has a default_message.
     * \throws sharg::design_error if there already is a positional list option.
     * \throws sharg::design_error if there are subcommands.
     *
     * \details
     *
     * The `config.validator` must be applicable to the given output variable (\p value).
     *
     * \stableapi{Since version 1.0.}
     */
    template <typename option_type, typename validator_type>
        requires (parsable<option_type> || parsable<std::ranges::range_value_t<option_type>>)
              && std::invocable<validator_type, option_type>
    void add_positional_option(option_type & value, config<validator_type> const & config)
    {
        check_parse_not_called("add_positional_option");
        verify_positional_option_config(config);

        if constexpr (detail::is_container_option<option_type>)
            has_positional_list_option = true; // keep track of a list option because there must be only one!

        auto operation = [this, &value, config]()
        {
            auto visit_fn = [&value, &config](auto & f)
            {
                f.add_positional_option(value, config);
            };

            std::visit(std::move(visit_fn), format);
        };

        operations.push_back(std::move(operation));
    }

    //!\cond DEV
    //!\brief A poison overload that catches calls to add_positional_option without explicitly passing a sharg::config.
    template <typename option_type>
    void add_positional_option(option_type &, detail::poison_config const &)
    {
        static_assert(detail::dependent_false_v<option_type>, "Forgot sharg::config?");
    }
    //!\endcond

    //!\}

    /*!\brief Initiates the actual command line parsing.
     *
     * \attention The function must be called at the very end of all parser
     * related code and should be enclosed in a try catch block as the parser may throw.
     *
     * \throws sharg::design_error if this function was already called before.
     * \throws sharg::design_error if the application name or subcommands contain illegal characters.
     * \throws sharg::option_declared_multiple_times if an option that is not a list was declared multiple times.
     * \throws sharg::user_input_error if an incorrect argument is given as (positional) option value.
     * \throws sharg::required_option_missing if the user did not provide a required option.
     * \throws sharg::too_many_arguments if the command line call contained more arguments than expected.
     * \throws sharg::too_few_arguments if the command line call contained less arguments than expected.
     * \throws sharg::validation_error if the argument was not excepted by the provided validator.
     *
     * \details
     *
     * When no specific key words are supplied, the sharg::parser
     * starts to process the command line for specified options, flags and
     * positional options.
     *
     * If the given command line input (`argv`) contains the following keywords (in order of checking), the parser
     * will exit (std::exit) with error code 0 after doing the following:
     *
     * - **-h/\--help** Prints the help page.
     * - **-hh/\--advanced-help** Prints the help page including advanced options.
     * - <b>\--version</b> Prints the version information.
     * - <b>\--export-help [format]</b> Prints the application description in the given format (html/man/ctd).
     * - <b>\--version-check false/0/true/1</b> Disable/enable update notifications.
     *
     * Example:
     *
     * \include test/snippet/parser_2.cpp
     *
     * The code above gives the following output when calling `--help`:
     *
     * ```console
     * MaxMuster$ ./age_app --help
     * The-Age-App
     * ===========
     *
     * OPTIONS
     *     -a, --user-age (signed 32 bit integer)
     *           Please specify your age.
     *
     * VERSION
     *     Last update:
     *     The-Age-App version:
     *     Sharg version: 0.1.0
     *
     * Thanks for using The-Age-App!
     *
     * ```
     *
     * If the user does something wrong, it looks like this:
     * ```console
     * MaxMuster$ ./age_app --foo
     * The Age App - [PARSER ERROR] Unknown option --foo. Please see -h/--help for more information.
     * ```
     *
     * ```console
     * MaxMuster$ ./age_app -a abc
     * The Age App - [PARSER ERROR] Value cast failed for option -a: Argument abc
     *                              could not be casted to type (signed 32 bit integer).
     * ```
     *
     * \stableapi{Since version 1.0.}
     */
    void parse()
    {
        if (parse_was_called)
            throw design_error("The function parse() must only be called once!");

        parse_was_called = true;

        // User input sanitization must happen before version check!
        verify_app_and_subcommand_names();

        // Determine the format and subcommand.
        determine_format_and_subcommand();

        // Apply all defered operations to the parser, e.g., `add_option`, `add_flag`, `add_positional_option`.
        for (auto & operation : operations)
            operation();

        // The version check, which might exit the program, must be called before calling parse on the format.
        run_version_check();

        // Parse the command line arguments.
        parse_format();

        // Exit after parsing any special format.
        if (!std::holds_alternative<detail::format_parse>(format))
            std::exit(EXIT_SUCCESS);
    }

    /*!\brief Returns a reference to the sub-parser instance if
     *       \link subcommand_parse subcommand parsing \endlink was enabled.
     *
     * \details
     * \stableapi{Since version 1.0.}
     */
    parser & get_sub_parser()
    {
        if (sub_parser == nullptr)
        {
            throw design_error("No subcommand was provided at the construction of the argument parser!");
        }

        return *sub_parser;
    }

    /*!\brief Checks whether the option identifier (`id`) was set on the command line by the user.
     * \tparam id_type Either type `char` or a type that a `std::string` is constructible from.
     * \param[in] id The short (`char`) or long (`std::string`) option identifier to search for.
     * \returns `true` if option identifier `id` was set on the command line by the user.
     * \throws sharg::design_error if the function is used incorrectly (see details below).
     *
     * \details
     *
     * You can only ask for option identifiers that were added to the parser beforehand via
     * `sharg::parser::add_option`.
     * As in the `sharg::parser::add_option` call, pass short identifiers as a `char` and long identifiers
     * as a `std::string` or a type that a `std::string` is constructible from (e.g. a `const char *`).
     *
     * ### Example
     *
     * \include test/snippet/is_option_set.cpp
     *
     * ### Exceptions
     *
     * This function throws a sharg::design_error if
     * * `sharg::parser::parse()` was not called before.
     * * a long identifier was passed (e.g. a `std::string`) that only consists of a single character. If you mean to
     *   pass a short identifier, please pass it as a `char` not a `std::string`.
     * * the option identifier cannot be found in the list of valid option identifiers that were added to the parser
     *   via `sharg::parser::add_option()` calls beforehand.
     *
     * \details
     * \stableapi{Since version 1.0.}
     */
    // clang-format off
    template <typename id_type>
        requires std::same_as<id_type, char> || std::constructible_from<std::string, id_type>
    bool is_option_set(id_type const & id) const
    // clang-format on
    {
        if (!parse_was_called)
            throw design_error{"You can only ask which options have been set after calling the function `parse()`."};

        // the detail::format_parse::find_option_id call in the end expects either a char or std::string
        using char_or_string_t = std::conditional_t<std::same_as<id_type, char>, char, std::string>;
        char_or_string_t short_or_long_id = {id}; // e.g. convert char * to string here if necessary

        if constexpr (!std::same_as<id_type, char>) // long id was given
        {
            if (short_or_long_id.size() == 1)
            {
                throw design_error{"Long option identifiers must be longer than one character! If " + short_or_long_id
                                   + "' was meant to be a short identifier, please pass it as a char ('') not a string"
                                     " (\"\")!"};
            }
        }

        if (!used_ids.contains(std::string{id}))
            throw design_error{"You can only ask for option identifiers that you added with add_option() before."};

        // we only need to search for an option before the `option_end_identifier` (`--`)
        auto option_end = std::find(format_arguments.begin(), format_arguments.end(), option_end_identifier);
        auto option_it = detail::format_parse::find_option_id(format_arguments.begin(), option_end, short_or_long_id);
        return option_it != option_end;
    }

    //!\name Structuring the Help Page
    //!\{

    /*!\brief Adds an help page section to the sharg::parser.
     * \param[in] title The title of the section.
     * \param[in] advanced_only If set to true, the section only shows when the user requested the advanced help page.
     * \throws sharg::design_error if sharg::parser::parse was already called.
     * \details
     *
     * This only affects the help page and other output formats.
     *
     * \stableapi{Since version 1.0.}
     */
    void add_section(std::string const & title, bool const advanced_only = false)
    {
        check_parse_not_called("add_section");

        auto operation = [this, title, advanced_only]()
        {
            auto visit_fn = [&title, advanced_only](auto & f)
            {
                f.add_section(title, advanced_only);
            };

            std::visit(std::move(visit_fn), format);
        };

        operations.push_back(std::move(operation));
    }

    /*!\brief Adds an help page subsection to the sharg::parser.
     * \param[in] title The title of the subsection.
     * \param[in] advanced_only If set to true, the section only shows when the user requested the advanced help page.
     * \throws sharg::design_error if sharg::parser::parse was already called.
     * \details
     *
     * This only affects the help page and other output formats.
     *
     * \stableapi{Since version 1.0.}
     */
    void add_subsection(std::string const & title, bool const advanced_only = false)
    {
        check_parse_not_called("add_subsection");

        auto operation = [this, title, advanced_only]()
        {
            auto visit_fn = [&title, advanced_only](auto & f)
            {
                f.add_subsection(title, advanced_only);
            };

            std::visit(std::move(visit_fn), format);
        };

        operations.push_back(std::move(operation));
    }

    /*!\brief Adds an help page text line to the sharg::parser.
     * \param[in] text The text to print.
     * \param[in] is_paragraph Whether to insert as paragraph or just a line (Default: false).
     * \param[in] advanced_only If set to true, the section only shows when the user requested the advanced help page.
     * \throws sharg::design_error if sharg::parser::parse was already called.
     * \details
     * If the line is not a paragraph (false), only one line break is appended, otherwise two line breaks are appended.
     * This only affects the help page and other output formats.
     *
     * \stableapi{Since version 1.0.}
     */
    void add_line(std::string const & text, bool is_paragraph = false, bool const advanced_only = false)
    {
        check_parse_not_called("add_line");

        auto operation = [this, text, is_paragraph, advanced_only]()
        {
            auto visit_fn = [&text, is_paragraph, advanced_only](auto & f)
            {
                f.add_line(text, is_paragraph, advanced_only);
            };

            std::visit(std::move(visit_fn), format);
        };

        operations.push_back(std::move(operation));
    }

    /*!\brief Adds an help page list item (key-value) to the sharg::parser.
     * \param[in] key  The key of the key-value pair of the list item.
     * \param[in] desc The value of the key-value pair of the list item.
     * \param[in] advanced_only If set to true, the section only shows when the user requested the advanced help page.
     * \throws sharg::design_error if sharg::parser::parse was already called.
     *
     * \details
     *
     * Note: This only affects the help page and other output formats.
     *
     * A list item is composed of a key (`key`) and value (`desc`)
     * and usually used for option identifier-description-pairs.
     * E.g.:
     *```console
     *     -a, --age LONG
     *            Super important integer for age.
     *```
     *
     * \stableapi{Since version 1.0.}
     */
    void add_list_item(std::string const & key, std::string const & desc, bool const advanced_only = false)
    {
        check_parse_not_called("add_list_item");

        auto operation = [this, key, desc, advanced_only]()
        {
            auto visit_fn = [&key, &desc, advanced_only](auto & f)
            {
                f.add_list_item(key, desc, advanced_only);
            };

            std::visit(std::move(visit_fn), format);
        };

        operations.push_back(std::move(operation));
    }

    /*!\brief Adds subcommands to the parser.
     * \param[in] subcommands A list of subcommands.
     * \details
     * Adds subcommands to the current parser. The list of subcommands is sorted and duplicates are removed.
     *
     * ### Example
     *
     * \include test/snippet/add_subcommands.cpp
     *
     * \experimentalapi{Experimental since version 1.1.2}
     */
    void add_subcommands(std::vector<std::string> const & subcommands)
    {
        auto & parser_subcommands = this->subcommands;
        parser_subcommands.insert(parser_subcommands.end(), subcommands.cbegin(), subcommands.cend());

        std::ranges::sort(parser_subcommands);
        auto const [first, last] = std::ranges::unique(parser_subcommands);
        parser_subcommands.erase(first, last);
    }
    //!\}

    /*!\brief Aggregates all parser related meta data (see sharg::parser_meta_data struct).
     *
     * \attention You should supply as much information as possible to help users
     * of the application.
     *
     * \details
     *
     * The meta information is assembled in a struct to provide a central access
     * point that can be easily extended.
     *
     * You can access the members directly:
     * (see sharg::parser_meta_data for a list of the info members)
     *
     * \include test/snippet/parser_3.cpp
     *
     * This will produce a nice help page when the user calls `-h` or `--help`:
     *
     * ```console
     * MaxMuster% ./penguin_app --help
     * Penguin_Parade - Organize your penguin parade
     * =============================================
     *
     * DESCRIPTION
     *     First Paragraph.
     *
     *     Second Paragraph.
     *
     * POSITIONAL ARGUMENTS
     *     ARGUMENT-1 List of STRING's
     *           Specify the names of the penguins.
     *
     * OPTIONS
     *     -d, --day (signed 32 bit integer)
     *           Please specify your preferred day.
     *     -m, --month (signed 32 bit integer)
     *           Please specify your preferred month.
     *     -y, --year (signed 32 bit integer)
     *           Please specify your preferred year.
     *
     * EXAMPLES
     *     ./penguin_parade Skipper Kowalski Rico Private -d 10 -m 02 -y 2017
     *
     * VERSION
     *     Last update: 12.01.2017
     *     Penguin_Parade version: 2.0.0
     *     Sharg version: 0.1.0
     * ```
     *
     * \stableapi{Since version 1.0.}
     */
    parser_meta_data info;

private:
    //!\brief Keeps track of whether the parse function has been called already.
    bool parse_was_called{false};

    //!\brief Keeps track of whether the user has added a positional list option to check if this was the very last.
    bool has_positional_list_option{false};

    //!\brief Set on construction and indicates whether the developer deactivates the version check calls completely.
    update_notifications version_check_dev_decision{};

    //!\brief Whether the **user** specified to perform the version check (true) or not (false), default unset.
    std::optional<bool> version_check_user_decision;

    //!\brief Befriend sharg::detail::test_accessor to grant access to version_check_future and format.
    friend struct ::sharg::detail::test_accessor;

    //!\brief The future object that keeps track of the detached version check call thread.
    std::future<bool> version_check_future;

    //!\brief Validates the application name to ensure an escaped server call.
    std::regex app_name_regex{"^[a-zA-Z0-9_-]+$"};

    //!\brief Signals the parser that no options follow this string but only positional arguments.
    static constexpr std::string_view const option_end_identifier{"--"};

    //!\brief Stores the sub-parser in case \link subcommand_parse subcommand parsing \endlink is enabled.
    std::unique_ptr<parser> sub_parser{nullptr};

    //!\brief Stores the sub-parser names in case \link subcommand_parse subcommand parsing \endlink is enabled.
    std::vector<std::string> subcommands{};

    /*!\brief The format of the parser that decides the behavior when
     *        calling the sharg::parser::parse function.
     *
     * \details
     *
     * The format is set in the function parser::init.
     */
    std::variant<detail::format_parse,
                 detail::format_help,
                 detail::format_short_help,
                 detail::format_version,
                 detail::format_html,
                 detail::format_man,
                 detail::format_tdl,
                 detail::format_copyright>
        format{detail::format_short_help{}};

    //!\brief List of option/flag identifiers (excluding -/--) that are already used.
    std::unordered_set<std::string> used_ids{"h", "hh", "help", "advanced-help", "export-help", "version", "copyright"};

    //!\brief The command line arguments that will be passed to the format.
    std::vector<std::string> format_arguments{};

    //!\brief The original command line arguments.
    std::vector<std::string> arguments{};

    //!\brief The command that lead to calling this parser, e.g. [./build/bin/raptor, build]
    std::vector<std::string> executable_name{};

    //!\brief Set of option identifiers (including -/--) that have been added via `add_option`.
    std::unordered_set<std::string> options{};

    //!\brief Vector of functions that stores all calls.
    std::vector<std::function<void()>> operations;

    /*!\brief Handles format and subcommand detection.
     * \throws sharg::too_few_arguments if option --export-help was specified without a value
     * \throws sharg::too_few_arguments if option --version-check was specified without a value
     * \throws sharg::validation_error if the value passed to option --export-help was invalid.
     * \throws sharg::validation_error if the value passed to option --version-check was invalid.
     * \throws sharg::user_input_error if the subcommand is unknown.
     * \details
     *
     * This function adds all command line parameters to the format_arguments member variable
     * to take advantage of the vector functionality later on. Additionally,
     * the format member variable is set, depending on which parameters are given
     * by the user:
     *
     * - **no arguments** If no arguments are provided on the commandline, the
     *                    sharg::detail::format_short_help is set.
     * - **-h/\--help** sets the format to sharg::detail::format_help
     * - **-hh/\--advanced-help** sets the format to sharg::detail::format_help
     *                           and show_advanced_options to `true`.
     * - <b>\--version</b> sets the format to sharg::detail::format_version.
     * - <b>\--export-help html</b> sets the format to sharg::detail::format_html.
     * - <b>\--export-help man</b> sets the format to sharg::detail::format_man.
     * - <b>\--export-help cwl</b> sets the format to sharg::detail::format_tdl{FileFormat::CWL}.
     * - <b>\--export-help ctd</b> sets the format to sharg::detail::format_tdl{FileFormat::CTD}.
     * - else the format is that to sharg::detail::format_parse
     *
     * If `--export-help` is specified with a value other than html, man, cwl or ctd, an sharg::parser_error is thrown.
     */
    void determine_format_and_subcommand()
    {
        assert(!arguments.empty());

        auto it = arguments.begin();
        std::string_view arg{*it};

        executable_name.emplace_back(arg);

        // Helper function for reading the next argument. This makes it more obvious that we are
        // incrementing `it` (version-check, and export-help).
        auto read_next_arg = [this, &it, &arg]() -> bool
        {
            assert(it != arguments.end());

            if (++it == arguments.end())
                return false;

            arg = *it;
            return true;
        };

        // Helper function for finding and processing subcommands.
        auto found_subcommand = [this, &it, &arg]() -> bool
        {
            if (subcommands.empty())
                return false;

            if (std::ranges::find(subcommands, arg) != subcommands.end())
            {
                sub_parser = std::make_unique<parser>(info.app_name + "-" + arg.data(),
                                                      std::vector<std::string>{it, arguments.end()},
                                                      update_notifications::off);

                // Add the original calls to the front, e.g. ["raptor"],
                // s.t. ["raptor", "build"] will be the list after constructing the subparser
                sub_parser->executable_name.insert(sub_parser->executable_name.begin(),
                                                   executable_name.begin(),
                                                   executable_name.end());
                return true;
            }
            else
            {
                // Positional options are forbidden by design.
                // Flags and options, which both start with '-', are allowed for the top-level parser.
                // Otherwise, this is an unknown subcommand.
                if (!arg.starts_with('-'))
                {
                    std::string message = "You specified an unknown subcommand! Available subcommands are: [";
                    for (std::string const & command : subcommands)
                        message += command + ", ";
                    message.replace(message.size() - 2, 2, "]. Use -h/--help for more information.");

                    throw user_input_error{message};
                }
            }

            return false;
        };

        // Process the arguments.
        for (; read_next_arg();)
        {
            // The argument is a known option.
            if (options.contains(std::string{arg}))
            {
                // No futher checks are needed.
                format_arguments.emplace_back(arg);

                // Consume the next argument (the option value) if possible.
                if (read_next_arg())
                {
                    format_arguments.emplace_back(arg);
                    continue;
                }
                else // Too few arguments. This is handled by format_parse.
                {
                    break;
                }
            }

            // If we have a subcommand, all further arguments are passed to the subparser.
            if (found_subcommand())
                break;

            if (arg == "-h" || arg == "--help")
            {
                format = detail::format_help{subcommands, version_check_dev_decision, false};
            }
            else if (arg == "-hh" || arg == "--advanced-help")
            {
                format = detail::format_help{subcommands, version_check_dev_decision, true};
            }
            else if (arg == "--version")
            {
                format = detail::format_version{};
            }
            else if (arg == "--copyright")
            {
                format = detail::format_copyright{};
            }
            else if (arg == "--export-help" || arg.starts_with("--export-help="))
            {
                arg.remove_prefix(std::string_view{"--export-help"}.size());

                // --export-help man
                if (arg.empty())
                {
                    if (!read_next_arg())
                        throw too_few_arguments{"Option --export-help must be followed by a value."};
                }
                else // --export-help=man
                {
                    arg.remove_prefix(1u);
                }

                if (arg == "html")
                    format = detail::format_html{subcommands, version_check_dev_decision};
                else if (arg == "man")
                    format = detail::format_man{subcommands, version_check_dev_decision};
                else if (arg == "ctd")
                    format = detail::format_tdl{detail::format_tdl::FileFormat::CTD};
                else if (arg == "cwl")
                    format = detail::format_tdl{detail::format_tdl::FileFormat::CWL};
                else
                    throw validation_error{"Validation failed for option --export-help: "
                                           "Value must be one of "
                                           + detail::supported_exports + "."};
            }
            else if (arg == "--version-check")
            {
                if (!read_next_arg())
                    throw too_few_arguments{"Option --version-check must be followed by a value."};

                if (arg == "1" || arg == "true")
                    version_check_user_decision = true;
                else if (arg == "0" || arg == "false")
                    version_check_user_decision = false;
                else
                    throw validation_error{"Value for option --version-check must be true (1) or false (0)."};
            }
            else
            {
                // Flags, positional options, options using an alternative syntax (--optionValue, --option=value), etc.
                format_arguments.emplace_back(arg);
            }
        }

        // A special format was set. We do not need to parse the format_arguments.
        if (!std::holds_alternative<detail::format_short_help>(format))
            return;

        // All special options have been handled. If there are arguments left or we have a subparser,
        // we call format_parse. Oterhwise, we print the short help (default variant).
        if (!format_arguments.empty() || sub_parser)
            format = detail::format_parse(format_arguments);
    }

    /*!\brief Checks whether the long identifier has already been used before.
    * \param[in] id The long identifier of the command line option/flag.
    * \returns `true` if an option or flag with the long identifier exists or `false`
    *          otherwise.
    */
    template <typename id_type>
    bool id_exists(id_type const & id)
    {
        if (detail::format_parse::is_empty_id(id))
            return false;
        return (!(used_ids.insert(std::string({id}))).second);
    }

    /*!\brief Verifies that the short and the long identifiers are correctly formatted.
     * \param[in] short_id The short identifier of the command line option/flag.
     * \param[in] long_id  The long identifier of the command line option/flag.
     * \throws sharg::design_error
     * \details Specifically, checks that identifiers haven't been used before,
     *          the length of long IDs is either empty or longer than one char,
     *          the characters used in the identifiers are all valid,
     *          and at least one of short_id or long_id is given.
     */
    void verify_identifiers(char const short_id, std::string const & long_id)
    {
        auto is_valid = [](char const c) -> bool
        {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') // alphanumeric
                || c == '@' || c == '_' || c == '-';                                          // additional characters
        };

        if (short_id == '\0' && long_id.empty())
            throw design_error{"Short and long identifiers may not both be empty."};

        if (short_id != '\0')
        {
            if (short_id == '-' || !is_valid(short_id))
                throw design_error{"Short identifiers may only contain alphanumeric characters, '_', or '@'."};
            if (id_exists(short_id))
                throw design_error{"Short identifier '" + std::string(1, short_id) + "' was already used before."};
        }

        if (!long_id.empty())
        {
            if (long_id.size() == 1)
                throw design_error{"Long identifiers must be either empty or longer than one character."};
            if (long_id[0] == '-')
                throw design_error{"Long identifiers may not use '-' as first character."};
            if (!std::ranges::all_of(long_id, is_valid))
                throw design_error{"Long identifiers may only contain alphanumeric characters, '_', '-', or '@'."};
            if (id_exists(long_id))
                throw design_error{"Long identifier '" + long_id + "' was already used before."};
        }
    }

    //!brief Verify the configuration given to a sharg::parser::add_option call.
    template <typename validator_t>
    void verify_option_config(config<validator_t> const & config)
    {
        verify_identifiers(config.short_id, config.long_id);

        if (config.short_id != '\0')
            options.emplace(std::string{"-"} + config.short_id);
        if (!config.long_id.empty())
            options.emplace(std::string{"--"} + config.long_id);

        if (config.required && !config.default_message.empty())
            throw design_error{"A required option cannot have a default message."};
    }

    //!brief Verify the configuration given to a sharg::parser::add_flag call.
    template <typename validator_t>
    void verify_flag_config(config<validator_t> const & config)
    {
        verify_identifiers(config.short_id, config.long_id);

        if (!config.default_message.empty())
            throw design_error{"A flag may not have a default message because the default is always `false`."};
    }

    //!brief Verify the configuration given to a sharg::parser::add_positional_option call.
    template <typename validator_t>
    void verify_positional_option_config(config<validator_t> const & config) const
    {
        if (config.short_id != '\0' || config.long_id != "")
            throw design_error{"Positional options are identified by their position on the command line. "
                               "Short or long ids are not permitted!"};

        if (config.advanced || config.hidden)
            throw design_error{"Positional options are always required and therefore cannot be advanced nor hidden!"};

        if (!subcommands.empty())
            throw design_error{"You may only specify flags and options for the top-level parser."};

        if (has_positional_list_option)
            throw design_error{"You added a positional option with a list value before so you cannot add "
                               "any other positional options."};

        if (!config.default_message.empty())
            throw design_error{"A positional option may not have a default message because it is always required."};
    }

    /*!\brief Throws a sharg::design_error if parse() was already called.
     * \param[in] function_name The name of the function that was called after parse().
     * \throws sharg::design_error if parse() was already called
     * \details
     * This function is used when calling functions which have no effect (add_line, add_option, ...) or unexpected
     * behavior (add_subcommands) after parse() was called.
     * Has no effect when parse() encounters a special format (help, version, ...), since those will terminate
     * the program.
     */
    inline void check_parse_not_called(std::string_view const function_name) const
    {
        if (parse_was_called)
            throw design_error{detail::to_string(function_name.data(), " may only be used before calling parse().")};
    }

    /*!\brief Verifies that the app and subcommand names are correctly formatted.
     * \throws sharg::design_error if the app name is not correctly formatted.
     * \throws sharg::design_error if the subcommand names are not correctly formatted.
     * \details
     * The app name must only contain alphanumeric characters, '_', or '-'.
     * The subcommand names must only contain alphanumeric characters, '_', or '-'.
     */
    inline void verify_app_and_subcommand_names() const
    {
        // Before creating the detail::version_checker, we have to make sure that
        // malicious code cannot be injected through the app name.
        if (!std::regex_match(info.app_name, app_name_regex))
        {
            throw design_error{("The application name must only contain alpha-numeric characters or '_' and '-' "
                                "(regex: \"^[a-zA-Z0-9_-]+$\").")};
        }

        for (auto & sub : this->subcommands)
        {
            if (!std::regex_match(sub, app_name_regex))
            {
                throw design_error{"The subcommand name must only contain alpha-numeric characters or '_' and '-' "
                                   "(regex: \"^[a-zA-Z0-9_-]+$\")."};
            }
        }
    }

    /*!\brief Runs the version check if the user has not disabled it.
     * \details
     * If the user has not disabled the version check, the function will start a detached thread that will call the
     * sharg::detail::version_checker and print a message if a new version is available.
     */
    inline void run_version_check()
    {
        detail::version_checker app_version{info.app_name, info.version, info.url};

        if (app_version.decide_if_check_is_performed(version_check_dev_decision, version_check_user_decision))
        {
            // must be done before calling parse on the format because this might std::exit
            std::promise<bool> app_version_prom;
            version_check_future = app_version_prom.get_future();
            app_version(std::move(app_version_prom));
        }
    }

    /*!\brief Parses the command line arguments according to the format.
     * \throws sharg::option_declared_multiple_times if an option that is not a list was declared multiple times.
     * \throws sharg::user_input_error if an incorrect argument is given as (positional) option value.
     * \throws sharg::required_option_missing if the user did not provide a required option.
     * \throws sharg::too_many_arguments if the command line call contained more arguments than expected.
     * \throws sharg::too_few_arguments if the command line call contained less arguments than expected.
     * \throws sharg::validation_error if the argument was not excepted by the provided validator.
     * \details
     * This function calls the parse function of the format member variable.
     */
    inline void parse_format()
    {
        auto format_parse_fn = [this]<typename format_t>(format_t & f)
        {
            if constexpr (std::same_as<format_t, detail::format_tdl>)
                f.parse(info, executable_name);
            else
                f.parse(info);
        };

        std::visit(std::move(format_parse_fn), format);
    }
};

} // namespace sharg
