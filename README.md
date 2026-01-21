# RayLibOdeRagDoll
demonstrates using OpenDE and Raylib to create a simple ragdolls

get ODE from https://bitbucket.org/odedevs/ode/downloads/

extract ode 0.16.6 into a directory at the same level as this project (see below)

ln -s ode-0.16.6 ode

I'd suggest building it with this configuration
./configure --enable-ou --enable-libccd --with-box-cylinder=libccd --with-drawstuff=none --disable-demos --with-libccd=internal

and run make, you should then be set to compile this project

note I have moved the ode location (so I can use it with other projects!)

You file structure should look like this

ode

raylib

RayLibOdeRagDoll



please feel free to get in touch via bedroomcoders.co.uk

