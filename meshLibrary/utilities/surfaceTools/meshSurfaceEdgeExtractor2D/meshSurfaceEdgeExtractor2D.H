/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | cfMesh: A library for mesh generation
   \\    /   O peration     |
    \\  /    A nd           | www.cfmesh.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2014-2017 Creative Fields, Ltd.
-------------------------------------------------------------------------------
Author
     Franjo Juretic (franjo.juretic@c-fields.com)

License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::Module::meshSurfaceEdgeExtractor2D

Description
    Stores boundary faces into patches and captures edges and corners

SourceFiles
    meshSurfaceEdgeExtractor2D.C

\*---------------------------------------------------------------------------*/

#ifndef meshSurfaceEdgeExtractor2D_H
#define meshSurfaceEdgeExtractor2D_H

#include "polyMeshGenModifier.H"
#include "boolList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace Module
{
// Forward declarations
class meshOctree;
class meshSurfaceEngine;

/*---------------------------------------------------------------------------*\
                 Class meshSurfaceEdgeExtractor2D Declaration
\*---------------------------------------------------------------------------*/

class meshSurfaceEdgeExtractor2D
{
    // Private data

        //- mesh
        polyMeshGen& mesh_;

        //- octree
        const meshOctree& meshOctree_;


    // Private member functions

        //- Disallow default bitwise copy construct
        meshSurfaceEdgeExtractor2D(const meshSurfaceEdgeExtractor2D&);

        //- Disallow default bitwise assignment
        void operator=(const meshSurfaceEdgeExtractor2D&);


public:

    //- Construct from mesh data
    meshSurfaceEdgeExtractor2D
    (
        polyMeshGen& mesh,
        const meshOctree& octree
    );

    //- Destructor
    ~meshSurfaceEdgeExtractor2D() = default;


    // Member Functions

        //- distribute boundary faces into patches
        void distributeBoundaryFaces();

        //- re-map points after edges have been extracted
        void remapBoundaryPoints();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Module
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
