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

#include "tetMeshGenerator.H"
#include "triSurf.H"
#include "demandDrivenData.H"
#include "meshOctreeCreator.H"
#include "tetMeshExtractorOctree.H"
#include "meshSurfaceEngine.H"
#include "meshSurfaceMapper.H"
#include "edgeExtractor.H"
#include "meshSurfaceEdgeExtractorNonTopo.H"
#include "surfaceMorpherCells.H"
#include "meshOptimizer.H"
#include "meshSurfaceOptimizer.H"
#include "topologicalCleaner.H"
#include "boundaryLayers.H"
#include "renameBoundaryPatches.H"
#include "checkMeshDict.H"
#include "triSurfacePatchManipulator.H"
#include "refineBoundaryLayers.H"
#include "triSurfaceMetaData.H"
#include "polyMeshGenGeometryModification.H"
#include "surfaceMeshGeometryModification.H"

// * * * * * * * * * * * * Private member functions  * * * * * * * * * * * * //

void Foam::Module::tetMeshGenerator::createTetMesh()
{
    // create tet Mesh from octree and Delaunay tets
    tetMeshExtractorOctree tme(*octreePtr_, meshDict_, mesh_);

    tme.createMesh();
}


void Foam::Module::tetMeshGenerator::surfacePreparation()
{
    // removes unnecessary cells and morph the boundary
    // such that there is only one boundary face per cell
    // It also checks topology of cells after morphing is performed
    do
    {
        surfaceMorpherCells* cmPtr = new surfaceMorpherCells(mesh_);
        cmPtr->morphMesh();
        deleteDemandDrivenData(cmPtr);
    }
    while (topologicalCleaner(mesh_).cleanTopology());
}


void Foam::Module::tetMeshGenerator::mapMeshToSurface()
{
    // calculate mesh surface
    meshSurfaceEngine* msePtr = new meshSurfaceEngine(mesh_);

    // map mesh surface on the geometry surface
    meshSurfaceMapper(*msePtr, *octreePtr_).mapVerticesOntoSurface();

    // untangle surface faces
    meshSurfaceOptimizer(*msePtr, *octreePtr_).untangleSurface();

    deleteDemandDrivenData(msePtr);
}


void Foam::Module::tetMeshGenerator::extractPatches()
{
    edgeExtractor extractor(mesh_, *octreePtr_);

    Info<< "Extracting edges" << endl;
    extractor.extractEdges();

    extractor.updateMeshPatches();
}


void Foam::Module::tetMeshGenerator::mapEdgesAndCorners()
{
    meshSurfaceEdgeExtractorNonTopo(mesh_, *octreePtr_);
}


void Foam::Module::tetMeshGenerator::optimiseMeshSurface()
{
    meshSurfaceEngine mse(mesh_);
    meshSurfaceOptimizer(mse, *octreePtr_).optimizeSurface();
}


void Foam::Module::tetMeshGenerator::generateBoundaryLayers()
{
    if (meshDict_.found("boundaryLayers"))
    {
        boundaryLayers bl(mesh_);

        const dictionary& bndLayers = meshDict_.subDict("boundaryLayers");

        label nLayers;
        if (bndLayers.readIfPresent("nLayers", nLayers))
        {
            if (nLayers > 0)
            {
                bl.addLayerForAllPatches();
            }
        }
        else if (bndLayers.found("patchBoundaryLayers"))
        {
            const dictionary& patchLayers =
                bndLayers.subDict("patchBoundaryLayers");

            for (const word& patchName : patchLayers.toc())
            {
                bl.addLayerForPatch(patchName);
            }
        }
    }
}


void Foam::Module::tetMeshGenerator::optimiseFinalMesh()
{
    // final optimisation
    const bool enforceConstraints =
        meshDict_.lookupOrDefault<bool>
        (
            "enforceGeometryConstraints",
            false
        );

    meshOptimizer optimizer(mesh_);
    if (enforceConstraints)
    {
        optimizer.enforceConstraints();
    }

    optimizer.optimizeSurface(*octreePtr_);

    optimizer.optimizeMeshFV();
    optimizer.optimizeLowQualityFaces();
    optimizer.optimizeBoundaryLayer(false);
    optimizer.untangleMeshFV();

    deleteDemandDrivenData(octreePtr_);

    mesh_.clearAddressingData();

    if (modSurfacePtr_)
    {
        polyMeshGenGeometryModification meshMod(mesh_, meshDict_);

        // revert the mesh into the original space
        meshMod.revertGeometryModification();

        // delete modified surface mesh
        deleteDemandDrivenData(modSurfacePtr_);
    }
}


void Foam::Module::tetMeshGenerator::projectSurfaceAfterBackScaling()
{
    if (!meshDict_.found("anisotropicSources"))
        return;

    deleteDemandDrivenData(octreePtr_);
    octreePtr_ = new meshOctree(*surfacePtr_);

    meshOctreeCreator
    (
        *octreePtr_,
        meshDict_
    ).createOctreeWithRefinedBoundary(20, 30);

    // calculate mesh surface
    meshSurfaceEngine mse(mesh_);

    // pre-map mesh surface
    meshSurfaceMapper mapper(mse, *octreePtr_);

    // map mesh surface on the geometry surface
    mapper.mapVerticesOntoSurface();

    optimiseFinalMesh();
}


void Foam::Module::tetMeshGenerator::refBoundaryLayers()
{
    if (meshDict_.isDict("boundaryLayers"))
    {
        refineBoundaryLayers refLayers(mesh_);

        refineBoundaryLayers::readSettings(meshDict_, refLayers);

        refLayers.refineLayers();

        labelLongList pointsInLayer;
        refLayers.pointsInBndLayer(pointsInLayer);

        meshOptimizer opt(mesh_);
        opt.lockPoints(pointsInLayer);
        opt.untangleBoundaryLayer();
    }
}


void Foam::Module::tetMeshGenerator::replaceBoundaries()
{
    renameBoundaryPatches rbp(mesh_, meshDict_);
}


void Foam::Module::tetMeshGenerator::renumberMesh()
{
    polyMeshGenModifier(mesh_).renumberMesh();
}


void Foam::Module::tetMeshGenerator::generateMesh()
{
    if (controller_.runCurrentStep("templateGeneration"))
    {
        createTetMesh();
    }

    if (controller_.runCurrentStep("surfaceTopology"))
    {
        surfacePreparation();
    }

    if (controller_.runCurrentStep("surfaceProjection"))
    {
        mapMeshToSurface();
    }

    if (controller_.runCurrentStep("patchAssignment"))
    {
        extractPatches();
    }

    if (controller_.runCurrentStep("edgeExtraction"))
    {
        mapEdgesAndCorners();

        optimiseMeshSurface();
    }

    if (controller_.runCurrentStep("boundaryLayerGeneration"))
    {
        generateBoundaryLayers();
    }

    if (controller_.runCurrentStep("meshOptimisation"))
    {
        optimiseFinalMesh();

        projectSurfaceAfterBackScaling();
    }

    if (controller_.runCurrentStep("boundaryLayerRefinement"))
    {
        refBoundaryLayers();
    }

    renumberMesh();

    replaceBoundaries();

    controller_.workflowCompleted();
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::Module::tetMeshGenerator::tetMeshGenerator(const Time& time)
:
    runTime_(time),
    surfacePtr_(nullptr),
    modSurfacePtr_(nullptr),
    meshDict_
    (
        IOobject
        (
            "meshDict",
            runTime_.system(),
            runTime_,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    ),
    octreePtr_(nullptr),
    mesh_(time),
    controller_(mesh_)
{
    try
    {
        if (true)
        {
            checkMeshDict cmd(meshDict_);
        }

        const fileName surfaceFile(meshDict_.lookup("surfaceFile"));

        surfacePtr_ = new triSurf(runTime_.path()/surfaceFile);

        if (true)
        {
            // save meta data with the mesh (surface mesh + its topology info)
            triSurfaceMetaData sMetaData(*surfacePtr_);
            const dictionary& surfMetaDict = sMetaData.metaData();

            mesh_.metaData().add("surfaceFile", surfaceFile, true);
            mesh_.metaData().add("surfaceMeta", surfMetaDict, true);
        }

        if (surfacePtr_->featureEdges().size() != 0)
        {
            // create surface patches based on the feature edges
            // and update the meshDict based on the given data
            triSurfacePatchManipulator manipulator(*surfacePtr_);

            const triSurf* surfaceWithPatches =
                manipulator.surfaceWithPatches(&meshDict_);

            // delete the old surface and assign the new one
            deleteDemandDrivenData(surfacePtr_);
            surfacePtr_ = surfaceWithPatches;
        }

        if (meshDict_.found("anisotropicSources"))
        {
            surfaceMeshGeometryModification surfMod(*surfacePtr_, meshDict_);

            modSurfacePtr_ = surfMod.modifyGeometry();

            octreePtr_ = new meshOctree(*modSurfacePtr_);
        }
        else
        {
            octreePtr_ = new meshOctree(*surfacePtr_);
        }

        meshOctreeCreator(*octreePtr_, meshDict_).createOctreeBoxes();

        generateMesh();
    }
    catch (const std::string& message)
    {
        Info<< message << endl;
    }
    catch (...)
    {
        WarningInFunction
            << "Meshing process terminated!" << endl;
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::Module::tetMeshGenerator::~tetMeshGenerator()
{
    deleteDemandDrivenData(surfacePtr_);
    deleteDemandDrivenData(octreePtr_);
    deleteDemandDrivenData(modSurfacePtr_);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void Foam::Module::tetMeshGenerator::writeMesh() const
{
    mesh_.write();
}


// ************************************************************************* //
