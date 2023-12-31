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
    Foam::Module::checkNonMappableCellConnections

Description
    Checks cell connections at the surface. Cells with all vertices at
    the boundary are considered problematic because they get severly skewed
    after being mapped to the geometry surface in most cases. This code performs
    checks of cells with all vertices at the boundary with the rest of the mesh.
    Such cells having a neighbour with all internal faces are removed.

SourceFiles
    checkNonMappableCellConnections.C

\*---------------------------------------------------------------------------*/

#ifndef checkNonMappableCellConnections_H
#define checkNonMappableCellConnections_H

#include "polyMeshGenModifier.H"
#include "HashSet.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace Module
{

/*---------------------------------------------------------------------------*\
               Class checkNonMappableCellConnections Declaration
\*---------------------------------------------------------------------------*/

class checkNonMappableCellConnections
{
    // Private data

        //- Reference to polyMeshGen
        polyMeshGen& mesh_;

        //- type of cell
        labelList cellType_;


    // Enumerations

        enum cellTypes_
        {
            NONE = 0,
            INTERNALCELL = 1,
            BNDCELL = 2,
            ALLBNDVERTEXCELL = 4,
            INTERNALFACEGROUP = 8
        };


    // Private member functions

        //- classify cells
        void findCellTypes();

        //- Disallow default bitwise copy construct
        checkNonMappableCellConnections(const checkNonMappableCellConnections&);

        //- Disallow default bitwise assignment
        void operator=(const checkNonMappableCellConnections&);


public:

    //- Construct from polyMeshGen and meshOctree
    checkNonMappableCellConnections(polyMeshGen& mesh);


    //- Destructor
    ~checkNonMappableCellConnections() = default;


    // Member Functions

        //- find problematic cells
        void findCells(labelHashSet& badCells);

        //- find and remove problematic cells
        bool removeCells();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Module
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
