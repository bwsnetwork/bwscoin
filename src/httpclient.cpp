/* * Copyright (c) 2021 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "httpclient.h"

#include <string>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/algorithm/string.hpp>

#include "net.h"

HttpClient::HttpClient(std::string host_port)
{
    std::vector<std::string> strings;
    boost::split(strings, host_port, boost::is_any_of(":"));

    if (strings.size() > 0 && strings[0].length() > 0)
        host = strings[0];

    if (strings.size() > 1 && strings[1].length() > 0) {
        auto i = std::stoi(strings[1]);

        auto limits = std::numeric_limits<uint16_t>();
        if (i >= limits.min() && i <= limits.max())
            port = static_cast<uint16_t>(i);
    }
}

HttpResponse HttpClient::get(std::string endpoint, UniValue query_params)
{
    return call(boost::beast::http::verb::get, endpoint, query_params, UniValue());
}

HttpResponse HttpClient::post(std::string endpoint, UniValue body)
{
    return call(boost::beast::http::verb::post, endpoint, UniValue(), body);
}

HttpResponse HttpClient::call(boost::beast::http::verb verb, std::string endpoint, UniValue query_params, UniValue body)
{
    if (endpoint.length() <= 0)
        return HttpResponse(HttpResponse::Failed, 400, "Bad parameters");

    boost::beast::http::response<boost::beast::http::string_body> response;

    try {
        // connection

        boost::asio::io_context ctx;
        boost::asio::ip::tcp::resolver resolver(ctx);
        boost::beast::tcp_stream stream(ctx);

        auto const ip = resolver.resolve(host, std::to_string(port));

        stream.connect(ip);

        // query string

        std::string query_string;
        std::map<std::string, UniValue> obj_map;
        query_params.getObjMap(obj_map);
        for (auto& p: obj_map)
            if (p.first.length() > 0 && p.second.isStr()) {
                auto& v = p.second.get_str();
                if (v.length() > 0) {
                    query_string += (query_string.length() == 0 ? "?" : "&");
                    query_string += (p.first + "=" + v);
                }
            }

        // request

        boost::beast::http::request<boost::beast::http::string_body> request{verb, endpoint + query_string, 11};

        request.set(boost::beast::http::field::host, host);
        request.set(boost::beast::http::field::user_agent, strSubVersion);
        request.set(boost::beast::http::field::content_type, "application/json");
        request.set(boost::beast::http::field::accept, "application/json");

        if (body.isObject()) {
            auto body_string = body.write();
            if (body_string.length() > 0)
                request.body() = body_string;
        }
        request.prepare_payload();

        boost::beast::http::write(stream, request);

        // response

        boost::beast::flat_buffer buffer;
        boost::beast::http::read(stream, buffer, response);

        // disconnect

        boost::beast::error_code ec;
        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        if (ec && ec != boost::beast::errc::not_connected)
            throw boost::beast::system_error{ec};

    } catch (std::exception const& e) {
        return HttpResponse(HttpResponse::Failed, 422, e.what());
    }

    uint16_t code = static_cast<uint16_t>(response.base().result());

    UniValue u;
    u.read(response.body());

    return HttpResponse(code == 200 ? HttpResponse::Ok : HttpResponse::Failed,
                        code, std::string(response.base().reason()), u);
}
