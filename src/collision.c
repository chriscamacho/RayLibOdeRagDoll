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

#include "collision.h"
#include "raylibODE.h"
#include "init.h"

#define MAX_CONTACTS 8

void nearCallback(void *data, dGeomID o1, dGeomID o2)
{
    struct PhysicsContext* ctx = (struct PhysicsContext*)data;
    int i;

    // exit without doing anything if the two bodies are connected by a joint
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    //if (b1==b2) return;
    if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact))
        return;
        
    geomInfo* gi1 = (geomInfo*)dGeomGetData(o1);
    if (gi1 && !gi1->collidable) return;
    geomInfo* gi2 = (geomInfo*)dGeomGetData(o2);
    if (gi2 && !gi2->collidable) return;

    // getting these just so can sometimes be a little bit of a black art!
    dContact contact[MAX_CONTACTS]; // up to MAX_CONTACTS contacts per body-body
    for (i = 0; i < MAX_CONTACTS; i++) {
        contact[i].surface.mode = dContactSlip1 | dContactSlip2 |
                                    dContactSoftERP | dContactSoftCFM | dContactApprox1;
        contact[i].surface.mu = 1000;
        contact[i].surface.slip1 = 0.0001;
        contact[i].surface.slip2 = 0.0001;
        contact[i].surface.soft_erp = 0.1;
        contact[i].surface.soft_cfm = 0.001;
      
        contact[i].surface.bounce = 0.001;
        contact[i].surface.bounce_vel = 0.001;

    }
    int numc = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom,
                        sizeof(dContact));
    if (numc) {
        dMatrix3 RI;
        dRSetIdentity(RI);
        for (i = 0; i < numc; i++) {
            dJointID c =
                dJointCreateContact(ctx->world, ctx->contactgroup, contact + i);
            dJointAttach(c, b1, b2);
        }
    }
}
