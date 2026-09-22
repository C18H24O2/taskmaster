/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   standalone.cppm                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kiroussa <oss@dynamicdispat.ch>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 22:00:27 by kiroussa          #+#    #+#             */
/*   Updated: 2026/09/22 22:00:28 by kiroussa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

export module taskmaster.standalone;

import std;
import taskmaster.cli.args;

export namespace taskmaster::cli
{

struct StandaloneOptions
{
	std::string configFile;

	static constexpr std::string_view modeName = "standalone";

	static constexpr std::array<LaunchArgument<StandaloneOptions>, 0> arguments{};
	static constexpr auto positionals = std::to_array<PositionalArgument<StandaloneOptions>>({
		{"configFile", &StandaloneOptions::configFile},
	});

	int Run() const;
};

} // namespace taskmaster::cli

int taskmaster::cli::StandaloneOptions::Run() const
{
	std::println("standalone: config = {}", configFile);
	return 0;
}
