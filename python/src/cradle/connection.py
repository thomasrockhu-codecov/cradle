from typing import Any, Dict, Optional

import msgpack  # type: ignore
import yaml  # type: ignore
import requests  # type: ignore
import websocket  # type: ignore
import uuid

import cradle.command as cmd
import cradle.exceptions as exc
import cradle.util as util


Request = Dict
Response = Any
WebsocketConnection = Any


class Connection:
    def __init__(self):
        self.read_config()
        self.connect_to_server()
        self.retrieve_realm_context()

    def read_config(self) -> None:
        with open('config.yml') as config_file:
            self.config = yaml.full_load(config_file)
        self.headers = {
            'Authorization': 'Bearer ' + self.config['api_token'],
            'Content-Type': 'application/json',
            'Accept': 'application/json'
        }
        self.realm = self.config['realm_name']

    def connect_to_server(self) -> None:
        """Connect to the CRADLE server."""
        self.ws = websocket.create_connection(self.config['cradle_url'])

    def retrieve_realm_context(self) -> None:
        """Get the context associated with the app in the config file."""
        url = f'{self.api_url}/iam/realms/{self.realm}/context'
        response = requests.get(url, headers=self.headers)
        response.raise_for_status()
        self.context_id = response.json()['id']

    @property
    def api_url(self) -> str:
        return self.config['api_url']

    @property
    def api_token(self) -> str:
        return self.config['api_token']

    def send_request(self, command: cmd.Command) -> None:
        performer = CommandPerformer(self.ws, self.context_id, command)
        performer.send_request()

    def receive_response(self, command: cmd.Command) -> Optional[cmd.Object]:
        performer = CommandPerformer(self.ws, self.context_id, command)
        return performer.receive_response()

    def perform_command(self, command: cmd.Command) -> Optional[cmd.Object]:
        performer = CommandPerformer(self.ws, self.context_id, command)
        return performer.perform()


class CommandPerformer:
    def __init__(self, ws: WebsocketConnection, context_id: str, command: cmd.Command):
        self.ws = ws
        self.context_id = context_id
        self.command = command
        self.request_id = uuid.uuid4().hex

    def perform(self) -> Optional[cmd.Object]:
        request = self.send_request()
        return self.receive_response(request)

    def send_request(self) -> Request:
        request = self._create_request()
        self.ws.send_binary(msgpack.packb(request, use_bin_type=True))
        return request

    def receive_response(self, matching_request: Optional[Request] = None) -> Optional[cmd.Object]:
        response = msgpack.unpackb(self.ws.recv(), use_list=False, raw=False)
        if matching_request is not None:
            self._check_response(matching_request, response)
        return self.command.decode_content(response['content'])

    def _create_request(self) -> Request:
        content = self.command.create_content()
        return \
            {
                'request_id': self.request_id,
                'content': content
            }

    def _check_response(self, request: Request, response: Response) -> None:
        if response['request_id'] != self.request_id:
            raise exc.RequestIdMismatchError(request, response)
        if util.union_tag(response['content']) == 'error':
            raise exc.ErrorResponseError(request, response)
