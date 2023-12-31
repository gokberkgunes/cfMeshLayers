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
    Foam::Module::boundaryPatch

Description
    Writes information for a given boundary patch

SourceFiles
    boundaryPatch.C

\*---------------------------------------------------------------------------*/

#ifndef boundaryPatch_H
#define boundaryPatch_H

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "boundaryPatchBase.H"
#include "typeInfo.H"

namespace Foam
{
namespace Module
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

class boundaryPatch
:
    public boundaryPatchBase
{
public:

    //- Runtime type information
    TypeName("patch");


    // Constructors

        boundaryPatch
        (
            const word&,
            const word&,
            const label,
            const label
        );

        boundaryPatch(const word& name, const dictionary&);


    // Return clone of the object

        virtual autoPtr<boundaryPatchBase> clone() const
        {
            return autoPtr<boundaryPatchBase>
            (
                new boundaryPatch
                (
                    patchName(),
                    patchType(),
                    patchSize(),
                    patchStart()
                )
            );
        }

        virtual autoPtr<boundaryPatchBase> clone(const boundaryPatch& wp) const
        {
            return autoPtr<boundaryPatchBase>
            (
                new boundaryPatch
                (
                    wp.patchName(),
                    wp.patchType(),
                    wp.patchSize(),
                    wp.patchStart()
                )
            );
        }


    //- Return as dictionary of entries
    dictionary dict() const;


    // Write

        //- write to Ostream
        void write(Ostream&) const;

        //- Write dictionary
        void writeDict(Ostream&) const;


    // Member Operators

        Ostream& operator<<(Ostream&) const;

        Istream& operator>>(Istream&);

        bool operator!=(const boundaryPatch&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Module
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
