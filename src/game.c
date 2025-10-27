#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "game.h"

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
    }

    gameState->gameOver = false;
    gameState->paused = false;
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
            
            if (gameState->bullets[i].x > SCREEN_WIDTH + gameState->bullets[i].width) {
                gameState->bullets[i].active = false;
            }
            
            if (gameState->bullets[i].y < -gameState->bullets[i].height || 
                gameState->bullets[i].y > SCREEN_HEIGHT + gameState->bullets[i].height) {
                gameState->bullets[i].active = false;
            }
        }
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (gameState->enemyBullets[i].active) {
            gameState->enemyBullets[i].x -= gameState->enemyBullets[i].speed * deltaTime;
            
            if (gameState->enemyBullets[i].x < -gameState->enemyBullets[i].width) {
                gameState->enemyBullets[i].active = false;
            }
            
            if (gameState->enemyBullets[i].y < -gameState->enemyBullets[i].height || 
                gameState->enemyBullets[i].y > SCREEN_HEIGHT + gameState->enemyBullets[i].height) {
                gameState->enemyBullets[i].active = false;
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
                if (gameState->enemies[i].x < SCREEN_WIDTH / 2) {
                    gameState->enemies[i].x = SCREEN_WIDTH / 2;
                } else if (gameState->enemies[i].x > SCREEN_WIDTH - gameState->enemies[i].width / 2) {
                    gameState->enemies[i].x = SCREEN_WIDTH - gameState->enemies[i].width / 2;
                }
            }
            
            if (gameState->enemies[i].x < -gameState->enemies[i].width && 
                gameState->enemies[i].type != ENEMY_BOSS) {
                gameState->enemies[i].active = false;
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
            
            if (gameState->powerups[i].x < -gameState->powerups[i].width) {
                gameState->powerups[i].active = false;
            }
        }
    }

    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (gameState->explosions[i].active) {
            gameState->explosions[i].currentLife -= deltaTime;
            if (gameState->explosions[i].currentLife <= 0) {
                gameState->explosions[i].active = false;
            }
        }
    }

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

    gameState->powerupSpawnTimer -= deltaTime;
    if (gameState->powerupSpawnTimer <= 0) {
        float x = SCREEN_WIDTH;
        
        float minY = POWERUP_HEIGHT / 2;
        float maxY = SCREEN_HEIGHT - POWERUP_HEIGHT;
        float y = (rand() % (int)(maxY - minY)) + minY;
        
        spawnPowerup(gameState, x, y);
        gameState->powerupSpawnTimer = POWERUP_SPAWN_DELAY;
    }

    gameState->level.backgroundOffset += gameState->level.scrollSpeed * 0.3f * deltaTime;  
    gameState->level.midgroundOffset += gameState->level.scrollSpeed * 0.7f * deltaTime;   
    gameState->level.foregroundOffset += gameState->level.scrollSpeed * 1.4f * deltaTime;  

    handleCollisions(gameState);

    if (gameState->level.bossSpawned && gameState->level.bossDefeated) {
        nextLevel(gameState);
    }

    if (gameState->player.lives <= 0) {
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
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!gameState->explosions[i].active) {
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
            
            gameState->explosions[i].x = x;
            gameState->explosions[i].y = y;
            gameState->explosions[i].width = size;
            gameState->explosions[i].height = size;
            gameState->explosions[i].lifespan = 0.5f;
            gameState->explosions[i].currentLife = 0.5f;
            gameState->explosions[i].active = true;
            break;
        }
    }
}

void handleCollisions(GameState* gameState) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gameState->bullets[i].active) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (gameState->enemies[j].active) {
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
                                gameState->level.bossDefeated = true;
                                
                                spawnPowerup(gameState, gameState->enemies[j].x, gameState->enemies[j].y);
                            }
                            
                            if (gameState->enemies[j].type != ENEMY_BOSS && rand() % 100 < 10) {
                                spawnPowerup(gameState, gameState->enemies[j].x, gameState->enemies[j].y);
                            }
                            
                            gameState->enemies[j].active = false;
                        }
                        break;
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (gameState->enemyBullets[i].active) {
            if (gameState->enemyBullets[i].x < gameState->player.x + gameState->player.width &&
                gameState->enemyBullets[i].x + gameState->enemyBullets[i].width > gameState->player.x &&
                gameState->enemyBullets[i].y < gameState->player.y + gameState->player.height &&
                gameState->enemyBullets[i].y + gameState->enemyBullets[i].height > gameState->player.y) {
                
                gameState->player.lives--;
                gameState->enemyBullets[i].active = false;
                
                createExplosion(gameState, gameState->player.x, gameState->player.y, gameState->player.width);
                
                gameState->player.isRapidFire = false;
                gameState->player.isDoubleBullet = false;
                gameState->player.powerupTimer = 0.0f;
            }
        }
    }
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (gameState->enemies[i].active) {
            if (gameState->enemies[i].x < gameState->player.x + gameState->player.width &&
                gameState->enemies[i].x + gameState->enemies[i].width > gameState->player.x &&
                gameState->enemies[i].y < gameState->player.y + gameState->player.height &&
                gameState->enemies[i].y + gameState->enemies[i].height > gameState->player.y) {
                
                gameState->player.lives--;
                
                createExplosion(gameState, gameState->player.x, gameState->player.y, gameState->player.width);
                createExplosion(gameState, gameState->enemies[i].x, gameState->enemies[i].y, gameState->enemies[i].width);
                
                if (gameState->enemies[i].type != ENEMY_BOSS) {
                    gameState->enemies[i].active = false;
                } else {
                    gameState->player.x = 50.0f;
                }
                
                gameState->player.isRapidFire = false;
                gameState->player.isDoubleBullet = false;
                gameState->player.powerupTimer = 0.0f;
            }
        }
    }
    
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (gameState->powerups[i].active) {
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
