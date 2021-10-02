# RayCollision - The Wolf3D Raycasting thingy

## What does it do?

I made this project to check if I could implement the raycasting algorithm used in Wolfenstein 3D.
However, this project doesn't use it to render 2.5D worlds. It is used to check for collisions.
In games you want to predict where the player (or any moving entity) will be in the next frame so
you can accurately position the player. This means we want to cast a ray from the players current
position towards the position the player will be in the next frame. If this ray hits a wall
we know the player must not go further than the wall's position.

## Great, but you said it is the Wolf3D thingy. It is not 3D!

Yes, my intentions were to use it for collision detection (as stated above). However,
the very same algorithm can be used to render Wolf3D-like worlds :)

## Immediate Mode OpenGL

What?? Why? This project started out as an experiment if I could put my old pentium III box
to good use. I installed Visual C++ 6.0 on it and started hacking on Win98. On this machine
I only had the standard GL driver on it that ships with every Windows version (it still
ships with Win10). It worked surprisingly well and the cool thing about immediat mode is
you get results pretty quickly so it is still quite good for 2D prototyping.



