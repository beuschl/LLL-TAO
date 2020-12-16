/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_TEMPLATES_SOCKET_H
#define NEXUS_LLP_TEMPLATES_SOCKET_H


#include <LLP/include/base_address.h>

#include <vector>
#include <cstdint>
#include <poll.h>


typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

namespace LLP
{

    /** Max send buffer size. **/
    const uint64_t MAX_SEND_BUFFER = 3 * 1024 * 1024; //3MB max send buffer


    /** Socket
     *
     *  Base Template class to handle outgoing / incoming LLP data for both
     *  Client and Server.
     *
     **/
    class Socket
    {
    public:

        /** Timeout flags. **/
        enum
        {
            READ  = (1 << 1),
            WRITE = (1 << 2),
            ALL   = (READ | WRITE)
        };


        /** Destructor for socket **/
        virtual ~Socket();


        /** GetAddress
         *
         *  Returns the address of the socket.
         *
         **/
        virtual BaseAddress GetAddress() const = 0;


        /** Reset
        *
        *  Resets the internal timers.
        *
        **/
        virtual void Reset() = 0;


        /** Attempts
         *
         *  Attempts to connect the socket to an external address
         *
         *  @param[in] addrConnect The address to connect to
         *
         *  @return true if the socket is in a valid state.
         *
         **/
        virtual bool Attempt(const BaseAddress &addrDest, std::uint32_t nTimeout = 3000) = 0;


        /** Available
         *
         *  Poll the socket to check for available data
         *
         *  @return the total bytes available for read
         *
         **/
        virtual std::int32_t Available() const = 0;


        /** Close
         *
         *  Clear resources associated with socket and return to invalid state.
         *
         **/
        virtual void Close() = 0;


        /** Read
         *
         *  Read data from the socket buffer non-blocking
         *
         *  @param[out] vData The byte vector to read into
         *  @param[in] nBytes The total bytes to read
         *
         *  @return the total bytes that were read
         *
         **/
        virtual std::int32_t Read(std::vector<std::uint8_t>& vData, size_t nBytes) = 0;


        /** Read
         *
         *  Read data from the socket buffer non-blocking
         *
         *  @param[out] vchData The char vector to read into
         *  @param[in] nBytes The total bytes to read
         *
         *  @return the total bytes that were read
         *
         **/
        virtual std::int32_t Read(std::vector<std::int8_t>& vchData, size_t nBytes) = 0;


        /** Write
         *
         *  Write data into the socket buffer non-blocking
         *
         *  @param[in] vData The byte vector of data to be written
         *  @param[in] nBytes The total bytes to write
         *
         *  @return the total bytes that were written
         *
         **/
        virtual std::int32_t Write(const std::vector<std::uint8_t>& vData, size_t nBytes) = 0;


        /** Flush
         *
         *  Flushes data out of the overflow buffer
         *
         *  @return the total bytes that were written
         *
         **/
        virtual std::int32_t Flush() = 0;


        /** Timeout
        *
        *  Determines if nTime seconds have elapsed since last Read / Write.
        *
        *  @param[in] nTime The time in seconds.
        *  @param[in] nFlags Flags to determine if checking reading or writing timeouts.
        *
        **/
        virtual bool Timeout(std::uint32_t nTime, std::uint8_t nFlags = ALL) const = 0;


        /** Buffered
         *
         *  Get the amount of data buffered.
         *
         **/
        virtual std::uint64_t Buffered() const = 0;


        /** IsNull
         *
         *  Checks if is in null state.
         *
         **/
        virtual bool IsNull() const = 0;


        /** Errors
         *
         *  Checks for any flags in the Error Handle.
         *
         **/
        virtual bool Errors() const = 0;


        /** Error
         *
         *  Give the message (c-string) of the error in the socket.
         *
         **/
        virtual const char* Error() const = 0;


        /** SetSSL
         *
         *  Creates or destroys the SSL object depending on the flag set.
         *
         *  @param[in] fSSL_ The flag to set SSL on or off.
         *
         **/
         virtual void SetSSL(bool fSSL) = 0;


         /** IsSSL
          *
          * Determines if socket is using SSL encryption.
          *
          **/
          virtual bool IsSSL() const = 0;

    };

}

#endif
