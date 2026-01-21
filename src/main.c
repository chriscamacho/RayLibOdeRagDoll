/*
 * Copyright (c) 2021-2026 Chris Camacho (codifies -  http://bedroomcoders.co.uk/)
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
#include "collision.h"

#include "assert.h"

/*
 * get ODE from https://bitbucket.org/odedevs/ode/downloads/
 *
 * extract ode 0.16.6 into the main directory of this project
 * make a symlink to ode or rename the directory to ode
 * 
 * cd into it
 * 
 * I'd suggest building it with this configuration
 * ./configure --enable-ou --enable-libccd --with-box-cylinder=libccd --with-drawstuff=none --disable-demos --with-libccd=internal
 *
 * and run make, you should then be set to compile this project
 * 
 * you should have a directory structure similar to this
 * 
 * raylib				<-	symlink to raylib-5.5
 * ode					<-	symlink to ode-0.16.6
 * RayLibOdeRagDoll		<-	this project
 */


int main(void)
{
    assert(sizeof(dReal) == sizeof(float));
    srand ( time(NULL) );

    // Physics context - local to main, holds all physics state
    PhysicsContext* physCtx = NULL;

    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1920/1.2;
    const int screenHeight = 1080/1.2;

    dSpaceID space;
    GraphicsContext graphics = { 0 };
    InitGraphics(&graphics, screenWidth, screenHeight, "raylib ODE - Rag Doll Physics Demo");

    // Define the camera to look into our 3d world
    Vector3 cameraTarget = { 4.0f, 2.0f, 1.0f };
    Camera camera = {(Vector3){ 12.0f, 8.0f, 12.0f }, cameraTarget,
                        (Vector3){ 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE};
    
    // Calculate initial yaw/pitch to look at target
    Vector3 toTarget = Vector3Subtract(cameraTarget, camera.position);
    float dist = Vector3Length(toTarget);
    float cameraYaw = atan2f(toTarget.z, toTarget.x);
    float cameraPitch = asinf(toTarget.y / dist);
    
    DisableCursor();  // Hide and lock cursor

    physCtx = InitPhysics(&space, &graphics);


    Vector3 debug = {0}; // general use
    
    // keep the physics fixed time in step with the render frame
    // rate which we don't know in advance
    float frameTime = 0; 
    float physTime = 0;
    const float physSlice = 1.0 / 240.0;
    const int maxPsteps = 6;

    //--------------------------------------------------------------------------------------
    //
    // Main game loop
    //
    //--------------------------------------------------------------------------------------
    while (!WindowShouldClose())            // Detect window close button or ESC key
    {
        //--------------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------

        
        // Update camera with mouse look (first-person style)
        cameraYaw += GetMouseDelta().x * 0.003f;
        cameraPitch += GetMouseDelta().y * 0.003f;
        cameraPitch = Clamp(cameraPitch, -1.5f, 1.5f);
        
        // Calculate forward, right, and up vectors from yaw/pitch
        Vector3 forward = { cosf(cameraYaw) * cosf(cameraPitch), sinf(cameraPitch), sinf(cameraYaw) * cosf(cameraPitch) };
        Vector3 up = { 0.0f, 1.0f, 0.0f };
        Vector3 right = Vector3CrossProduct(forward, up);
        
        // Camera movement speed
        float moveSpeed = 0.1f * GetFrameTime() * 60.0f;
        
        // W/S - forward/backward in camera direction
        if (IsKeyDown(KEY_W)) {
            camera.position = Vector3Add(camera.position, Vector3Scale(forward, moveSpeed));
        }
        if (IsKeyDown(KEY_S)) {
            camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, moveSpeed));
        }
        
        // A/D - strafe left/right
        if (IsKeyDown(KEY_D)) {
            camera.position = Vector3Add(camera.position, Vector3Scale(right, moveSpeed));
        }
        if (IsKeyDown(KEY_A)) {
            camera.position = Vector3Subtract(camera.position, Vector3Scale(right, moveSpeed));
        }
        
        // Q/E - up/down (global Y)
        if (IsKeyDown(KEY_E)) {
            camera.position.y += moveSpeed;
        }
        if (IsKeyDown(KEY_Q)) {
            camera.position.y -= moveSpeed;
        }
        
        // Update target based on new position
        camera.target = Vector3Add(camera.position, forward);
        
        bool spcdn = IsKeyDown(KEY_SPACE);
        
        for (int i = 0; i < NUM_OBJ; i++) {
            const dReal* pos = dBodyGetPosition(physCtx->obj[i]);
            if (spcdn) {
                // apply force if the space key is held
                const dReal* v = dBodyGetLinearVel(physCtx->obj[0]);
                if (v[1] < 10 && pos[1]<10) { // cap upwards velocity and don't let it get too high
                    dBodyEnable (physCtx->obj[i]); // case its gone to sleep
                    dMass mass;
                    dBodyGetMass (physCtx->obj[i], &mass);
                    // give some object more force than others
                    float f = (6+(((float)i/NUM_OBJ)*4)) * mass.mass;
                    dBodyAddForce(physCtx->obj[i], rndf(-f,f), f*10, rndf(-f,f));
                }
            }

            
            if(pos[1]<-10) {
                // teleport back if fallen off the ground
                dBodySetPosition(physCtx->obj[i], dRandReal() * 80 - 40,
                                        12 + rndf(1,2), dRandReal() * 80 - 40);
                dBodySetLinearVel(physCtx->obj[i], 0, 0, 0);
                dBodySetAngularVel(physCtx->obj[i], 0, 0, 0);
            }
        }
        
        // Apply lifting force to ragdolls when space is held
        if (spcdn) {
            for (int i = 0; i < physCtx->ragdollCount; i++) {
                if (physCtx->ragdolls[i] && physCtx->ragdolls[i]->bodies[RAGDOLL_HEAD]) {
                    dBodyEnable(physCtx->ragdolls[i]->bodies[RAGDOLL_HEAD]);
                    // Calculate total mass of all body parts in the ragdoll
                    float totalMass = 0.0f;
                    for (int j = 0; j < physCtx->ragdolls[i]->bodyCount; j++) {
                        if (physCtx->ragdolls[i]->bodies[j]) {
                            dMass partMass;
                            dBodyGetMass(physCtx->ragdolls[i]->bodies[j], &partMass);
                            totalMass += partMass.mass;
                        }
                    }
                    // Lift force based on total ragdoll mass (60 * total mass)
                    float liftForce = 60.0f * totalMass;
                    dBodyAddForce(physCtx->ragdolls[i]->bodies[RAGDOLL_HEAD], 
                                  rndf(-10, 10), liftForce + rndf(-5, 5), rndf(-10, 10));
                }
            }
        }
        
        // Reset rag dolls if they fall off the plane
        for (int i = 0; i < physCtx->ragdollCount; i++) {
            if (physCtx->ragdolls[i] && physCtx->ragdolls[i]->bodies[RAGDOLL_TORSO]) {
                const dReal* pos = dBodyGetPosition(physCtx->ragdolls[i]->bodies[RAGDOLL_TORSO]);
                if (pos[1] < -10) {
                    // Re-create rag doll at a new random spawn position
                    FreeRagdoll(physCtx->ragdolls[i]);
                    physCtx->ragdolls[i] = CreateRagdoll(space, physCtx->world, GetRagdollSpawnPosition(), &graphics);
                }
            }
        }


        if (IsKeyPressed(KEY_L)) { graphics.lights[0].enabled = !graphics.lights[0].enabled; UpdateLightValues(graphics.shader, graphics.lights[0]);}

        // update the light shader with the camera view position
        SetShaderValue(graphics.shader, graphics.shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position.x, SHADER_UNIFORM_VEC3);

        frameTime += GetFrameTime();
        int pSteps = 0;
        physTime = GetTime(); 
        
        while (frameTime > physSlice) {
            // check for collisions
            dSpaceCollide(space, physCtx, &nearCallback);
            
            // step the world
            dWorldQuickStep(physCtx->world, physSlice);  // NB fixed time step is important
            dJointGroupEmpty(physCtx->contactgroup);
            
            frameTime -= physSlice;
            pSteps++;
            if (pSteps > maxPsteps) {
                frameTime = 0;
                break;      
            }
        }
        
        physTime = GetTime() - physTime;    



        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
     
        BeginDrawing();

        ClearBackground(BLACK);

        BeginMode3D(camera);
            // NB normally you wouldn't be drawing the collision meshes
            // instead you'd iterrate all the bodies get a user data pointer
            // from the body you'd previously set and use that to look up
            // what you are rendering oriented and positioned as per the
            // body
            drawAllSpaceGeoms(space, &graphics);


        EndMode3D();


        if (pSteps > maxPsteps) DrawText("WARNING CPU overloaded lagging real time", 10, 0, 20, RED);
        DrawText(TextFormat("%2i FPS", GetFPS()), 10, 20, 20, WHITE);
        DrawText("Rag Doll Physics Demo", 10, 40, 20, WHITE);
        DrawText("Press SPACE to apply force to objects", 10, 60, 20, WHITE);
        DrawText("Vehicle code available for future use", 10, 80, 20, GRAY);
        DrawText(TextFormat("debug %4.4f %4.4f %4.4f",debug.x,debug.y,debug.z), 10, 100, 20, WHITE);
        DrawText(TextFormat("Phys steps per frame %i",pSteps), 10, 120, 20, WHITE);
        DrawText(TextFormat("Phys time per frame %f",physTime), 10, 140, 20, WHITE);
        DrawText(TextFormat("total time per frame %f",frameTime), 10, 160, 20, WHITE);
        DrawText(TextFormat("objects %i",NUM_OBJ), 10, 180, 20, WHITE);
        DrawText(TextFormat("ragdolls %i",physCtx->ragdollCount), 10, 200, 20, WHITE);

        EndDrawing();

    }
    //----------------------------------------------------------------------------------

	// san test it was working - nothing was leaking !!!
	//void* test = RL_MALLOC(1024);
	//(void)test;
    
    //--------------------------------------------------------------------------------------
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CleanupGraphics(&graphics, physCtx);

    dSpaceDestroy(space);      // Implicitly destroys all geoms (including simple objects)

    CloseWindow();              // Close window and OpenGL context
    //------------------------------------------------------------------------------------


    
    return 0;
}
