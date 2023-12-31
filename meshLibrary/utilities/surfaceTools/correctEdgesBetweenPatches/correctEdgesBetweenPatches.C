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

\*---------------------------------------------------------------------------*/

#include "correctEdgesBetweenPatches.H"
#include "demandDrivenData.H"
#include "meshSurfaceEngine.H"
#include "decomposeCells.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

const Foam::Module::meshSurfaceEngine&
Foam::Module::correctEdgesBetweenPatches::meshSurface() const
{
    if (!msePtr_)
        msePtr_ = new meshSurfaceEngine(mesh_);

    return *msePtr_;
}


// delete mesh surface
void Foam::Module::correctEdgesBetweenPatches::clearMeshSurface()
{
    deleteDemandDrivenData(msePtr_);
}


void Foam::Module::correctEdgesBetweenPatches::replaceBoundary()
{
    clearMeshSurface();

    polyMeshGenModifier(mesh_).replaceBoundary
    (
        patchNames_,
        newBoundaryFaces_,
        newBoundaryOwners_,
        newBoundaryPatches_
    );
}


void Foam::Module::correctEdgesBetweenPatches::decomposeCorrectedCells()
{
    if (decompose_)
    {
        clearMeshSurface();

        decomposeCells dc(mesh_);
        dc.decomposeMesh(decomposeCell_);
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::Module::correctEdgesBetweenPatches::correctEdgesBetweenPatches
(
    polyMeshGen& mesh
)
:
    mesh_(mesh),
    msePtr_(nullptr),
    patchNames_(mesh.boundaries().size()),
    patchTypes_(mesh.boundaries().size()),
    newBoundaryFaces_(),
    newBoundaryOwners_(),
    newBoundaryPatches_(),
    decomposeCell_(mesh_.cells().size(), false),
    decompose_(false)
{
    const PtrList<boundaryPatch>& boundaries = mesh_.boundaries();
    forAll(boundaries, patchI)
    {
        patchNames_[patchI] = boundaries[patchI].patchName();
        patchTypes_[patchI] = boundaries[patchI].patchType();
    }

    //decomposeProblematicFaces();

    decomposeConcaveFaces();

    patchCorrection();

    decomposeCorrectedCells();
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::Module::correctEdgesBetweenPatches::~correctEdgesBetweenPatches()
{
    deleteDemandDrivenData(msePtr_);
}


// ************************************************************************* //
