#include <cradle/websocket/client.hpp>
#include <cradle/websocket/server.hpp>

#include <thread>

#include <cradle/io/base64.hpp>
#include <cradle/websocket/messages.hpp>

using namespace cradle;

TEST_CASE("websocket client/server", "[ws]")
{
    websocket_server server;
    server.listen(41072);
    std::thread server_thread([&](){ server.run(); });

    optional<websocket_test_response> test_response;

    {
        websocket_client client;
        client.set_message_handler(
            [&](websocket_server_message const& message)
            {
                switch (get_tag(message))
                {
                 case websocket_server_message_tag::CACHE_RESPONSE:
                  {
                    auto response = as_cache_response(message);
                    REQUIRE(response.key == "test_key");
                    REQUIRE(response.value == some(string("test_value")));
                    client.send(
                        make_websocket_client_message_with_test(
                            websocket_test_query{"Hello, Patches!"}));
                    break;
                  }
                 case websocket_server_message_tag::TEST:
                  {
                    test_response = as_test(message);
                    client.send(
                        make_websocket_client_message_with_kill(nil));
                    client.close();
                    break;
                  }
                 default:
                    FAIL("unexpected message");
                }
            });
        client.set_open_handler(
            [&]()
            {
                client.send(
                    make_websocket_client_message_with_registration(
                        websocket_registration_message{"Kasey"}));
                client.send(
                    make_websocket_client_message_with_cache_insert(
                        make_websocket_cache_insert("test_key", "test_value")));
                client.send(
                    make_websocket_client_message_with_cache_query("test_key"));
            });
        client.connect("ws://localhost:41072");
        client.run();
    }

    server_thread.join();

    REQUIRE(test_response);
    REQUIRE(test_response->name == "Kasey");
    REQUIRE(test_response->message == "Hello, Patches!");
}
