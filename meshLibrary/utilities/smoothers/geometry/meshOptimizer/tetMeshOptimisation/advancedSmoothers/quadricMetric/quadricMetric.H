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
    Foam::Module::quadricMetric

Description
    Mesh smoothing using quadric metric. The metric is strictly convex

SourceFiles
    quadricMetric.C

\*---------------------------------------------------------------------------*/

#ifndef quadricMetric_H
#define quadricMetric_H

#include "simplexSmoother.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace Module
{

// Forward declarations
class partTetMeshSimplex;

/*---------------------------------------------------------------------------*\
                        Class quadricMetric Declaration
\*---------------------------------------------------------------------------*/

class quadricMetric
:
    public simplexSmoother
{
    // Private data

        //- free vertex
        point& p_;

        //- normals of triangles forming the outer hull
        DynList<vector, 64> normals_;

        //- centres of triangles forming the outer hull
        DynList<point, 64> centres_;


    // Private member functions

        //- evaluate the value of the metric
        scalar evaluateMetric() const;

        //- evaluate metric gradients
        void evaluateGradients(vector& grad, tensor& gradGrad) const;


public:

    //- Constructor
    quadricMetric(partTetMeshSimplex& simplex);

    //- Destructor
    ~quadricMetric() = default;


    // Member functions

        //- find the best position such that all tets making a simplex
        //- have a positive volume
        void optimizeNodePosition(const scalar tol = 0.001);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Module
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
