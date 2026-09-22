/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cppm                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kiroussa <oss@dynamicdispat.ch>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 22:00:09 by kiroussa          #+#    #+#             */
/*   Updated: 2026/09/22 22:00:28 by kiroussa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

export module taskmaster.client;

import std;
import taskmaster.cli.args;

export namespace taskmaster::cli
{

struct ClientOptions
{
	std::string ip = "127.0.0.1";
	int port = 6969;

	static constexpr std::string_view modeName = "client";

	static constexpr auto arguments = std::to_array<LaunchArgument<ClientOptions>>({
		{'i', "ip", "Server IP to connect to", &ClientOptions::ip},
		{'p', "port", "Server port to connect to", &ClientOptions::port},
	});
	static constexpr std::array<PositionalArgument<ClientOptions>, 0> positionals{};

	int Run() const;
};

} // namespace taskmaster::cli

int taskmaster::cli::ClientOptions::Run() const
{
	std::println("client: connecting to {}:{}", ip, port);
	return 0;
}
