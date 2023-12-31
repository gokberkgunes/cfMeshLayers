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
    Foam::Module::tetMeshOptimisation

Description
    Mesh smoothing without any topological changes

SourceFiles
    tetMeshOptimisation.C

\*---------------------------------------------------------------------------*/

#ifndef tetMeshOptimisation_H
#define tetMeshOptimisation_H

#include "DynList.H"
#include "parPartTet.H"
#include "boundBox.H"
#include "labelLongList.H"
#include "boolList.H"

#include <map>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace Module
{

// Forward declarations
class partTetMesh;
class meshSurfaceEngine;

/*---------------------------------------------------------------------------*\
                     Class tetMeshOptimisation Declaration
\*---------------------------------------------------------------------------*/

class tetMeshOptimisation
{
    // Private data

        //- reference to the tet mesh
        partTetMesh& tetMesh_;


    // Private member functions needed for parallel runs

        //- make sure that all processors have the same points
        //- marked as negativeNode
        void unifyNegativePoints(boolList& negativeNode) const;

        //- exchange data with other processors
        void exchangeData
        (
            std::map<label, DynList<parPartTet>>& m,
            boolList* negativeNodePtr = nullptr
        );

        //- update buffer layer points
        void updateBufferLayerPoints();

        //- make sure that coordinates of moved points remain the same
        //- on all processor containing those points
        void unifyCoordinatesParallel(const boolList* negativeNodePtr = nullptr);


public:

    //- Construct from tet mesh
    tetMeshOptimisation(partTetMesh& mesh);

    //- Destructor
    ~tetMeshOptimisation() = default;


    // Member Functions

        //- untangle mesh by using Patrik Knupp's simple metric
        void optimiseUsingKnuppMetric(const label nInterations = 5);

        //- smooth using mesh untangler
        void optimiseUsingMeshUntangler(const label nIerations = 5);

        //- smooth using volume optimizer
        void optimiseUsingVolumeOptimizer(const label nIterations = 10);

        //- smooth boundary using the volume optimizer
        void optimiseBoundaryVolumeOptimizer
        (
            const label nIterations = 3,
            const bool nonShrinking = false
        );

        //- smooth boundary using shrinking surface laplace
        void optimiseBoundarySurfaceLaplace(const label nIterations = 3);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Module
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
