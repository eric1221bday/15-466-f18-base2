#pragma once

#include "Mode.hpp"

#include "Scene.hpp"
#include "MeshBuffer.hpp"
#include "GL.hpp"
#include "Connection.hpp"
#include "Game.hpp"
#include "cereal/archives/binary.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <sstream>
#include <unordered_map>

// The 'GameMode' mode is the main gameplay mode:

struct GameMode: public Mode
{
    GameMode(Client &client);
    virtual ~GameMode();

    //handle_event is called when new mouse or keyboard events are received:
    // (note that this might be many times per frame or never)
    //The function should return 'true' if it handled the event.
    virtual bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;

    //update is called at the start of a new frame, after events are handled:
    virtual void update(float elapsed) override;

    //draw is called after update:
    virtual void draw(glm::uvec2 const &drawable_size) override;

    void show_win_menu();
    void show_waiting_menu();

    //------- game state -------
    Game state;
    std::unordered_map<uint32_t, std::pair<Scene::Object *, Scene::Transform *>> bullets;
    controls current_controls;
    int8_t player_id = -1;

    //------ networking ------
    Client &client; //client object; manages connection to server.
};
