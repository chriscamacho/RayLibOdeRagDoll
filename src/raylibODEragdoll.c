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

#include <ode/ode.h>
#include "raylibODE.h"
#include "raylibODEragdoll.h"
#include "init.h"

// Get a spawn position within the defined ragdoll spawn volume
Vector3 GetRagdollSpawnPosition(void)
{
    Vector3 pos;
    pos.x = rndf(RAGDOLL_SPAWN_CENTER_X - RAGDOLL_SPAWN_HALF_EXTENT, RAGDOLL_SPAWN_CENTER_X + RAGDOLL_SPAWN_HALF_EXTENT);
    pos.y = rndf(RAGDOLL_SPAWN_MIN_Y, RAGDOLL_SPAWN_MAX_Y);
    pos.z = rndf(RAGDOLL_SPAWN_CENTER_Z - RAGDOLL_SPAWN_HALF_EXTENT, RAGDOLL_SPAWN_CENTER_Z + RAGDOLL_SPAWN_HALF_EXTENT);
    return pos;
}


// Rag doll creation - generic structure for eventual neural network muscle control
// Creates a humanoid rag doll with configurable joint motors
// actually way more complex than the vehicle stuff !
RagDoll* CreateRagdoll(dSpaceID space, dWorldID world, Vector3 position, struct GraphicsContext* ctx)
{
    RagDoll *ragdoll = RL_MALLOC(sizeof(RagDoll));
    ragdoll->bodyCount = RAGDOLL_BODY_COUNT;
    ragdoll->jointCount = 9;  // neck, shoulders, elbows, hips, knees
    ragdoll->motorCount = 0;  // No motors initially, can be added for neural network control

    // Allocate arrays for bodies, geoms, joints, and motors
    ragdoll->bodies = RL_MALLOC(ragdoll->bodyCount * sizeof(dBodyID));
    ragdoll->geoms = RL_MALLOC(ragdoll->bodyCount * sizeof(dGeomID));
    ragdoll->joints = RL_MALLOC(ragdoll->jointCount * sizeof(dJointID));
    ragdoll->motors = RL_MALLOC(ragdoll->jointCount * sizeof(dJointID));  // Potential motors

    dMass m;

    // Body dimensions
    // TODO should be header defines
    float headRadius = 0.25f;
    float torsoWidth = 0.4f;
    float torsoHeight = 0.6f;
    float torsoDepth = 0.25f;
    float armLength = 0.35f;
    float armRadius = 0.1f;
    float legLength = 0.45f;
    float legRadius = 0.12f;

    // Ragdoll specific textures (consistent across all ragdolls)
    Texture* headTex = &ctx->sphereTextures[1];        // beach-ball.png
    Texture* torsoTex = &ctx->boxTextures[0];         // crate.png
    Texture* limbTex = &ctx->cylinderTextures[1];     // cylinder2.png

    // Create head
    dMassSetSphere(&m, 1, headRadius);
    dMassAdjust(&m, 5.0f);
    ragdoll->bodies[RAGDOLL_HEAD] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_HEAD], &m);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_HEAD],
                     position.x, position.y + 1.6f, position.z);
    ragdoll->geoms[RAGDOLL_HEAD] = dCreateSphere(space, headRadius);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_HEAD], ragdoll->bodies[RAGDOLL_HEAD]);
    dGeomSetData(ragdoll->geoms[RAGDOLL_HEAD], CreateGeomInfo(true, headTex, 1.0f, 1.0f));

    // Create torso
    dMassSetBox(&m, 1, torsoWidth, torsoHeight, torsoDepth);
    dMassAdjust(&m, 30.0f);
    ragdoll->bodies[RAGDOLL_TORSO] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_TORSO], &m);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_TORSO],
                     position.x, position.y + 0.9f, position.z);
    ragdoll->geoms[RAGDOLL_TORSO] = dCreateBox(space, torsoWidth, torsoHeight, torsoDepth);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_TORSO], ragdoll->bodies[RAGDOLL_TORSO]);
    dGeomSetData(ragdoll->geoms[RAGDOLL_TORSO], CreateGeomInfo(true, torsoTex, 1.0f, 1.0f));

    // Create arms - initialize mass for each individually
    // ODE cylinders are along Z-axis by default
    // Rotate geoms 90° around Y to orient along X-axis
    dMatrix3 R_arm;
    dRFromAxisAndAngle(R_arm, 0, 1, 0, M_PI * 0.5);

    // Left upper arm
    dMassSetCylinder(&m, 1, 3, armRadius, armLength);
    dMassAdjust(&m, 3.0f);
    ragdoll->bodies[RAGDOLL_LEFT_UPPER_ARM] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_LEFT_UPPER_ARM], &m);
    ragdoll->geoms[RAGDOLL_LEFT_UPPER_ARM] = dCreateCylinder(space, armRadius, armLength);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_LEFT_UPPER_ARM], ragdoll->bodies[RAGDOLL_LEFT_UPPER_ARM]);
    dGeomSetOffsetWorldRotation(ragdoll->geoms[RAGDOLL_LEFT_UPPER_ARM], R_arm);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_LEFT_UPPER_ARM],
                     position.x - 0.35f, position.y + 1.1f, position.z);
    dGeomSetData(ragdoll->geoms[RAGDOLL_LEFT_UPPER_ARM], CreateGeomInfo(true, limbTex, 1.0f, 1.0f));

    // Left lower arm
    dMassSetCylinder(&m, 1, 3, armRadius, armLength);
    dMassAdjust(&m, 3.0f);
    ragdoll->bodies[RAGDOLL_LEFT_LOWER_ARM] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_LEFT_LOWER_ARM], &m);
    ragdoll->geoms[RAGDOLL_LEFT_LOWER_ARM] = dCreateCylinder(space, armRadius, armLength);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_LEFT_LOWER_ARM], ragdoll->bodies[RAGDOLL_LEFT_LOWER_ARM]);
    dGeomSetOffsetWorldRotation(ragdoll->geoms[RAGDOLL_LEFT_LOWER_ARM], R_arm);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_LEFT_LOWER_ARM],
                     position.x - 0.35f - armLength, position.y + 1.1f, position.z);
    dGeomSetData(ragdoll->geoms[RAGDOLL_LEFT_LOWER_ARM], CreateGeomInfo(true, limbTex, 1.0f, 1.0f));

    // Right upper arm
    dMassSetCylinder(&m, 1, 3, armRadius, armLength);
    dMassAdjust(&m, 3.0f);
    ragdoll->bodies[RAGDOLL_RIGHT_UPPER_ARM] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_RIGHT_UPPER_ARM], &m);
    ragdoll->geoms[RAGDOLL_RIGHT_UPPER_ARM] = dCreateCylinder(space, armRadius, armLength);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_RIGHT_UPPER_ARM], ragdoll->bodies[RAGDOLL_RIGHT_UPPER_ARM]);
    dGeomSetOffsetWorldRotation(ragdoll->geoms[RAGDOLL_RIGHT_UPPER_ARM], R_arm);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_RIGHT_UPPER_ARM],
                     position.x + 0.35f, position.y + 1.1f, position.z);
    dGeomSetData(ragdoll->geoms[RAGDOLL_RIGHT_UPPER_ARM], CreateGeomInfo(true, limbTex, 1.0f, 1.0f));

    // Right lower arm
    dMassSetCylinder(&m, 1, 3, armRadius, armLength);
    dMassAdjust(&m, 3.0f);
    ragdoll->bodies[RAGDOLL_RIGHT_LOWER_ARM] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_RIGHT_LOWER_ARM], &m);
    ragdoll->geoms[RAGDOLL_RIGHT_LOWER_ARM] = dCreateCylinder(space, armRadius, armLength);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_RIGHT_LOWER_ARM], ragdoll->bodies[RAGDOLL_RIGHT_LOWER_ARM]);
    dGeomSetOffsetWorldRotation(ragdoll->geoms[RAGDOLL_RIGHT_LOWER_ARM], R_arm);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_RIGHT_LOWER_ARM],
                     position.x + 0.35f + armLength, position.y + 1.1f, position.z);
    dGeomSetData(ragdoll->geoms[RAGDOLL_RIGHT_LOWER_ARM], CreateGeomInfo(true, limbTex, 1.0f, 1.0f));

    // Create legs - initialize mass for each individually
    // ODE cylinders are along Z-axis by default
    // Rotate geoms 90° around X to orient along Y-axis
    dMatrix3 R_leg;
    dRFromAxisAndAngle(R_leg, 1, 0, 0, M_PI * 0.5);

    // Left upper leg
    dMassSetCylinder(&m, 1, 3, legRadius, legLength);
    dMassAdjust(&m, 8.0f);
    ragdoll->bodies[RAGDOLL_LEFT_UPPER_LEG] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_LEFT_UPPER_LEG], &m);
    ragdoll->geoms[RAGDOLL_LEFT_UPPER_LEG] = dCreateCylinder(space, legRadius, legLength);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_LEFT_UPPER_LEG], ragdoll->bodies[RAGDOLL_LEFT_UPPER_LEG]);
    dGeomSetOffsetWorldRotation(ragdoll->geoms[RAGDOLL_LEFT_UPPER_LEG], R_leg);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_LEFT_UPPER_LEG],
                     position.x - 0.15f, position.y + 0.45f, position.z);
    dGeomSetData(ragdoll->geoms[RAGDOLL_LEFT_UPPER_LEG], CreateGeomInfo(true, limbTex, 1.0f, 1.0f));

    // Left lower leg
    dMassSetCylinder(&m, 1, 3, legRadius, legLength);
    dMassAdjust(&m, 8.0f);
    ragdoll->bodies[RAGDOLL_LEFT_LOWER_LEG] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_LEFT_LOWER_LEG], &m);
    ragdoll->geoms[RAGDOLL_LEFT_LOWER_LEG] = dCreateCylinder(space, legRadius, legLength);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_LEFT_LOWER_LEG], ragdoll->bodies[RAGDOLL_LEFT_LOWER_LEG]);
    dGeomSetOffsetWorldRotation(ragdoll->geoms[RAGDOLL_LEFT_LOWER_LEG], R_leg);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_LEFT_LOWER_LEG],
                     position.x - 0.15f, position.y, position.z);
    dGeomSetData(ragdoll->geoms[RAGDOLL_LEFT_LOWER_LEG], CreateGeomInfo(true, limbTex, 1.0f, 1.0f));

    // Right upper leg
    dMassSetCylinder(&m, 1, 3, legRadius, legLength);
    dMassAdjust(&m, 8.0f);
    ragdoll->bodies[RAGDOLL_RIGHT_UPPER_LEG] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_RIGHT_UPPER_LEG], &m);
    ragdoll->geoms[RAGDOLL_RIGHT_UPPER_LEG] = dCreateCylinder(space, legRadius, legLength);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_RIGHT_UPPER_LEG], ragdoll->bodies[RAGDOLL_RIGHT_UPPER_LEG]);
    dGeomSetOffsetWorldRotation(ragdoll->geoms[RAGDOLL_RIGHT_UPPER_LEG], R_leg);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_RIGHT_UPPER_LEG],
                     position.x + 0.15f, position.y + 0.45f, position.z);
    dGeomSetData(ragdoll->geoms[RAGDOLL_RIGHT_UPPER_LEG], CreateGeomInfo(true, limbTex, 1.0f, 1.0f));

    // Right lower leg
    dMassSetCylinder(&m, 1, 3, legRadius, legLength);
    dMassAdjust(&m, 8.0f);
    ragdoll->bodies[RAGDOLL_RIGHT_LOWER_LEG] = dBodyCreate(world);
    dBodySetMass(ragdoll->bodies[RAGDOLL_RIGHT_LOWER_LEG], &m);
    ragdoll->geoms[RAGDOLL_RIGHT_LOWER_LEG] = dCreateCylinder(space, legRadius, legLength);
    dGeomSetBody(ragdoll->geoms[RAGDOLL_RIGHT_LOWER_LEG], ragdoll->bodies[RAGDOLL_RIGHT_LOWER_LEG]);
    dGeomSetOffsetWorldRotation(ragdoll->geoms[RAGDOLL_RIGHT_LOWER_LEG], R_leg);
    dBodySetPosition(ragdoll->bodies[RAGDOLL_RIGHT_LOWER_LEG],
                     position.x + 0.15f, position.y, position.z);
    dGeomSetData(ragdoll->geoms[RAGDOLL_RIGHT_LOWER_LEG], CreateGeomInfo(true, limbTex, 1.0f, 1.0f));

    // Create joints connecting body parts

    // Neck joint (Hinge between head and torso)
    ragdoll->joints[0] = dJointCreateHinge(world, 0);
    dJointAttach(ragdoll->joints[0], ragdoll->bodies[RAGDOLL_HEAD], ragdoll->bodies[RAGDOLL_TORSO]);
    dJointSetHingeAnchor(ragdoll->joints[0], position.x, position.y + 1.35f, position.z);
    dJointSetHingeAxis(ragdoll->joints[0], 1, 0, 0);
    dJointSetHingeParam(ragdoll->joints[0], dParamLoStop, -0.5f);
    dJointSetHingeParam(ragdoll->joints[0], dParamHiStop, 0.5f);

    // Left shoulder joint (Universal)
    ragdoll->joints[1] = dJointCreateUniversal(world, 0);
    dJointAttach(ragdoll->joints[1], ragdoll->bodies[RAGDOLL_TORSO], ragdoll->bodies[RAGDOLL_LEFT_UPPER_ARM]);
    dJointSetUniversalAnchor(ragdoll->joints[1], position.x - 0.3f, position.y + 1.2f, position.z);
    dJointSetUniversalAxis1(ragdoll->joints[1], 0, 0, 1);
    dJointSetUniversalAxis2(ragdoll->joints[1], 1, 0, 0);
    // Axis 1 (Z): limit forward/backward swing
    dJointSetUniversalParam(ragdoll->joints[1], dParamLoStop, -2.0f);
    dJointSetUniversalParam(ragdoll->joints[1], dParamHiStop, 1.5f);
    // Axis 2 (X): limit side-to-side movement
    dJointSetUniversalParam(ragdoll->joints[1], dParamLoStop2, -1.5f);
    dJointSetUniversalParam(ragdoll->joints[1], dParamHiStop2, 1.5f);

    // Left elbow joint (Hinge) - only bends backward
    ragdoll->joints[2] = dJointCreateHinge(world, 0);
    dJointAttach(ragdoll->joints[2], ragdoll->bodies[RAGDOLL_LEFT_UPPER_ARM], ragdoll->bodies[RAGDOLL_LEFT_LOWER_ARM]);
    const dReal* leftElbowPos = dBodyGetPosition(ragdoll->bodies[RAGDOLL_LEFT_LOWER_ARM]);
    dJointSetHingeAnchor(ragdoll->joints[2], leftElbowPos[0] + armLength/2, leftElbowPos[1], leftElbowPos[2]);
    dJointSetHingeAxis(ragdoll->joints[2], 0, 0, 1);
    dJointSetHingeParam(ragdoll->joints[2], dParamLoStop, 0.0f);      // Can't bend forward
    dJointSetHingeParam(ragdoll->joints[2], dParamHiStop, 2.5f);      // Max bend ~143 degrees

    // Right shoulder joint (Universal)
    ragdoll->joints[3] = dJointCreateUniversal(world, 0);
    dJointAttach(ragdoll->joints[3], ragdoll->bodies[RAGDOLL_TORSO], ragdoll->bodies[RAGDOLL_RIGHT_UPPER_ARM]);
    dJointSetUniversalAnchor(ragdoll->joints[3], position.x + 0.3f, position.y + 1.2f, position.z);
    dJointSetUniversalAxis1(ragdoll->joints[3], 0, 0, 1);
    dJointSetUniversalAxis2(ragdoll->joints[3], 1, 0, 0);
    // Axis 1 (Z): limit forward/backward swing
    dJointSetUniversalParam(ragdoll->joints[3], dParamLoStop, -2.0f);
    dJointSetUniversalParam(ragdoll->joints[3], dParamHiStop, 1.5f);
    // Axis 2 (X): limit side-to-side movement
    dJointSetUniversalParam(ragdoll->joints[3], dParamLoStop2, -1.5f);
    dJointSetUniversalParam(ragdoll->joints[3], dParamHiStop2, 1.5f);

    // Right elbow joint (Hinge) - only bends backward
    ragdoll->joints[4] = dJointCreateHinge(world, 0);
    dJointAttach(ragdoll->joints[4], ragdoll->bodies[RAGDOLL_RIGHT_UPPER_ARM], ragdoll->bodies[RAGDOLL_RIGHT_LOWER_ARM]);
    const dReal* rightElbowPos = dBodyGetPosition(ragdoll->bodies[RAGDOLL_RIGHT_LOWER_ARM]);
    dJointSetHingeAnchor(ragdoll->joints[4], rightElbowPos[0] - armLength/2, rightElbowPos[1], rightElbowPos[2]);
    dJointSetHingeAxis(ragdoll->joints[4], 0, 0, 1);
    dJointSetHingeParam(ragdoll->joints[4], dParamLoStop, 0.0f);      // Can't bend forward
    dJointSetHingeParam(ragdoll->joints[4], dParamHiStop, 2.5f);      // Max bend ~143 degrees

    // Left hip joint (Universal)
    ragdoll->joints[5] = dJointCreateUniversal(world, 0);
    dJointAttach(ragdoll->joints[5], ragdoll->bodies[RAGDOLL_TORSO], ragdoll->bodies[RAGDOLL_LEFT_UPPER_LEG]);
    dJointSetUniversalAnchor(ragdoll->joints[5], position.x - 0.15f, position.y + 0.6f, position.z);
    dJointSetUniversalAxis1(ragdoll->joints[5], 1, 0, 0);
    dJointSetUniversalAxis2(ragdoll->joints[5], 0, 0, 1);
    // Axis 1 (X): limit leg swing forward/back
    dJointSetUniversalParam(ragdoll->joints[5], dParamLoStop, -1.5f);
    dJointSetUniversalParam(ragdoll->joints[5], dParamHiStop, 2.0f);
    // Axis 2 (Z): limit leg rotation
    dJointSetUniversalParam(ragdoll->joints[5], dParamLoStop2, -1.0f);
    dJointSetUniversalParam(ragdoll->joints[5], dParamHiStop2, 1.0f);

    // Left knee joint (Hinge) - only bends backward
    ragdoll->joints[6] = dJointCreateHinge(world, 0);
    dJointAttach(ragdoll->joints[6], ragdoll->bodies[RAGDOLL_LEFT_UPPER_LEG], ragdoll->bodies[RAGDOLL_LEFT_LOWER_LEG]);
    const dReal* leftKneePos = dBodyGetPosition(ragdoll->bodies[RAGDOLL_LEFT_LOWER_LEG]);
    dJointSetHingeAnchor(ragdoll->joints[6], leftKneePos[0], leftKneePos[1] + legLength/2, leftKneePos[2]);
    dJointSetHingeAxis(ragdoll->joints[6], 1, 0, 0);
    dJointSetHingeParam(ragdoll->joints[6], dParamLoStop, 0.0f);       // Can't bend forward
    dJointSetHingeParam(ragdoll->joints[6], dParamHiStop, 2.5f);       // Max bend ~143 degrees

    // Right hip joint (Universal)
    ragdoll->joints[7] = dJointCreateUniversal(world, 0);
    dJointAttach(ragdoll->joints[7], ragdoll->bodies[RAGDOLL_TORSO], ragdoll->bodies[RAGDOLL_RIGHT_UPPER_LEG]);
    dJointSetUniversalAnchor(ragdoll->joints[7], position.x + 0.15f, position.y + 0.6f, position.z);
    dJointSetUniversalAxis1(ragdoll->joints[7], 1, 0, 0);
    dJointSetUniversalAxis2(ragdoll->joints[7], 0, 0, 1);
    // Axis 1 (X): limit leg swing forward/back
    dJointSetUniversalParam(ragdoll->joints[7], dParamLoStop, -1.5f);
    dJointSetUniversalParam(ragdoll->joints[7], dParamHiStop, 2.0f);
    // Axis 2 (Z): limit leg rotation
    dJointSetUniversalParam(ragdoll->joints[7], dParamLoStop2, -1.0f);
    dJointSetUniversalParam(ragdoll->joints[7], dParamHiStop2, 1.0f);

    // Right knee joint (Hinge) - only bends backward
    ragdoll->joints[8] = dJointCreateHinge(world, 0);
    dJointAttach(ragdoll->joints[8], ragdoll->bodies[RAGDOLL_RIGHT_UPPER_LEG], ragdoll->bodies[RAGDOLL_RIGHT_LOWER_LEG]);
    const dReal* rightKneePos = dBodyGetPosition(ragdoll->bodies[RAGDOLL_RIGHT_LOWER_LEG]);
    dJointSetHingeAnchor(ragdoll->joints[8], rightKneePos[0], rightKneePos[1] + legLength/2, rightKneePos[2]);
    dJointSetHingeAxis(ragdoll->joints[8], 1, 0, 0);
    dJointSetHingeParam(ragdoll->joints[8], dParamLoStop, 0.0f);        // Can't bend forward
    dJointSetHingeParam(ragdoll->joints[8], dParamHiStop, 2.5f);        // Max bend ~143 degrees

    return ragdoll;
}

// Update rag doll motors with neural network control values
// motorForces array should have one value per joint for control
void UpdateRagdollMotors(RagDoll *ragdoll, float *motorForces)
{
    if (!ragdoll || !motorForces) return;

    for (int i = 0; i < ragdoll->jointCount; i++) {
        dJointID joint = ragdoll->joints[i];
        int jointType = dJointGetType(joint);

        // For hinge joints, set velocity and max force for motor control
        if (jointType == dJointTypeHinge) {
            dJointSetHingeParam(joint, dParamVel, motorForces[i]);
            dJointSetHingeParam(joint, dParamFMax, fabsf(motorForces[i]) > 0.001f ? 50.0f : 0.0f);
        }
        // For universal joints, control both axes
        else if (jointType == dJointTypeUniversal) {
            // motorForces[i] controls axis 1, motorForces[i + jointCount] would control axis 2
            dJointSetUniversalParam(joint, dParamVel, motorForces[i]);
            dJointSetUniversalParam(joint, dParamVel2, motorForces[i + ragdoll->jointCount]);
            dJointSetUniversalParam(joint, dParamFMax, fabsf(motorForces[i]) > 0.001f ? 50.0f : 0.0f);
            dJointSetUniversalParam(joint, dParamFMax2, fabsf(motorForces[i + ragdoll->jointCount]) > 0.001f ? 50.0f : 0.0f);
        }
    }
}

// Draw rag doll using the existing drawGeom system
void DrawRagdoll(RagDoll *ragdoll, struct GraphicsContext* ctx)
{
    if (!ragdoll) return;

    for (int i = 0; i < ragdoll->bodyCount; i++) {
        if (ragdoll->geoms[i]) {
            drawGeom(ragdoll->geoms[i], ctx);
        }
    }
}

// Free rag doll resources - properly destroys ODE objects before freeing wrapper arrays
void FreeRagdoll(RagDoll *ragdoll, PhysicsContext *ctx)
{
    if (!ragdoll) return;

    // Destroy ODE bodies and their geoms
    if (ragdoll->bodies) {
        for (int i = 0; i < ragdoll->bodyCount; i++) {
            if (ragdoll->bodies[i]) {
                // Remove geom from space before destroying body
                if (ragdoll->geoms && ragdoll->geoms[i] && ctx->space) {
                    dSpaceRemove(*ctx->space, ragdoll->geoms[i]);
                }
                dBodyDestroy(ragdoll->bodies[i]);
            }
        }
    }

    // Destroy ODE geoms (indexed by bodyCount)
    if (ragdoll->geoms) {
        for (int i = 0; i < ragdoll->bodyCount; i++) {
            if (ragdoll->geoms[i]) {
                dGeomDestroy(ragdoll->geoms[i]);
            }
        }
    }

    // Destroy ODE joints (indexed by jointCount)
    if (ragdoll->joints) {
        for (int i = 0; i < ragdoll->jointCount; i++) {
            if (ragdoll->joints[i]) {
                dJointDestroy(ragdoll->joints[i]);
            }
        }
    }

    // Destroy ODE motors (indexed by jointCount)
    if (ragdoll->motors) {
        for (int i = 0; i < ragdoll->motorCount; i++) {
            if (ragdoll->motors[i]) {
                dJointDestroy(ragdoll->motors[i]);
            }
        }
    }

    // Free wrapper arrays
    if (ragdoll->bodies) RL_FREE(ragdoll->bodies);
    if (ragdoll->geoms) RL_FREE(ragdoll->geoms);
    if (ragdoll->joints) RL_FREE(ragdoll->joints);
    if (ragdoll->motors) RL_FREE(ragdoll->motors);

    RL_FREE(ragdoll);
}
