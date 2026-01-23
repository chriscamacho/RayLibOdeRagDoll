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

#include "raylib.h"
#include "raymath.h"

#include "rlights.h"

#include <ode/ode.h>
#include "raylibODE.h"
#include "raylibODEvehicle.h"
#include "raylibODEragdoll.h"
#include "init.h"

// Helper to allocate geomInfo with collision flag, optional texture, and UV scale
geomInfo* CreateGeomInfo(bool collidable, Texture* texture, float uvScaleU, float uvScaleV)
{
    geomInfo* gi = RL_MALLOC(sizeof(geomInfo));
    gi->collidable = collidable;
    gi->texture = texture;
    gi->uvScaleU = uvScaleU;
    gi->uvScaleV = uvScaleV;
    return gi;
}

// a lot of this stuff doesn't change too often 
// so no point polluting main.c with it...

void InitGraphics(GraphicsContext* ctx, int width, int height, const char* title)
{
    InitWindow(width, height, title);
    SetWindowState(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);

    // Load models
    ctx->box = LoadModelFromMesh(GenMeshCube(1, 1, 1));
    ctx->ball = LoadModelFromMesh(GenMeshSphere(.5, 32, 32));
    ctx->cylinder = LoadModel("data/cylinder.obj");

    // Load sphere textures
    ctx->sphereTextures[0] = LoadTexture("data/ball.png");
    ctx->sphereTextures[1] = LoadTexture("data/beach-ball.png");
    ctx->sphereTextures[2] = LoadTexture("data/earth.png");

    // Load box textures
    ctx->boxTextures[0] = LoadTexture("data/crate.png");
    ctx->boxTextures[1] = LoadTexture("data/grid.png");

    // Load cylinder textures
    ctx->cylinderTextures[0] = LoadTexture("data/drum.png");
    ctx->cylinderTextures[1] = LoadTexture("data/cylinder2.png");

    // Load ground texture
    ctx->groundTexture = LoadTexture("data/grass.png");

    // Apply default textures to models (overridden by per-instance textures)
    ctx->box.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = ctx->boxTextures[0];
    ctx->ball.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = ctx->sphereTextures[0];
    ctx->cylinder.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = ctx->cylinderTextures[0];

    // Load shader and set up uniforms
    ctx->shader = LoadShader("data/simpleLight.vs", "data/simpleLight.fs");
    ctx->shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(ctx->shader, "matModel");
    ctx->shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(ctx->shader, "viewPos");

    // Set ambient light
    int amb = GetShaderLocation(ctx->shader, "ambient");
    SetShaderValue(ctx->shader, amb, (float[4]){0.2, 0.2, 0.2, 1.0}, SHADER_UNIFORM_VEC4);

    // Apply shader to models
    ctx->box.materials[0].shader = ctx->shader;
    ctx->ball.materials[0].shader = ctx->shader;
    ctx->cylinder.materials[0].shader = ctx->shader;

    // Create lights
    ctx->lights[0] = CreateLight(LIGHT_POINT, (Vector3){-25, 25, 25}, Vector3Zero(),
                                (Color){128, 128, 128, 255}, ctx->shader);
    ctx->lights[1] = CreateLight(LIGHT_POINT, (Vector3){-25, 25, -25}, Vector3Zero(),
                                (Color){64, 64, 64, 255}, ctx->shader);
}

PhysicsContext* InitPhysics(dSpaceID* space, GraphicsContext* gfxCtx)
{
    // Allocate physics context
    PhysicsContext* ctx = RL_MALLOC(sizeof(PhysicsContext));
    if (!ctx) return NULL;
    
    // Initialize arrays to NULL for safe cleanup
    for (int i = 0; i < MAX_RAGDOLLS; i++) {
        ctx->ragdolls[i] = NULL;
    }

    dInitODE2(0);
    dAllocateODEDataForThread(dAllocateMaskAll);

    ctx->world = dWorldCreate();
    printf("phys iterations per step %i\n", dWorldGetQuickStepNumIterations(ctx->world));
    *space = dHashSpaceCreate(NULL);
    ctx->space = space;  // Store space pointer for cleanup
    ctx->contactgroup = dJointGroupCreate(0);
    dWorldSetGravity(ctx->world, 0, -9.8, 0);

    dWorldSetAutoDisableFlag(ctx->world, 1);
    dWorldSetAutoDisableLinearThreshold(ctx->world, 0.05);
    dWorldSetAutoDisableAngularThreshold(ctx->world, 0.05);
    dWorldSetAutoDisableSteps(ctx->world, 4);

    // Create ground "plane"
    dGeomID planeGeom = dCreateBox(*space, PLANE_SIZE, PLANE_THICKNESS, PLANE_SIZE);
    dGeomSetPosition(planeGeom, 0, -PLANE_THICKNESS / 2.0, 0);
    dGeomSetData(planeGeom, CreateGeomInfo(true, &gfxCtx->groundTexture, 25.0f, 25.0f));

    // Create random simple objects with random textures
    for (int i = 0; i < NUM_OBJ; i++) {
        ctx->obj[i] = dBodyCreate(ctx->world);
        dGeomID geom;
        dMatrix3 R;
        dMass m;
        Texture* tex = NULL;
        float typ = rndf(0, 1);
        if (typ < .25) {  // box
            Vector3 s = (Vector3){rndf(0.25, .5), rndf(0.25, .5), rndf(0.25, .5)};
            geom = dCreateBox(*space, s.x, s.y, s.z);
            dMassSetBox(&m, 10, s.x, s.y, s.z);
            // Random box texture: crate or grid
            tex = &gfxCtx->boxTextures[(int)rndf(0, 2)];
        } else if (typ < .5) {  // sphere
            float r = rndf(0.25, .4);
            geom = dCreateSphere(*space, r);
            dMassSetSphere(&m, 10, r);
            // Random sphere texture: ball, beach-ball, or earth
            tex = &gfxCtx->sphereTextures[(int)rndf(0, 3)];
        } else if (typ < .75) {  // cylinder
            float l = rndf(0.4, 1);
            float r = rndf(0.125, .5);
            geom = dCreateCylinder(*space, r, l);
            dMassSetCylinder(&m, 10, 3, r, l);
            // Random cylinder texture: drum or cylinder2
            tex = &gfxCtx->cylinderTextures[(int)rndf(0, 2)];
        } else {  // composite of cylinder with 2 spheres
            float l = rndf(.25, .5);
            geom = dCreateCylinder(*space, 0.125, l);
            dGeomID geom2 = dCreateSphere(*space, l / 2);
            dGeomID geom3 = dCreateSphere(*space, l / 2);

            dMass m2, m3;
            dMassSetSphere(&m2, 5, l / 2);
            dMassTranslate(&m2, 0, 0, l - 0.125);
            dMassSetSphere(&m3, 5, l / 2);
            dMassTranslate(&m3, 0, 0, -l + 0.125);
            dMassSetCylinder(&m, 5, 3, .25, l);
            dMassAdd(&m2, &m3);
            dMassAdd(&m, &m2);

            dGeomSetBody(geom2, ctx->obj[i]);
            dGeomSetBody(geom3, ctx->obj[i]);
            dGeomSetOffsetPosition(geom2, 0, 0, l - 0.125);
            dGeomSetOffsetPosition(geom3, 0, 0, -l + 0.125);
            
            // Compound objects use cylinder texture
            tex = &gfxCtx->cylinderTextures[(int)rndf(0, 2)];
            
            // Set textures for all geoms in compound object
            dGeomSetData(geom, CreateGeomInfo(true, tex, 1.0f, 1.0f));
            dGeomSetData(geom2, CreateGeomInfo(true, tex, 1.0f, 1.0f));
            dGeomSetData(geom3, CreateGeomInfo(true, tex, 1.0f, 1.0f));
        }

        // Random position and rotation (offset from ragdoll area)
        dBodySetPosition(ctx->obj[i], dRandReal() * 6 + 5, 4 + (i / 10), dRandReal() * 6 - 3);
        dRFromAxisAndAngle(R, dRandReal() * 2.0 - 1.0,
                           dRandReal() * 2.0 - 1.0,
                           dRandReal() * 2.0 - 1.0,
                           dRandReal() * M_PI * 2 - M_PI);
        dBodySetRotation(ctx->obj[i], R);
        dGeomSetBody(geom, ctx->obj[i]);
        dBodySetMass(ctx->obj[i], &m);
        
        // Set geomInfo with texture
        dGeomSetData(geom, CreateGeomInfo(true, tex, 1.0f, 1.0f));
    }

    // Create ragdolls
    ctx->ragdollCount = MAX_RAGDOLLS;
    for (int i = 0; i < ctx->ragdollCount; i++) {
        ctx->ragdolls[i] = CreateRagdoll(*space, ctx->world, GetRagdollSpawnPosition(), gfxCtx);
    }

    return ctx;
}

void CleanupPhysics(PhysicsContext* ctx)
{
    if (!ctx) return;

    // Free ragdolls
    for (int i = 0; i < ctx->ragdollCount; i++) {
        if (ctx->ragdolls[i]) {
            FreeRagdoll(ctx->ragdolls[i], ctx);
        }
    }

    // Clean up ODE resources
    dJointGroupEmpty(ctx->contactgroup);
    dJointGroupDestroy(ctx->contactgroup);
    dWorldDestroy(ctx->world);
    dCloseODE();

    RL_FREE(ctx);
}

void CleanupGraphics(GraphicsContext* ctx, PhysicsContext* physCtx)
{
    // Clean up physics first
    CleanupPhysics(physCtx);

    // Clean up graphics resources
    UnloadModel(ctx->box);
    UnloadModel(ctx->ball);
    UnloadModel(ctx->cylinder);
    
    // Unload all textures
    UnloadTexture(ctx->sphereTextures[0]);
    UnloadTexture(ctx->sphereTextures[1]);
    UnloadTexture(ctx->sphereTextures[2]);
    UnloadTexture(ctx->boxTextures[0]);
    UnloadTexture(ctx->boxTextures[1]);
    UnloadTexture(ctx->cylinderTextures[0]);
    UnloadTexture(ctx->cylinderTextures[1]);
    UnloadTexture(ctx->groundTexture);
    
    UnloadShader(ctx->shader);
}
