/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cppm                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kiroussa <oss@dynamicdispat.ch>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 22:00:22 by kiroussa          #+#    #+#             */
/*   Updated: 2026/09/22 22:00:28 by kiroussa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

export module taskmaster.server;

import std;
import taskmaster.cli.args;

export namespace taskmaster::cli
{

struct ServerOptions
{
	std::string ip = "127.0.0.1";
	int port = 6969;
	std::string configFile;

	static constexpr std::string_view modeName = "server";

	static constexpr auto arguments = std::to_array<LaunchArgument<ServerOptions>>({
		{'i', "ip", "Address to listen on", &ServerOptions::ip},
		{'p', "port", "Port to listen on", &ServerOptions::port},
	});
	static constexpr auto positionals = std::to_array<PositionalArgument<ServerOptions>>({
		{"configFile", &ServerOptions::configFile},
	});

	int Run() const;
};

} // namespace taskmaster::cli

int taskmaster::cli::ServerOptions::Run() const
{
	std::println("server: listening on {}:{}, config = {}", ip, port, configFile);
	return 0;
}
