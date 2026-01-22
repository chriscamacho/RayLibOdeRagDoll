/*
 * Copyright (c) 2021-26 Chris Camacho (codifies -  http://bedroomcoders.co.uk/)
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
 
#ifndef RAYLIBODE_H
#define RAYLIBODE_H

#include "raylib.h"
#include "raymath.h"

#include <ode/ode.h>

void rayToOdeMat(Matrix* mat, dReal* R);
void odeToRayMat(const dReal* R, Matrix* matrix);

// Geometry user data - stores collision flag, texture reference, and UV scale
typedef struct geomInfo {
    bool collidable;
    Texture* texture;
    float uvScaleU;
    float uvScaleV;
} geomInfo;

// Helper to allocate geomInfo with collision flag, optional texture, and UV scale
geomInfo* CreateGeomInfo(bool collidable, Texture* texture, float uvScaleU, float uvScaleV);

// Object counts
#define NUM_OBJ 50
#define MAX_RAGDOLLS 12

// Plane configuration
#define PLANE_SIZE 100.0f
#define PLANE_THICKNESS 1.0f

// Forward declaration
struct RagDoll;

// Physics context - holds all physics state
typedef struct PhysicsContext {
    dWorldID world;
    dSpaceID* space;              // Pointer to space (set by InitPhysics)
    dJointGroupID contactgroup;
    dBodyID obj[NUM_OBJ];
    struct RagDoll* ragdolls[MAX_RAGDOLLS];
    int ragdollCount;
} PhysicsContext;

// Forward declaration - GraphicsContext is defined in init.h
struct GraphicsContext;

void drawAllSpaceGeoms(dSpaceID space, struct GraphicsContext* ctx);
void drawGeom(dGeomID geom, struct GraphicsContext* ctx);

// Random float in range [min, max]
float rndf(float min, float max);

#endif // RAYLIBODE_H


