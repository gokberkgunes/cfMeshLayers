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

#include "demandDrivenData.H"
#include "volumeOptimizer.H"
#include "tetrahedron.H"
#include "partTetMeshSimplex.H"

//#define DEBUGSmooth

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

const Foam::vector Foam::Module::volumeOptimizer::dirVecs[8] =
    {
        vector(-1.0, -1.0, -1.0),
        vector(1.0, -1.0, -1.0),
        vector(-1.0, 1.0, -1.0),
        vector(1.0, 1.0, -1.0),
        vector(-1.0, -1.0, 1.0),
        vector(1.0, -1.0, 1.0),
        vector(-1.0, 1.0, 1.0),
        vector(1.0, 1.0, 1.0)
    };

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

Foam::Module::volumeOptimizer::volumeOptimizer(partTetMeshSimplex& simplex)
:
    simplexSmoother(simplex)
{}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void Foam::Module::volumeOptimizer::optimizeNodePosition(const scalar tol)
{
    point& p = points_[pointI_];

    if (!bb_.contains(p))
        p = 0.5*(bb_.max() + bb_.min());

    const scalar scale = 1.0/bb_.mag();
    forAll(points_, pI)
        points_[pI] *= scale;
    bb_.min() *= scale;
    bb_.max() *= scale;

    // find the optimum using divide and conquer
    const scalar func = optimiseDivideAndConquer(tol);
    const point copyP = p;

    // check if the location can be improved using the steepest descent
    const scalar funcAfter = optimiseSteepestDescent(tol);

    if (funcAfter > func)
        p = copyP;

    // scale back to the original size
    forAll(points_, pI)
        points_[pI] /= scale;
    bb_.min() /= scale;
    bb_.max() /= scale;
}


// ************************************************************************* //
