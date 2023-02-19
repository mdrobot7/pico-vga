drawPolygon() and fillPolygon() are the key to everything.

the way all 2d and 3d things are going to be handled are as "point clouds" -- just lists of points

a point:
- x, y, z, other coordinates
- 2 links to other points (to make the sides of a triangle)
- basically, make a tree

all shapes (point, line, rectangle, triangle, sprite, etc) are going to be handled as lists of points with a given fill. 3d is basically handled the same way

circles (in 2d) are going to be handled as their own thing, since listing all of the points of a circle in RAM would be a waste and it's just better to compute it on the fly.

in 3d, the points will all make vertices of triangles, each triangle will have a texture/pattern projected onto it, and then it will be rendered

for repetitive calculations, it's probably good to use the hardware "spinlocks" (or whatever they're called) in the rp2040 to accelerate repetitive calculations. these have to be programmed through registers, but they will do sets of calculations a lot faster than the cpu