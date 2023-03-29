// --------------------------------------------------------------------------------------------------------
// Copyright (c) 2006-2023, Knut Reinert & Freie Universität Berlin
// Copyright (c) 2016-2023, Knut Reinert & MPI für molekulare Genetik
// This file may be used, modified and/or redistributed under the terms of the 3-clause BSD-License
// shipped with this file and also available at: https://github.com/seqan/sharg-parser/blob/main/LICENSE.md
// --------------------------------------------------------------------------------------------------------

/*!\file
 * \author Svenja Mehringer <svenja.mehringer AT fu-berlin.de>
 * \brief Provides the format_man struct and its helper functions.
 */

#pragma once

#include <sharg/detail/format_base.hpp>
#include <sharg/test/tmp_filename.hpp>

namespace sharg::detail
{

/*!\brief The format that prints the help page information formatted for a man page to std::cout.
 * \ingroup parser
 *
 * \details
 *
 * The help page printing is not done immediately, because the user might not
 * provide meta information, positional options, etc. in the correct order.
 * In addition the needed order would be different from the parse format.
 * Thus the calls are stored (parser_set_up_calls and positional_option_calls)
 * and only evaluated when calling sharg::detail::format_help_base::parse.
 *
 * \remark For a complete overview, take a look at \ref parser
 */
class format_man : public format_help_base<format_man>
{
    //!\brief The CRTP base class type.
    using base_type = format_help_base<format_man>;

    //!\brief Befriend the base class to give access to the private member functions.
    friend base_type;

    //!\brief Whether to call man and open the man page.
    bool open_man_page{false};

public:
    /*!\name Constructors, destructor and assignment
     * \{
     */
    format_man() = default;                                  //!< Defaulted.
    format_man(format_man const & pf) = default;             //!< Defaulted.
    format_man & operator=(format_man const & pf) = default; //!< Defaulted.
    format_man(format_man &&) = default;                     //!< Defaulted.
    format_man & operator=(format_man &&) = default;         //!< Defaulted.
    ~format_man() = default;                                 //!< Defaulted.

    //!\copydoc sharg::detail::format_help_base::format_help_base
    format_man(std::vector<std::string> const & names,
               update_notifications const version_updates,
               bool const advanced = false,
               bool const open_man_page = false) :
        base_type{names, version_updates, advanced},
        open_man_page{open_man_page} {};
    //!\}

    /*!\brief Initiates the printing of the man page to std::cout or opens it in man.
     * \param[in] parser_meta The meta information that are needed for a detailed man page.
     */
    void parse(parser_meta_data & parser_meta)
    {
        if (!open_man_page)
            return base_type::parse(parser_meta);

        sharg::test::tmp_filename tmp_file{parser_meta.app_name.c_str()};

        {
            std::ofstream out{tmp_file.get_path()};
            std::streambuf * coutbuf = std::cout.rdbuf();
            std::cout.rdbuf(out.rdbuf());

            base_type::parse(parser_meta);

            std::cout.rdbuf(coutbuf);
        }

        std::string command{"/usr/bin/man -l "};
        command += tmp_file.get_path().c_str();
        if (std::system(command.c_str()) != 0)
            throw sharg::parser_error{"Unexpected failure."}; // LCOV_EXCL_LINE
    }

private:
    //!\brief Prints a help page header in man page format to std::cout.
    void print_header()
    {
        std::ostream_iterator<char> out(std::cout);

        // Print .TH line.
        std::cout << ".TH ";
        std::transform(meta.app_name.begin(),
                       meta.app_name.end(),
                       out,
                       [](unsigned char c)
                       {
                           return std::toupper(c);
                       });
        std::cout << " " << std::to_string(meta.man_page_section) << " \"" << meta.date << "\" \"";
        std::transform(meta.app_name.begin(),
                       meta.app_name.end(),
                       out,
                       [](unsigned char c)
                       {
                           return std::tolower(c);
                       });
        std::cout << " " << meta.version << "\" \"" << meta.man_page_title << "\"\n";

        // Print NAME section.
        std::cout << ".SH NAME\n" << meta.app_name << " \\- " << meta.short_description << std::endl;
    }

    /*!\brief Prints a section title in man page format to std::cout.
     * \param[in] title The title of the section to print.
     */
    void print_section(std::string const & title)
    {
        std::ostream_iterator<char> out(std::cout);
        std::cout << ".SH ";
        std::transform(title.begin(),
                       title.end(),
                       out,
                       [](unsigned char c)
                       {
                           return std::toupper(c);
                       });
        std::cout << "\n";
        is_first_in_section = true;
    }

    /*!\brief Prints a subsection title in man page format to std::cout.
     * \param[in] title The title of the subsection to print.
     */
    void print_subsection(std::string const & title)
    {
        std::cout << ".SS " << title << "\n";
        is_first_in_section = true;
    }

    /*!\brief Prints a help page section in man page format to std::cout.
     *
     * \param[in] text The text to print.
     * \param[in] line_is_paragraph Whether to insert as paragraph
     *            or just a line (only one line break if not a paragraph).
     */
    void print_line(std::string const & text, bool const line_is_paragraph)
    {
        if (!is_first_in_section && line_is_paragraph)
            std::cout << ".sp\n";
        else if (!is_first_in_section && !line_is_paragraph)
            std::cout << ".br\n";

        std::cout << text << "\n";
        is_first_in_section = false;
    }

    /*!\brief Prints a help page list_item in man page format to std::cout.
     * \param[in] term The key of the key-value pair of the list item.
     * \param[in] desc The value of the key-value pair of the list item.
     *
     * \details
     *
     * A list item is composed of a key (term) and value (desc)
     * and usually used for option identifier-description-pairs.
     */
    void print_list_item(std::string const & term, std::string const & desc)
    {
        std::cout << ".TP\n" << term << "\n" << desc << "\n";
        is_first_in_section = false;
    }

    //!\brief Prints a help page footer in man page format.
    void print_footer()
    {
        // no footer
    }

    /*!\brief Format string as in_bold.
     * \param[in] str The input string to format in bold.
     * \returns The string `str` wrapped in bold formatting.
     */
    std::string in_bold(std::string const & str)
    {
        return "\\fB" + str + "\\fR";
    }

    //!\brief Needed for correct indentation and line breaks.
    bool is_first_in_section{true};
};

} // namespace sharg::detail
