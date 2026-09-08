#pragma once

// The classic Utah teapot, tessellated at runtime from its 10 base Bezier
// patches (mirrored to the full 32-patch teapot) using the fixed-function
// OpenGL evaluator API (glMap2d / glEvalMesh2), just like the original
// glutSolidTeapot()/glutWireTeapot() implementation this data comes from.
namespace TeapotGeometry {

// Renders the teapot centered at the origin, upright, roughly 2 units tall.
// `grid` controls tessellation density per patch (higher = smoother).
void Draw(int grid);

}
