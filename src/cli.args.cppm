/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cli.args.cppm                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kiroussa <oss@dynamicdispat.ch>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 22:00:13 by kiroussa          #+#    #+#             */
/*   Updated: 2026/09/22 22:00:28 by kiroussa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

export module taskmaster.cli.args;

import std;

export namespace taskmaster::cli
{

template <typename T>
using FieldPtr = std::variant<
	std::string T::*,
	int T::*,
	double T::*,
	bool T::*
>;

template <typename T>
constexpr bool AssignField(T& target, FieldPtr<T> field, std::string_view text)
{
	return std::visit([&](auto ptr) -> bool {
		using F = std::remove_reference_t<decltype(target.*ptr)>;
		if constexpr (std::is_same_v<F, std::string>)
		{
			target.*ptr = std::string(text);
			return true;
		}
		else if constexpr (std::is_same_v<F, bool>)
		{
			target.*ptr = true;
			return true;
		}
		else
		{
			F value{};
			auto [p, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
			if (ec != std::errc{} || p != text.data() + text.size())
				return false;
			target.*ptr = value;
			return true;
		}
	}, field);
}

template <typename T>
struct LaunchArgument
{
	char shortName;
	std::string_view longName;
	std::string_view description;
	FieldPtr<T> fieldPtr;

	constexpr bool NeedsArg() const
	{
		return !std::holds_alternative<bool T::*>(fieldPtr);
	}
};

template <typename T>
struct PositionalArgument
{
	std::string_view name;
	FieldPtr<T> fieldPtr;
};

template <typename T>
concept ModeOptions =
	std::default_initializable<T> &&
	requires {
		{ T::modeName } -> std::convertible_to<std::string_view>;
		{ T::arguments } -> std::ranges::range;
		{ T::positionals } -> std::ranges::range;
	} &&
	requires(const T& t) {
		{ t.Run() } -> std::same_as<int>;
	};

} // namespace taskmaster::cli
