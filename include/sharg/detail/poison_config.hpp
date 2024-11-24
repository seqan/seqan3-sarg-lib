// SPDX-FileCopyrightText: 2006-2024, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2024, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause

/*!\file
 * \author Enrico Seiler <enrico.seiler AT fu-berlin.de>
 * \brief Provides sharg::detail::poison_config.
 */

#pragma once

#include <sharg/config.hpp>

namespace sharg::detail
{

/*!\brief This struct is used to prevent the user from calling sharg::parser::add_option, sharg::parser::add_flag,
 *        and sharg::parser::add_positional_option without a sharg::config.
 * \ingroup parser
 * \details
 * This prevents (and produces a legible error message) for calls like:
 * ```
 * parser.add_option(value, {.short_id = 'i', .long_id = "int", .description = "Desc."});
 * ```
 * instead of
 * ```
 * parser.add_option(value, sharg::config{.short_id = 'i', .long_id = "int", .description = "Desc."});
 * ```
 */
struct poison_config
{
    char short_id{'\0'};           //!< Same as sharg::config::short_id.
    std::string long_id{};         //!< Same as sharg::config::long_id.
    std::string description{};     //!< Same as sharg::config::description.
    std::string default_message{}; //!< Same as sharg::config::default_message.
    bool advanced{false};          //!< Same as sharg::config::advanced.
    bool hidden{false};            //!< Same as sharg::config::hidden.
    bool required{false};          //!< Same as sharg::config::required.
    std::any validator{};          //!< Prevents CTAD inside a function call, which would cause a compiler error.
};

/*!\brief A validator used for comparing the size of sharg::config and sharg::poison_config.
 * \ingroup parser
 * \details
 * The `sizeof(std::any)` is typically `16`, while `sizeof(sharg::detail::default_validator)` is `1`.
 * `poison_config_size_comp_validator` provides a validator whose size is the same as the size of `std::any`.
 */
struct poison_config_size_comp_validator : public detail::default_validator
{
    std::any validator{}; //!< A member such that the sizes of sharg::config and sharg::poison_config are the same.
};

/*!\concept sharg::detail::poison_config_valid
 * \ingroup parser
 * \brief Concept that checks that sharg::poison_config has the same members as sharg::config.
 * \tparam validator_t The validator to use. Defaults to sharg::detail::poison_config_size_comp_validator.
 * \details
 * * Sizes of sharg::config and sharg::detail::poison_config are the same.
 * * sharg::config and sharg::detail::poison_config have the same member types (except validator).
 * * sharg::detail::poison_config can be constructed with designated initializers from sharg::config's members.
 * * sharg::config can be constructed with designated initializers from sharg::detail::poison_config's members.
 * The latter two ensure that the order of the members is the same.
 */
template <typename validator_t = poison_config_size_comp_validator>
concept poison_config_valid =
    (sizeof(poison_config) == sizeof(config<validator_t>))
    && std::same_as<decltype(poison_config{}.short_id), decltype(config<validator_t>{}.short_id)>
    && std::same_as<decltype(poison_config{}.long_id), decltype(config<validator_t>{}.long_id)>
    && std::same_as<decltype(poison_config{}.description), decltype(config<validator_t>{}.description)>
    && std::same_as<decltype(poison_config{}.default_message), decltype(config<validator_t>{}.default_message)>
    && std::same_as<decltype(poison_config{}.advanced), decltype(config<validator_t>{}.advanced)>
    && std::same_as<decltype(poison_config{}.hidden), decltype(config<validator_t>{}.hidden)>
    && std::same_as<decltype(poison_config{}.required), decltype(config<validator_t>{}.required)>
    && requires (config<validator_t> cfg, poison_config poison_cfg) {
           {
               poison_config{.short_id = cfg.short_id,
                             .long_id = cfg.long_id,
                             .description = cfg.description,
                             .default_message = cfg.default_message,
                             .advanced = cfg.advanced,
                             .hidden = cfg.hidden,
                             .required = cfg.required,
                             .validator = cfg.validator}
           };
           {
               config<validator_t>{.short_id = poison_cfg.short_id,
                                   .long_id = poison_cfg.long_id,
                                   .description = poison_cfg.description,
                                   .default_message = poison_cfg.default_message,
                                   .advanced = poison_cfg.advanced,
                                   .hidden = poison_cfg.hidden,
                                   .required = poison_cfg.required,
                                   .validator = std::any_cast<validator_t>(poison_cfg.validator)}
           };
       };

static_assert(poison_config_valid<>, "sharg::detail::poison_config must have the same members as sharg::config!");

/*!\brief This is a workaround for compilers that do not implement CWG2518 (GCC 11, GCC 12).
 * \ingroup parser
 * \sa https://en.cppreference.com/w/cpp/language/if#Constexpr_if
 * \sa https://cplusplus.github.io/CWG/issues/2518.html
 * \details
 * Before CWG2518, a (discarded) statement couldn't be false in every case, e.g.
 * ```cpp
 * template <typename option_type>
 * void add_option(option_type)
 * {
 *     if constexpr (std::is_same_v<option_type, int>)
 *     {
 *         return;
 *     }
 *     else
 *     {
 *         static_assert(false, "Should never happen");                          // invalid before CWG2518
 *         static_assert(dependent_false_v<option_type>, "Should never happen"); // valid
 *     }
 * }
 * ```
 */
template <typename>
inline constexpr bool dependent_false_v = false;

} // namespace sharg::detail
