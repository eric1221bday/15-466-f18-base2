#include "Connection.hpp"
#include "Game.hpp"
#include "cereal/archives/binary.hpp"

#include <iostream>
#include <sstream>
#include <set>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage:\n\t./server <port>" << std::endl;
        return 1;
    }

    Server server(argv[1]);

    Game state;

    while (true) {

        //(2) call the current mode's "update" function to deal with elapsed time:
        auto current_time = std::chrono::high_resolution_clock::now();
        static auto previous_time = current_time;
        float elapsed = std::chrono::duration<float>(current_time - previous_time).count();

        //if frames are taking a very long time to process,
        //lag to avoid spiral of death:
        elapsed = std::min(0.1f, elapsed);

        server.poll([&](Connection *c, Connection::Event evt)
                    {
                        if (evt == Connection::OnOpen) {
                        }
                        else if (evt == Connection::OnClose) {
                        }
                        else {
                            assert(evt == Connection::OnRecv);
                            if (c->recv_buffer[0] == 'h') {
                                c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);

                                std::this_thread::sleep_for(1s);
                                if (server.connections.size() == 1) {
                                    int8_t id = 0;
                                    c->send_raw("i", 1);
                                    c->send_raw(&id, 1);
                                    std::cout << c << ": Sent player id: " << static_cast<int16_t>(id) << std::endl;
                                }
                                else if (server.connections.size() == 2) {
                                    int8_t id = 1;
                                    c->send_raw("i", 1);
                                    c->send_raw(&id, 1);
                                    std::cout << c << ": Sent player id: " << static_cast<int16_t>(id) << std::endl;
                                    state.game_started = true;
                                }
                                else {
                                    std::cout << "this is a two player game!" << std::endl;
                                    c->close();
                                }
                            }
                            else if (c->recv_buffer[0] == 'c') {
                                if (c->recv_buffer.size() < 2 + 3 * sizeof(bool)) {
                                    return; //wait for more data
                                }
                                else {
                                    if (static_cast<int8_t>(c->recv_buffer[1]) == 0) {
                                        memcpy(&state.left_turret_controls.left,
                                               c->recv_buffer.data() + 2,
                                               sizeof(bool));
                                        memcpy(&state.left_turret_controls.right,
                                               c->recv_buffer.data() + 2 + sizeof(bool),
                                               sizeof(bool));
                                        memcpy(&state.left_turret_controls.fire,
                                               c->recv_buffer.data() + 2 + 2 * sizeof(bool),
                                               sizeof(bool));
                                    }
                                    else if (static_cast<int8_t>(c->recv_buffer[1]) == 1) {
                                        memcpy(&state.right_turret_controls.left,
                                               c->recv_buffer.data() + 2,
                                               sizeof(bool));
                                        memcpy(&state.right_turret_controls.right,
                                               c->recv_buffer.data() + 2 + sizeof(bool),
                                               sizeof(bool));
                                        memcpy(&state.right_turret_controls.fire,
                                               c->recv_buffer.data() + 2 + 2 * sizeof(bool),
                                               sizeof(bool));
                                    }
                                    else {
                                        std::cout << "there is a problem, recieved player id: "
                                                  << static_cast<int16_t>(c->recv_buffer[1]) << std::endl;
                                    }
                                    c->recv_buffer
                                        .erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2 + 3 * sizeof(bool));
                                }
                            }
                        }
                    }, 0.01);

        if (state.game_started) {
            state.update(elapsed);

            for (auto &connection : server.connections) {
                std::ostringstream state_stream(std::ios_base::binary);
                {
                    cereal::BinaryOutputArchive state_out(state_stream);
                    state_out(state);
                }
                std::string state_str = state_stream.str();

                connection.send_raw("s", 1);
                size_t msg_length = state_str.length();
                connection.send_raw(&msg_length, sizeof(size_t));
                connection.send_raw(state_str.c_str(), state_str.length());
            }
        }

        //every second or so, dump the current paddle position:
        static auto then = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (now > then + std::chrono::seconds(1)) {
            then = now;
            if (!state.game_started) {
                std::cout << "Game not started" << std::endl;
            }
        }

        // limit execution rate
        auto end_time = std::chrono::high_resolution_clock::now();
        auto process_duration = std::chrono::duration<float>(end_time - current_time);
        if (process_duration.count() < 0.005f) {
            std::this_thread::sleep_for(5ms - process_duration);
        }
        previous_time = current_time;
    }
}
