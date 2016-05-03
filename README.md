# OpenGL
This the a half-semester project on COMS 4160 Computer Graphics, and is divided into four parts:



Part1: simple VBO & file load & openGL turntable

In this assignment you are going to load in an OBJ file, display it on-screen, and allow the user to rotate it by moving the mouse.

Your program should be called "glrender" and it should take one argument: the name of an OBJ file (of triangles only) for the geometry it is to load in and display.

Almost all of this assignment can be done using the code you have already written (in the case of reading an OBJ file) or that you can repurpose from the example code posted on the wiki. In particular, see the example code for rotating a triangle.

The steps your code must take are the following:


read in an OBJ file (given as the first argument to the program - use the parsing code already done for your raytracer)
put the vertex data, per-triangle, into an array (as is done in the example code)
load that vertex data into a vertex buffer object (VBO), as is done in the example code.
use the mouse handling function to properly convert from mouse x values to the angles necessary for viewing, just like what is done in the example code for rotating triangle.
in the display function, apply a transformation to your vertex data that is rotated properly according to the mouse movement. this part is exactly the same as for the rotating triangle example.

Note that your geometry will be displayed in an orthographic projection - there is no need to include perspective at this point (we will add it in a later milestone).

Note that in this implementation, it is assumed that your OBJ model fits into a 2x2x2 cube (in fact, to look reasonable it should fit into a 1x1x1 cube centered at the origin). This will ensure that, at vertex shader time, the object properly fits into the 2x2x2 cube that defines the renderable space. I will verify that the currently-posted OBJ files fit this description - in the meantime you can just make a simple OBJ file of a few triangles that meets that specification.

You should base your code off the example for rotating a triangle: that example calculates colors in the CPU code, and has minimal vertex and fragment shaders - these are flat shaders which will allow you to see the detail in your model. You do not need to write shaders in this assignment.




Part 2: transformations and more opengl

This assignment was locked Apr 24 at 11:59pm.
For this assignment, you will extend your glrender program to include a simple camera control that will let you position and aim a perspective camera, and add in basic blinn/phong lighting at the vertices. All of these computations should be done in the vertex shader.

Your camera control should be a simple "crystal ball" interface: to the user, the camera appears to be pointed at a fixed origin and can rotate around it along latitude & longitude via the mouse. In this sense, it's as if the camera is located on a sphere, with an adjustable radius, always pointing in the direction opposite the sphere's normal.

For controls: moving the mouse to the left should make the view appear to circle around the origin to the left (an object in the center should appear to rotate to the right) - the opposite should happen when moving the mouse to the right. Pushing the mouse forward should make the camera view appear to move up in latitude - the opposite should happen when pulling the mouse backwards. The objects and the scene lights (a point light at [100 100 100], and a small ambient light) should appear to be fixed.

The "z" key should move the camera closer to the origin, and the "x" key should pull the camera away from the origin. At all times, any objects in the scene should appear upright - the camera does not twist to either side but stays oriented with it's left-right axis aligned with latitude, and its up-down axis aligned with longitude. The camera can move along to within 5 degrees of the zenith and nadir - this constraint will prevent your camera from getting into a position where it is oriented completely downward or completely upward. The camera can move in to as close as 2 units from the "origin" around which it rotates, and as far away as 50 units.

Your triangles (read from an OBJ file) should be sent once only to the graphics card - I suggest using glBufferSubData(). The other attributes (for example: rotation angles, lighting parameters, or a perspective/transformation matrix) should be updated in the display callback through glUniform().

notes:


It should be clear from running your program that a) the light(s) and geometry appear fixed, with the camera rotating around the center of the scene, and b) that you are computing phong shading at the vertices -i.e there will be positions where specular highlights appear.

You do NOT need to implement shared vertex meshes, but you DO need to implement a method to compute the normal at each vertex as the average of the normals on the triangles that surround that vertex. The averaged normals should be send down to the vertex shader, where they will be used to do the blinn/phong calculation (*before* any application of projective transformation)

The easiest way to compute these normals is as follows:
1. make an array of normals that contain the normals for each triangle: e.g. tri_norms[] (computed via crossproduct)
2. make an array of vectors, one for each unique vertex, each initialized to the zero vector, e.g. vert_norms[]
3. go through the array of triangle vertex ids:
if triangle i has vertex j, add tri_norms[i] to vert_nroms[j]
(you will be adding each triangle's normal to 3 different vertex normals)
4. when done, normalize all the vert_norms.




Part3: Bezier Surfaces
In this assignment, you will extend your current glrender program to allow for more complex geometry.


You will implement bezier surfaces, from which you will need to extract triangles, and render them in OpenGL. The level of sampling resolution should be controlled by the "<" and ">" keys, increasing and decreasing the level of detail. As a minimum, you should take at least as many samples in U and V as there are control points in that direction.

You will need to read a custom file format which describes the rectangular array of control points. As mentioned in class, bezier surface evaluation really just relies on bezier curve evaluation - this simplicity makes for decreased development time.

File Format for Bezier Surfaces

The file format is as follows: the first line states the number of surfaces in the file. After that, for each surface there is the following:

a line with two integers (“u_deg” and “v_deg”) specifying the degree of the curve/surface in U and V (minimum is "1" for each)
a series of lines specifying the control points for the Bezier curve or surface. There will be (v_deg+1) lines, each with (u_deg+1) columns, of XYZ values. In this arrangement, it is expected that U runs from left to right, and V from *bottom* to top, of the array as you see it in the file.

Example Input File

this file specifies a degree-1 x degree-1 (i.e bilinear) surface (a square in this case), followed by a degree-3 x degree-2 bezier surface:

_
2
1 1
0.0 2.0 0.0 0.0 2.0 2.0
0.0 0.0 0.0 0.0 0.0 2.0
3 2
0.0 2.0 0.0 0.0 1.8 1.0 0.0 1.6 2.0 0.0 1.3 3.0
0.0 1.0 0.0 0.0 1.0 1.0 0.0 1.0 2.0 0.0 1.0 3.0
1.0 0.0 0.0 1.0 0.0 1.0 1.0 0.0 2.0 1.0 0.0 3.0

Program Name & Arguments

Your program should still be called glrender : it should take 1 argument - the name of a scene file (which will contain 1 or more bezier surface in the above format.).



Part4: fragment shaders

For your final assignment, you will implement blinn/phong shading in a fragment shader.

You have already implemented blinn/phong in your vertex shader. To have this calculation performed in the fragment shader - towards the end of the pipeline - you just have to pass the values for lighting-dependent calculations (e.g. surface normal, vector to light, vector to eye) from the vertex shader where it is calculated, on to the fragment shader. By passing them as "varying", they will be interpolated and input to the fragment shader, at which point you can complete the lighting calculation at the fragment level. In addition, the uniforms that describe the light and material parameters should get picked up in the fragment shader, instead of the vertex shader (as was the case in the previous assignment)

Your program, called glrender, should be able to render both .obj files, and .txt files (if the latter, it is assumed that the file is a set of Bezier Patches.)


OPTIONAL: In addition to the blinn/phong shading, implement a 3d checkerboard shader, the two shaders switchable by the "s" key. The checkboard pattern is a simple black & white checkerboard: by default, each 1x1x1 cube of cartesian space is either white or black, alternating. Typing the "t" key makes the checkerboard finer, typing the "g" key makes it coarser. Any geometry in the scene should appear to be embedded in this material, in effect the geometry should look like it is carved out of a material consisting of black & white alternating cubes. Since this checkerboard is calculated in world space, it it likely that you will have to pass the world space position of each vertex on to the fragment shader (not the camera space position!)

You will need to switch between these two modes when the "s" key is hit - an easy way to do this is to pass a "uniform" state through the shaders - if it's 1.0, one shading method is used, if it's 0.0 the other shading method is used.
