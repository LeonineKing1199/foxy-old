#include <tuple>
#include <string>
#include <utility>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <boost/container/pmr/unsynchronized_pool_resource.hpp>

#include "foxy/coroutine.hpp"
#include "foxy/detail/client.hpp"

#include "foxy/test/stream.hpp"

#include <catch.hpp>

namespace pmr   = boost::container::pmr;
namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;

using asio::ip::tcp;

namespace
{

auto make_request(
  asio::io_context&                  io,
  pmr::unsynchronized_pool_resource& pool) -> foxy::awaitable<void>
{
  auto stream = tcp::socket(io);

  auto const* host = "www.google.com";
  auto const* port = "80";

  auto request = http::request<
    http::empty_body, http::basic_fields<pmr::polymorphic_allocator<char>>
  >(
    http::verb::get, "/", 11,
    http::empty_body::value_type(),
    std::addressof(pool));

  http::response_parser<
    http::basic_string_body<
      char, std::char_traits<char>, pmr::polymorphic_allocator<char>
    >,
    pmr::polymorphic_allocator<char>
  >
  parser(
    std::piecewise_construct,
    std::make_tuple(std::addressof(pool)),
    std::make_tuple(std::addressof(pool)));

  auto buffer = beast::basic_flat_buffer<pmr::polymorphic_allocator<char>>(
    std::addressof(pool));

  co_await foxy::proto::async_send_request(
    stream, host, port, request, parser, buffer);

  auto msg = parser.release();

  CHECK(msg.result_int() == 200);
  CHECK(msg.body().size() > 0);

  co_return;
}

} // anonymous

TEST_CASE("Our new client prototype")
{
  SECTION("should be used to develop without disrupting APIs... yet")
  {
    asio::io_context                  io;
    pmr::unsynchronized_pool_resource pool;

    foxy::co_spawn(
      io,
      [&]() { return make_request(io, pool); },
      foxy::detached);

    io.run();
  }
}