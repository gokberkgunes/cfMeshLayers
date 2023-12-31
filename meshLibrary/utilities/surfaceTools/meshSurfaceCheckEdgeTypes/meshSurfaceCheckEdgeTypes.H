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
    Foam::Module::meshSurfaceCheckEdgeTypes

Description
    Checks if any of the surface is tangled at any of its vertices. This
    is performed by checking the dot product between the vertex normal and
    the normals of the faces attached to the vertex.

SourceFiles
    meshSurfaceCheckEdgeTypes.C

\*---------------------------------------------------------------------------*/

#ifndef meshSurfaceCheckEdgeTypes_H
#define meshSurfaceCheckEdgeTypes_H

#include "polyMeshGenModifier.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace Module
{

// Forward declarations
class meshSurfaceEngine;

/*---------------------------------------------------------------------------*\
                  Class meshSurfaceCheckEdgeTypes Declaration
\*---------------------------------------------------------------------------*/

class meshSurfaceCheckEdgeTypes
{
    // Private data

        //- mesh surface
        const meshSurfaceEngine& surfaceEngine_;

        //- a list which classifies each edge
        List<direction> edgeType_;


    // Private member functions

        //- check feature edges and classify them
        void classifyEdges();

        //- Disallow default bitwise copy construct
        meshSurfaceCheckEdgeTypes
        (
            const meshSurfaceCheckEdgeTypes&
        );

        //- Disallow default bitwise assignment
        void operator=(const meshSurfaceCheckEdgeTypes&);


public:

    // Enumerators

        enum edgeTypes_
        {
            NONE = 0,
            PATCHEDGE = 1,
            FEATUREEDGE = 2,
            CONVEXEDGE = 4,
            CONCAVEEDGE = 8,
            UNDETERMINED = 16
        };


    //- Construct from meshSurfaceEngine
    meshSurfaceCheckEdgeTypes(const meshSurfaceEngine&);

    //- Destructor
    ~meshSurfaceCheckEdgeTypes() = default;


    // Member Functions

        //- return the information about classified edges
        const List<direction>& edgeTypes() const
        {
            return edgeType_;
        }

        //- return indices of convex edges
        void convexEdges(labelLongList& convexEdges) const;

        //- return indices of concave edges
        void concaveEdges(labelLongList& concaveEdges) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Module
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
