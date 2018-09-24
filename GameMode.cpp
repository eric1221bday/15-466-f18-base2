#include "GameMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>
#include <sstream>


Load<MeshBuffer> meshes(LoadTagDefault, []()
{
    return new MeshBuffer(data_path("indirect-pong.pnc"));
});

Load<GLuint> meshes_for_vertex_color_program(LoadTagDefault, []()
{
    return new GLuint(meshes->make_vao_for_program(vertex_color_program->program));
});

static Scene::Transform *ball_transform = nullptr;

static Scene::Transform *left_turret_transform = nullptr;

static Scene::Transform *right_turret_transform = nullptr;

static Scene::Camera *camera = nullptr;

static Scene *current_scene = nullptr;

static glm::quat turret_base_rotation;

Load<Scene> scene(LoadTagDefault, []()
{
    Scene *ret = new Scene;
    current_scene = ret;

    //load transform hierarchy:
    ret->load(data_path("indirect-pong.scene"), [](Scene &s, Scene::Transform *t, std::string const &m)
    {
        Scene::Object *obj = s.new_object(t);

        obj->program = vertex_color_program->program;
        obj->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
        obj->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
        obj->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;

        MeshBuffer::Mesh const &mesh = meshes->lookup(m);
        obj->vao = *meshes_for_vertex_color_program;
        obj->start = mesh.start;
        obj->count = mesh.count;
    });

    //look up paddle and ball transforms:
    for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
        if (t->name == "Left_Turret") {
            if (left_turret_transform) throw std::runtime_error("Multiple 'Left_Turret' transforms in scene.");
            turret_base_rotation = t->rotation;
            left_turret_transform = t;
        }
        if (t->name == "Right_Turret") {
            if (right_turret_transform) throw std::runtime_error("Multiple 'Right_Turret' transforms in scene.");
            right_turret_transform = t;
        }
        if (t->name == "Ball") {
            if (ball_transform) throw std::runtime_error("Multiple 'Ball' transforms in scene.");
            ball_transform = t;
        }
    }
    if (!left_turret_transform) throw std::runtime_error("No 'Left_Turret' transform in scene.");
    if (!right_turret_transform) throw std::runtime_error("No 'Right_Turret' transform in scene.");
    if (!ball_transform) throw std::runtime_error("No 'Ball' transform in scene.");

    //look up the camera:
    for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
        if (c->transform->name == "Camera") {
            if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
            camera = c;
        }
    }
    if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");
    return ret;
});

GameMode::GameMode(Client &client_)
    : client(client_)
{
    client.connection.send_raw("h", 1); //send a 'hello' to the server
}

GameMode::~GameMode()
{
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{
    //ignore any keys that are the result of automatic key repeat:
    if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
        return false;
    }

    // handle tracking the state of WSAD for movement control:
    if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
        if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
            current_controls.fire = (evt.type == SDL_KEYDOWN);
            return true;
        }
        if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
            current_controls.left = (evt.type == SDL_KEYDOWN);
            return true;
        }
        else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
            current_controls.right = (evt.type == SDL_KEYDOWN);
            return true;
        }
    }

    return false;
}

void GameMode::update(float elapsed)
{

    if (client.connection && player_id != -1) {
        //send controls to server:
        if (current_controls.left || current_controls.right || current_controls.fire) {
            client.connection.send_raw("c", 1);
            client.connection.send_raw(&player_id, 1);
            client.connection.send_raw(&current_controls.left, sizeof(bool));
            client.connection.send_raw(&current_controls.right, sizeof(bool));
            client.connection.send_raw(&current_controls.fire, sizeof(bool));

            current_controls = {current_controls.left, current_controls.right, false};
        }
    }

    client.poll([&](Connection *c, Connection::Event event)
                {
                    if (event == Connection::OnOpen) {
                        //probably won't get this.
                    }
                    else if (event == Connection::OnClose) {
                        std::cerr << "Lost connection to server." << std::endl;
                    }
                    else {
                        assert(event == Connection::OnRecv);

                        if (c->recv_buffer[0] == 'i') {
                            if (c->recv_buffer.size() < 2) {
                                return; //wait for more data
                            }
                            else {
                                player_id = c->recv_buffer[1];
                                c->recv_buffer
                                    .erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 2);
                                std::cout << "designated as player: " << static_cast<int16_t>(player_id) << std::endl;
                            }
                        }
                        else if (c->recv_buffer[0] == 's') {
                            if (c->recv_buffer.size() < 1 + sizeof(size_t)) {
                                return; //wait for more data
                            }
                            else {
                                size_t state_str_length;
                                memcpy(&state_str_length, c->recv_buffer.data() + 1, sizeof(size_t));
                                if (c->recv_buffer.size() < 1 + sizeof(size_t) + state_str_length) {
                                    return;
                                }
                                else {
                                    std::string state_str(c->recv_buffer.data() + 1 + sizeof(size_t),
                                                          c->recv_buffer.data() + 1 + sizeof(size_t)
                                                              + state_str_length);
                                    std::istringstream state_ss(state_str, std::ios_base::binary);

                                    cereal::BinaryInputArchive state_in(state_ss);
                                    state_in(state);
                                    c->recv_buffer
                                        .erase(c->recv_buffer.begin(),
                                               c->recv_buffer.begin() + 1 + sizeof(size_t) + state_str_length);
                                }

                            }
                        }
                        c->recv_buffer.clear();
                    }
                });

    if (!state.game_started && Mode::current.get() == this) {
        show_waiting_menu();
    }
    else if (state.game_started && Mode::current.get() != this && !state.left_player_won && !state.right_player_won) {
        Mode::set_current(shared_from_this());
    }

    if ((state.left_player_won || state.right_player_won) && Mode::current.get() == this) {
        show_win_menu();
    }

    //check if there were destroyed bullets
    auto bullet_it = bullets.begin();

    while (bullet_it != bullets.end()) {
        if (state.bullets.find(bullet_it->first) == state.bullets.end()) {
            current_scene->delete_object(bullet_it->second.first);
            current_scene->delete_transform(bullet_it->second.second);
            bullet_it = bullets.erase(bullet_it);
        }
        else {
            bullet_it++;
        }
    }

    for (auto const &elem : state.bullets) {
        if (bullets.find(elem.first) == bullets.end()) {
            Scene::Transform *t = current_scene->new_transform();
            t->position = glm::vec3(elem.second.first.x, elem.second.first.y, 0.0f);

            Scene::Object *obj = current_scene->new_object(t);

            obj->program = vertex_color_program->program;
            obj->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
            obj->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
            obj->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;

            MeshBuffer::Mesh const &mesh = meshes->lookup("Bullet");
            obj->vao = *meshes_for_vertex_color_program;
            obj->start = mesh.start;
            obj->count = mesh.count;

            bullets[elem.first] = {obj, t};
        }
        else {
            bullets.at(elem.first).second->position = glm::vec3(elem.second.first.x, elem.second.first.y, 0.0f);
        }
    }

    //copy game state to scene positions:
    ball_transform->position.x = state.ball.x;
    ball_transform->position.y = state.ball.y;

    left_turret_transform->rotation =
        turret_base_rotation * glm::angleAxis(state.left_turret_angle, glm::vec3(0.0f, 1.0f, 0.0f));
    right_turret_transform->rotation =
        turret_base_rotation * glm::angleAxis(state.right_turret_angle, glm::vec3(0.0f, 1.0f, 0.0f));
}

void GameMode::draw(glm::uvec2 const &drawable_size)
{
    camera->aspect = drawable_size.x / float(drawable_size.y);

    glClearColor(0.25f, 0.0f, 0.5f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //set up basic OpenGL state:
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //set up light positions:
    glUseProgram(vertex_color_program->program);

    glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
    glUniform3fv(vertex_color_program->sun_direction_vec3,
                 1,
                 glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
    glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
    glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));

    scene->draw(camera);

    if (Mode::current.get() == this) {
        glDisable(GL_DEPTH_TEST);

        //draw scores
        std::stringstream ss;
        for (uint32_t i = 0; i < state.left_player_points; i++) {
            ss << "*";
        }
        std::string message = ss.str();
        float height = 0.1f;
        draw_text(message, glm::vec2(-1.5, 0.8f), height,
                  glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        ss.str(std::string());

        for (uint32_t i = 0; i < state.right_player_points; i++) {
            ss << "*";
        }
        message = ss.str();
        float width = text_width(message, height);
        draw_text(message, glm::vec2(1.5 - width, 0.8f), height,
                  glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    GL_ERRORS();
}

void GameMode::show_win_menu()
{
    std::shared_ptr<MenuMode> menu = std::make_shared<MenuMode>();

    std::shared_ptr<Mode> game = shared_from_this();
    menu->background = game;

    if (state.left_player_won) {
        menu->choices.emplace_back("LEFT PLAYER WINS");
    }
    else if (state.right_player_won) {
        menu->choices.emplace_back("RIGHT PLAYER WINS");
    }

    menu->choices.emplace_back("QUIT", []()
    { Mode::set_current(nullptr); });

    menu->selected = 1;

    Mode::set_current(menu);
}

void GameMode::show_waiting_menu()
{
    std::shared_ptr<MenuMode> menu = std::make_shared<MenuMode>();

    std::shared_ptr<Mode> game = shared_from_this();
    menu->background = game;

    menu->choices.emplace_back("WAITING FOR PLAYER");

    menu->selected = 0;

    Mode::set_current(menu);
}
