#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <utility>
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/utility.hpp"

namespace glm
{
template<typename Archive> void serialize(Archive &archive, glm::vec2 &v2)
{
    archive(v2.x, v2.y);
}
}

struct controls
{
    bool left = false;
    bool right = false;
    bool fire = false;
};

struct Game
{
    glm::vec2 ball = glm::vec2(0.0f, 0.0f);
    glm::vec2 ball_velocity = glm::vec2(0.0f, -2.0f);
    float left_turret_angle = 0.0f;
    float right_turret_angle = M_PI;
    controls left_turret_controls;
    controls right_turret_controls;
    std::unordered_map<uint32_t, std::pair<glm::vec2, glm::vec2>> bullets;
    bool game_started = false;
    bool left_player_won = false;
    bool right_player_won = false;
    uint32_t left_player_points = 0, right_player_points = 0;

    void update(float time);

    static constexpr const float FrameWidth = 38.0f;
    static constexpr const float FrameHeight = 25.0f;
    static constexpr const float BallRadius = 0.5f;
    static constexpr const float BulletRadius = 0.1f;
    static constexpr const float BulletSpeed = 15.0f;
    static constexpr const float RotateSpeed = 0.06f;
    static constexpr const float BarrelLength = 2.5f;
    static constexpr const uint32_t PointLimit = 5;
    const glm::vec2 LeftTurretLoc = glm::vec2(-16.77f, 0.0f);
    const glm::vec2 RightTurretLoc = glm::vec2(16.77f, 0.0f);

    // This method lets cereal know which data members to serialize
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(
            ball,
            ball_velocity,
            left_turret_angle,
            right_turret_angle,
            bullets,
            game_started,
            left_player_points,
            right_player_points,
            left_player_won,
            right_player_won
        ); // serialize things by passing them to the archive
    }
};
