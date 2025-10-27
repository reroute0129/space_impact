#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>
#include <GL/glew.h>

#include "game.h"

typedef enum {
    SPRITE_PLAYER,
    SPRITE_ENEMY_SMALL,
    SPRITE_ENEMY_MEDIUM,
    SPRITE_ENEMY_LARGE,
    SPRITE_ENEMY_BOSS,
    SPRITE_BULLET,
    SPRITE_ENEMY_BULLET,
    SPRITE_POWERUP_HEALTH,
    SPRITE_POWERUP_RAPID_FIRE,
    SPRITE_POWERUP_DOUBLE_BULLET,
    SPRITE_EXPLOSION,
    SPRITE_BACKGROUND,
    SPRITE_HUD_LIFE,
    SPRITE_COUNT
} SpriteType;

bool initRenderer();

void destroyRenderer();

void renderGame(GameState* gameState);

void renderGameOver(GameState* gameState);

#endif 
