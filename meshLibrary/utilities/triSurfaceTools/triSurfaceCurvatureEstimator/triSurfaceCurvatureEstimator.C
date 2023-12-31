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

#include "triSurfaceCurvatureEstimator.H"
#include "demandDrivenData.H"

//#define DEBUGMorph

# ifdef DEBUGMorph
#include <sstream>
# endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

Foam::Module::triSurfaceCurvatureEstimator::triSurfaceCurvatureEstimator
(
    const triSurf& surface
)
:
    surface_(surface),
    edgePointCurvature_(),
    patchPositions_(),
    gaussianCurvature_(),
    meanCurvature_(),
    maxCurvature_(),
    minCurvature_(),
    maxCurvatureVector_(),
    minCurvatureVector_()
{
    calculateEdgeCurvature();
    calculateSurfaceCurvatures();
    //calculateGaussianCurvature();
    //calculateMeanCurvature();
    //calculateMinAndMaxCurvature();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

Foam::scalar Foam::Module::triSurfaceCurvatureEstimator::edgePointCurvature
(
    const label pI
) const
{
    return edgePointCurvature_[pI];
}


Foam::scalar Foam::Module::triSurfaceCurvatureEstimator::curvatureAtEdge
(
    const label edgeI
) const
{
    const edge& edg = surface_.edges()[edgeI];

    const scalar k1 = edgePointCurvature_[edg.start()];
    const scalar k2 = edgePointCurvature_[edg.end()];

    return (k1 + k2)/2.0;
}


Foam::scalar
Foam::Module::triSurfaceCurvatureEstimator::gaussianCurvatureAtTriangle
(
    const label triI
) const
{
    const labelledTri& tri = surface_[triI];

    scalar curv(0.0);
    for (label i = 0; i < 3; ++i)
        curv += gaussianCurvature_[tri[i]][patchPositions_(triI, i)];
    curv /= 3.0;

    return curv;
}


Foam::scalar Foam::Module::triSurfaceCurvatureEstimator::meanCurvatureAtTriangle
(
    const label triI
) const
{
    const labelledTri& tri = surface_[triI];

    scalar curv(0.0);
    for (label i = 0; i < 3; ++i)
        curv += meanCurvature_[tri[i]][patchPositions_(triI, i)];
    curv /= 3.0;

    return curv;
}


Foam::scalar Foam::Module::triSurfaceCurvatureEstimator::maxCurvatureAtTriangle
(
    const label triI
) const
{
    const labelledTri& tri = surface_[triI];

    scalar curv(0.0);
    for (label i = 0; i < 3; ++i)
        curv += maxCurvature_[tri[i]][patchPositions_(triI, i)];
    curv /= 3.0;

    return curv;
}


Foam::scalar Foam::Module::triSurfaceCurvatureEstimator::minCurvatureAtTriangle
(
    const label triI
) const
{
    const labelledTri& tri = surface_[triI];

    scalar curv(0.0);
    for (label i = 0; i < 3; ++i)
        curv += minCurvature_[tri[i]][patchPositions_(triI, i)];
    curv /= 3.0;

    return curv;
}


Foam::vector
Foam::Module::triSurfaceCurvatureEstimator::maxCurvatureVectorAtTriangle
(
    const label triI
) const
{
    const labelledTri& tri = surface_[triI];

    vector curv(vector::zero);
    for (label i = 0; i < 3; ++i)
        curv += maxCurvatureVector_[tri[i]][patchPositions_(triI, i)];
    curv /= 3.0;

    return curv;
}


Foam::vector
Foam::Module::triSurfaceCurvatureEstimator::minCurvatureVectorAtTriangle
(
    const label triI
) const
{
    const labelledTri& tri = surface_[triI];

    vector curv(vector::zero);
    for (label i = 0; i < 3; ++i)
        curv += minCurvatureVector_[tri[i]][patchPositions_(triI, i)];
    curv /= 3.0;

    return curv;
}


// ************************************************************************* //
