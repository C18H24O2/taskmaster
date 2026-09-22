/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cli.cppm                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kiroussa <oss@dynamicdispat.ch>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 22:02:56 by kiroussa          #+#    #+#             */
/*   Updated: 2026/09/22 22:03:43 by kiroussa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

module;

#include <getopt.h>
#include <cstdio>

export module taskmaster.cli;

import std;
import taskmaster.cli.args;
import taskmaster.client;
import taskmaster.server;
import taskmaster.standalone;

namespace taskmaster::cli
{

template <typename Tuple, typename Func>
constexpr void ForEachMode(Func&& f)
{
	[&]<std::size_t... Is>(std::index_sequence<Is...>) {
		(f.template operator()<std::tuple_element_t<Is, Tuple>>(), ...);
	}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <typename Tuple>
struct TupleToVariant;

template <typename... Ts>
struct TupleToVariant<std::tuple<Ts...>>
{
	static_assert((ModeOptions<Ts> && ...), "every mode type must satisfy ModeOptions");
	using type = std::variant<Ts...>;
};

template <typename T, std::size_t N>
struct GetoptTable
{
	std::array<char, N * 2 + 3> shortOpts{};
	std::array<option, N + 2> longOpts{}; // +2 for --help and end sentinel

	consteval GetoptTable(const std::array<LaunchArgument<T>, N>& args)
	{
		std::size_t si = 0;
		shortOpts[si++] = ':';

		for (std::size_t i = 0; i < N; ++i)
		{
			if (args[i].shortName == 'h')
				throw "shortName 'h' is reserved for --help";
			for (std::size_t j = i + 1; j < N; ++j)
				if (args[i].shortName == args[j].shortName)
					throw "duplicate shortName in this mode's arguments";

			shortOpts[si++] = args[i].shortName;
			if (args[i].NeedsArg())
				shortOpts[si++] = ':';

			longOpts[i] = {
				args[i].longName.data(),
				args[i].NeedsArg() ? required_argument : no_argument,
				nullptr,
				args[i].shortName
			};
		}

		shortOpts[si++] = 'h';
		longOpts[N] = { "help", no_argument, nullptr, 'h' };
		longOpts[N + 1] = { nullptr, 0, nullptr, 0 };
	}
};

template <typename T>
void PrintModeUsage(std::string_view prog)
{
	std::string posPart;
	for (auto&& p : T::positionals)
		posPart += std::string(" <") + std::string(p.name) + ">";

	if constexpr (T::arguments.size() > 0)
		std::println("Usage: {} {} [options...]{}", prog, T::modeName, posPart);
	else
		std::println("Usage: {} {}{}", prog, T::modeName, posPart);

	std::println("\nOptions:");
	if constexpr (T::arguments.size() > 0)
	{
		std::size_t maxLen = 0;
		for (auto&& a : T::arguments)
			maxLen = std::max(maxLen, a.longName.size());
		for (auto&& a : T::arguments)
		{
			std::string spacing(maxLen - a.longName.size(), ' ');
			std::println("  -{}, --{}{}  {}", a.shortName, a.longName, spacing, a.description);
		}
	}
	std::println("  -h, --help  Display this help message");
}

template <typename Tuple>
void PrintTopUsage(std::string_view prog)
{
	std::string names;
	ForEachMode<Tuple>([&]<typename T>() {
		if (!names.empty())
			names += "|";
		names += T::modeName;
	});
	std::println(stderr, "Usage: {} <{}> [options...]", prog, names);
}

enum class ParseStatus { Ok, Help, Error };

template <typename T>
struct ParsedMode
{
	T value{};
	ParseStatus status;
};

template <typename T>
ParsedMode<T> ParseMode(std::string_view prog, std::span<char*> args)
{
	static constexpr std::size_t N = T::arguments.size();
	static constexpr GetoptTable<T, N> table{T::arguments};

	T target{};

	int argc = static_cast<int>(args.size());
	char** argv = args.data();
	optind = 1;
	opterr = 0;

	int opt;
	while ((opt = getopt_long(argc, argv, table.shortOpts.data(), table.longOpts.data(), nullptr)) != -1)
	{
		if (opt == 'h')
			return { std::move(target), ParseStatus::Help };

		if (opt == '?' || opt == ':')
		{
			if (opt == ':')
				std::println(stderr, "{}: option requires an argument -- '{}'", prog, static_cast<char>(optopt));
			else if (optopt != 0)
				std::println(stderr, "{}: invalid option -- '{}'", prog, static_cast<char>(optopt));
			else
				std::println(stderr, "{}: invalid option -- '{}'", prog, std::string_view(argv[optind - 1]));
			return { std::move(target), ParseStatus::Error };
		}

		for (auto&& arg : T::arguments)
		{
			if (arg.shortName != opt)
				continue;

			std::string_view value = arg.NeedsArg() ? std::string_view(optarg) : std::string_view{};
			if (!AssignField(target, arg.fieldPtr, value))
			{
				std::println(stderr, "{}: invalid value for --{}: '{}'", prog, arg.longName, value);
				return { std::move(target), ParseStatus::Error };
			}
			break;
		}
	}

	auto remaining = args.subspan(optind);
	static constexpr std::size_t P = T::positionals.size();

	if (remaining.size() < P)
	{
		std::println(stderr, "{}: missing required argument '{}'", prog, T::positionals[remaining.size()].name);
		return { std::move(target), ParseStatus::Error };
	}
	if (remaining.size() > P)
	{
		std::println(stderr, "{}: unexpected argument '{}'", prog, std::string_view(remaining[P]));
		return { std::move(target), ParseStatus::Error };
	}
	for (std::size_t i = 0; i < P; ++i)
	{
		if (!AssignField(target, T::positionals[i].fieldPtr, remaining[i]))
		{
			std::println(stderr, "{}: invalid value for '{}': '{}'", prog, T::positionals[i].name, remaining[i]);
			return { std::move(target), ParseStatus::Error };
		}
	}

	return { std::move(target), ParseStatus::Ok };
}

template <typename Tuple>
std::optional<typename TupleToVariant<Tuple>::type> ParseTopLevel(int argc, char** argv)
{
	using ModeVariant = typename TupleToVariant<Tuple>::type;
	std::string_view prog = argv[0];

	if (argc < 2)
	{
		PrintTopUsage<Tuple>(prog);
		return std::nullopt;
	}

	std::string_view modeName = argv[1];
	std::span<char*> rest(argv + 1, static_cast<std::size_t>(argc - 1));
	rest[0] = argv[0];

	std::optional<ModeVariant> result;
	bool matchedName = false;

	ForEachMode<Tuple>([&]<typename T>()
	{
		if (matchedName || T::modeName != modeName)
			return;
		matchedName = true;

		auto parsed = ParseMode<T>(prog, rest);
		if (parsed.status == ParseStatus::Ok)
			result = std::move(parsed.value);
		else if (parsed.status == ParseStatus::Help)
			PrintModeUsage<T>(prog);
	});

	if (!matchedName)
	{
		std::println(stderr, "{}: unknown mode -- \"{}\"", prog, modeName);
		PrintTopUsage<Tuple>(prog);
	}

	return result;
}

using AllModes = std::tuple<ClientOptions, ServerOptions, StandaloneOptions>;

export int Run(int argc, char** argv)
{
	auto mode = ParseTopLevel<AllModes>(argc, argv);
	if (!mode)
		return 1;

	return std::visit([]<typename T>(T& opts) { return opts.Run(); }, *mode);
}

} // namespace taskmaster::cli
