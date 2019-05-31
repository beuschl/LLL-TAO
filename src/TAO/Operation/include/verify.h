/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_INCLUDE_VERIFY_H
#define NEXUS_TAO_OPERATION_INCLUDE_VERIFY_H

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{
    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Forward declarations. */
        class Contract;


        /** Execute
         *
         *  Verfies a given contract post-states
         *
         *  @param[in] contract The contract to execute
         *  @param[in] nFlags The flags to execute with.
         *
         *  @return True if operations verfied successfully, false otherwise.
         *
         **/
        bool Verify(const Contract& contract, const uint8_t nFlags);

    }
}

#endif