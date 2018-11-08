# ACCELERATED GRAPHICS (Pi1, 2, 3 - AARCH32, Pi3 AARCH64)
>
This is the same as GLES except I am doing a simple rotation of shapes to look at frame rate. I was surprised the screen didn't tear as I didn't double buffer yet. The frame rate can go a lot higher as I am disposing of the structures and completely rebuilding them for each frame which is silly.

The rotation really is supposed to happen in the shader but I am still trying to work out the shader compiler so this is a haxx physically moving the vertex values.

Anyhow I will get a proper 3D model of a plane or something to get a better idea of framerate and keep all the vertex buffer data. I am expecting to be a lot faster.
