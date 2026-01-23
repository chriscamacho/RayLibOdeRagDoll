/*
 * Copyright (c) 2021 Chris Camacho (codifies -  http://bedroomcoders.co.uk/)
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
#include "rlgl.h"

#include <ode/ode.h>
#include "raylibODE.h"
#include "raylibODEvehicle.h"
#include "init.h"

// Random float in range [min, max]
float rndf(float min, float max)
{
    return ((float)rand() / (float)(RAND_MAX)) * (max - min) + min;
}

// optionally a geom can have user data, in this case
// the only info our user data has is if the geom
// should collide or not
// should be pointer to struct which includes render texture and UV scale etc

// position rotation scale all done with the models transform...
// TODO check there isn't a new raylib function that does this now _pro _ex or similar!
void MyDrawModel(Model model, Color tint)
{
    
    for (int i = 0; i < model.meshCount; i++)
    {
        Color color = model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;

        Color colorTint = WHITE;
        colorTint.r = (unsigned char)((((float)color.r/255.0)*((float)tint.r/255.0))*255.0f);
        colorTint.g = (unsigned char)((((float)color.g/255.0)*((float)tint.g/255.0))*255.0f);
        colorTint.b = (unsigned char)((((float)color.b/255.0)*((float)tint.b/255.0))*255.0f);
        colorTint.a = (unsigned char)((((float)color.a/255.0)*((float)tint.a/255.0))*255.0f);

        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = colorTint;
        DrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]], model.transform);
        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = color;
    }
}


// these two just convert to column major and minor
void rayToOdeMat(Matrix* m, dReal* R) {
    R[ 0] = m->m0;   R[ 1] = m->m4;   R[ 2] = m->m8;    R[ 3] = 0;
    R[ 4] = m->m1;   R[ 5] = m->m5;   R[ 6] = m->m9;    R[ 7] = 0;
    R[ 8] = m->m2;   R[ 9] = m->m6;   R[10] = m->m10;   R[11] = 0;
    R[12] = 0;       R[13] = 0;       R[14] = 0;        R[15] = 1;   
}

// sets a raylib matrix from an ODE rotation matrix
void odeToRayMat(const dReal* R, Matrix* m)
{
    m->m0 = R[0];  m->m1 = R[4];  m->m2 = R[8];      m->m3 = 0;
    m->m4 = R[1];  m->m5 = R[5];  m->m6 = R[9];      m->m7 = 0;
    m->m8 = R[2];  m->m9 = R[6];  m->m10 = R[10];    m->m11 = 0;
    m->m12 = 0;    m->m13 = 0;    m->m14 = 0;        m->m15 = 1;
}

// called by draw all geoms
void drawGeom(dGeomID geom, struct GraphicsContext* ctx) {
    const dReal* pos = dGeomGetPosition(geom);
    const dReal* rot = dGeomGetRotation(geom);
    int class = dGeomGetClass(geom);
    Model* m = 0;
    dVector3 size;
    if (class == dBoxClass) {
        m = &ctx->box;
        dGeomBoxGetLengths(geom, size);
    } else if (class == dSphereClass) {
        m = &ctx->ball;
        float r = dGeomSphereGetRadius(geom);
        size[0] = size[1] = size[2] = (r*2);
    } else if (class == dCylinderClass) {
        m = &ctx->cylinder;
        dReal l,r;
        dGeomCylinderGetParams (geom, &r, &l);
        size[0] = size[1] = r*2;
        size[2] = l;
    } else if (class == dCapsuleClass) {
        m = &ctx->cylinder;
        dReal l,r;
        dGeomCapsuleGetParams (geom, &r, &l);
        size[0] = size[1] = r*2;
        size[2] = l;
	}
    if (!m) return;

    Matrix matScale = MatrixScale(size[0], size[1], size[2]);
    Matrix matRot;
    odeToRayMat(rot, &matRot);
    Matrix matTran = MatrixTranslate(pos[0], pos[1], pos[2]);

    m->transform = MatrixMultiply(MatrixMultiply(matScale, matRot), matTran);

    // Apply per-instance texture if specified in geomInfo
    geomInfo* gi = (geomInfo*)dGeomGetData(geom);
    if (gi && gi->texture) {
        m->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *gi->texture;
        
        // Apply UV scale for texture tiling
        Vector2 uvScale = { gi->uvScaleU, gi->uvScaleV };
        int uvLoc = GetShaderLocation(ctx->shader, "texCoordScale");
        SetShaderValue(ctx->shader, uvLoc, &uvScale.x, SHADER_UNIFORM_VEC2);
    }
    
    //dBodyID b = dGeomGetBody(geom);
    //Color c = WHITE;
    //if (b) if (!dBodyIsEnabled(b)) c = RED;
    Color c = WHITE;

    MyDrawModel(*m, c);
    m->transform = MatrixIdentity();
}

// draw all the geoms in a space
void drawAllSpaceGeoms(dSpaceID space, struct GraphicsContext* ctx) {
    int ng = dSpaceGetNumGeoms(space);
    for (int i=0; i<ng; i++) {
        dGeomID geom = dSpaceGetGeom(space, i);
        geomInfo* gi = (geomInfo*)dGeomGetData(geom);
        if (!gi || gi->collidable)
        {
            drawGeom(geom, ctx);
        }
    }
}
