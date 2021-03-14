from http.server import BaseHTTPRequestHandler
import http
import argparse
from urllib.parse import urlparse
from urllib.parse import urlencode
from urllib.parse import parse_qs
from urllib.parse import unquote
import socket, ssl, pprint
from io import BytesIO

XSS = "'\"><img src=\"\" onerror=alert(\"\")>"
FILE_PATH = "/Users/aleks/Desktop/tp-thirdsem/myproxy/requests/POSTauth.mail.rufe133c21211f65cb1240f2354a24b4eac6be149529c92188d25ee97e149666db"

def make_https_request(host, data):
    port = 443
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(10)
        ssl_sock = ssl.wrap_socket(s)
        ssl_sock.connect((host, port))
        ssl_sock.sendall(data.encode())

        data = ssl_sock.recv(100000024)

    return data.decode()


def make_http_request(host, data):
    port = 80
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        ssl_sock = s
        ssl_sock.connect((host, port))
        ssl_sock.sendall(data.encode())
        data = ssl_sock.recv(1000024)

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

class VulnerabilitySearcher:
    def __init__(self, request):
        self.request = request

    def check_query_params(self):
        request = HTTPRequest(self.request.encode())
        host = request.headers["Host"]
        parsed_url = urlparse(request.path)
        parsed_query = parse_qs(parsed_url.query)

        if parsed_query:
            prepare_query(parsed_query)
            for i in generate_xss_dict(parsed_query):
                print(f"New query params: {i}")
                query_string = urlencode(i)
                new_request = self.request.replace(parsed_url.query, unquote(query_string))
                response = make_https_request(host, new_request)
                if response.find(XSS) != -1:
                    print(f"Found: {i}")

    def check_body_params(self):
        request = HTTPRequest(self.request.encode())
        host = request.headers["Host"]
        if "Content-Type" in request.headers.keys() and \
            request.headers["Content-Type"] == "application/x-www-form-urlencoded":
            pattern = "\n\n"
            pos = request_str.find(pattern)
            request_body = request_str[pos+len(pattern):]
            parsed_body_parameters = parse_qs(request_body)
            prepare_query(parsed_body_parameters)

            if parsed_body_parameters:
                for i in generate_xss_dict(parsed_body_parameters):
                    print(f"New body params: {i}")
                    query_string = urlencode(i)

                    old_content_length = f"Content-Length: {request.headers['Content-Length']}"
                    new_content_length = f"Content-Length: {len(unquote(query_string))}"

                    new_request = self.request.replace(
                        request_body, 
                        unquote(query_string)
                    ).replace(
                        old_content_length, 
                        new_content_length
                    )

                    response = make_https_request(host, new_request)
                    if response.find(XSS) != -1:
                        print(f"Found: {i}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Search vulnarabilities")

    parser.add_argument("-f", "--filepath", metavar="", required=True, type=argparse.FileType('r'), help="path to file with request")
    parser.add_argument("-b", "--body", action="store_true", help="flag to check body params")
    parser.add_argument("-q", "--query", action="store_true", help="flag to check query params")

    args = parser.parse_args()

    request_str = ""  # we don't really need this because it's python but ...
    request_lines = args.filepath.readlines()
    request_str = "".join(request_lines)

    searcher = VulnerabilitySearcher(request_str)
    if args.query:
        searcher.check_query_params()
    if args.body:
        searcher.check_body_params()


