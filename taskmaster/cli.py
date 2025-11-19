# -*- coding: utf-8 -*-

import argparse
import pathlib

from .config import TMConfig

def main():
    parser = argparse.ArgumentParser(prog='taskmaster', description='A supervisor clone.')

    subparsers = parser.add_subparsers(dest='mode', required=True, 
                                        help='Operation mode')

    standalone = subparsers.add_parser('run', help='Run in standalone mode')
    standalone.add_argument('config', help='Configuration file (yaml, toml, json)', type=pathlib.Path)
    standalone.add_argument('--daemon', '-d', help='Run as a daemon',
                            action='store_true')

    server = subparsers.add_parser('server', help='Run in server mode')
    server.add_argument('config', help='Configuration file (yaml, toml, json)', type=pathlib.Path)
    server.add_argument('--socket', metavar='PATH',
                        help='Socket path for server',
                        default='/var/run/taskmaster.sock')
    server.add_argument('--daemon', '-d', help='Run as a daemon',
                            action='store_true')

    client = subparsers.add_parser('client', help='Run in client mode')
    client.add_argument('--socket', metavar='PATH',
                        help='Socket path for server',
                        default='/var/run/taskmaster.sock')

    args: argparse.Namespace
    try:
        args = parser.parse_args()
    except Exception as e:
        parser.print_help()
        print(e)
        exit(1)

    if args.mode == 'run':
        try:
            cfg = TMConfig.from_file(args.config, getattr(args, "socket", None), args.daemon)
            import sys
            print(cfg, file=sys.stderr)
        except Exception as e:
            print(e)
            exit(1)

