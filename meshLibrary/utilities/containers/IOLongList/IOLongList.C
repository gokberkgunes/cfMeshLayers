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

Description
    An IOLongList of a given type is a list which supports automated
    input and output.

\*---------------------------------------------------------------------------*/

#include "IOLongList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class T, int Offset>
Foam::Module::IOLongList<T, Offset>::IOLongList(const IOobject& io)
:
    regIOobject(io),
    LongList<T, Offset>()
{
    if
    (
        io.readOpt() == IOobject::MUST_READ
     || (io.readOpt() == IOobject::READ_IF_PRESENT && headerOk())
    )
    {
        readStream(typeName) >> *this;
        close();
    }
}


template<class T, int Offset>
Foam::Module::IOLongList<T, Offset>::IOLongList
(
    const IOobject& io,
    const label size
)
:
    regIOobject(io),
    LongList<T, Offset>(size)
{}


template<class T, int Offset>
Foam::Module::Module::IOLongList<T, Offset>::IOLongList
(
    const IOobject& io,
    const LongList<T, Offset>& list
)
:
    regIOobject(io),
    LongList<T, Offset>()
{
    if (io.readOpt() == IOobject::READ_IF_PRESENT && headerOk())
    {
        readStream(typeName) >> *this;
        close();
    }

    LongList<T, Offset>::operator=(list);
}


template<class T, int Offset>
void Foam::Module::IOLongList<T, Offset>::operator=
(
    const IOLongList<T, Offset>& rhs
)
{
    LongList<T, Offset>::operator=(rhs);
}


template<class T, int Offset>
void Foam::Module::IOLongList<T, Offset>::operator=
(
    const LongList<T, Offset>& rhs
)
{
    LongList<T, Offset>::operator=(rhs);
}


template<class T, int Offset>
bool Foam::Module::IOLongList<T, Offset>::writeData(Ostream& os) const
{
    return (os << *this).good();
}


// ************************************************************************* //
