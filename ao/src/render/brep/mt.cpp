#include "ao/render/brep/mt.hpp"

namespace Kernel {

static const uint8_t VERTEX_LOOP[] = {6, 4, 5, 1, 3, 2, 6};

// Based on which vertices are filled, this map tells you which
// edges to interpolate between when forming zero, one, or two
// triangles.
// (filled vertex is first in the pair)
static const int EDGE_MAP[16][2][3][2] = {
    {{{-1,-1}, {-1,-1}, {-1,-1}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // ----
    {{{ 0, 2}, { 0, 1}, { 0, 3}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // ---0
    {{{ 1, 0}, { 1, 2}, { 1, 3}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // --1-
    {{{ 1, 2}, { 1, 3}, { 0, 3}}, {{ 0, 3}, { 0, 2}, { 1, 2}}}, // --10
    {{{ 2, 0}, { 2, 3}, { 2, 1}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // -2--
    {{{ 0, 3}, { 2, 3}, { 2, 1}}, {{ 2, 1}, { 0, 1}, { 0, 3}}}, // -2-0
    {{{ 1, 0}, { 2, 0}, { 2, 3}}, {{ 2, 3}, { 1, 3}, { 1, 0}}}, // -21-
    {{{ 2, 3}, { 1, 3}, { 0, 3}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // -210

    {{{ 3, 0}, { 3, 1}, { 3, 2}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // 3---
    {{{ 3, 2}, { 0, 2}, { 0, 1}}, {{ 0, 1}, { 3, 1}, { 3, 2}}}, // 3--0
    {{{ 1, 2}, { 3, 2}, { 3, 0}}, {{ 3, 0}, { 1, 0}, { 1, 2}}}, // 3-1-
    {{{ 1, 2}, { 3, 2}, { 0, 2}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // 3-10
    {{{ 3, 0}, { 3, 1}, { 2, 1}}, {{ 2, 1}, { 2, 0}, { 3, 0}}}, // 32--
    {{{ 3, 1}, { 2, 1}, { 0, 1}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // 32-0
    {{{ 3, 0}, { 1, 0}, { 2, 0}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // 321-
    {{{-1,-1}, {-1,-1}, {-1,-1}}, {{-1,-1}, {-1,-1}, {-1,-1}}}, // 3210
};

void TetMarcher::operator()(const std::array<XTree<3>*, 8>& ts)
{
    for (unsigned i=0; i < 8; ++i)
    {
        eval->set(ts[i]->vert3(), i);
    }
    auto vs = eval->values(8);

    // Loop over the six tetrahedra that make up a voxel cell
    for (int t = 0; t < 6; ++t)
    {
        // Find vertex positions for this tetrahedron
        const uint8_t vertices[] = {0, 7, VERTEX_LOOP[t], VERTEX_LOOP[t+1]};

        // Build up the bitmask for this tetrahedron
        uint8_t mask = 0;
        for (unsigned i=0; i < 3; ++i)
        {
            mask |= (vs[vertices[i]] < 0) << i;
        }

        // Iterate over up to two triangles in the tet
        for (unsigned t=0; t < 2 && EDGE_MAP[mask][t][0][0] != -1; ++t)
        {
            uint32_t tri[3];
            for (int v=0; v < 3; ++v)
            {
                /*  Construct an order-agnostic dual edge key  */
                auto a = ts[vertices[EDGE_MAP[mask][t][v][0]]];
                auto b = ts[vertices[EDGE_MAP[mask][t][v][1]]];
                Key _k = (a < b) ? Key(a, b) : Key(b, a);

                auto k = indices.find(_k);
                if (k == indices.end())
                {
                    auto pt = interp.between(_k.first->vert, _k.second->vert);
                    k = indices.insert({_k, verts.size()}).first;
                    verts.push_back(pt);
                }
                tri[v] = k->second;
            }
            tris.push_back({tri[0], tri[1], tri[2]});
        }
    }
}

}   // namespace Kernel