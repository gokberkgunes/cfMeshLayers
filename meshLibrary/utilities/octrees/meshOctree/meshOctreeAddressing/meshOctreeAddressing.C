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

#include "meshOctreeAddressing.H"
#include "demandDrivenData.H"
#include "IOdictionary.H"
#include "helperFunctions.H"
#include "triSurf.H"
#include "meshOctreeModifier.H"

// #define DEBUGSearch

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void Foam::Module::meshOctreeAddressing::clearOut()
{
    clearNodeAddressing();
    clearBoxTypes();
    clearOctreeFaces();
    clearAddressing();
    clearParallelAddressing();
}


void Foam::Module::meshOctreeAddressing::clearNodeAddressing()
{
    nNodes_ = 0;
    deleteDemandDrivenData(octreePointsPtr_);
    deleteDemandDrivenData(nodeLabelsPtr_);
    deleteDemandDrivenData(nodeLeavesPtr_);

    deleteDemandDrivenData(nodeTypePtr_);
}


void Foam::Module::meshOctreeAddressing::clearBoxTypes()
{
    deleteDemandDrivenData(boxTypePtr_);
}


void Foam::Module::meshOctreeAddressing::clearOctreeFaces()
{
    deleteDemandDrivenData(octreeFacesPtr_);
    deleteDemandDrivenData(octreeFacesOwnersPtr_);
    deleteDemandDrivenData(octreeFacesNeighboursPtr_);
}


void Foam::Module::meshOctreeAddressing::clearAddressing()
{
    deleteDemandDrivenData(leafFacesPtr_);
    deleteDemandDrivenData(nodeFacesPtr_);
    deleteDemandDrivenData(leafLeavesPtr_);
    deleteDemandDrivenData(octreeEdgesPtr_);
    deleteDemandDrivenData(edgeLeavesPtr_);
    deleteDemandDrivenData(leafEdgesPtr_);
    deleteDemandDrivenData(nodeEdgesPtr_);
    deleteDemandDrivenData(faceEdgesPtr_);
    deleteDemandDrivenData(edgeFacesPtr_);
}


void Foam::Module::meshOctreeAddressing::clearParallelAddressing()
{
    deleteDemandDrivenData(globalPointLabelPtr_);
    deleteDemandDrivenData(globalPointToLocalPtr_);
    deleteDemandDrivenData(pointProcsPtr_);
    deleteDemandDrivenData(globalFaceLabelPtr_);
    deleteDemandDrivenData(globalFaceToLocalPtr_);
    deleteDemandDrivenData(faceProcsPtr_);
    deleteDemandDrivenData(globalLeafLabelPtr_);
    deleteDemandDrivenData(globalLeafToLocalPtr_);
    deleteDemandDrivenData(leafAtProcsPtr_);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::Module::meshOctreeAddressing::meshOctreeAddressing
(
    const meshOctree& mo,
    const dictionary& dict,
    bool useDATABoxes
)
:
    octree_(mo),
    meshDict_(dict),
    useDATABoxes_(useDATABoxes),
    nNodes_(0),
    octreePointsPtr_(nullptr),
    nodeLabelsPtr_(nullptr),
    nodeLeavesPtr_(nullptr),
    boxTypePtr_(nullptr),
    nodeTypePtr_(nullptr),
    octreeFacesPtr_(nullptr),
    octreeFacesOwnersPtr_(nullptr),
    octreeFacesNeighboursPtr_(nullptr),
    leafFacesPtr_(nullptr),
    nodeFacesPtr_(nullptr),
    leafLeavesPtr_(nullptr),
    octreeEdgesPtr_(nullptr),
    edgeLeavesPtr_(nullptr),
    leafEdgesPtr_(nullptr),
    nodeEdgesPtr_(nullptr),
    faceEdgesPtr_(nullptr),
    edgeFacesPtr_(nullptr),
    globalPointLabelPtr_(nullptr),
    globalPointToLocalPtr_(nullptr),
    pointProcsPtr_(nullptr),
    globalFaceLabelPtr_(nullptr),
    globalFaceToLocalPtr_(nullptr),
    faceProcsPtr_(nullptr),
    globalLeafLabelPtr_(nullptr),
    globalLeafToLocalPtr_(nullptr),
    leafAtProcsPtr_(nullptr)
{
    if (!useDATABoxes && dict.found("keepCellsIntersectingBoundary"))
    {
        useDATABoxes_ = readBool(dict.lookup("keepCellsIntersectingBoundary"));
    }

    if (dict.lookupOrDefault<bool>("nonManifoldMeshing", false))
    {
        useDATABoxes_ = true;
    }

    if (Pstream::parRun())
    {
        meshOctreeModifier om(const_cast<meshOctree&>(octree_));
        om.addLayerFromNeighbouringProcessors();
    }

    // check for glued regions
    checkGluedRegions();
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::Module::meshOctreeAddressing::~meshOctreeAddressing()
{
    clearOut();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

bool Foam::Module::meshOctreeAddressing::isIntersectedFace(const label fI) const
{
    const labelLongList& owner = octreeFaceOwner();
    const labelLongList& neighbour = octreeFaceNeighbour();

    if (neighbour[fI] < 0)
    {
        return false;
    }

    Map<label> nAppearances;
    DynList<label> triangles;
    octree_.containedTriangles(owner[fI], triangles);
    forAll(triangles, triI)
    {
        if (nAppearances.found(triangles[triI]))
        {
            ++nAppearances[triangles[triI]];
        }
        else
        {
            nAppearances.insert(triangles[triI], 1);
        }
    }

    triangles.clear();
    octree_.containedTriangles(neighbour[fI], triangles);
    forAll(triangles, triI)
    {
        if (nAppearances.found(triangles[triI]))
        {
            ++nAppearances[triangles[triI]];
        }
        else
        {
            nAppearances.insert(triangles[triI], 1);
        }
    }

    forAllConstIters(nAppearances, iter)
    {
        if (iter() == 2)
        {
            if
            (
                octree_.returnLeaf(owner[fI]).level() ==
                octree_.returnLeaf(neighbour[fI]).level()
            )
            {
                return true;
            }

            // check intersection by geometric testing
            const triSurf& surf = octree_.surface();
            const pointField& points = this->octreePoints();
            const VRWGraph& faces = this->octreeFaces();

            face f(faces.sizeOfRow(fI));
            forAll(f, pI)
            {
                f[pI] = faces(fI, pI);
            }

            if
            (
                help::doFaceAndTriangleIntersect
                (
                    surf,
                    iter.key(),
                    f,
                    points
                )
            )
            {
                return true;
            }
        }
    }

    return false;
}


bool Foam::Module::meshOctreeAddressing::isIntersectedEdge(const label eI) const
{
    const VRWGraph& edgeCubes = this->edgeLeaves();

    Map<label> nAppearances;
    DynList<label> triangles;
    bool sameLevel(true);

    forAllRow(edgeCubes, eI, i)
    {
        const label leafI = edgeCubes(eI, i);
        if (!octree_.hasContainedTriangles(leafI))
        {
            return false;
        }

        if
        (
            octree_.returnLeaf(leafI).level()
         != octree_.returnLeaf(edgeCubes(eI, 0)).level()
        )
        {
            sameLevel = false;
        }

        triangles.clear();
        octree_.containedTriangles(leafI, triangles);
        forAll(triangles, triI)
        {
            if (nAppearances.found(triangles[triI]))
            {
                ++nAppearances[triangles[triI]];
            }
            else
            {
                nAppearances.insert(triangles[triI], 1);
            }
        }
    }

    forAllConstIters(nAppearances, iter)
    {
        if (iter() == edgeCubes.sizeOfRow(eI))
        {
            if (sameLevel)
            {
                return true;
            }

            // check for geometric intersection
            const LongList<edge>& edges = this->octreeEdges();
            const pointField& points = this->octreePoints();
            point intersection(vector::zero);

            if
            (
                help::triLineIntersection
                (
                    octree_.surface(),
                    iter.key(),
                    points[edges[eI].start()],
                    points[edges[eI].end()],
                    intersection
                )
            )
            {
                return true;
            }
        }
    }

    return false;
}


void Foam::Module::meshOctreeAddressing::edgeIntersections
(
    const label eI,
    DynList<point>& intersections
) const
{
    intersections.clear();

    const LongList<edge>& edges = this->octreeEdges();
    const pointField& points = this->octreePoints();
    const VRWGraph& edgeCubes = this->edgeLeaves();
    const scalar tol =
        SMALL*mag(points[edges[eI].start()] - points[edges[eI].end()]);

    Map<label> nAppearances;
    DynList<label> triangles;

    forAllRow(edgeCubes, eI, i)
    {
        const label leafI = edgeCubes(eI, i);
        if (!octree_.hasContainedTriangles(leafI))
        {
            return;
        }

        triangles.clear();
        octree_.containedTriangles(leafI, triangles);
        forAll(triangles, triI)
        {
            if (nAppearances.found(triangles[triI]))
            {
                ++nAppearances[triangles[triI]];
            }
            else
            {
                nAppearances.insert(triangles[triI], 1);
            }
        }
    }

    point intersection(vector::zero);

    forAllConstIters(nAppearances, iter)
    {
        if (iter() == edgeCubes.sizeOfRow(eI))
        {
            // check for geometric intersection
            const bool intersectionExists =
                help::triLineIntersection
                (
                    octree_.surface(),
                    iter.key(),
                    points[edges[eI].start()],
                    points[edges[eI].end()],
                    intersection
                );

            if (intersectionExists)
            {
                bool store(true);
                forAll(intersections, i)
                {
                    if (mag(intersections[i] - intersection) <= tol)
                    {
                        store = false;
                    }
                }

                if (store)
                {
                    intersections.append(intersection);
                }
            }
        }
    }
}


void Foam::Module::meshOctreeAddressing::cubesAroundEdge
(
    const label leafI,
    const direction eI,
    FixedList<label, 4>& edgeCubes
) const
{
    const VRWGraph& nl = this->nodeLabels();
    const label nodeI = nl(leafI, meshOctreeCubeCoordinates::edgeNodes_[eI][0]);
    const FRWGraph<label, 8>& pLeaves = this->nodeLeaves();

    switch(eI)
    {
        case 0: case 1: case 2: case 3:
        {
            edgeCubes[0] = pLeaves(nodeI, 1);
            edgeCubes[1] = pLeaves(nodeI, 3);
            edgeCubes[2] = pLeaves(nodeI, 5);
            edgeCubes[3] = pLeaves(nodeI, 7);
        } break;
        case 4: case 5: case 6: case 7:
        {
            edgeCubes[0] = pLeaves(nodeI, 2);
            edgeCubes[1] = pLeaves(nodeI, 3);
            edgeCubes[2] = pLeaves(nodeI, 6);
            edgeCubes[3] = pLeaves(nodeI, 7);
        } break;
        case 8: case 9: case 10: case 11:
        {
            edgeCubes[0] = pLeaves(nodeI, 4);
            edgeCubes[1] = pLeaves(nodeI, 5);
            edgeCubes[2] = pLeaves(nodeI, 6);
            edgeCubes[3] = pLeaves(nodeI, 7);
        } break;
        default:
        {
            FatalErrorInFunction
                << "Invalid edge specified!!" << abort(FatalError);
        } break;
    };
}


Foam::label Foam::Module::meshOctreeAddressing::findEdgeCentre
(
    const label leafI,
    const direction eI
) const
{
    if (octree_.isQuadtree() && eI >= 8)
    {
        return -1;
    }

    const meshOctreeCubeBasic& oc = octree_.returnLeaf(leafI);
    const VRWGraph& nl = this->nodeLabels();
    const label nodeI = nl(leafI, meshOctreeCubeCoordinates::edgeNodes_[eI][0]);
    const FRWGraph<label, 8>& pLeaves = this->nodeLeaves();

    const direction level = oc.level();

    label fI(-1);
    switch(eI)
    {
        case 0: case 1: case 2: case 3:
        {
            fI = 1;
        } break;
        case 4: case 5: case 6: case 7:
        {
            fI = 3;
        } break;
        case 8: case 9: case 10: case 11:
        {
            fI = 5;
        } break;
        default:
        {
            FatalErrorInFunction
                << "Invalid edge specified!!" << abort(FatalError);
        } break;
    };

    for (label i = 0; i < 4; ++i)
    {
        const label fNode = meshOctreeCubeCoordinates::faceNodes_[fI][i];

        if (pLeaves(nodeI, fNode) < 0)
        {
            continue;
        }

        const label leafJ = pLeaves(nodeI, fNode);
        if (octree_.returnLeaf(leafJ).level() > level)
        {
            const label shift = (i + 2)%4;
            return nl(leafJ, meshOctreeCubeCoordinates::faceNodes_[fI][shift]);
        }
    }

    return -1;
}


// ************************************************************************* //
