// SPDX-FileCopyrightText: 2006-2024, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2024, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause

/*!\file
 * \author Svenja Mehringer <svenja.mehringer AT fu-berlin.de>
 * \brief Provides the format_parse class.
 */

#pragma once

#include <sharg/std/charconv>

#include <sharg/concept.hpp>
#include <sharg/detail/format_base.hpp>
#include <sharg/detail/id_pair.hpp>

namespace sharg::detail
{

/*!\brief The format that organizes the actual parsing of command line arguments.
 * \ingroup parser
 *
 * \details
 *
 * In order to be independent of the options value type, we do not want to store
 * parameters/options/flags/.. directly (though a variant might work, it is hacky).
 * Directly parsing is also difficult, since the order of parsing options/flags
 * is non trivial (e.g. ambiguousness of '-g 4' => option+value or flag+positional).
 * Therefore, we store the parsing calls of the developer in a function object,
 * (format_parse::option_and_flag_calls, sharg::detail::format_parse::positional_option_calls)
 * executing them in a new order when calling format_parse::parse().
 * This enables us to parse any option type and resolve any ambiguousness, so no
 * additional restrictions apply to the developer when setting up the parser.
 *
 * Order of parsing:
 * -#. Options            (order within as specified by the developer)
 * -#. Flags              (order within as specified by the developer)
 * -#. Positional Options (order within as specified by the developer)
 *
 * When parsing flags and options, the identifiers (and values) are removed from
 * the vector format_parse::arguments. That way, options that are specified multiple times,
 * but are no container type, can be identified and an error is reported.
 *
 * \remark For a complete overview, take a look at \ref parser
 */
class format_parse : public format_base
{
public:
    /*!\name Constructors, destructor and assignment
     * \{
     */
    format_parse() = delete;                                     //!< Deleted.
    format_parse(format_parse const & pf) = default;             //!< Defaulted.
    format_parse & operator=(format_parse const & pf) = default; //!< Defaulted.
    format_parse(format_parse &&) = default;                     //!< Defaulted.
    format_parse & operator=(format_parse &&) = default;         //!< Defaulted.
    ~format_parse() = default;                                   //!< Defaulted.

    /*!\brief The constructor of the parse format.
     * \param[in] cmd_arguments The command line arguments to parse.
     */
    format_parse(std::vector<std::string> cmd_arguments) : arguments{std::move(cmd_arguments)}
    {}
    //!\}

    /*!\brief Adds an sharg::detail::get_option call to be evaluated later on.
     * \copydetails sharg::parser::add_option
     */
    template <typename option_type, typename validator_t>
    void add_option(option_type & value, config<validator_t> const & config)
    {
        option_calls.push_back(
            [this, &value, config]()
            {
                get_option(value, config);
            });
    }

    /*!\brief Adds a get_flag call to be evaluated later on.
     * \copydetails sharg::parser::add_flag
     */
    template <typename validator_t>
    void add_flag(bool & value, config<validator_t> const & config)
    {
        flag_calls.push_back(
            [this, &value, config]()
            {
                get_flag(value, config.short_id, config.long_id);
            });
    }

    /*!\brief Adds a get_positional_option call to be evaluated later on.
     * \copydetails sharg::parser::add_positional_option
     */
    template <typename option_type, typename validator_t>
    void add_positional_option(option_type & value, config<validator_t> const & config)
    {
        positional_option_calls.push_back(
            [this, &value, config]()
            {
                get_positional_option(value, config.validator);
            });
    }

    //!\brief Initiates the actual command line parsing.
    void parse(parser_meta_data const & /*meta*/)
    {
        end_of_options_it = std::find(arguments.begin(), arguments.end(), "--");

        // parse options first, because we need to rule out -keyValue pairs
        // (e.g. -AnoSpaceAfterIdentifierA) before parsing flags
        for (auto && f : option_calls)
            f();

        for (auto && f : flag_calls)
            f();

        check_for_unknown_ids();

        if (end_of_options_it != arguments.end())
            *end_of_options_it = ""; // remove -- before parsing positional arguments

        for (auto && f : positional_option_calls)
            f();

        check_for_left_over_args();
    }

    // functions are not needed for command line parsing but are part of the format help interface.
    //!\cond
    void add_section(std::string const &, bool const)
    {}
    void add_subsection(std::string const &, bool const)
    {}
    void add_line(std::string const &, bool, bool const)
    {}
    void add_list_item(std::string const &, std::string const &, bool const)
    {}
    //!\endcond

    //!\brief Checks whether `id` is empty.
    template <typename id_type>
    static bool is_empty_id(id_type const & id)
    {
        if constexpr (std::same_as<id_type, char>)
            return id == '\0';
        else
            return id.empty();
    }

    /*!\brief Finds the position of a short/long identifier in format_parse::arguments.
     * \tparam iterator_type The type of iterator that defines the range to search in.
     * \tparam id_type The identifier type; must be either of type `char` if it denotes a short identifier or
     *                 std::string if it denotes a long identifier.
     * \param[in] begin_it The iterator where to start the search of the identifier.
     * \param[in] end_it The iterator one past the end of where to search the identifier.
     * \param[in] id The identifier to search for (must not contain dashes).
     * \returns An iterator pointing to the first occurrence of `id` in the list pointed to by `begin_it`
     *          or `end_it` if it is not contained.
     *
     * \details
     *
     * **Valid short-id value pairs are: `-iValue`, `-i=Value`, or `-i Value`**
     * If the `id` passed to this function is of type `char`, it is assumed to be a short identifier.
     * The `id` is found by comparing the prefix of every argument in arguments to the `id` prepended with a single `-`.
     *
     * **Valid long id value pairs are: `--id=Value`, `--id Value`**.
     * If the `id` passed to this function is of type `std::string`, it is assumed to be a long identifier.
     * The `id` is found by comparing every argument in arguments to `id` prepended with two dashes (`--`)
     * or a prefix of such followed by the equal sign `=`.
     */
    template <typename iterator_type>
    static iterator_type find_option_id(iterator_type begin_it, iterator_type end_it, detail::id_pair const & id)
    {
        bool const short_id_empty{id.empty_short_id()};
        bool const long_id_empty{id.empty_long_id()};

        if (short_id_empty && long_id_empty)
            return end_it;

        std::string const short_id = prepend_dash(id.short_id);
        std::string const long_id_equals = prepend_dash(id.long_id) + "=";
        std::string_view const long_id_space = [&long_id_equals]()
        {
            std::string_view tmp{long_id_equals};
            tmp.remove_suffix(1u);
            return tmp;
        }();

        auto cmp = [&](std::string_view const current_arg)
        {
            // check if current_arg starts with "-o", i.e. it correctly identifies all short notations:
            // "-ovalue", "-o=value", and "-o value".
            if (!short_id_empty && current_arg.starts_with(short_id))
                return true;

            // only "--opt Value" or "--opt=Value" are valid
            if (!long_id_empty && (current_arg == long_id_space || current_arg.starts_with(long_id_equals)))
                return true;

            return false;
        };

        return std::find_if(begin_it, end_it, cmp);
    }

private:
    //!\brief Describes the result of parsing the user input string given the respective option value type.
    enum class option_parse_result
    {
        success,       //!< Parsing of user input was successful.
        error,         //!< There was some error while trying to parse the user input.
        overflow_error //!< Parsing was successful but the arithmetic value would cause an overflow.
    };

    /*!\brief Appends a double dash to a long identifier and returns it.
    * \param[in] long_id The name of the long identifier.
    * \returns The input long name prepended with a double dash.
    */
    static std::string prepend_dash(std::string const & long_id)
    {
        return {"--" + long_id};
    }

    /*!\brief Appends a double dash to a short identifier and returns it.
    * \param[in] short_id The name of the short identifier.
    * \returns The input short name prepended with a single dash.
    */
    static std::string prepend_dash(char const short_id)
    {
        return {'-', short_id};
    }

    /*!\brief Returns "-[short_id]/--[long_id]" if both are non-empty or just one of them if the other is empty.
    * \param[in] short_id The name of the short identifier.
    * \param[in] long_id  The name of the long identifier.
    * \returns The short_id prepended with a single dash and the long_id prepended with a double dash, separated by '/'.
    */
    std::string combine_option_names(char const short_id, std::string const & long_id)
    {
        if (short_id == '\0')
            return prepend_dash(long_id);
        else if (long_id.empty())
            return prepend_dash(short_id);
        else // both are set (note: both cannot be empty, this is caught before)
            return prepend_dash(short_id) + "/" + prepend_dash(long_id);
    }

    /*!\brief Returns true and removes the long identifier if it is in format_parse::arguments.
     * \param[in] long_id The long identifier of the flag to check.
     */
    bool flag_is_set(std::string const & long_id)
    {
        auto it = std::find(arguments.begin(), end_of_options_it, prepend_dash(long_id));

        if (it != end_of_options_it)
            *it = ""; // remove seen flag

        return (it != end_of_options_it);
    }

    /*!\brief Returns true and removes the short identifier if it is in format_parse::arguments.
     * \param[in] short_id The short identifier of the flag to check.
     */
    bool flag_is_set(char const short_id)
    {
        // short flags need special attention, since they could be grouped (-rGv <=> -r -G -v)
        for (std::string & arg : arguments)
        {
            if (arg[0] == '-' && arg.size() > 1 && arg[1] != '-') // is option && not dash && no long option
            {
                auto pos = arg.find(short_id);

                if (pos != std::string::npos)
                {
                    arg.erase(pos, 1); // remove seen bool

                    if (arg == "-") // if flag is empty now
                        arg = "";

                    return true;
                }
            }
        }
        return false;
    }

    /*!\brief Tries to parse an input string into a value using the stream `operator>>`.
     * \tparam option_t Must model sharg::istreamable.
     * \param[out] value Stores the parsed value.
     * \param[in] in The input argument to be parsed.
     * \returns sharg::option_parse_result::error if `in` could not be parsed via the stream
     *          operator and otherwise sharg::option_parse_result::success.
     */
    template <typename option_t>
        requires istreamable<option_t>
    option_parse_result parse_option_value(option_t & value, std::string const & in)
    {
        std::istringstream stream{in};
        stream >> value;

        if (stream.fail() || !stream.eof())
            return option_parse_result::error;

        return option_parse_result::success;
    }

    /*!\brief Sets an option value depending on the keys found in sharg::enumeration_names<option_t>.
     * \tparam option_t Must model sharg::named_enumeration.
     * \param[out] value Stores the parsed value.
     * \param[in] in The input argument to be parsed.
     * \throws sharg::user_input_error if `in` is not a key in sharg::enumeration_names<option_t>.
     * \returns sharg::option_parse_result::success.
     */
    template <named_enumeration option_t>
    option_parse_result parse_option_value(option_t & value, std::string const & in)
    {
        auto map = sharg::enumeration_names<option_t>;

        if (auto it = map.find(in); it == map.end())
        {
            std::string keys = [&map]()
            {
                std::vector<std::pair<std::string_view, option_t>> key_value_pairs(map.begin(), map.end());

                std::sort(key_value_pairs.begin(),
                          key_value_pairs.end(),
                          [](auto pair1, auto pair2)
                          {
                              if constexpr (std::totally_ordered<option_t>)
                              {
                                  if (pair1.second != pair2.second)
                                      return pair1.second < pair2.second;
                              }

                              return pair1.first < pair2.first;
                          }); // needed for deterministic output when using unordered maps

                std::string result{'['};
                for (auto const & [key, value] : key_value_pairs)
                    result += std::string{key.data()} + ", ";
                result.replace(result.size() - 2, 2, "]"); // replace last ", " by "]"
                return result;
            }();

            throw user_input_error{"You have chosen an invalid input value: " + in + ". Please use one of: " + keys};
        }
        else
        {
            value = it->second;
        }

        return option_parse_result::success;
    }

    //!\cond
    option_parse_result parse_option_value(std::string & value, std::string const & in)
    {
        value = in;
        return option_parse_result::success;
    }
    //!\endcond

    /*!\brief Parses the given option value and appends it to the target container.
     * \tparam container_option_t Must model sharg::detail::is_container_option and
     *                            its value_type must be parseable via parse_option_value
     * \tparam format_parse_t Needed to make the function "dependent" (i.e. do instantiation in the second phase of
     *                        two-phase lookup) as the requires clause needs to be able to access the other
     *                        parse_option_value overloads.
     *
     * \param[out] value The container that stores the parsed value.
     * \param[in] in The input argument to be parsed.
     * \returns A sharg::option_parse_result whether parsing was successful or not.
     */
    // clang-format off
    template <detail::is_container_option container_option_t, typename format_parse_t = format_parse>
        requires requires (format_parse_t fp,
                           typename container_option_t::value_type & container_value,
                           std::string const & in)
        {
            {fp.parse_option_value(container_value, in)} -> std::same_as<option_parse_result>;
        }
    // clang-format on
    option_parse_result parse_option_value(container_option_t & value, std::string const & in)
    {
        typename container_option_t::value_type tmp{};

        auto res = parse_option_value(tmp, in);

        if (res == option_parse_result::success)
            value.push_back(tmp);

        return res;
    }

    /*!\brief Tries to parse an input string into an arithmetic value.
     * \tparam option_t The option value type; must model std::is_arithmetic_v.
     * \param[out] value Stores the parsed value.
     * \param[in] in The input argument to be parsed.
     * \returns sharg::option_parse_result::error if `in` could not be parsed to an arithmetic type
     *          via std::from_chars, sharg::option_parse_result::overflow_error if `in` could be parsed but the
     *          value is too large for the respective type, and otherwise sharg::option_parse_result::success.
     *
     * \details
     *
     * This function delegates to std::from_chars.
     */
    template <typename option_t>
        requires std::is_arithmetic_v<option_t> && istreamable<option_t>
    option_parse_result parse_option_value(option_t & value, std::string const & in)
    {
        auto res = std::from_chars(&in[0], &in[in.size()], value);

        if (res.ec == std::errc::result_out_of_range)
            return option_parse_result::overflow_error;
        else if (res.ec == std::errc::invalid_argument || res.ptr != &in[in.size()])
            return option_parse_result::error;

        return option_parse_result::success;
    }

    /*!\brief Tries to parse an input string into a boolean value.
     * \param[out] value Stores the parsed value.
     * \param[in] in The input argument to be parsed.
     * \returns A sharg::option_parse_result whether parsing was successful or not.
     *
     * \details
     *
     * This function accepts the strings "0" or "false" which sets sets `value` to `false` or "1" or "true" which
     * sets `value` to `true`.
     */
    option_parse_result parse_option_value(bool & value, std::string const & in)
    {
        if (in == "0")
            value = false;
        else if (in == "1")
            value = true;
        else if (in == "true")
            value = true;
        else if (in == "false")
            value = false;
        else
            return option_parse_result::error;

        return option_parse_result::success;
    }

    /*!\brief Tries to parse an input string into boolean value.
     * \param[in] res A result value of parsing an input string to the respective option value type.
     * \param[in] option_name The name of the option whose input was parsed.
     * \param[in] input_value The original user input in question.
     *
     * \throws sharg::user_input_error if `res` was not sharg::option_parse_result::success.
     */
    template <typename option_type>
    void throw_on_input_error(option_parse_result const res,
                              std::string const & option_name,
                              std::string const & input_value)
    {
        std::string msg{"Value parse failed for " + option_name + ": "};

        if (res == option_parse_result::error)
        {
            throw user_input_error{msg + "Argument " + input_value + " could not be parsed as type "
                                   + get_type_name_as_string(option_type{}) + "."};
        }

        if constexpr (std::is_arithmetic_v<option_type>)
        {
            if (res == option_parse_result::overflow_error)
            {
                throw user_input_error{msg + "Numeric argument " + input_value + " is not in the valid range ["
                                       + std::to_string(std::numeric_limits<option_type>::min()) + ","
                                       + std::to_string(std::numeric_limits<option_type>::max()) + "]."};
            }
        }

        assert(res == option_parse_result::success); // if nothing was thrown, the result must have been a success
    }

    /*!\brief Handles value retrieval for options based on different key-value pairs.
     *
     * \param[out] value     Stores the value found in arguments, parsed by parse_option_value.
     * \param[in]  option_it The iterator where the option identifier was found.
     * \param[in]  id        The option identifier supplied on the command line.
     *
     * \throws sharg::too_few_arguments if the option was not followed by a value.
     * \throws sharg::user_input_error if the given option value was invalid.
     *
     * \details
     *
     * The value at option_it is inspected whether it is an '-key value', '-key=value'
     * or '-keyValue' pair and the input is extracted accordingly. The input
     * will then be tried to be parsed into the `value` parameter.
     *
     * Returns true on success and false otherwise.
     */
    template <typename option_type, typename id_type>
    bool identify_and_retrieve_option_value(option_type & value,
                                            std::vector<std::string>::iterator & option_it,
                                            id_type const & id)
    {
        if (option_it != end_of_options_it)
        {
            std::string input_value;
            size_t id_size = (prepend_dash(id)).size();

            if ((*option_it).size() > id_size) // identifier includes value (-keyValue or -key=value)
            {
                if ((*option_it)[id_size] == '=') // -key=value
                {
                    if ((*option_it).size() == id_size + 1) // malformed because no value follows '-i='
                        throw too_few_arguments("Missing value for option " + prepend_dash(id));
                    input_value = (*option_it).substr(id_size + 1);
                }
                else // -kevValue
                {
                    input_value = (*option_it).substr(id_size);
                }

                *option_it = ""; // remove used identifier-value pair
            }
            else // -key value
            {
                *option_it = ""; // remove used identifier
                ++option_it;
                if (option_it == end_of_options_it) // should not happen
                    throw too_few_arguments("Missing value for option " + prepend_dash(id));
                input_value = *option_it;
                *option_it = ""; // remove value
            }

            auto res = parse_option_value(value, input_value);
            throw_on_input_error<option_type>(res, prepend_dash(id), input_value);

            return true;
        }
        return false;
    }

    /*!\brief Handles value retrieval (non container type) options.
     *
     * \param[out] value Stores the value found in arguments, parsed by parse_option_value.
     * \param[in] id The option identifier supplied on the command line.
     *
     * \throws sharg::option_declared_multiple_times
     *
     * \details
     *
     * If the option identifier is found in format_parse::arguments, the value of
     * the following position in arguments is tried to be parsed given the respective option value type
     * and the identifier and value argument are removed from arguments.
     *
     * Returns true on success and false otherwise. This is needed to catch
     * the user error of supplying multiple arguments for the same
     * (non container!) option by specifying the short AND long identifier.
     */
    template <typename option_type, typename id_type>
    bool get_option_by_id(option_type & value, id_type const & id)
    {
        auto it = find_option_id(arguments.begin(), end_of_options_it, id);

        if (it != end_of_options_it)
            identify_and_retrieve_option_value(value, it, id);

        if (find_option_id(it, end_of_options_it, id) != end_of_options_it) // should not be found again
            throw option_declared_multiple_times("Option " + prepend_dash(id)
                                                 + " is no list/container but declared multiple times.");

        return (it != end_of_options_it); // first search was successful or not
    }

    /*!\brief Handles value retrieval (container type) options.
     *
     * \param[out] value Stores all values found in arguments, parsed by parse_option_value.
     * \param[in]  id    The option identifier supplied on the command line.
     *
     * \details
     *
     * Since option_type is a container, the option is a list and can be parsed
     * multiple times.
     *
     */
    template <detail::is_container_option option_type, typename id_type>
    bool get_option_by_id(option_type & value, id_type const & id)
    {
        auto it = find_option_id(arguments.begin(), end_of_options_it, id);
        bool seen_at_least_once{it != end_of_options_it};

        if (seen_at_least_once)
            value.clear();

        while (it != end_of_options_it)
        {
            identify_and_retrieve_option_value(value, it, id);
            it = find_option_id(it, end_of_options_it, id);
        }

        return seen_at_least_once;
    }

    /*!\brief Checks format_parse::arguments for unknown options/flags.
     *
     * \throws sharg::unknown_option
     *
     * \details
     *
     * This function is used by format_parse::parse() AFTER all flags and options
     * specified by the developer were parsed and therefore removed from arguments.
     * Thus, all remaining flags/options are unknown.
     *
     * In addition this function removes "--" (if specified) from arguments to
     * clean arguments for positional option retrieval.
     */
    void check_for_unknown_ids()
    {
        for (auto it = arguments.begin(); it != end_of_options_it; ++it)
        {
            std::string arg{*it};
            if (!arg.empty() && arg[0] == '-') // may be an identifier
            {
                if (arg == "-")
                {
                    continue; // positional option
                }
                else if (arg[1] != '-' && arg.size() > 2) // one dash, but more than one character (-> multiple flags)
                {
                    throw unknown_option("Unknown flags " + expand_multiple_flags(arg)
                                         + ". In case this is meant to be a non-option/argument/parameter, "
                                         + "please specify the start of arguments with '--'. "
                                         + "See -h/--help for program information.");
                }
                else // unknown short or long option
                {
                    throw unknown_option("Unknown option " + arg
                                         + ". In case this is meant to be a non-option/argument/parameter, "
                                         + "please specify the start of non-options with '--'. "
                                         + "See -h/--help for program information.");
                }
            }
        }
    }

    /*!\brief Checks format_parse::arguments for unknown options/flags.
     *
     * \throws sharg::too_many_arguments
     *
     * \details
     *
     * This function is used by format_parse::parse() AFTER all flags, options
     * and positional options specified by the developer were parsed and
     * therefore removed from arguments.
     * Thus, all remaining non-empty arguments are too much.
     */
    void check_for_left_over_args()
    {
        if (std::find_if(arguments.begin(),
                         arguments.end(),
                         [](std::string const & s)
                         {
                             return (s != "");
                         })
            != arguments.end())
            throw too_many_arguments("Too many arguments provided. Please see -h/--help for more information.");
    }

    /*!\brief Handles command line option retrieval.
     *
     * \param[out] value The variable in which to store the given command line argument.
     * \param[in] config A configuration object to customise the sharg::parser behaviour. See sharg::config.
     *
     * \throws sharg::option_declared_multiple_times
     * \throws sharg::validation_error
     * \throws sharg::required_option_missing
     *
     * \details
     *
     * This function
     * - checks if the option is required but not set,
     * - retrieves any value found by the short or long identifier,
     * - throws on (mis)use of both identifiers for non-container type values,
     * - re-throws the validation exception with appended option information.
     */
    template <typename option_type, typename validator_t>
    void get_option(option_type & value, config<validator_t> const & config)
    {
        bool short_id_is_set{get_option_by_id(value, config.short_id)};
        bool long_id_is_set{get_option_by_id(value, config.long_id)};

        // if value is no container we need to check for multiple declarations
        if (short_id_is_set && long_id_is_set && !detail::is_container_option<option_type>)
            throw option_declared_multiple_times("Option " + combine_option_names(config.short_id, config.long_id)
                                                 + " is no list/container but specified multiple times");

        if (short_id_is_set || long_id_is_set)
        {
            try
            {
                config.validator(value);
            }
            catch (std::exception & ex)
            {
                throw validation_error(std::string("Validation failed for option ")
                                       + combine_option_names(config.short_id, config.long_id) + ": " + ex.what());
            }
        }
        else // option is not set
        {
            // check if option is required
            if (config.required)
                throw required_option_missing("Option " + combine_option_names(config.short_id, config.long_id)
                                              + " is required but not set.");
        }
    }

    /*!\brief Handles command line flags, whether they are set or not.
     *
     * \param[out] value    The variable which shows if the flag is turned off (default) or on.
     * \param[in]  short_id The short identifier for the flag (e.g. 'i').
     * \param[in]  long_id  The long identifier for the flag (e.g. "integer").
     *
     */
    void get_flag(bool & value, char const short_id, std::string const & long_id)
    {
        // `|| value` is needed to keep the value if it was set before.
        // It must be last because `flag_is_set` removes the flag from the arguments.
        value = flag_is_set(short_id) || flag_is_set(long_id) || value;
    }

    /*!\brief Handles command line positional option retrieval.
     *
     * \param[out] value     The variable in which to store the given command line argument.
     * \param[in]  validator The validator applied to the value after parsing (callable).
     *
     * \throws sharg::parser_error
     * \throws sharg::too_few_arguments
     * \throws sharg::validation_error
     * \throws sharg::design_error
     *
     * \details
     *
     * This function assumes that
     * -#) arguments has been stripped from all known options and flags
     * -#) arguments has been checked for unknown options
     * -#) arguments does not contain "--" anymore
     *  Thus we can simply iterate over non empty entries of arguments.
     *
     * This function
     * - checks if the user did not provide enough arguments,
     * - retrieves the next (no container type) or all (container type) remaining non empty value/s in arguments
     */
    template <typename option_type, typename validator_type>
    void get_positional_option(option_type & value, validator_type && validator)
    {
        ++positional_option_count;
        auto it = std::find_if(arguments.begin(),
                               arguments.end(),
                               [](std::string const & s)
                               {
                                   return (s != "");
                               });

        if (it == arguments.end())
            throw too_few_arguments("Not enough positional arguments provided (Need at least "
                                    + std::to_string(positional_option_calls.size())
                                    + "). See -h/--help for more information.");

        if constexpr (detail::is_container_option<
                          option_type>) // vector/list will be filled with all remaining arguments
        {
            assert(positional_option_count == positional_option_calls.size()); // checked on set up.

            value.clear();

            while (it != arguments.end())
            {
                auto res = parse_option_value(value, *it);
                std::string id = "positional option" + std::to_string(positional_option_count);
                throw_on_input_error<option_type>(res, id, *it);

                *it = ""; // remove arg from arguments
                it = std::find_if(it,
                                  arguments.end(),
                                  [](std::string const & s)
                                  {
                                      return (s != "");
                                  });
                ++positional_option_count;
            }
        }
        else
        {
            auto res = parse_option_value(value, *it);
            std::string id = "positional option" + std::to_string(positional_option_count);
            throw_on_input_error<option_type>(res, id, *it);

            *it = ""; // remove arg from arguments
        }

        try
        {
            validator(value);
        }
        catch (std::exception & ex)
        {
            throw validation_error("Validation failed for positional option " + std::to_string(positional_option_count)
                                   + ": " + ex.what());
        }
    }

    //!\brief Stores get_option calls to be evaluated when calling format_parse::parse().
    std::vector<std::function<void()>> option_calls;
    //!\brief Stores get_flag calls to be evaluated when calling format_parse::parse().
    std::vector<std::function<void()>> flag_calls;
    //!\brief Stores get_positional_option calls to be evaluated when calling format_parse::parse().
    std::vector<std::function<void()>> positional_option_calls;
    //!\brief Keeps track of the number of specified positional options.
    unsigned positional_option_count{0};
    //!\brief Vector of command line arguments.
    std::vector<std::string> arguments;
    //!\brief Artificial end of arguments if \-- was seen.
    std::vector<std::string>::iterator end_of_options_it;
};

} // namespace sharg::detail
