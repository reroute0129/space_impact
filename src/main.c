#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "game.h"
#include "renderer.h"
#include "resources.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1200

GameState gameState;
GLFWwindow* window;

static bool keyUpPressed = false;
static bool keyDownPressed = false;
static bool keyLeftPressed = false;
static bool keyRightPressed = false;

void updatePlayerDirection(GameState* gameState) {
    if (keyUpPressed && keyLeftPressed) {
        gameState->player.direction = DIR_UP_LEFT;
    } else if (keyUpPressed && keyRightPressed) {
        gameState->player.direction = DIR_UP_RIGHT;
    } else if (keyDownPressed && keyLeftPressed) {
        gameState->player.direction = DIR_DOWN_LEFT;
    } else if (keyDownPressed && keyRightPressed) {
        gameState->player.direction = DIR_DOWN_RIGHT;
    } else if (keyUpPressed) {
        gameState->player.direction = DIR_UP;
    } else if (keyDownPressed) {
        gameState->player.direction = DIR_DOWN;
    } else if (keyLeftPressed) {
        gameState->player.direction = DIR_LEFT;
    } else if (keyRightPressed) {
        gameState->player.direction = DIR_RIGHT;
    } else {
        gameState->player.direction = DIR_NONE;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_UP:
                keyUpPressed = true;
                break;
            case GLFW_KEY_DOWN:
                keyDownPressed = true;
                break;
            case GLFW_KEY_RIGHT:
                keyRightPressed = true;
                break;
            case GLFW_KEY_LEFT:
                keyLeftPressed = true;
                break;
            case GLFW_KEY_SPACE:
                fireBullet(&gameState);
                break;
            case GLFW_KEY_ESCAPE:
                gameState.gameOver = true;
                break;
            case GLFW_KEY_ENTER:
                if (gameState.gameOver) {
                    initGame(&gameState);
                }
                break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_UP:
                keyUpPressed = false;
                break;
            case GLFW_KEY_DOWN:
                keyDownPressed = false;
                break;
            case GLFW_KEY_RIGHT:
                keyRightPressed = false;
                break;
            case GLFW_KEY_LEFT:
                keyLeftPressed = false;
                break;
        }
    }
    
    updatePlayerDirection(&gameState);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

bool initOpenGL() {
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Space Impact", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        return false;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

int main() {
    srand(time(NULL));

    if (!initOpenGL()) {
        return -1;
    }

    if (!initRenderer()) {
        glfwTerminate();
        return -1;
    }

    if (!loadResources()) {
        destroyRenderer();
        glfwTerminate();
        return -1;
    }

    initGame(&gameState);

    double lastTime = glfwGetTime();
    double deltaTime = 0.0;
    double frameTime = 1.0 / 60.0; 

    while (!glfwWindowShouldClose(window) && !gameState.gameOver) {
        double currentTime = glfwGetTime();
        deltaTime += currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();

        while (deltaTime >= frameTime) {
            updateGame(&gameState, frameTime);
            deltaTime -= frameTime;
        }

        renderGame(&gameState);

        glfwSwapBuffers(window);
    }

    if (gameState.gameOver) {
        renderGameOver(&gameState);
        glfwSwapBuffers(window);

        while (!glfwWindowShouldClose(window) && gameState.gameOver) {
            glfwPollEvents();
        }
    }

    destroyRenderer();
    glfwTerminate();

    return 0;
}
