/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2005-2007 Franjo Juretic
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Application
    Test for smoothers

Description
    - reads the mesh and tries to untangle negative volume cells

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "meshOptimizer.H"
#include "Time.H"
#include "polyMeshGen.H"
#include "triSurf.H"
#include "coordinateModifier.H"
#include "surfaceMeshGeometryModification.H"
#include "polyMeshGenGeometryModification.H"
#include "demandDrivenData.H"

using namespace Foam;
using namespace Foam::Module;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Main program:

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"

    IOdictionary meshDict
    (
        IOobject
        (
            "meshDict",
            runTime.system(),
            runTime,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    );

    polyMeshGen pmg(runTime);

    Info<< "Starting reading mesh" << endl;
    pmg.read();
    Info<< "Finished reading mesh" << endl;

    const triSurf* surfPtr(nullptr);
    surfPtr = new triSurf(fileName(meshDict.lookup("surfaceFile")));

    surfaceMeshGeometryModification surfaceModification(*surfPtr, meshDict);

    const triSurf* modSurfPtr = surfaceModification.modifyGeometry();

    surfaceMeshGeometryModification backSurfModification(*modSurfPtr, meshDict);

    const triSurf* backModSurfPtr =
        backSurfModification.revertGeometryModification();

    // check if the starting surface and the modified surface are the same
    const pointField& pts = surfPtr->points();
    const pointField& bPts = backModSurfPtr->points();
    forAll(pts, pointI)
    {
        const scalar dst = mag(pts[pointI] - bPts[pointI]);

        if (dst > SMALL)
            Info<< "Point " << pointI << " does not match by " << dst << endl;
    }

    modSurfPtr->writeSurface("modifiedSurface.fms");
    deleteDemandDrivenData(modSurfPtr);
    deleteDemandDrivenData(backModSurfPtr);

    // check the point field
    const pointFieldPMG& points = pmg.points();
    pointField ptsCopy(points.size());
    forAll(points, pI)
        ptsCopy[pI] = points[pI];

    polyMeshGenGeometryModification meshModification(pmg, meshDict);
    meshModification.modifyGeometry();
    //meshModification.revertGeometryModification();

    forAll(ptsCopy, pI)
    {
        const scalar dist = mag(ptsCopy[pI] - points[pI]);

        if (dist > SMALL)
            Info<< "Point " << pI << " does not match by " << dist << endl;
    }

    pmg.write();

    Info<< "End\n" << endl;
    return 0;
}


// ************************************************************************* //
