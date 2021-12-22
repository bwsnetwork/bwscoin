/* * Copyright (c) 2021 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#ifndef BWSCOIN_HTTP_CLIENT_H
#define BWSCOIN_HTTP_CLIENT_H

#include <string>
#include <vector>

#include <boost/beast/http.hpp>

#include "univalue.h"

/**
 * A UniValue based HTTP response body for consistent use in
 * conjunction with the HttpClient below.
 * If the status is Failed, check the http_code and message for details.
 * If the status is Ok, check the body for details, if needed.
 */
struct HttpResponse
{
    enum Status {
        Failed = 0,
        Ok = 1
    };

    HttpResponse(Status status = Ok, uint16_t http_code = 200, std::string message = "Successful", UniValue body = UniValue())
        : status(status), http_code(http_code), message(message), body(body) {}

    Status status;
    uint16_t http_code;
    std::string message;
    UniValue body;
};


/**
 * A Boost based synchronous HTTP client
 */
class HttpClient
{
public:

    HttpClient(std::string host_port = "127.0.0.1:50011");

    HttpClient(std::string host, uint16_t)
        : host(host), port(port) {}

    HttpResponse get(std::string endpoint, UniValue query_params);

    HttpResponse post(std::string endpoint, UniValue body);

private:

    HttpResponse call(boost::beast::http::verb verb, std::string endpoint, UniValue query_params, UniValue body);

private:

    std::string host;
    uint16_t port;
};

#endif // BWSCOIN_HTTP_CLIENT_H
