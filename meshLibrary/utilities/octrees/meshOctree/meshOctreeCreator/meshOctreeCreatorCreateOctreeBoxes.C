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

#include "meshOctreeCreator.H"
#include "meshOctreeModifier.H"
#include "IOdictionary.H"
#include "boundBox.H"
#include "triSurf.H"
#include "meshOctreeAutomaticRefinement.H"

# ifdef USE_OMP
#include <omp.h>
# endif

// * * * * * * * * * * * * Private member functions  * * * * * * * * * * * * //

void Foam::Module::meshOctreeCreator::setRootCubeSizeAndRefParameters()
{
    meshOctreeModifier octreeModifier(octree_);
    if (octreeModifier.isRootInitialisedAccess())
    {
        return;
    }

    if (!meshDictPtr_)
    {
        WarningInFunction
            << "meshDict is not available" << endl;
        return;
    }

    const scalar maxSize
    (
        scalingFactor_ *
        readScalar(meshDictPtr_->lookup("maxCellSize"))
    );

    octreeModifier.searchRangeAccess() = 0.25*maxSize;

    const triSurf& surface = octree_.surface();
    boundBox& rootBox = octreeModifier.rootBoxAccess();
    const point c = (rootBox.max() + rootBox.min()) / 2.0;

    scalar size = rootBox.max().x() - rootBox.min().x();
    if ((rootBox.max().y() - rootBox.min().y()) > size)
    {
        size = rootBox.max().y() - rootBox.min().y();
    }
    if ((rootBox.max().z() - rootBox.min().z()) > size)
    {
        size = rootBox.max().z() - rootBox.min().z();
    }

    size += 0.5*maxSize;

    globalRefLevel_ = 0;
    bool finished;
    do
    {
        finished = false;

        const scalar lSize = size/Foam::pow(label(2), label(globalRefLevel_));
        // Or: const scalar lSize = size/(1L << globalRefLevel_);

        if (lSize <(maxSize*(1.0 - SMALL)))
        {
            const scalar bbSize =
                0.5*maxSize*Foam::pow(label(2), label(globalRefLevel_));
            // Or: const scalar bbSize = 0.5*maxSize*(1L << globalRefLevel_);

            rootBox.max() = c + point(bbSize, bbSize, bbSize);
            rootBox.min() = c - point(bbSize, bbSize, bbSize);
            finished = true;
        }
        else
        {
            ++globalRefLevel_;
        }
    } while (!finished);

    // exchange data
    if (Pstream::parRun())
    {
        label l = globalRefLevel_;
        reduce(l, maxOp<label>());
        globalRefLevel_ = l;
    }

    Info<< "Root box " << rootBox << endl;
    Info<< "Requested cell size corresponds to octree level "
        << label(globalRefLevel_) << endl;

    octreeModifier.isRootInitialisedAccess() = true;

    //surfRefLevel_ = globalRefLevel_;
    forAll(surfRefLevel_, triI)
    {
        surfRefLevel_[triI].append(std::make_pair(globalRefLevel_, 0.0));
    }

    // set other refinement parameters
    // set boundary ref level
    if (meshDictPtr_->found("boundaryCellSize"))
    {
        direction boundaryRefLevel_ = globalRefLevel_;
        scalar cs(readScalar(meshDictPtr_->lookup("boundaryCellSize")));
        cs *= (scalingFactor_ + SMALL);

        octreeModifier.searchRangeAccess() = cs;

        label addLevel(0);
        do
        {
            finished = false;

            const scalar lSize = maxSize/Foam::pow(label(2), addLevel);
            // Or: const scalar lSize = maxSize/(1L << addLevel);

            if (lSize <= cs)
            {
                finished = true;
            }
            else
            {
                ++addLevel;
            }
        } while (!finished);

        boundaryRefLevel_ += addLevel;

        // exchange data
        if (Pstream::parRun())
        {
            label l = boundaryRefLevel_;
            reduce(l, maxOp<label>());
            boundaryRefLevel_ = l;
        }

        Info<< "Requested boundary cell size corresponds to octree level "
            << label(boundaryRefLevel_) << endl;

        scalar thickness(0.0);
        if
        (
            meshDictPtr_->readIfPresent
            (
                "boundaryCellSizeRefinementThickness",
                thickness
            )
        )
        {
            thickness = mag(thickness);
        }

        forAll(surfRefLevel_, triI)
        {
            surfRefLevel_[triI][0] =
                std::make_pair(boundaryRefLevel_, thickness);
        }
    }

    // set patch-wise ref levels
    if (meshDictPtr_->found("patchCellSize"))
    {
        patchRefinementList refPatches;

        if (meshDictPtr_->isDict("patchCellSize"))
        {
            const dictionary& dict = meshDictPtr_->subDict("patchCellSize");
            const wordList patchNames = dict.toc();

            const wordList allPatches = surface.patchNames();

            refPatches.setSize(allPatches.size());

            label counter(0);

            forAll(patchNames, patchI)
            {
                if (!dict.isDict(patchNames[patchI]))
                {
                    continue;
                }

                const dictionary& patchDict = dict.subDict(patchNames[patchI]);
                const scalar cs = readScalar(patchDict.lookup("cellSize"));

                labelList matchedIDs = surface.findPatches(patchNames[patchI]);
                forAll(matchedIDs, matchI)
                {
                    refPatches[counter] =
                        patchRefinement(allPatches[matchedIDs[matchI]], cs);
                    ++counter;
                }
            }

            refPatches.setSize(counter);
        }
        else
        {
            patchRefinementList prl(meshDictPtr_->lookup("patchCellSize"));
            refPatches.transfer(prl);
        }

        forAll(refPatches, patchI)
        {
            const label regionI = refPatches[patchI].patchInSurface(surface);

            if (regionI < 0)
            {
                continue;
            }

            scalar cs = refPatches[patchI].cellSize();
            cs *= (scalingFactor_ + SMALL);

            label addLevel(0);
            do
            {
                finished = false;

                const scalar lSize = maxSize/Foam::pow(label(2), addLevel);
                // Or: const scalar lSize = maxSize/(1L << addLevel);

                if (lSize <= cs)
                {
                    finished = true;
                }
                else
                {
                    ++addLevel;
                }
            } while (!finished);

            if (Pstream::parRun())
            {
                reduce(addLevel, maxOp<label>());
            }

            const direction level = globalRefLevel_ + addLevel;

            std::pair<direction, scalar> pp(level, 0.0);
            forAll(surface, triI)
            {
                if (surface[triI].region() == regionI)
                {
                    surfRefLevel_[triI].append(pp);
                }
            }
        }
    }

    if (meshDictPtr_->found("subsetCellSize"))
    {
        patchRefinementList refPatches;

        if (meshDictPtr_->isDict("subsetCellSize"))
        {
            const dictionary& dict = meshDictPtr_->subDict("subsetCellSize");
            const wordList patchNames = dict.toc();

            refPatches.setSize(patchNames.size());
            label counter(0);

            forAll(patchNames, patchI)
            {
                if (!dict.isDict(patchNames[patchI]))
                {
                    continue;
                }

                const dictionary& patchDict = dict.subDict(patchNames[patchI]);
                const scalar cs = readScalar(patchDict.lookup("cellSize"));

                refPatches[counter] = patchRefinement(patchNames[patchI], cs);
                ++counter;
            }

            refPatches.setSize(counter);
        }
        else
        {
            patchRefinementList srl(meshDictPtr_->lookup("subsetCellSize"));
            refPatches.transfer(srl);
        }

        forAll(refPatches, patchI)
        {
            const word subsetName = refPatches[patchI].patchName();

            const label subsetID = surface.facetSubsetIndex(subsetName);
            if (subsetID < 0)
            {
                Warning << "Surface subset " << subsetName
                    << " does not exist" << endl;
                continue;
            }

            scalar cs = refPatches[patchI].cellSize();
            cs *= (scalingFactor_ + SMALL);

            label addLevel(0);
            do
            {
                finished = false;

                const scalar lSize = maxSize/Foam::pow(label(2), addLevel);
                // Or: const scalar lSize = maxSize/(1L << addLevel);

                if (lSize <= cs)
                {
                    finished = true;
                }
                else
                {
                    ++addLevel;
                }
            } while (!finished);

            if (Pstream::parRun())
            {
                reduce(addLevel, maxOp<label>());
            }

            labelLongList subsetFaces;
            surface.facetsInSubset(subsetID, subsetFaces);
            const direction level = globalRefLevel_ + addLevel;

            const std::pair<direction, scalar> pp(level, 0.0);
            forAll(subsetFaces, tI)
            {
                surfRefLevel_[subsetFaces[tI]].append(pp);
            }
        }
    }

    if (meshDictPtr_->found("localRefinement"))
    {
        if (meshDictPtr_->isDict("localRefinement"))
        {
            const dictionary& dict = meshDictPtr_->subDict("localRefinement");
            const wordList entries = dict.toc();

            // map patch name to its index
            std::map<word, label> patchToIndex;
            forAll(surface.patches(), patchI)
            {
                patchToIndex[surface.patches()[patchI].name()] = patchI;
            }

            // map a facet subset name to its index
            std::map<word, label> setToIndex;
            DynList<label> setIDs;
            surface.facetSubsetIndices(setIDs);
            forAll(setIDs, i)
            {
                setToIndex[surface.facetSubsetName(setIDs[i])] = setIDs[i];
            }

            // set refinement for these entries
            forAll(entries, dictI)
            {
                if (!dict.isDict(entries[dictI]))
                {
                    continue;
                }

                const word& pName = entries[dictI];
                const dictionary& patchDict = dict.subDict(pName);

                label nLevel(0);
                if
                (
                    patchDict.readIfPresent
                    (
                        "additionalRefinementLevels",
                        nLevel
                    )
                )
                {
                    //
                }
                else if (patchDict.found("cellSize"))
                {
                    const scalar cs =
                        readScalar(patchDict.lookup("cellSize"));

                    do
                    {
                        finished = false;

                        const scalar lSize =
                            maxSize/Foam::pow(label(2), nLevel);
                        // Or: const scalar lSize = maxSize/(1L << nLevel);

                        if (lSize <= cs)
                        {
                            finished = true;
                        }
                        else
                        {
                            ++nLevel;
                        }
                    } while (!finished);
                }

                scalar refinementThickness(0.0);
                patchDict.readIfPresent
                (
                    "refinementThickness",
                    refinementThickness
                );

                const direction level = globalRefLevel_ + nLevel;

                const labelList matchedPatches = surface.findPatches(pName);

                std::pair<direction, scalar> rp(level, refinementThickness);

                forAll(matchedPatches, matchI)
                {
                    // patch-based refinement
                    const label patchI = matchedPatches[matchI];

                    forAll(surface, triI)
                    {
                        if (surface[triI].region() == patchI)
                        {
                            surfRefLevel_[triI].append(rp);
                        }
                    }
                }
                if (setToIndex.find(pName) != setToIndex.end())
                {
                    // this is a facet subset
                    const label subsetId = setToIndex[pName];

                    labelLongList facetsInSubset;
                    surface.facetsInSubset(subsetId, facetsInSubset);

                    forAll(facetsInSubset, i)
                    {
                        const label triI = facetsInSubset[i];
                        surfRefLevel_[triI].append(rp);
                    }
                }
            }
        }
    }
}


void Foam::Module::meshOctreeCreator::refineInsideAndUnknownBoxes()
{
    const direction cType = meshOctreeCube::INSIDE + meshOctreeCube::UNKNOWN;

    refineBoxes(globalRefLevel_, cType);
}


void Foam::Module::meshOctreeCreator::createOctreeBoxes()
{
    // set root cube size in order to achieve desired maxCellSize
    Info<< "Setting root cube size and refinement parameters" << endl;
    setRootCubeSizeAndRefParameters();

    // refine to required boundary resolution
    Info<< "Refining boundary" << endl;
    refineBoundary();

    // refine parts intersected by surface meshes acting as refinement sources
    refineBoxesIntersectingSurfaces();

    // refine parts intersected by edge meshes acting as refinement sources
    refineBoxesIntersectingEdgeMeshes();

    // perform automatic octree refinement
    if (!Pstream::parRun())
    {
        Info<< "Performing automatic refinement" << endl;
        meshOctreeAutomaticRefinement autoRef(octree_, *meshDictPtr_, false);

        if (hexRefinement_)
        {
            autoRef.activateHexRefinement();
        }

        autoRef.automaticRefinement();
    }

    // set up inside-outside information
    createInsideOutsideInformation();

    // refine INSIDE and UNKNOWN boxes to to the given cell size
    refineInsideAndUnknownBoxes();

    // refine boxes inside the geometry objects
    refineBoxesContainedInObjects();

    // make sure that INSIDE and UNKNOWN neighbours of DATA boxes
    // have the same or higher refinement level
    refineBoxesNearDataBoxes(1);

    // distribute octree such that each processor has the same number
    // of leaf boxes which will be used as mesh cells
    if (Pstream::parRun())
    {
        loadDistribution(true);
    }
}


void Foam::Module::meshOctreeCreator::createOctreeWithRefinedBoundary
(
    const direction maxLevel,
    const label nTrianglesInLeaf
)
{
    const triSurf& surface = octree_.surface();
    surface.facetEdges();
    surface.edgeFacets();
    const boundBox& rootBox = octree_.rootBox();
    meshOctreeModifier octreeModifier(octree_);
    List<meshOctreeSlot>& slots = octreeModifier.dataSlotsAccess();

    while (true)
    {
        const LongList<meshOctreeCube*>& leaves =
            octreeModifier.leavesAccess();

        label nMarked(0);

        # ifdef USE_OMP
        # pragma omp parallel reduction(+ : nMarked)
        # endif
        {
            # ifdef USE_OMP
            meshOctreeSlot* slotPtr = &slots[omp_get_thread_num()];
            # else
            meshOctreeSlot* slotPtr = &slots[0];
            # endif

            # ifdef USE_OMP
            # pragma omp for schedule(dynamic, 20)
            # endif
            forAll(leaves, leafI)
            {
                meshOctreeCube& oc = *leaves[leafI];

                DynList<label> ct;
                octree_.containedTriangles(leafI, ct);

                if ((oc.level() < maxLevel) && (ct.size() > nTrianglesInLeaf))
                {
                    oc.refineCube(surface, rootBox, slotPtr);
                    ++nMarked;
                }
            }
        }

        if (nMarked == 0)
        {
            break;
        }

        octreeModifier.createListOfLeaves();

    }
}


// ************************************************************************* //
