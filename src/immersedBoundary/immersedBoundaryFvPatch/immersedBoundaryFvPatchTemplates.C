/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

\*---------------------------------------------------------------------------*/

#include "immersedBoundaryFvPatch.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::FieldField<Foam::Field, Type> >
Foam::immersedBoundaryFvPatch::sendAndReceive
(
    const Field<Type>& psi
) const
{
    tmp<FieldField<Field, Type> > tprocPsi
    (
        new FieldField<Field, Type>(Pstream::nProcs())
    );
    FieldField<Field, Type>& procPsi = tprocPsi();

    forAll (procPsi, procI)
    {
        procPsi.set
        (
            procI,
            new Field<Type>
            (
                ibProcCentres()[procI].size(),
                pTraits<Type>::zero
            )
        );
    }

    if (Pstream::parRun())
    {
        // Send
        for (label procI = 0; procI < Pstream::nProcs(); procI++)
        {
            if (procI != Pstream::myProcNo())
            {
                // Do not send empty lists
                if (!ibProcCells()[procI].empty())
                {
                    Field<Type> curPsi(psi, ibProcCells()[procI]);

                    // Parallel data exchange
                    OPstream toProc
                    (
                        Pstream::blocking,
                        procI,
                        curPsi.size()*sizeof(Type)
                    );

                    toProc << curPsi;
                }
            }
        }

        // Receive
        for (label procI = 0; procI < Pstream::nProcs(); procI++)
        {
            if (procI != Pstream::myProcNo())
            {
                // Do not receive empty lists
                if (!procPsi[procI].empty())
                {
                    // Parallel data exchange
                    IPstream fromProc
                    (
                        Pstream::blocking,
                        procI,
                        procPsi[procI].size()*sizeof(Type)
                    );

                    fromProc >> procPsi[procI];
                }
            }
        }
    }

    return tprocPsi;
}


template<class Type>
Foam::tmp<Foam::Field<Type> >
Foam::immersedBoundaryFvPatch::toIbPoints
(
    const Field<Type>& triValues
) const
{
    if (triValues.size() != ibMesh().size())
    {
        FatalErrorIn
        (
            "template<class Type>\n"
            "Foam::tmp<Foam::Field<Type> >\n"
            "immersedBoundaryFvPatch::toIbPoints\n"
            "(\n"
            "    const Field<Type>& triValues\n"
            ") const"
        )   << "Field size does not correspond to size of immersed boundary "
            << "triangulated surface for patch " << name() << nl
            << "Field size = " << triValues.size()
            << " surface size = " << ibMesh().size()
            << abort(FatalError);
    }

    const labelList& ibc = ibCells();

    tmp<Field<Type> > tIbPsi
    (
        new Field<Type>(ibc.size(), pTraits<Type>::zero)
    );
    Field<Type>& ibPsi = tIbPsi();

    const labelList& hf = hitFaces();

    forAll (ibPsi, cellI)
    {
        ibPsi[cellI] = triValues[hf[cellI]];
    }

    return tIbPsi;
}


template<class Type>
Foam::tmp<Foam::Field<Type> >
Foam::immersedBoundaryFvPatch::toIbPoints
(
    const tmp<Field<Type> >& ttriValues
) const
{
    tmp<Field<Type> > tint = toIbPoints(ttriValues());
    ttriValues.clear();
    return tint;

}


template<class Type>
Foam::tmp<Foam::Field<Type> >
Foam::immersedBoundaryFvPatch::toTriFaces
(
    const Field<Type>& ibValues
) const
{
    if (ibValues.size() != ibCells().size())
    {
        FatalErrorIn
        (
            "template<class Type>\n"
            "Foam::tmp<Foam::Field<Type> >\n"
            "immersedBoundaryFvPatch::toTriFaces\n"
            "(\n"
            "    const Field<Type>& ibValues\n"
            ") const"
        )   << "Field size does not correspond to size of IB points "
            << "triangulated surface for patch " << name() << nl
            << "Field size = " << size()
            << " IB points size = " << ibCells().size()
            << abort(FatalError);
    }

    const labelListList& ctfAddr = cellsToTriAddr();
    const scalarListList& ctfWeights = cellsToTriWeights();

    tmp<Field<Type> > tIbPsi
    (
        new Field<Type>(ctfAddr.size(), pTraits<Type>::zero)
    );
    Field<Type>& ibPsi = tIbPsi();

    // Do interpolation
    forAll (ctfAddr, triI)
    {
        const labelList& curAddr = ctfAddr[triI];
        const scalarList& curWeights = ctfWeights[triI];

        forAll (curAddr, cellI)
        {
            ibPsi[triI] += curWeights[cellI]*ibValues[curAddr[cellI]];
        }
    }

    return tIbPsi;
}


template<class Type>
Foam::tmp<Foam::Field<Type> >
Foam::immersedBoundaryFvPatch::toTriFaces
(
    const tmp<Field<Type> >& tibValues
) const
{
    tmp<Field<Type> > tint = toTriFaces(tibValues());
    tibValues.clear();
    return tint;

}

// ************************************************************************* //
