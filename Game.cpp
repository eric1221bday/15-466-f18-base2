#include "Game.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/projection.hpp>

void Game::update(float time)
{
    if (left_player_points >= PointLimit || right_player_points >= PointLimit) {
        game_started = false;
    }

    auto bullet_it = bullets.begin();

    while (bullet_it != bullets.end()) {

        glm::vec2 back_normal = bullet_it->second.first - ball;

        bullets.at(bullet_it->first) =
            {bullet_it->second.first + bullet_it->second.second * time, bullet_it->second.second};
        glm::vec2 pos = bullet_it->second.first;
        if (pos.x >= 0.5f * FrameWidth - BulletRadius || pos.x <= -0.5f * FrameWidth + BulletRadius
            || pos.y >= 0.5f * FrameHeight - BulletRadius || pos.y <= -0.5f * FrameHeight + BulletRadius) {
            bullet_it = bullets.erase(bullet_it);
        }
        else if (glm::length(back_normal) <= (BallRadius + BulletRadius)) {
            ball_velocity += glm::proj(bullet_it->second.second, back_normal) * 0.5f;
            bullet_it = bullets.erase(bullet_it);
        }
        else {
            bullet_it++;
        }
    }

    ball += ball_velocity * time;
    if (ball.x >= 0.5f * FrameWidth - BallRadius) {
        left_player_points++;
        ball = glm::vec2(0.0f, 0.0f);
        ball_velocity = glm::vec2(0.0f, -2.0f);
    }
    if (ball.x <= -0.5f * FrameWidth + BallRadius) {
        right_player_points++;
        ball = glm::vec2(0.0f, 0.0f);
        ball_velocity = glm::vec2(0.0f, -2.0f);
    }
    if (ball.y >= 0.5f * FrameHeight - BallRadius) {
        ball_velocity.y = -std::abs(ball_velocity.y);
    }
    if (ball.y <= -0.5f * FrameHeight + BallRadius) {
        ball_velocity.y = std::abs(ball_velocity.y);
    }

    if (left_turret_controls.left) {
        left_turret_angle += RotateSpeed;
    }
    if (left_turret_controls.right) {
        left_turret_angle -= RotateSpeed;
    }
    if (right_turret_controls.left) {
        right_turret_angle += RotateSpeed;
    }
    if (right_turret_controls.right) {
        right_turret_angle -= RotateSpeed;
    }

    static uint32_t bullet_id = 0;
    static float left_fire_timeout = 1.0f;
    left_fire_timeout += time;
    if (left_turret_controls.fire && left_fire_timeout > 1.0f) {
        left_fire_timeout = 0.0f;
        bullets[bullet_id] = {
            LeftTurretLoc + glm::vec2(std::cos(left_turret_angle), std::sin(left_turret_angle)) * BarrelLength,
            glm::vec2(std::cos(left_turret_angle), std::sin(left_turret_angle)) * BulletSpeed};
        bullet_id++;
    }

    static float right_fire_timeout = 1.0f;
    right_fire_timeout += time;
    if (right_turret_controls.fire && right_fire_timeout > 1.0f) {
        right_fire_timeout = 0.0f;
        bullets[bullet_id] = {
            RightTurretLoc + glm::vec2(std::cos(right_turret_angle), std::sin(right_turret_angle)) * BarrelLength,
            glm::vec2(std::cos(right_turret_angle), std::sin(right_turret_angle)) * BulletSpeed};
        bullet_id++;
    }

    left_turret_controls = {false, false, false};
    right_turret_controls = {false, false, false};

//    auto do_point = [this](glm::vec2 const &pt)
//    {
//        glm::vec2 to = ball - pt;
//        float len2 = glm::dot(to, to);
//        if (len2 > BallRadius * BallRadius) return;
//        //if point is inside ball, make ball velocity outward-going:
//        float d = glm::dot(ball_velocity, to);
//        ball_velocity += ((std::abs(d) - d) / len2) * to;
//    };

//    do_point(glm::vec2(paddle.x - 0.5f * PaddleWidth, paddle.y));
//    do_point(glm::vec2(paddle.x + 0.5f * PaddleWidth, paddle.y));

//    auto do_edge = [&](glm::vec2 const &a, glm::vec2 const &b)
//    {
//        float along = glm::dot(ball - a, b - a);
//        float max = glm::dot(b - a, b - a);
//        if (along <= 0.0f || along >= max) return;
//        do_point(glm::mix(a, b, along / max));
//    };

//    do_edge(glm::vec2(paddle.x + 0.5f * PaddleWidth, paddle.y), glm::vec2(paddle.x - 0.5f * PaddleWidth, paddle.y));

}
