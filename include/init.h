/*
 * Copyright (c) 2026 Chris Camacho (codifies -  http://bedroomcoders.co.uk/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef INIT_H
#define INIT_H

#include "raylib.h"
#include <ode/ode.h>
#include "raylibODEragdoll.h"
#include "rlights.h"

// Object counts
#define NUM_OBJ 50
#define MAX_RAGDOLLS 12

// Plane configuration
#define PLANE_SIZE 100.0f
#define PLANE_THICKNESS 1.0f

// Physics context - holds all physics state
typedef struct PhysicsContext {
    dWorldID world;
    dJointGroupID contactgroup;
    dBodyID obj[NUM_OBJ];
    RagDoll* ragdolls[MAX_RAGDOLLS];
    int ragdollCount;
} PhysicsContext;

// Graphics context - holds all rendering resources
typedef struct GraphicsContext {
    Model box;
    Model ball;
    Model cylinder;
    
    // Texture arrays for different geometry types
    Texture sphereTextures[3];      // ball.png, beach-ball.png, earth.png
    Texture boxTextures[2];         // crate.png, grid.png
    Texture cylinderTextures[2];    // drum.png, cylinder2.png
    Texture groundTexture;          // grass.png
    
    Shader shader;
    Light lights[MAX_LIGHTS];
} GraphicsContext;

// Initialize graphics resources and window
void InitGraphics(GraphicsContext* ctx, int width, int height, const char* title);

// Initialize the physics world and create all objects
// Returns pointer to PhysicsContext (caller responsible for passing to CleanupPhysics)
PhysicsContext* InitPhysics(dSpaceID* space, GraphicsContext* gfxCtx);

// Clean up application resources
void CleanupGraphics(GraphicsContext* ctx, PhysicsContext* physCtx);

#endif // INIT_H
