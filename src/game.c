#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "game.h"

static void respawnEnemyRight(GameState* gameState, int idx);

// Simple uniform grid for broad-phase collision between bullets and enemies
#define GRID_CELL_SIZE 32
// Use fixed screen size here to avoid macro order issues
#define GRID_COLS ((480 + GRID_CELL_SIZE - 1) / GRID_CELL_SIZE)
#define GRID_ROWS ((320 + GRID_CELL_SIZE - 1) / GRID_CELL_SIZE)
#define GRID_MAX_PER_CELL 64

static int enemyGrid[GRID_ROWS][GRID_COLS][GRID_MAX_PER_CELL];
static int enemyGridCount[GRID_ROWS][GRID_COLS];

static int enemyBulletGrid[GRID_ROWS][GRID_COLS][GRID_MAX_PER_CELL];
static int enemyBulletGridCount[GRID_ROWS][GRID_COLS];

static int powerupGrid[GRID_ROWS][GRID_COLS][GRID_MAX_PER_CELL];
static int powerupGridCount[GRID_ROWS][GRID_COLS];

static int clampInt(int v, int min, int max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static void buildEnemyGrid(GameState* gameState) {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            enemyGridCount[r][c] = 0;
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gameState->enemies[i].active) {
            continue;
        }

        float exMin = gameState->enemies[i].x;
        float exMax = gameState->enemies[i].x + gameState->enemies[i].width;
        float eyMin = gameState->enemies[i].y;
        float eyMax = gameState->enemies[i].y + gameState->enemies[i].height;

        int colStart = clampInt((int)(exMin / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int colEnd   = clampInt((int)(exMax / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int rowStart = clampInt((int)(eyMin / GRID_CELL_SIZE), 0, GRID_ROWS - 1);
        int rowEnd   = clampInt((int)(eyMax / GRID_CELL_SIZE), 0, GRID_ROWS - 1);

        for (int r = rowStart; r <= rowEnd; r++) {
            for (int c = colStart; c <= colEnd; c++) {
                int count = enemyGridCount[r][c];
                if (count < GRID_MAX_PER_CELL) {
                    enemyGrid[r][c][count] = i;
                    enemyGridCount[r][c] = count + 1;
                }
            }
        }
    }
}

static void buildEnemyBulletGrid(GameState* gameState) {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            enemyBulletGridCount[r][c] = 0;
        }
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!gameState->enemyBullets[i].active) {
            continue;
        }

        float bxMin = gameState->enemyBullets[i].x;
        float bxMax = gameState->enemyBullets[i].x + gameState->enemyBullets[i].width;
        float byMin = gameState->enemyBullets[i].y;
        float byMax = gameState->enemyBullets[i].y + gameState->enemyBullets[i].height;

        int colStart = clampInt((int)(bxMin / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int colEnd   = clampInt((int)(bxMax / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int rowStart = clampInt((int)(byMin / GRID_CELL_SIZE), 0, GRID_ROWS - 1);
        int rowEnd   = clampInt((int)(byMax / GRID_CELL_SIZE), 0, GRID_ROWS - 1);

        for (int r = rowStart; r <= rowEnd; r++) {
            for (int c = colStart; c <= colEnd; c++) {
                int count = enemyBulletGridCount[r][c];
                if (count < GRID_MAX_PER_CELL) {
                    enemyBulletGrid[r][c][count] = i;
                    enemyBulletGridCount[r][c] = count + 1;
                }
            }
        }
    }
}

static void buildPowerupGrid(GameState* gameState) {
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            powerupGridCount[r][c] = 0;
        }
    }

    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!gameState->powerups[i].active) {
            continue;
        }

        float pxMin = gameState->powerups[i].x;
        float pxMax = gameState->powerups[i].x + gameState->powerups[i].width;
        float pyMin = gameState->powerups[i].y;
        float pyMax = gameState->powerups[i].y + gameState->powerups[i].height;

        int colStart = clampInt((int)(pxMin / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int colEnd   = clampInt((int)(pxMax / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int rowStart = clampInt((int)(pyMin / GRID_CELL_SIZE), 0, GRID_ROWS - 1);
        int rowEnd   = clampInt((int)(pyMax / GRID_CELL_SIZE), 0, GRID_ROWS - 1);

        for (int r = rowStart; r <= rowEnd; r++) {
            for (int c = colStart; c <= colEnd; c++) {
                int count = powerupGridCount[r][c];
                if (count < GRID_MAX_PER_CELL) {
                    powerupGrid[r][c][count] = i;
                    powerupGridCount[r][c] = count + 1;
                }
            }
        }
    }
}

#define PLAYER_SPEED 150.0f
#define BULLET_SPEED 300.0f
#define ENEMY_BULLET_SPEED 200.0f
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320
#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 16
#define BULLET_WIDTH 8
#define BULLET_HEIGHT 4
#define POWERUP_WIDTH 16
#define POWERUP_HEIGHT 16
#define POWERUP_DURATION 10.0f
#define BULLET_COOLDOWN 0.5f
#define RAPID_FIRE_COOLDOWN 0.2f
#define ENEMY_SPAWN_DELAY 2.0f
#define POWERUP_SPAWN_DELAY 15.0f

void initGame(GameState* gameState) {
    gameState->player.x = 50.0f;
    gameState->player.y = SCREEN_HEIGHT / 2.0f;
    gameState->player.width = PLAYER_WIDTH;
    gameState->player.height = PLAYER_HEIGHT;
    gameState->player.direction = DIR_NONE;
    gameState->player.lives = 3;
    gameState->player.isRapidFire = false;
    gameState->player.isDoubleBullet = false;
    gameState->player.powerupTimer = 0.0f;
    gameState->player.score = 0;
    gameState->player.bulletCooldown = 0.0f;

    gameState->level.number = 1;
    gameState->level.scrollSpeed = 50.0f;
    gameState->level.enemySpawnRate = 3.0f;
    gameState->level.backgroundOffset = 0.0f;
    gameState->level.midgroundOffset = 0.0f;
    gameState->level.foregroundOffset = 0.0f;
    gameState->level.bossSpawned = false;
    gameState->level.bossDefeated = false;

    gameState->enemySpawnTimer = 0.0f;
    gameState->powerupSpawnTimer = 0.0f;

    for (int i = 0; i < MAX_BULLETS; i++) {
        gameState->bullets[i].active = false;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        gameState->enemies[i].active = false;
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        gameState->enemyBullets[i].active = false;
    }

    for (int i = 0; i < MAX_POWERUPS; i++) {
        gameState->powerups[i].active = false;
    }

    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        gameState->explosions[i].active = false;
        gameState->explosions[i].persistent = false;
    }

    gameState->gameOver = false;
    gameState->paused = false;
    gameState->benchmarkMode = false;
    gameState->benchmarkSpawnBand = 0.10f;
}

void updateGame(GameState* gameState, float deltaTime) {
    if (gameState->gameOver || gameState->paused) {
        return;
    }

    float diagonalFactor = 0.7071f; 
    
    switch (gameState->player.direction) {
        case DIR_UP:
            gameState->player.y -= PLAYER_SPEED * deltaTime;
            break;
        case DIR_DOWN:
            gameState->player.y += PLAYER_SPEED * deltaTime;
            break;
        case DIR_LEFT:
            gameState->player.x -= PLAYER_SPEED * deltaTime;
            break;
        case DIR_RIGHT:
            gameState->player.x += PLAYER_SPEED * deltaTime;
            break;
        case DIR_UP_LEFT:
            gameState->player.y -= PLAYER_SPEED * diagonalFactor * deltaTime;
            gameState->player.x -= PLAYER_SPEED * diagonalFactor * deltaTime;
            break;
        case DIR_UP_RIGHT:
            gameState->player.y -= PLAYER_SPEED * diagonalFactor * deltaTime;
            gameState->player.x += PLAYER_SPEED * diagonalFactor * deltaTime;
            break;
        case DIR_DOWN_LEFT:
            gameState->player.y += PLAYER_SPEED * diagonalFactor * deltaTime;
            gameState->player.x -= PLAYER_SPEED * diagonalFactor * deltaTime;
            break;
        case DIR_DOWN_RIGHT:
            gameState->player.y += PLAYER_SPEED * diagonalFactor * deltaTime;
            gameState->player.x += PLAYER_SPEED * diagonalFactor * deltaTime;
            break;
        default:
            break;
    }
    
    if (gameState->player.x < gameState->player.width / 2) {
        gameState->player.x = gameState->player.width / 2;
    } else if (gameState->player.x > SCREEN_WIDTH - gameState->player.width / 2) {
        gameState->player.x = SCREEN_WIDTH - gameState->player.width / 2;
    }
    
    if (gameState->player.y < gameState->player.height / 2) {
        gameState->player.y = gameState->player.height / 2;
    } else if (gameState->player.y > SCREEN_HEIGHT - gameState->player.height) {
        gameState->player.y = SCREEN_HEIGHT - gameState->player.height;
    }

    if (gameState->player.bulletCooldown > 0) {
        gameState->player.bulletCooldown -= deltaTime;
    }

    if (gameState->player.powerupTimer > 0) {
        gameState->player.powerupTimer -= deltaTime;
        if (gameState->player.powerupTimer <= 0) {
            gameState->player.isRapidFire = false;
            gameState->player.isDoubleBullet = false;
        }
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gameState->bullets[i].active) {
            gameState->bullets[i].x += gameState->bullets[i].speed * deltaTime;

            if (gameState->benchmarkMode) {
                float offLeft = -gameState->bullets[i].width;
                float offRight = SCREEN_WIDTH + gameState->bullets[i].width;
                if (gameState->bullets[i].speed < 0.0f && gameState->bullets[i].x < offLeft) {
                    gameState->bullets[i].x = SCREEN_WIDTH - gameState->bullets[i].width / 2.0f;
                    float minY = BULLET_HEIGHT / 2.0f;
                    float maxY = SCREEN_HEIGHT - BULLET_HEIGHT;
                    gameState->bullets[i].y = minY + (float)(rand() % (int)(maxY - minY + 1));
                } else if (gameState->bullets[i].speed > 0.0f && gameState->bullets[i].x > offRight) {
                    gameState->bullets[i].x = gameState->bullets[i].width / 2.0f;
                    float minY = BULLET_HEIGHT / 2.0f;
                    float maxY = SCREEN_HEIGHT - BULLET_HEIGHT;
                    gameState->bullets[i].y = minY + (float)(rand() % (int)(maxY - minY + 1));
                }
            } else {
                if (gameState->bullets[i].x > SCREEN_WIDTH + gameState->bullets[i].width) {
                    gameState->bullets[i].active = false;
                }
                if (gameState->bullets[i].y < -gameState->bullets[i].height || 
                    gameState->bullets[i].y > SCREEN_HEIGHT + gameState->bullets[i].height) {
                    gameState->bullets[i].active = false;
                }
            }
        }
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (gameState->enemyBullets[i].active) {
            gameState->enemyBullets[i].x -= gameState->enemyBullets[i].speed * deltaTime;

            if (gameState->benchmarkMode) {
                if (gameState->enemyBullets[i].x < -gameState->enemyBullets[i].width) {
                    gameState->enemyBullets[i].x = SCREEN_WIDTH - gameState->enemyBullets[i].width / 2.0f;
                    float minY = BULLET_HEIGHT / 2.0f;
                    float maxY = SCREEN_HEIGHT - BULLET_HEIGHT;
                    gameState->enemyBullets[i].y = minY + (float)(rand() % (int)(maxY - minY + 1));
                }
            } else {
                if (gameState->enemyBullets[i].x < -gameState->enemyBullets[i].width) {
                    gameState->enemyBullets[i].active = false;
                }
                if (gameState->enemyBullets[i].y < -gameState->enemyBullets[i].height || 
                    gameState->enemyBullets[i].y > SCREEN_HEIGHT + gameState->enemyBullets[i].height) {
                    gameState->enemyBullets[i].active = false;
                }
            }
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (gameState->enemies[i].active) {
            gameState->enemies[i].x -= gameState->enemies[i].speed * deltaTime;
            
            switch (gameState->enemies[i].type) {
                case ENEMY_SMALL:
                    gameState->enemies[i].movementPattern += 3.0f * deltaTime;
                    gameState->enemies[i].y += sinf(gameState->enemies[i].movementPattern) * 1.5f;
                    break;
                case ENEMY_MEDIUM:
                    gameState->enemies[i].movementPattern -= deltaTime;
                    if (gameState->enemies[i].movementPattern <= 0) {
                        gameState->enemies[i].speed = (rand() % 2 == 0) ? 
                            fabs(gameState->enemies[i].speed) : 
                            -fabs(gameState->enemies[i].speed);
                        gameState->enemies[i].movementPattern = (rand() % 3) + 1.0f;
                    }
                    gameState->enemies[i].y += gameState->enemies[i].speed * 0.3f * deltaTime;
                    break;
                case ENEMY_LARGE:
                    gameState->enemies[i].bulletCooldown -= deltaTime;
                    if (gameState->enemies[i].bulletCooldown <= 0) {
                        for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                            if (!gameState->enemyBullets[j].active) {
                                float bulletY = gameState->enemies[i].y;
                                
                                if (bulletY < BULLET_HEIGHT/2) {
                                    bulletY = BULLET_HEIGHT/2;
                                } else if (bulletY > SCREEN_HEIGHT - BULLET_HEIGHT) {
                                    bulletY = SCREEN_HEIGHT - BULLET_HEIGHT;
                                }
                                
                                gameState->enemyBullets[j].x = gameState->enemies[i].x - gameState->enemyBullets[j].width;
                                gameState->enemyBullets[j].y = bulletY;
                                gameState->enemyBullets[j].width = BULLET_WIDTH;
                                gameState->enemyBullets[j].height = BULLET_HEIGHT;
                                gameState->enemyBullets[j].speed = ENEMY_BULLET_SPEED;
                                gameState->enemyBullets[j].active = true;
                                gameState->enemies[i].bulletCooldown = 2.0f;
                                break;
                            }
                        }
                    }
                    break;
                case ENEMY_BOSS:
                    gameState->enemies[i].movementPattern += deltaTime;
                    
                    float newY = SCREEN_HEIGHT / 2 + sinf(gameState->enemies[i].movementPattern) * (SCREEN_HEIGHT / 3);
                    
                    if (newY < gameState->enemies[i].height / 2) {
                        newY = gameState->enemies[i].height / 2;
                    } else if (newY > SCREEN_HEIGHT - gameState->enemies[i].height) {
                        newY = SCREEN_HEIGHT - gameState->enemies[i].height;
                    }
                    
                    gameState->enemies[i].y = newY;
                    
                    gameState->enemies[i].bulletCooldown -= deltaTime;
                    if (gameState->enemies[i].bulletCooldown <= 0) {
                        for (int b = 0; b < 3; b++) {
                            for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                                if (!gameState->enemyBullets[j].active) {
                                    float bulletY = gameState->enemies[i].y + (b - 1) * 20.0f;
                                    
                                    if (bulletY < BULLET_HEIGHT/2) {
                                        bulletY = BULLET_HEIGHT/2;
                                    } else if (bulletY > SCREEN_HEIGHT - BULLET_HEIGHT) {
                                        bulletY = SCREEN_HEIGHT - BULLET_HEIGHT;
                                    }
                                    
                                    gameState->enemyBullets[j].x = gameState->enemies[i].x - gameState->enemyBullets[j].width;
                                    gameState->enemyBullets[j].y = bulletY;
                                    gameState->enemyBullets[j].width = BULLET_WIDTH;
                                    gameState->enemyBullets[j].height = BULLET_HEIGHT;
                                    gameState->enemyBullets[j].speed = ENEMY_BULLET_SPEED;
                                    gameState->enemyBullets[j].active = true;
                                    break;
                                }
                            }
                        }
                        gameState->enemies[i].bulletCooldown = 1.0f;
                    }
                    break;
            }
            
            if (gameState->enemies[i].y < gameState->enemies[i].height / 2) {
                gameState->enemies[i].y = gameState->enemies[i].height / 2;
                if (gameState->enemies[i].type == ENEMY_MEDIUM) {
                    gameState->enemies[i].speed = fabs(gameState->enemies[i].speed);
                }
            } else if (gameState->enemies[i].y > SCREEN_HEIGHT - gameState->enemies[i].height) {
                gameState->enemies[i].y = SCREEN_HEIGHT - gameState->enemies[i].height;
                if (gameState->enemies[i].type == ENEMY_MEDIUM) {
                    gameState->enemies[i].speed = -fabs(gameState->enemies[i].speed);
                }
            }
            
            if (gameState->enemies[i].type == ENEMY_BOSS) {
                if (!gameState->benchmarkMode) {
                    if (gameState->enemies[i].x < SCREEN_WIDTH / 2) {
                        gameState->enemies[i].x = SCREEN_WIDTH / 2;
                    } else if (gameState->enemies[i].x > SCREEN_WIDTH - gameState->enemies[i].width / 2) {
                        gameState->enemies[i].x = SCREEN_WIDTH - gameState->enemies[i].width / 2;
                    }
                }
            }
            
            if (!gameState->benchmarkMode) {
                if (gameState->enemies[i].x < -gameState->enemies[i].width && 
                    gameState->enemies[i].type != ENEMY_BOSS) {
                    gameState->enemies[i].active = false;
                }
            } else {
                float wrapThreshold = -gameState->enemies[i].width * 0.25f;
                if (gameState->enemies[i].x < wrapThreshold) {
                    float enemyHeight = gameState->enemies[i].height;
                    float minY = enemyHeight / 2.0f;
                    float maxY = SCREEN_HEIGHT - enemyHeight;
                    float bandFrac = gameState->benchmarkSpawnBand;
                    if (bandFrac <= 0.0f) bandFrac = 0.10f;
                    if (bandFrac > 1.0f) bandFrac = 1.0f;
                    float band = SCREEN_WIDTH * bandFrac;
                    float jitter = (float)(rand() % (int)(band + 1));
                    gameState->enemies[i].x = (SCREEN_WIDTH - gameState->enemies[i].width / 2.0f) - jitter;
                    gameState->enemies[i].y = minY + (float)(rand() % (int)(maxY - minY + 1));
                    gameState->enemies[i].movementPattern = (float)(rand() % 628) / 100.0f;
                    if (gameState->enemies[i].type == ENEMY_LARGE || gameState->enemies[i].type == ENEMY_BOSS) {
                        gameState->enemies[i].bulletCooldown = (rand() % 3) * 0.5f + 0.2f;
                    }
                }
            }
        }
    }

    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (gameState->powerups[i].active) {
            gameState->powerups[i].x -= gameState->powerups[i].speed * deltaTime;
            
            if (gameState->powerups[i].y < gameState->powerups[i].height / 2) {
                gameState->powerups[i].y = gameState->powerups[i].height / 2;
            } else if (gameState->powerups[i].y > SCREEN_HEIGHT - gameState->powerups[i].height) {
                gameState->powerups[i].y = SCREEN_HEIGHT - gameState->powerups[i].height;
            }
            
            if (!gameState->benchmarkMode) {
                if (gameState->powerups[i].x < -gameState->powerups[i].width) {
                    gameState->powerups[i].active = false;
                }
            } else {
                if (gameState->powerups[i].x < -gameState->powerups[i].width) {
                    gameState->powerups[i].x = SCREEN_WIDTH - gameState->powerups[i].width / 2.0f;
                    float minY = gameState->powerups[i].height / 2.0f;
                    float maxY = SCREEN_HEIGHT - gameState->powerups[i].height;
                    gameState->powerups[i].y = minY + (float)(rand() % (int)(maxY - minY + 1));
                    gameState->powerups[i].type = (PowerupType)(rand() % 3);
                }
            }
        }
    }

    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (gameState->explosions[i].active) {
            gameState->explosions[i].currentLife -= deltaTime;
            if (gameState->explosions[i].currentLife <= 0) {
                if (gameState->benchmarkMode && gameState->explosions[i].persistent) {
                    gameState->explosions[i].currentLife = gameState->explosions[i].lifespan;
                } else {
                    gameState->explosions[i].active = false;
                }
            }
        }
    }

    if (!gameState->benchmarkMode) {
        gameState->enemySpawnTimer -= deltaTime;
        if (gameState->enemySpawnTimer <= 0 && !gameState->level.bossSpawned) {
            int enemyCount = 0;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (gameState->enemies[i].active) {
                    enemyCount++;
                }
            }
            
            if (gameState->player.score >= gameState->level.number * 10) {
                spawnBoss(gameState);
                gameState->level.bossSpawned = true;
            } else {
                EnemyType type;
                int r = rand() % 100;
                if (r < 60) {
                    type = ENEMY_SMALL;
                } else if (r < 85) {
                    type = ENEMY_MEDIUM;
                } else {
                    type = ENEMY_LARGE;
                }
                
                spawnEnemy(gameState, type);
                gameState->enemySpawnTimer = gameState->level.enemySpawnRate;
            }
        }
    }

    if (!gameState->benchmarkMode) {
        gameState->powerupSpawnTimer -= deltaTime;
        if (gameState->powerupSpawnTimer <= 0) {
            float x = SCREEN_WIDTH;
            
            float minY = POWERUP_HEIGHT / 2;
            float maxY = SCREEN_HEIGHT - POWERUP_HEIGHT;
            float y = (rand() % (int)(maxY - minY)) + minY;
            
            spawnPowerup(gameState, x, y);
            gameState->powerupSpawnTimer = POWERUP_SPAWN_DELAY;
        }
    }

    gameState->level.backgroundOffset += gameState->level.scrollSpeed * 0.3f * deltaTime;  
    gameState->level.midgroundOffset += gameState->level.scrollSpeed * 0.7f * deltaTime;   
    gameState->level.foregroundOffset += gameState->level.scrollSpeed * 1.4f * deltaTime;  

    handleCollisions(gameState);

    if (gameState->level.bossSpawned && gameState->level.bossDefeated) {
        nextLevel(gameState);
    }

    if (!gameState->benchmarkMode && gameState->player.lives <= 0) {
        gameState->gameOver = true;
    }
}

void fireBullet(GameState* gameState) {
    if (gameState->player.bulletCooldown > 0) {
        return;
    }
    
    gameState->player.bulletCooldown = gameState->player.isRapidFire ? RAPID_FIRE_COOLDOWN : BULLET_COOLDOWN;
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!gameState->bullets[i].active) {
            float bulletY = gameState->player.y;
            
            if (bulletY < BULLET_HEIGHT/2) {
                bulletY = BULLET_HEIGHT/2;
            } else if (bulletY > SCREEN_HEIGHT - BULLET_HEIGHT) {
                bulletY = SCREEN_HEIGHT - BULLET_HEIGHT;
            }
            
            gameState->bullets[i].x = gameState->player.x + gameState->player.width / 2;
            gameState->bullets[i].y = bulletY;
            gameState->bullets[i].width = BULLET_WIDTH;
            gameState->bullets[i].height = BULLET_HEIGHT;
            gameState->bullets[i].speed = BULLET_SPEED;
            gameState->bullets[i].active = true;
            
            if (gameState->player.isDoubleBullet) {
                for (int j = i + 1; j < MAX_BULLETS; j++) {
                    if (!gameState->bullets[j].active) {
                        float secondBulletY = gameState->player.y - 10;
                        
                        if (secondBulletY < BULLET_HEIGHT/2) {
                            secondBulletY = BULLET_HEIGHT/2;
                        } else if (secondBulletY > SCREEN_HEIGHT - BULLET_HEIGHT) {
                            secondBulletY = SCREEN_HEIGHT - BULLET_HEIGHT;
                        }
                        
                        gameState->bullets[j].x = gameState->player.x + gameState->player.width / 2;
                        gameState->bullets[j].y = secondBulletY;
                        gameState->bullets[j].width = BULLET_WIDTH;
                        gameState->bullets[j].height = BULLET_HEIGHT;
                        gameState->bullets[j].speed = BULLET_SPEED;
                        gameState->bullets[j].active = true;
                        break;
                    }
                }
            }
            break;
        }
    }
}

void spawnEnemy(GameState* gameState, EnemyType type) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gameState->enemies[i].active) {
            gameState->enemies[i].x = SCREEN_WIDTH + 20;
            
            float enemyHeight;
            switch (type) {
                case ENEMY_SMALL:
                    enemyHeight = 12;
                    break;
                case ENEMY_MEDIUM:
                    enemyHeight = 16;
                    break;
                case ENEMY_LARGE:
                    enemyHeight = 24;
                    break;
                case ENEMY_BOSS:
                    enemyHeight = 48;
                    break;
                default:
                    enemyHeight = 16;
                    break;
            }
            
            float minY = enemyHeight / 2;
            float maxY = SCREEN_HEIGHT - enemyHeight;
            float spawnY = (rand() % (int)(maxY - minY)) + minY;
            
            gameState->enemies[i].y = spawnY;
            gameState->enemies[i].active = true;
            gameState->enemies[i].type = type;
            gameState->enemies[i].movementPattern = (float)(rand() % 628) / 100.0f; 
            
            switch (type) {
                case ENEMY_SMALL:
                    gameState->enemies[i].width = 16;
                    gameState->enemies[i].height = 12;
                    gameState->enemies[i].speed = 80.0f + (gameState->level.number * 5.0f);
                    gameState->enemies[i].health = 1;
                    gameState->enemies[i].score = 30;
                    break;
                case ENEMY_MEDIUM:
                    gameState->enemies[i].width = 24;
                    gameState->enemies[i].height = 16;
                    gameState->enemies[i].speed = 60.0f + (gameState->level.number * 3.0f);
                    gameState->enemies[i].health = 2;
                    gameState->enemies[i].score = 50;
                    break;
                case ENEMY_LARGE:
                    gameState->enemies[i].width = 32;
                    gameState->enemies[i].height = 24;
                    gameState->enemies[i].speed = 40.0f + (gameState->level.number * 2.0f);
                    gameState->enemies[i].health = 3;
                    gameState->enemies[i].score = 150;
                    gameState->enemies[i].bulletCooldown = (rand() % 3) + 1.0f;
                    break;
                case ENEMY_BOSS:
                    gameState->enemies[i].width = 64;
                    gameState->enemies[i].height = 48;
                    gameState->enemies[i].speed = 20.0f;
                    gameState->enemies[i].health = 10 + (gameState->level.number * 5);
                    gameState->enemies[i].score = 200 * gameState->level.number;
                    gameState->enemies[i].movementPattern = 0.0f;
                    break;
                default:
                    break;
            }
            break;
        }
    }
}

void spawnBoss(GameState* gameState) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gameState->enemies[i].active) {
            float bossWidth = 64;
            float bossHeight = 48;
            
            float bossX = SCREEN_WIDTH - bossWidth;
            if (bossX < SCREEN_WIDTH / 2) {
                bossX = SCREEN_WIDTH / 2;
            }
            
            gameState->enemies[i].x = bossX;
            gameState->enemies[i].y = SCREEN_HEIGHT / 2;
            gameState->enemies[i].width = bossWidth;
            gameState->enemies[i].height = bossHeight;
            gameState->enemies[i].speed = 20.0f;
            gameState->enemies[i].health = 10 + (gameState->level.number * 5);
            gameState->enemies[i].type = ENEMY_BOSS;
            gameState->enemies[i].active = true;
            gameState->enemies[i].bulletCooldown = 1.0f;
            gameState->enemies[i].score = 100 * gameState->level.number;
            gameState->enemies[i].movementPattern = 0.0f;
            break;
        }
    }
}

void spawnPowerup(GameState* gameState, float x, float y) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!gameState->powerups[i].active) {
            if (y < POWERUP_HEIGHT / 2) {
                y = POWERUP_HEIGHT / 2;
            } else if (y > SCREEN_HEIGHT - POWERUP_HEIGHT) {
                y = SCREEN_HEIGHT - POWERUP_HEIGHT;
            }
            
            gameState->powerups[i].x = x;
            gameState->powerups[i].y = y;
            gameState->powerups[i].width = POWERUP_WIDTH;
            gameState->powerups[i].height = POWERUP_HEIGHT;
            gameState->powerups[i].speed = 60.0f;
            gameState->powerups[i].active = true;
            
            int type = rand() % 3;
            if (gameState->player.lives < 3 && rand() % 100 < 40) {
                gameState->powerups[i].type = POWERUP_HEALTH;
            } else {
                gameState->powerups[i].type = (PowerupType)type;
            }
            break;
        }
    }
}

void createExplosion(GameState* gameState, float x, float y, float size) {
    int index = -1;

    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!gameState->explosions[i].active) {
            index = i;
            break;
        }
    }

    if (index < 0 && gameState->benchmarkMode) {
        float lowestLife = 1e9f;
        int best = -1;

        for (int i = 0; i < MAX_EXPLOSIONS; i++) {
            if (!gameState->explosions[i].persistent) {
                float life = gameState->explosions[i].currentLife;
                if (life < lowestLife) {
                    lowestLife = life;
                    best = i;
                }
            }
        }

        if (best < 0) {
            for (int i = 0; i < MAX_EXPLOSIONS; i++) {
                float life = gameState->explosions[i].currentLife;
                if (life < lowestLife) {
                    lowestLife = life;
                    best = i;
                }
            }
        }

        index = best;
    }

    if (index < 0) {
        return;
    }

    if (x < size / 2) {
        x = size / 2;
    } else if (x > SCREEN_WIDTH - size / 2) {
        x = SCREEN_WIDTH - size / 2;
    }
    
    if (y < size / 2) {
        y = size / 2;
    } else if (y > SCREEN_HEIGHT - size) {
        y = SCREEN_HEIGHT - size;
    }

    gameState->explosions[index].x = x;
    gameState->explosions[index].y = y;
    gameState->explosions[index].width = size;
    gameState->explosions[index].height = size;
    gameState->explosions[index].lifespan = 0.5f;
    gameState->explosions[index].currentLife = 0.5f;
    gameState->explosions[index].active = true;
    gameState->explosions[index].persistent = false;
}

void handleCollisions(GameState* gameState) {
    buildEnemyGrid(gameState);
    buildEnemyBulletGrid(gameState);
    buildPowerupGrid(gameState);

    bool enemyPlayerChecked[MAX_ENEMIES] = {false};

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gameState->bullets[i].active) {
            float bxMin = gameState->bullets[i].x;
            float bxMax = gameState->bullets[i].x + gameState->bullets[i].width;
            float byMin = gameState->bullets[i].y;
            float byMax = gameState->bullets[i].y + gameState->bullets[i].height;

            int colStart = clampInt((int)(bxMin / GRID_CELL_SIZE), 0, GRID_COLS - 1);
            int colEnd   = clampInt((int)(bxMax / GRID_CELL_SIZE), 0, GRID_COLS - 1);
            int rowStart = clampInt((int)(byMin / GRID_CELL_SIZE), 0, GRID_ROWS - 1);
            int rowEnd   = clampInt((int)(byMax / GRID_CELL_SIZE), 0, GRID_ROWS - 1);

            for (int r = rowStart; r <= rowEnd && gameState->bullets[i].active; r++) {
                for (int c = colStart; c <= colEnd && gameState->bullets[i].active; c++) {
                    int count = enemyGridCount[r][c];
                    for (int idx = 0; idx < count && gameState->bullets[i].active; idx++) {
                        int j = enemyGrid[r][c][idx];
                        if (!gameState->enemies[j].active) {
                            continue;
                        }

                        if (gameState->bullets[i].x < gameState->enemies[j].x + gameState->enemies[j].width &&
                            gameState->bullets[i].x + gameState->bullets[i].width > gameState->enemies[j].x &&
                            gameState->bullets[i].y < gameState->enemies[j].y + gameState->enemies[j].height &&
                            gameState->bullets[i].y + gameState->bullets[i].height > gameState->enemies[j].y) {

                            gameState->enemies[j].health--;
                            gameState->bullets[i].active = false;

                            if (gameState->enemies[j].health <= 0) {
                                gameState->player.score += gameState->enemies[j].score;

                                createExplosion(gameState, gameState->enemies[j].x, gameState->enemies[j].y,
                                                gameState->enemies[j].width * 1.5f);

                                if (gameState->enemies[j].type == ENEMY_BOSS) {
                                    if (!gameState->benchmarkMode) {
                                        gameState->level.bossDefeated = true;
                                        spawnPowerup(gameState, gameState->enemies[j].x, gameState->enemies[j].y);
                                    } else {
                                        spawnPowerup(gameState, gameState->enemies[j].x, gameState->enemies[j].y);
                                    }
                                }

                                if (gameState->enemies[j].type != ENEMY_BOSS && rand() % 100 < 10) {
                                    spawnPowerup(gameState, gameState->enemies[j].x, gameState->enemies[j].y);
                                }

                                if (gameState->benchmarkMode) {
                                    respawnEnemyRight(gameState, j);
                                } else {
                                    gameState->enemies[j].active = false;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Enemy bullets vs player
    {
        float pxMin = gameState->player.x;
        float pxMax = gameState->player.x + gameState->player.width;
        float pyMin = gameState->player.y;
        float pyMax = gameState->player.y + gameState->player.height;

        int colStart = clampInt((int)(pxMin / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int colEnd   = clampInt((int)(pxMax / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int rowStart = clampInt((int)(pyMin / GRID_CELL_SIZE), 0, GRID_ROWS - 1);
        int rowEnd   = clampInt((int)(pyMax / GRID_CELL_SIZE), 0, GRID_ROWS - 1);

        for (int r = rowStart; r <= rowEnd; r++) {
            for (int c = colStart; c <= colEnd; c++) {
                int count = enemyBulletGridCount[r][c];
                for (int idx = 0; idx < count; idx++) {
                    int i = enemyBulletGrid[r][c][idx];
                    if (!gameState->enemyBullets[i].active) {
                        continue;
                    }

                    if (gameState->enemyBullets[i].x < gameState->player.x + gameState->player.width &&
                        gameState->enemyBullets[i].x + gameState->enemyBullets[i].width > gameState->player.x &&
                        gameState->enemyBullets[i].y < gameState->player.y + gameState->player.height &&
                        gameState->enemyBullets[i].y + gameState->enemyBullets[i].height > gameState->player.y) {

                        if (!gameState->benchmarkMode) {
                            gameState->player.lives--;
                        }
                        gameState->enemyBullets[i].active = false;

                        createExplosion(gameState, gameState->player.x, gameState->player.y, gameState->player.width);

                        gameState->player.isRapidFire = false;
                        gameState->player.isDoubleBullet = false;
                        gameState->player.powerupTimer = 0.0f;
                    }
                }
            }
        }
    }

    // Enemies vs player
    {
        float pxMin = gameState->player.x;
        float pxMax = gameState->player.x + gameState->player.width;
        float pyMin = gameState->player.y;
        float pyMax = gameState->player.y + gameState->player.height;

        int colStart = clampInt((int)(pxMin / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int colEnd   = clampInt((int)(pxMax / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int rowStart = clampInt((int)(pyMin / GRID_CELL_SIZE), 0, GRID_ROWS - 1);
        int rowEnd   = clampInt((int)(pyMax / GRID_CELL_SIZE), 0, GRID_ROWS - 1);

        for (int r = rowStart; r <= rowEnd; r++) {
            for (int c = colStart; c <= colEnd; c++) {
                int count = enemyGridCount[r][c];
                for (int idx = 0; idx < count; idx++) {
                    int i = enemyGrid[r][c][idx];
                    if (enemyPlayerChecked[i]) {
                        continue;
                    }
                    enemyPlayerChecked[i] = true;

                    if (!gameState->enemies[i].active) {
                        continue;
                    }

                    if (gameState->enemies[i].x < gameState->player.x + gameState->player.width &&
                        gameState->enemies[i].x + gameState->enemies[i].width > gameState->player.x &&
                        gameState->enemies[i].y < gameState->player.y + gameState->player.height &&
                        gameState->enemies[i].y + gameState->enemies[i].height > gameState->player.y) {

                        if (!gameState->benchmarkMode) {
                            gameState->player.lives--;
                        }

                        createExplosion(gameState, gameState->player.x, gameState->player.y, gameState->player.width);
                        createExplosion(gameState, gameState->enemies[i].x, gameState->enemies[i].y, gameState->enemies[i].width);

                        if (gameState->enemies[i].type != ENEMY_BOSS) {
                            if (gameState->benchmarkMode) {
                                respawnEnemyRight(gameState, i);
                            } else {
                                gameState->enemies[i].active = false;
                            }
                        } else {
                            gameState->player.x = 50.0f;
                        }

                        gameState->player.isRapidFire = false;
                        gameState->player.isDoubleBullet = false;
                        gameState->player.powerupTimer = 0.0f;
                    }
                }
            }
        }
    }

    // Powerups vs player
    {
        float pxMin = gameState->player.x;
        float pxMax = gameState->player.x + gameState->player.width;
        float pyMin = gameState->player.y;
        float pyMax = gameState->player.y + gameState->player.height;

        int colStart = clampInt((int)(pxMin / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int colEnd   = clampInt((int)(pxMax / GRID_CELL_SIZE), 0, GRID_COLS - 1);
        int rowStart = clampInt((int)(pyMin / GRID_CELL_SIZE), 0, GRID_ROWS - 1);
        int rowEnd   = clampInt((int)(pyMax / GRID_CELL_SIZE), 0, GRID_ROWS - 1);

        for (int r = rowStart; r <= rowEnd; r++) {
            for (int c = colStart; c <= colEnd; c++) {
                int count = powerupGridCount[r][c];
                for (int idx = 0; idx < count; idx++) {
                    int i = powerupGrid[r][c][idx];
                    if (!gameState->powerups[i].active) {
                        continue;
                    }

                    if (gameState->powerups[i].x < gameState->player.x + gameState->player.width &&
                        gameState->powerups[i].x + gameState->powerups[i].width > gameState->player.x &&
                        gameState->powerups[i].y < gameState->player.y + gameState->player.height &&
                        gameState->powerups[i].y + gameState->powerups[i].height > gameState->player.y) {

                        switch (gameState->powerups[i].type) {
                            case POWERUP_HEALTH:
                                if (gameState->player.lives < 3) {
                                    gameState->player.lives++;
                                }
                                break;
                            case POWERUP_RAPID_FIRE:
                                gameState->player.isRapidFire = true;
                                gameState->player.powerupTimer = POWERUP_DURATION;
                                break;
                            case POWERUP_DOUBLE_BULLET:
                                gameState->player.isDoubleBullet = true;
                                gameState->player.powerupTimer = POWERUP_DURATION;
                                break;
                        }

                        gameState->powerups[i].active = false;
                    }
                }
            }
        }
    }
}

void nextLevel(GameState* gameState) {
    gameState->level.number++;
    gameState->level.bossSpawned = false;
    gameState->level.bossDefeated = false;
    gameState->level.scrollSpeed += 5.0f;
    gameState->level.enemySpawnRate *= 0.6f;
    if (gameState->level.enemySpawnRate < 1.0f) {
        gameState->level.enemySpawnRate = 1.0f;
    }
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        gameState->enemies[i].active = false;
    }
    
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        gameState->enemyBullets[i].active = false;
    }
    
    gameState->enemySpawnTimer = 2.0f;
    gameState->powerupSpawnTimer = POWERUP_SPAWN_DELAY / 2;
}

static void respawnEnemyRight(GameState* gameState, int idx) {
    float bandFrac = gameState->benchmarkMode ? gameState->benchmarkSpawnBand : 0.10f;
    if (bandFrac <= 0.0f) bandFrac = 0.10f;
    if (bandFrac > 1.0f) bandFrac = 1.0f;
    float band = SCREEN_WIDTH * bandFrac;
    float jitter = (float)(rand() % (int)(band + 1));

    Enemy* e = &gameState->enemies[idx];
    float minY = e->height / 2.0f;
    float maxY = SCREEN_HEIGHT - e->height;
    e->x = (SCREEN_WIDTH - e->width / 2.0f) - jitter;
    e->y = minY + (float)(rand() % (int)(maxY - minY + 1));
    e->movementPattern = (float)(rand() % 628) / 100.0f;
    if (e->type == ENEMY_LARGE) {
        e->bulletCooldown = (rand() % 3) * 0.5f + 0.2f;
    } else if (e->type == ENEMY_BOSS) {
        e->bulletCooldown = 0.5f;
    }
    switch (e->type) {
        case ENEMY_SMALL: e->health = 1; break;
        case ENEMY_MEDIUM: e->health = 2; break;
        case ENEMY_LARGE: e->health = 3; break;
        case ENEMY_BOSS: e->health = 100; break;
    }
    e->active = true;
}

void prepareBenchmarkScene(GameState* gameState, int density) {
    if (density < 0) density = 0;
    if (density > 100) density = 100;

    gameState->benchmarkMode = true;
    gameState->benchmarkSpawnBand = 0.10f;
    gameState->player.lives = 9999;
    gameState->player.bulletCooldown = 0.0f;

    gameState->level.scrollSpeed = 80.0f;
    gameState->level.enemySpawnRate = 0.15f;
    gameState->enemySpawnTimer = 0.0f;
    gameState->powerupSpawnTimer = 2.0f;

    int targetBullets = (MAX_BULLETS * density) / 100;
    int targetEnemyBullets = (MAX_ENEMY_BULLETS * density) / 100;
    int targetEnemies = (MAX_ENEMIES * density) / 100;
    int targetPowerups = (MAX_POWERUPS * density) / 100;
    int targetExplosions = (MAX_EXPLOSIONS * density) / 100;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (i < targetBullets) {
            gameState->bullets[i].active = true;
            gameState->bullets[i].width = BULLET_WIDTH;
            gameState->bullets[i].height = BULLET_HEIGHT;
            gameState->bullets[i].speed = -BULLET_SPEED;
            gameState->bullets[i].x = (float)(rand() % SCREEN_WIDTH);
            gameState->bullets[i].y = (float)(rand() % (SCREEN_HEIGHT - BULLET_HEIGHT)) + BULLET_HEIGHT / 2.0f;
        } else {
            gameState->bullets[i].active = false;
        }
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (i < targetEnemyBullets) {
            gameState->enemyBullets[i].active = true;
            gameState->enemyBullets[i].width = BULLET_WIDTH;
            gameState->enemyBullets[i].height = BULLET_HEIGHT;
            gameState->enemyBullets[i].speed = ENEMY_BULLET_SPEED;
            gameState->enemyBullets[i].x = (float)(SCREEN_WIDTH - (rand() % (SCREEN_WIDTH / 2)));
            gameState->enemyBullets[i].y = (float)(rand() % (SCREEN_HEIGHT - BULLET_HEIGHT)) + BULLET_HEIGHT / 2.0f;
        } else {
            gameState->enemyBullets[i].active = false;
        }
    }

    int enemiesPlaced = 0;
    int bossTarget = 0;
    if (targetEnemies > 0) {
        bossTarget = 10;
        if (bossTarget > targetEnemies) bossTarget = targetEnemies;
    }
    int bossesPlaced = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemiesPlaced >= targetEnemies) {
            gameState->enemies[i].active = false;
            continue;
        }

        EnemyType type;
        if (bossesPlaced < bossTarget) {
            type = ENEMY_BOSS;
            bossesPlaced++;
        } else {
            int r = rand() % 100;
            if (r < 60) type = ENEMY_SMALL; else if (r < 85) type = ENEMY_MEDIUM; else type = ENEMY_LARGE;
        }

        gameState->enemies[i].type = type;
        gameState->enemies[i].active = true;
        gameState->enemies[i].movementPattern = (float)(rand() % 628) / 100.0f;

        switch (type) {
            case ENEMY_SMALL:
                gameState->enemies[i].width = 16;
                gameState->enemies[i].height = 12;
                gameState->enemies[i].speed = 90.0f + (gameState->level.number * 5.0f);
                gameState->enemies[i].health = 1;
                gameState->enemies[i].score = 30;
                break;
            case ENEMY_MEDIUM:
                gameState->enemies[i].width = 24;
                gameState->enemies[i].height = 16;
                gameState->enemies[i].speed = 70.0f + (gameState->level.number * 3.0f);
                gameState->enemies[i].health = 2;
                gameState->enemies[i].score = 50;
                break;
            case ENEMY_LARGE:
                gameState->enemies[i].width = 32;
                gameState->enemies[i].height = 24;
                gameState->enemies[i].speed = 50.0f + (gameState->level.number * 2.0f);
                gameState->enemies[i].health = 3;
                gameState->enemies[i].score = 150;
                gameState->enemies[i].bulletCooldown = 0.2f;
                break;
            case ENEMY_BOSS:
                gameState->enemies[i].width = 64;
                gameState->enemies[i].height = 48;
                gameState->enemies[i].speed = 20.0f;
                gameState->enemies[i].health = 100;
                gameState->enemies[i].score = 1000;
                gameState->enemies[i].movementPattern = 0.0f;
                gameState->enemies[i].bulletCooldown = 0.5f;
                break;
        }

        
        float bandFrac = gameState->benchmarkSpawnBand;
        if (bandFrac <= 0.0f) bandFrac = 0.10f;
        if (bandFrac > 1.0f) bandFrac = 1.0f;
        float exMin, exMax;
        if (type == ENEMY_BOSS && bossTarget > 0) {
            float bossBand = bandFrac * 3.0f;
            if (bossBand < 0.30f) bossBand = 0.30f;
            if (bossBand > 1.0f) bossBand = 1.0f;
            exMin = SCREEN_WIDTH * (1.0f - bossBand);
            exMax = SCREEN_WIDTH - gameState->enemies[i].width / 2.0f;
            float bandWidth = (exMax - exMin);
            if (bandWidth < 1.0f) bandWidth = 1.0f;
            int bossIndex = bossesPlaced - 1;
            if (bossIndex < 0) bossIndex = 0;
            if (bossTarget < 1) bossTarget = 1;
            float slot = bandWidth / (float)bossTarget;
            float jitter = slot * 0.2f * ((float)(rand() % 100) / 100.0f);
            gameState->enemies[i].x = exMin + slot * bossIndex + slot * 0.4f + jitter;
        } else {
            exMin = SCREEN_WIDTH * (1.0f - bandFrac);
            exMax = SCREEN_WIDTH - gameState->enemies[i].width / 2.0f;
            gameState->enemies[i].x = exMin + (float)(rand() % (int)(exMax - exMin + 1));
        }
        float eyMin = gameState->enemies[i].height / 2.0f;
        float eyMax = SCREEN_HEIGHT - gameState->enemies[i].height;
        gameState->enemies[i].y = eyMin + (float)(rand() % (int)(eyMax - eyMin + 1));

        enemiesPlaced++;
    }

    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (i < targetPowerups) {
            gameState->powerups[i].active = true;
            gameState->powerups[i].width = POWERUP_WIDTH;
            gameState->powerups[i].height = POWERUP_HEIGHT;
            gameState->powerups[i].speed = 60.0f;
            gameState->powerups[i].type = (PowerupType)(rand() % 3);
            gameState->powerups[i].x = SCREEN_WIDTH - (float)(rand() % (SCREEN_WIDTH / 3));
            gameState->powerups[i].y = (float)(rand() % (SCREEN_HEIGHT - POWERUP_HEIGHT)) + POWERUP_HEIGHT / 2.0f;
        } else {
            gameState->powerups[i].active = false;
        }
    }

    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (i < targetExplosions) {
            gameState->explosions[i].active = true;
            gameState->explosions[i].width = 24.0f;
            gameState->explosions[i].height = 24.0f;
            gameState->explosions[i].lifespan = 0.6f;
            gameState->explosions[i].currentLife = 0.6f * (float)(rand() % 100) / 100.0f;
            gameState->explosions[i].persistent = true;
            gameState->explosions[i].x = (float)(rand() % SCREEN_WIDTH);
            gameState->explosions[i].y = (float)(rand() % SCREEN_HEIGHT);
        } else {
            gameState->explosions[i].active = false;
            gameState->explosions[i].persistent = false;
        }
    }
}
