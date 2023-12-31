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

#include "lineRefinement.H"
#include "addToRunTimeSelectionTable.H"
#include "boundBox.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace Module
{
    defineTypeNameAndDebug(lineRefinement, 0);
    addToRunTimeSelectionTable(objectRefinement, lineRefinement, dictionary);
}
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::Module::lineRefinement::lineRefinement()
:
    objectRefinement(),
    p0_(),
    p1_()
{}


Foam::Module::lineRefinement::lineRefinement
(
    const word& name,
    const scalar cellSize,
    const direction additionalRefLevels,
    const point& p0,
    const point& p1
)
:
    objectRefinement(),
    p0_(p0),
    p1_(p1)
{
    setName(name);
    setCellSize(cellSize);
    setAdditionalRefinementLevels(additionalRefLevels);
}


Foam::Module::lineRefinement::lineRefinement
(
    const word& name,
    const dictionary& dict
)
:
    objectRefinement(name, dict)
{
    this->operator=(dict);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

bool Foam::Module::lineRefinement::intersectsObject(const boundBox& bb) const
{
    // check if the cube contains start point or end point
    const scalar l = bb.max().x() - bb.min().x();

    const point min = bb.min() - l*vector(SMALL, SMALL, SMALL);
    const point max = bb.max() + l*vector(SMALL, SMALL, SMALL);

    // check for intersections of line with the cube faces
    const vector v(p1_ - p0_);
    if (mag(v.x()) > SMALL)
    {
        if
        (
            ((p0_.x() < min.x()) && (p1_.x() < min.x()))
         || ((p0_.x() > max.x()) && (p1_.x() > max.x()))
        )
            return false;

        {
            // x-min face
            const vector n(-1, 0, 0);
            const scalar t = (n & (min - p0_))/(n & v);
            const point i = p0_ + t*v;
            if
            (
                (t > -SMALL) && (t <(1.0 + SMALL))
            )
                if
                (
                    (i.y() > min.y()) && (i.y() < max.y())
                 && (i.z() > min.z()) && (i.z() < max.z())
                )
                    return true;
        }
        {
            // x-max face
            const vector n(1, 0, 0);
            const scalar t = (n & (max - p0_))/(n & v);
            const point i = p0_ + t*v;
            if
            (
                (t > -SMALL) && (t <(1.0 + SMALL))
            )
                if
                (
                    (i.y() > min.y()) && (i.y() < max.y())
                 && (i.z() > min.z()) && (i.z() < max.z())
                )
                    return true;
        }
    }

    if (mag(v.y()) > SMALL)
    {
        if
        (
            ((p0_.y() < min.y()) && (p1_.y() < min.y()))
         || ((p0_.y() > max.y()) && (p1_.y() > max.y()))
        )
            return false;

        {
            // y-min face
            const vector n(0, -1, 0);
            const scalar t = (n & (min - p0_))/(n & v);
            const point i = p0_ + t*v;
            if
            (
                (t > -SMALL) && (t <(1.0 + SMALL))
            )
                if
                (
                    (i.x() > min.x()) && (i.x() < max.x())
                 && (i.z() > min.z()) && (i.z() < max.z())
                )
                    return true;
        }
        {
            // y-max face
            const vector n(0, 1, 0);
            const scalar t = (n & (max - p0_))/(n & v);
            const point i = p0_ + t*v;
            if
            (
                (t > -SMALL) && (t <(1.0 + SMALL))
            )
                if
                (
                    (i.x() > min.x()) && (i.x() < max.x())
                 && (i.z() > min.z()) && (i.z() < max.z())
                )
                    return true;
        }
    }
    if (mag(v.z()) > SMALL)
    {
        if
        (
            ((p0_.z() < min.z()) && (p1_.z() < min.z()))
         || ((p0_.z() > max.z()) && (p1_.z() > max.z()))
        )
            return false;

        {
            // z-min face
            const vector n(0, 0, -1);
            const scalar t = (n & (min - p0_))/(n & v);
            const point i = p0_ + t*v;
            if
            (
                (t > -SMALL) && (t <(1.0 + SMALL))
             && (i.x() > min.x()) && (i.x() < max.x())
             && (i.y() > min.y()) && (i.y() < max.y())
            )
                return true;
        }
        {
            // z-min face
            const vector n(0, 0, 1);
            const scalar t = (n & (max - p0_))/(n & v);
            const point i = p0_ + t*v;
            if
            (
                (t > -SMALL) && (t <(1.0 + SMALL))
             && (i.x() > min.x()) && (i.x() < max.x())
             && (i.y() > min.y()) && (i.y() < max.y())
            )
                return true;
        }
    }

    if
    (
        (p0_.x() > min.x()) && (p0_.x() < max.x())
     && (p0_.y() > min.y()) && (p0_.y() < max.y())
     && (p0_.z() > min.z()) && (p0_.z() < max.z())
    )
        return true;

    return false;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

Foam::dictionary Foam::Module::lineRefinement::dict(bool /*ignoreType*/) const
{
    dictionary dict;

    if (additionalRefinementLevels() == 0 && cellSize() >= 0.0)
    {
        dict.add("cellSize", cellSize());
    }
    else
    {
        dict.add("additionalRefinementLevels", additionalRefinementLevels());
    }

    dict.add("type", type());

    dict.add("p0", p0_);
    dict.add("p1", p1_);

    return dict;
}


void Foam::Module::lineRefinement::write(Ostream& os) const
{
    os  << " type:   " << type()
        << " p0: " << p0_
        << " p1: " << p1_;
}


void Foam::Module::lineRefinement::writeDict(Ostream& os, bool subDict) const
{
    if (subDict)
    {
        os << indent << token::BEGIN_BLOCK << incrIndent << nl;
    }

    if (additionalRefinementLevels() == 0 && cellSize() >= 0.0)
    {
        os.writeKeyword("cellSize") << cellSize() << token::END_STATEMENT << nl;
    }
    else
    {
        os.writeKeyword("additionalRefinementLevels")
            << additionalRefinementLevels()
            << token::END_STATEMENT << nl;
    }

    // only write type for derived types
    if (type() != typeName_())
    {
        os.writeKeyword("type") << type() << token::END_STATEMENT << nl;
    }

    os.writeKeyword("p0") << p0_ << token::END_STATEMENT << nl;
    os.writeKeyword("p1") << p1_ << token::END_STATEMENT << nl;

    if (subDict)
    {
        os << decrIndent << indent << token::END_BLOCK << endl;
    }
}


void Foam::Module::lineRefinement::operator=(const dictionary& d)
{
    // allow as embedded sub-dictionary "coordinateSystem"
    const dictionary& dict =
    (
        d.found(typeName_())
      ? d.subDict(typeName_())
      : d
    );

    // unspecified centre is (0 0 0)
    if (!dict.readIfPresent("p0", p0_))
    {
        FatalErrorInFunction
            << "Entry p0 is not specified!" << exit(FatalError);

        p0_ = vector::zero;
    }

    if (!dict.readIfPresent("p1", p1_))
    {
        FatalErrorInFunction
            << "Entry p1 is not specified!" << exit(FatalError);

        p1_ = vector::zero;
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

Foam::Ostream& Foam::Module::lineRefinement::operator<<
(
    Ostream& os
) const
{
    os << "name " << name() << nl;
    os << "cell size " << cellSize() << nl;
    os << "additionalRefinementLevels " << additionalRefinementLevels() << endl;

    write(os);
    return os;
}


// ************************************************************************* //
