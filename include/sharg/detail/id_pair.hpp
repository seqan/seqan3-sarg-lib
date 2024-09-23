// SPDX-FileCopyrightText: 2006-2024, Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2024, Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause

/*!\file
 * \author Enrico Seiler <enrico.seiler AT fu-berlin.de>
 * \brief Provides sharg::detail::id_pair.
 */

#pragma once

#include <algorithm>
#include <string>
#include <unordered_set>

#include <sharg/platform.hpp>

namespace sharg::detail
{

/*!\brief A simple struct to store a short and a long identifier for an option.
 * \ingroup parser
 */
struct id_pair
{
    char short_id{};       //!< The short identifier for the option.
    std::string long_id{}; //!< The long identifier for the option.

    id_pair() = default;                            //!< Defaulted.
    id_pair(id_pair const &) = default;             //!< Defaulted.
    id_pair & operator=(id_pair const &) = default; //!< Defaulted.
    id_pair(id_pair &&) = default;                  //!< Defaulted.
    id_pair & operator=(id_pair &&) = default;      //!< Defaulted.
    ~id_pair() = default;                           //!< Defaulted.

    //!\brief Construct an id_pair from a short ID.
    id_pair(char const short_id) : short_id{short_id}
    {}

    //!\brief Construct an id_pair from a long ID.
    id_pair(std::string long_id) : long_id{std::move(long_id)}
    {}

    //!\brief Construct an id_pair from a short and long ID.
    id_pair(char const short_id, std::string long_id) : short_id{short_id}, long_id{std::move(long_id)}
    {}

    /*!\brief Two id_pairs are equal if their short **or** long ID is equal.
     * Empty IDs are not considered for equality. If both IDs are empty, the id_pairs are considered **not** equal.
     */
    friend bool operator==(id_pair const & lhs, id_pair const & rhs)
    {
        return (!lhs.empty_short_id() && lhs.short_id == rhs.short_id)
            || (!lhs.empty_long_id() && lhs.long_id == rhs.long_id);
    }

    /*!\brief Compares the given short ID with the short ID of the id_pair.
     * Returns false if the id_pair's short ID is empty.
     */
    friend bool operator==(id_pair const & lhs, char const & rhs)
    {
        return !lhs.empty_short_id() && lhs.short_id == rhs;
    }

    /*!\brief Compares the given long ID with the long ID of the id_pair.
     * Returns false if the id_pair's long ID is empty.
     */
    friend bool operator==(id_pair const & lhs, std::string const & rhs)
    {
        return !lhs.empty_long_id() && lhs.long_id == rhs;
    }

    //!\brief Returns true if the short ID is empty.
    bool empty_short_id() const noexcept
    {
        return empty(short_id);
    }

    //!\brief Returns true if the long ID is empty.
    bool empty_long_id() const noexcept
    {
        return empty(long_id);
    }

    //!\brief Returns true if both IDs are empty.
    bool empty() const noexcept
    {
        return empty_short_id() && empty_long_id();
    }

    //!\brief Checks whether id is empty.
    template <typename id_type>
    static bool empty(id_type const & id) noexcept;

    // Note: The following two functions are declared, but not defined.
    // We first need to specialise std::hash<id_pair> in the std namespace.
    // After that, we can define the functions.
    // Defining them here would generate std::hash<id_pair> before the specialisation.
    // 1.) Now there are two specialisations for std::hash<id_pair> (error)
    // 2.) The default-generated std::hash<id_pair> does actually not work

    //!\brief Finds an id_pair in a set of used ids.
    template <typename id_type>
    static auto find(std::unordered_set<id_pair> const & used_ids, id_type const & id);

    //!\brief Checks whether an id is already contained in a set of used ids.
    template <typename id_type>
    static bool contains(std::unordered_set<id_pair> const & used_ids, id_type const & id);
};

} // namespace sharg::detail

namespace std
{

/*!\brief Hash specialization for sharg::detail::id_pair.
 * \ingroup parser
 */
template <>
struct hash<sharg::detail::id_pair>
{
    //!\brief Computes the hash value for a given id_pair.
    size_t operator()(sharg::detail::id_pair const & value) const noexcept
    {
        size_t const h1 = std::hash<char>{}(value.short_id);
        size_t const h2 = std::hash<std::string>{}(value.long_id);
        return h1 ^ (h2 << 1);
    }
};

} // namespace std

namespace sharg::detail
{

template <typename id_type>
inline bool id_pair::empty(id_type const & id) noexcept
{
    if constexpr (std::same_as<id_type, id_pair>)
        return id.empty();
    else if constexpr (std::same_as<id_type, char>)
        return id == '\0';
    else
        return id.empty();
}

template <typename id_type>
inline auto id_pair::find(std::unordered_set<id_pair> const & used_ids, id_type const & id)
{
    if (empty(id))
        return used_ids.end();

    return std::ranges::find_if(used_ids,
                                [&id](id_pair const & pair)
                                {
                                    return pair == id;
                                });
}

template <typename id_type>
inline bool id_pair::contains(std::unordered_set<id_pair> const & used_ids, id_type const & id)
{
    return find(used_ids, id) != used_ids.end();
}

} // namespace sharg::detail
