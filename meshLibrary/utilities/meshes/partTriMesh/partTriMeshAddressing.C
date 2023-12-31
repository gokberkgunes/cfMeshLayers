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
#include "meshSurfacePartitioner.H"
#include "partTriMesh.H"
#include "meshSurfaceEngine.H"
#include "triSurfModifier.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void Foam::Module::partTriMesh::createPointsAndTrias
(
    const List<direction>& useFace
)
{
    const labelList& facePatch = mPart_.boundaryFacePatches();
    const meshSurfaceEngine& meshSurface = mPart_.surfaceEngine();
    const pointFieldPMG& points = meshSurface.points();
    const vectorField& faceCentres = meshSurface.faceCentres();
    const labelList& bPoints = meshSurface.boundaryPoints();
    const labelList& bp = meshSurface.bp();
    const faceList::subList& bFaces = meshSurface.boundaryFaces();

    meshSurfacePointLabelInTriMesh_.setSize(bPoints.size());
    meshSurfacePointLabelInTriMesh_ = -1;
    labelList nodeLabelForFace(bFaces.size(), -1);

    label nTriPoints(0);
    forAll(bFaces, bfI)
    {
        if (useFace[bfI])
        {
            const face& bf = bFaces[bfI];

            // create a point in the face centre
            if (bf.size() > 3)
            {
                nodeLabelForFace[bfI] = nTriPoints++;
            }

            // create points at face points
            forAll(bf, pI)
            {
                const label bpI = bp[bf[pI]];

                if (meshSurfacePointLabelInTriMesh_[bpI] == -1)
                {
                    meshSurfacePointLabelInTriMesh_[bpI] = nTriPoints++;
                }
            }

            // create triangles
            if (bf.size() > 3)
            {
                forAll(bf, eI)
                {
                    // add a triangle connected to face centre
                    labelledTri tri
                    (
                        meshSurfacePointLabelInTriMesh_[bp[bf[eI]]],
                        meshSurfacePointLabelInTriMesh_[bp[bf.nextLabel(eI)]],
                        nodeLabelForFace[bfI],
                        facePatch[bfI]
                    );

                    surf_.appendTriangle(tri);

                    // add a triangle for shape
                    labelledTri secondTri
                    (
                        meshSurfacePointLabelInTriMesh_[bp[bf[eI]]],
                        meshSurfacePointLabelInTriMesh_[bp[bf.nextLabel(eI)]],
                        meshSurfacePointLabelInTriMesh_[bp[bf.prevLabel(eI)]],
                        facePatch[bfI]
                    );

                    surf_.appendTriangle(secondTri);
                }
            }
            else
            {
                // face is a triangle
                labelledTri tri
                (
                    meshSurfacePointLabelInTriMesh_[bp[bf[0]]],
                    meshSurfacePointLabelInTriMesh_[bp[bf[1]]],
                    meshSurfacePointLabelInTriMesh_[bp[bf[2]]],
                    facePatch[bfI]
                );

                surf_.appendTriangle(tri);

                // add a triangle for shape
                forAll(bf, eI)
                {
                    labelledTri secondTri
                    (
                        meshSurfacePointLabelInTriMesh_[bp[bf[eI]]],
                        meshSurfacePointLabelInTriMesh_[bp[bf.nextLabel(eI)]],
                        meshSurfacePointLabelInTriMesh_[bp[bf.prevLabel(eI)]],
                        facePatch[bfI]
                    );

                    surf_.appendTriangle(secondTri);
                }
            }
        }
    }

    // add points
    triSurfModifier sMod(surf_);
    pointField& pts = sMod.pointsAccess();
    pts.setSize(nTriPoints);

    pointType_.setSize(nTriPoints);
    pointType_ = NONE;

    pointLabelInMeshSurface_.setSize(pts.size());
    pointLabelInMeshSurface_ = -1;

    forAll(meshSurfacePointLabelInTriMesh_, bpI)
    {
        if (meshSurfacePointLabelInTriMesh_[bpI] != -1)
        {
            const label npI = meshSurfacePointLabelInTriMesh_[bpI];
            pointLabelInMeshSurface_[npI] = bpI;
            pts[npI] = points[bPoints[bpI]];
            pointType_[npI] |= SMOOTH;
        }
    }

    forAll(nodeLabelForFace, bfI)
    {
        if (nodeLabelForFace[bfI] != -1)
        {
            const label npI = nodeLabelForFace[bfI];
            pts[npI] = faceCentres[bfI];
            pointType_[npI] = FACECENTRE;
        }
    }

    // set CORNER and FEATUREEDGE flags to surface points
    for (const label corneri : mPart_.corners())
    {
        if (meshSurfacePointLabelInTriMesh_[corneri] != -1)
        {
            pointType_[meshSurfacePointLabelInTriMesh_[corneri]] |= CORNER;
        }
    }

    for (const label edgei : mPart_.edgePoints())
    {
        const label pI = meshSurfacePointLabelInTriMesh_[edgei];
        if (pI != -1)
        {
            pointType_[pI] |= FEATUREEDGE;
        }
    }

    // create addressing for parallel runs
    if (Pstream::parRun())
    {
        createParallelAddressing
        (
            meshSurfacePointLabelInTriMesh_,
            nodeLabelForFace
        );

        createBufferLayers();
    }

    // calculate point facets addressing
    surf_.pointFacets();
}


// ************************************************************************* //
