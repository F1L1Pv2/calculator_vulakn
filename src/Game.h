#ifndef GAME_GAME
#define GAME_GAME

#include "loader.h"
#include "Window.h"

#include "common.h"

#ifdef DEBUG
    #define getModel(x) getModelInner(x)
    #define getTexture(x) getTextureInner(x)
#else
    #define getModel(x) getModelInner(x, x##_LEN)
    #define getTexture(x) getTextureInner(x, x##_LEN)
#endif


struct GameCallbacks{
#ifdef DEBUG
    MeshView* (*getModelInner)(const char* filename);
    Texture* (*getTextureInner)(const char* filename);
#else
    MeshView* (*getModelInner)(unsigned char* filename, unsigned int len);
    Texture* (*getTextureInner)(unsigned char* filename, unsigned int len);
#endif
    void (*drawMesh)(MeshView* model, Texture* texture, InstanceData data);
};

struct Game{
    Window* window;
    UniformBufferObject* ubo;
    float yaw = 0.0;
    float pitch = 0.0;
    vec3 cameraPos = {0,0,-2};
    bool fullscreen = false;
    float Time = 0.0f;
    bool player_spec = false;

    vec3 cube_pos = {0.0,-10.0f, -4.0f};
    vec3 cube_vel = {0.0,-10.0f, 0.0f};
    vec3 gravity = {0.0, 9.807f, 0.0f};

    vec3 player_pos = {0.0, 0.0f, 0.0f};
    vec3 player_vel = {0.0, 0.0f, 0.0f};
    float player_rot = 0.0f;
    vec3 player_size = {0.5, 1.8, 0.5};

    bool crouching = false;
    float target_size = 1.8f;


    GameCallbacks callbacks;
};

Game* game;

extern "C"
{
    EXPORT_FN void update_game(double deltaTime, Game* gameIN);
    EXPORT_FN void init_game(Game* gameIN);
}

#endif