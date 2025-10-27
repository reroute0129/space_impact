#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "renderer.h"
#include "resources.h"

static const char* vertexShaderSource = 
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"uniform mat4 model;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);\n"
"    TexCoord = aTexCoord;\n"
"}\n";

static const char* fragmentShaderSource = 
"#version 330 core\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D texture1;\n"
"uniform vec4 color;\n"
"void main()\n"
"{\n"
"    vec4 texColor = texture(texture1, TexCoord);\n"
"    if(texColor.a < 0.1)\n"
"        discard;\n"
"    FragColor = texColor * color;\n"
"}\n";

typedef struct {
    GLuint shaderProgram;
    GLuint VAO, VBO, EBO;
    GLuint textures[SPRITE_COUNT];
    GLint modelLoc;
    GLint projectionLoc;
    GLint colorLoc;
} Renderer;

static Renderer renderer;

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
        return 0;
    }
    
    return shader;
}

static GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        return 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

bool initRenderer() {
    renderer.shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (renderer.shaderProgram == 0) {
        return false;
    }
    
    renderer.modelLoc = glGetUniformLocation(renderer.shaderProgram, "model");
    renderer.projectionLoc = glGetUniformLocation(renderer.shaderProgram, "projection");
    renderer.colorLoc = glGetUniformLocation(renderer.shaderProgram, "color");
    
    float vertices[] = {
         0.5f,  0.5f,         1.0f, 0.0f,   
         0.5f, -0.5f,         1.0f, 1.0f,   
        -0.5f, -0.5f,         0.0f, 1.0f,   
        -0.5f,  0.5f,         0.0f, 0.0f    
    };
    
    unsigned int indices[] = {
        0, 1, 3,  
        1, 2, 3   
    };
    
    glGenVertexArrays(1, &renderer.VAO);
    glGenBuffers(1, &renderer.VBO);
    glGenBuffers(1, &renderer.EBO);
    glBindVertexArray(renderer.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderer.VBO);    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
    
    float projectionMatrix[16] = {
        2.0f/480.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f/320.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    glUseProgram(renderer.shaderProgram);
    
    glUniformMatrix4fv(renderer.projectionLoc, 1, GL_FALSE, projectionMatrix);
    
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glUniform4fv(renderer.colorLoc, 1, color);
    
    return true;
}

void destroyRenderer() {
    glDeleteVertexArrays(1, &renderer.VAO);
    glDeleteBuffers(1, &renderer.VBO);
    glDeleteBuffers(1, &renderer.EBO);
    glDeleteProgram(renderer.shaderProgram);
    
    glDeleteTextures(SPRITE_COUNT, renderer.textures);
}

void setTexture(SpriteType type, GLuint textureID) {
    renderer.textures[type] = textureID;
}

void renderSprite(SpriteType type, float x, float y, float width, float height, float r, float g, float b, float a) {
    float modelMatrix[16] = {
        width, 0.0f, 0.0f, 0.0f,
        0.0f, height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, 0.0f, 1.0f
    };
    
    glUniformMatrix4fv(renderer.modelLoc, 1, GL_FALSE, modelMatrix);
    
    float color[4] = {r, g, b, a};
    glUniform4fv(renderer.colorLoc, 1, color);
    
    glBindTexture(GL_TEXTURE_2D, renderer.textures[type]);
    
    glBindVertexArray(renderer.VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderGame(GameState* gameState) {
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    float farScrollPos = fmodf(gameState->level.backgroundOffset, 256.0f);
    
    renderSprite(SPRITE_BACKGROUND, 256.0f - farScrollPos, 160, 256, 320, 0.4f, 0.4f, 0.5f, 0.3f);
    
    renderSprite(SPRITE_BACKGROUND, 512.0f - farScrollPos, 160, 256, 320, 0.4f, 0.4f, 0.5f, 0.3f);
    
    renderSprite(SPRITE_BACKGROUND, 768.0f - farScrollPos, 160, 256, 320, 0.4f, 0.4f, 0.5f, 0.3f);
    
    renderSprite(SPRITE_BACKGROUND, 0.0f - farScrollPos, 160, 256, 320, 0.4f, 0.4f, 0.5f, 0.3f);

    float midScrollPos = fmodf(gameState->level.midgroundOffset, 256.0f);
    
    renderSprite(SPRITE_BACKGROUND, 256.0f - midScrollPos, 160, 256, 320, 0.5f, 0.5f, 0.6f, 0.5f);

    renderSprite(SPRITE_BACKGROUND, 512.0f - midScrollPos, 160, 256, 320, 0.5f, 0.5f, 0.6f, 0.5f);
    
    renderSprite(SPRITE_BACKGROUND, 768.0f - midScrollPos, 160, 256, 320, 0.5f, 0.5f, 0.6f, 0.5f);
    
    renderSprite(SPRITE_BACKGROUND, 0.0f - midScrollPos, 160, 256, 320, 0.5f, 0.5f, 0.6f, 0.5f);
    
    float nearScrollPos = fmodf(gameState->level.foregroundOffset, 256.0f);
    
    renderSprite(SPRITE_BACKGROUND, 256.0f - nearScrollPos, 160, 256, 320, 0.7f, 0.7f, 0.8f, 0.7f);
    
    renderSprite(SPRITE_BACKGROUND, 512.0f - nearScrollPos, 160, 256, 320, 0.7f, 0.7f, 0.8f, 0.7f);
    
    renderSprite(SPRITE_BACKGROUND, 768.0f - nearScrollPos, 160, 256, 320, 0.7f, 0.7f, 0.8f, 0.7f);
    
    renderSprite(SPRITE_BACKGROUND, 0.0f - nearScrollPos, 160, 256, 320, 0.7f, 0.7f, 0.8f, 0.7f);
    
    renderSprite(SPRITE_PLAYER, gameState->player.x, gameState->player.y, 
               gameState->player.width, gameState->player.height, 1.0f, 1.0f, 1.0f, 1.0f);
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gameState->bullets[i].active) {
            renderSprite(SPRITE_BULLET, gameState->bullets[i].x, gameState->bullets[i].y, 
                       gameState->bullets[i].width, gameState->bullets[i].height, 1.0f, 1.0f, 0.5f, 1.0f);
        }
    }
    
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (gameState->enemyBullets[i].active) {
            renderSprite(SPRITE_ENEMY_BULLET, gameState->enemyBullets[i].x, gameState->enemyBullets[i].y, 
                       gameState->enemyBullets[i].width, gameState->enemyBullets[i].height, 1.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (gameState->enemies[i].active) {
            SpriteType spriteType;
            switch (gameState->enemies[i].type) {
                case ENEMY_SMALL:
                    spriteType = SPRITE_ENEMY_SMALL;
                    break;
                case ENEMY_MEDIUM:
                    spriteType = SPRITE_ENEMY_MEDIUM;
                    break;
                case ENEMY_LARGE:
                    spriteType = SPRITE_ENEMY_LARGE;
                    break;
                case ENEMY_BOSS:
                    spriteType = SPRITE_ENEMY_BOSS;
                    break;
                default:
                    spriteType = SPRITE_ENEMY_SMALL;
                    break;
            }
            renderSprite(spriteType, gameState->enemies[i].x, gameState->enemies[i].y, 
                       gameState->enemies[i].width, gameState->enemies[i].height, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
    
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (gameState->powerups[i].active) {
            SpriteType spriteType;
            float r = 1.0f, g = 1.0f, b = 1.0f;
            
            switch (gameState->powerups[i].type) {
                case POWERUP_HEALTH:
                    spriteType = SPRITE_POWERUP_HEALTH;
                    r = 0.0f; g = 1.0f; b = 0.0f;
                    break;
                case POWERUP_RAPID_FIRE:
                    spriteType = SPRITE_POWERUP_RAPID_FIRE;
                    r = 1.0f; g = 1.0f; b = 0.0f;
                    break;
                case POWERUP_DOUBLE_BULLET:
                    spriteType = SPRITE_POWERUP_DOUBLE_BULLET;
                    r = 0.0f; g = 0.5f; b = 1.0f;
                    break;
                default:
                    spriteType = SPRITE_POWERUP_HEALTH;
                    break;
            }
            renderSprite(spriteType, gameState->powerups[i].x, gameState->powerups[i].y, 
                       gameState->powerups[i].width, gameState->powerups[i].height, r, g, b, 1.0f);
        }
    }
    
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (gameState->explosions[i].active) {
            float alpha = gameState->explosions[i].currentLife / gameState->explosions[i].lifespan;
            renderSprite(SPRITE_EXPLOSION, gameState->explosions[i].x, gameState->explosions[i].y, 
                       gameState->explosions[i].width, gameState->explosions[i].height, 1.0f, 0.7f, 0.0f, alpha);
        }
    }
    
    char scoreText[32];
    sprintf(scoreText, "SCORE: %d", gameState->player.score);
    
    
}

void renderGameOver(GameState* gameState) {
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    char scoreText[32];
    sprintf(scoreText, "FINAL SCORE: %d", gameState->player.score);
    
}
