# -*- coding: utf-8 -*-

import json
import pathlib
import tomllib as toml
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from enum import Enum
import traceback


KNOWN_KEYS = [
    'services',

    'services.?.name',
    'services.?.command',

    'services.?.deploy.replicas',
    'services.?.deploy.autostart',
    'services.?.deploy.restart',

    'services.?.health.return_codes',
    'services.?.health.timeout',
    'services.?.health.retries',

    'services.?.lifecycle.stop_signal',
    'services.?.lifecycle.stop_timeout',

    'services.?.log.stdout',
    'services.?.log.stderr',

    'services.?.env.cwd',
    'services.?.env.user',
    'services.?.env.group',
    'services.?.env.umask',
    'services.?.env.variables',
    'services.?.env.variables.?',
]


class TMServiceRestartPolicy(str, Enum):
    always = 'always'
    unexpected = 'on-failure'
    unless_stopped = 'unless-stopped'
    never = 'no'


@dataclass(frozen=True)
class TMServiceDeployConfig:
    # The number of parallel processes replicas to run
    replicas: int = 1
    # Whether to start the service when taskmaster starts
    autostart: bool = False
    # When the service should restart
    restart: TMServiceRestartPolicy = TMServiceRestartPolicy.never


@dataclass(frozen=True)
class TMServiceHealthConfig:
    # The time to wait for a service to be running before considering it healthy
    timeout: int = 0
    # The number of times to retry the service
    retries: int = 0
    # The return codes that are considered successful exits
    return_codes: list[int] = field(default_factory=lambda: [0])


@dataclass(frozen=True)
class TMServiceLifecycleConfig:
    # The signal to send to the service to stop it
    stop_signal: str = 'SIGTERM'
    # The time to wait for the service to stop before sending a SIGKILL
    stop_timeout: int = 10


@dataclass(frozen=True)
class TMServiceLogConfig:
    # The path to the stdout log file
    stdout: str = '/dev/stdout'
    # The path to the stderr log file
    stderr: str = '/dev/stderr'


@dataclass(frozen=True)
class TMServiceEnvConfig:
    # The working directory of the service
    cwd: str = '.'
    # The user to run the service as (either a user name or UID)
    user: str = '-1'
    # The group to run the service as (either a group name or GID)
    group: str = '-1'
    # The umask to use when creating the service's files
    umask: str = '0022'
    # A dictionary of environment variables to set
    variables: dict[str, str] = field(default_factory=lambda: {})


@dataclass(frozen=True)
class TMServiceConfig:
    name: str
    command: str
    deploy: TMServiceDeployConfig = field(default_factory=TMServiceDeployConfig)
    health: TMServiceHealthConfig = field(default_factory=TMServiceHealthConfig)
    lifecycle: TMServiceLifecycleConfig = field(default_factory=TMServiceLifecycleConfig)
    log: TMServiceLogConfig = field(default_factory=TMServiceLogConfig)
    env: TMServiceEnvConfig = field(default_factory=TMServiceEnvConfig)


def dict_to_dataclass(data: dict, cls: type):
    """Convert nested dict to dataclass instance."""
    if not hasattr(cls, '__dataclass_fields__'):
        return data
    
    field_values = {}
    for field_name, field_def in cls.__dataclass_fields__.items():
        if field_name not in data:
            continue
        
        value = data[field_name]
        field_type = field_def.type
        
        if hasattr(field_type, '__dataclass_fields__'):
            value = dict_to_dataclass(value, field_type)
        elif isinstance(field_type, type) and issubclass(field_type, Enum):
            value = field_type(value)
        
        field_values[field_name] = value
    
    return cls(**field_values)


class TMConfig:
    path: pathlib.Path
    socket: pathlib.Path | None
    daemon: bool
    services: list[TMServiceConfig]

    def __init__(
            self,
            path: pathlib.Path,
            socket: pathlib.Path,
            daemon: bool,
            data: dict[str, object]
    ):
        self.path = path
        self.socket = socket
        self.daemon = daemon
        self.data = data

        def check_keys(d: dict[str, object], keys: list[str], depth=0):
            for k in d.keys():
                next_keys = []
                for k2 in keys:
                    try:
                        first = k2.split('.')[depth]
                    except IndexError:
                        continue
                    if first == '?':
                        next_keys.append(k2)
                        continue
                    if k == first:
                        next_keys.append(k2)
                        continue
                if len(next_keys) == 0:
                    raise ValueError(f'Unknown key {k}')
                v = d[k]
                if type(v) is dict:
                    check_keys(v, next_keys, depth + 1)
        check_keys(self.data, KNOWN_KEYS)

        self.services = []
        smap = self.data.get('services', {})
        if type(smap) is not dict:
            raise ValueError('services must be a dictionary')
        for name in smap.keys():
            sdata = smap[name]
            sdata['name'] = name
            try:
                self.services.append(dict_to_dataclass(sdata, TMServiceConfig))
            except Exception as e:
                traceback.print_exc()
                raise ValueError(f'Invalid service {name}: {e}')

    def __repr__(self):
        return f'TMConfig(path={self.path}, socket={self.socket}, daemon={self.daemon}, services={self.services})'


    @staticmethod
    def from_file(
            path: pathlib.Path,
            socket: pathlib.Path,
            daemon: bool
    ) -> 'TMConfig':
        data: dict[str, object]

        ext = path.suffix.lower()
        if ext == '.toml':
            with path.open('rb') as f:
                data = toml.load(f)
        elif ext == '.json':
            with path.open('rb') as f:
                data = json.load(f)
        elif ext == '.xml':
            def xml_to_dict(element) -> dict[str, object] | object | None:
                if len(element) == 0:
                    return element.text.strip() if element.text else None
                
                result = {}
                for child in element:
                    v = xml_to_dict(child)
                    if v is not None:
                        result[child.tag] = v
                return result

            tree = ET.parse(path)
            root = tree.getroot()

            d = xml_to_dict(root)
            data = d if d is not None and type(d) is dict else {}
        else:
            raise ValueError(f'The filetype could not be recognized, please rename the configuration file with a supported extension (xml, toml, json)')
        if len(data) == 0:
            raise ValueError('Empty configuration file')

        return TMConfig(path, socket, daemon, data)
