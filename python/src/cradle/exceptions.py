import json


class CommandError(RuntimeError):
    def __init__(self, synopsis, request, response):
        super().__init__()
        self.synopsis = synopsis
        self.request = request
        self.response = response

    def __str__(self):
        return (self.synopsis + '\n'
                + 'Request: ' + json.dumps(self.request, indent=4) + '\n'
                + 'Response: ' + json.dumps(self.response, indent=4))


class RequestIdMismatchError(CommandError):
    def __init__(self, request, response):
        synopsis = 'Mismatched request IDs'
        super().__init__(synopsis, request, response)


class ErrorResponseError(CommandError):
    def __init__(self, request, response):
        synopsis = 'Received error response'
        super().__init__(synopsis, request, response)
