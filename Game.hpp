#pragma once

#include <glm/glm.hpp>

namespace glm
{
template<typename Archive> void serialize(Archive &archive, glm::vec2 &v2)
{
    archive(v2.x, v2.y);
}
}

struct Game
{
    glm::vec2 paddle = glm::vec2(0.0f, -3.0f);
    glm::vec2 ball = glm::vec2(0.0f, 0.0f);
    glm::vec2 ball_velocity = glm::vec2(0.0f, -2.0f);
    bool game_started = false;

    void update(float time);

    static constexpr const float FrameWidth = 10.0f;
    static constexpr const float FrameHeight = 8.0f;
    static constexpr const float PaddleWidth = 2.0f;
    static constexpr const float BallRadius = 0.5f;

    // This method lets cereal know which data members to serialize
    template<class Archive>
    void serialize(Archive &archive)
    {
        archive(paddle, ball, ball_velocity, game_started); // serialize things by passing them to the archive
    }
};
