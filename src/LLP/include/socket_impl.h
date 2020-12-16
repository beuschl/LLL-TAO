/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_SOCKET_IMPL_H
#define NEXUS_LLP_INCLUDE_SOCKET_IMPL_H


#include <LLP/include/base_address.h>
#include <LLP/templates/socket.h>


#include <vector>
#include <cstdint>
#include <mutex>
#include <atomic>

#include <poll.h>

namespace LLP
{
    /** Socket
     *
     *  Base Template class to handle outgoing / incoming LLP data for both
     *  Client and Server.
     *
     **/
    class SocketImpl : public Socket
    {

        /** Mutex for thread synchronization. **/
        mutable std::mutex SOCKET_MUTEX;


        /* SSL object */
        SSL *pSSL;

    protected:

        /** Mutex to protect buffered data. **/
        mutable std::mutex DATA_MUTEX;


        /** Keep track of last time data was sent. **/
        std::atomic<std::uint64_t> nLastSend;


        /** Keep track of last time data was received. **/
        std::atomic<std::uint64_t> nLastRecv;


        /** The error codes for socket. **/
        std::atomic<std::int32_t> nError;


        /** Oversize buffer for large packets. **/
        std::vector<std::uint8_t> vBuffer;


        /** Flag to catch if buffer write failed. **/
        std::atomic<bool> fBufferFull;


    public:


        /** Flag to detect consecutive errors. **/
        std::atomic<std::uint32_t> nConsecutiveErrors;


        /** The address of this connection. */
        BaseAddress addr;


        pollfd POLL;


        /** The default constructor. **/
        SocketImpl();


        /** Copy constructor. **/
        SocketImpl(const SocketImpl& socket);


        /** The socket constructor. **/
        SocketImpl(std::int32_t nSocketIn, const BaseAddress &addrIn, bool fSSL = false);


        /** Constructor for socket
         *
         *  @param[in] addrConnect The address to connect socket to
         *
         **/
        SocketImpl(const BaseAddress &addrConnect, bool fSSL = false);


        /** Destructor for socket **/
        virtual ~SocketImpl();


        /** GetAddress
         *
         *  Returns the address of the socket.
         *
         **/
        BaseAddress GetAddress() const override;


        /** Reset
        *
        *  Resets the internal timers.
        *
        **/
        void Reset() override;


        /** Attempts
         *
         *  Attempts to connect the socket to an external address
         *
         *  @param[in] addrConnect The address to connect to
         *
         *  @return true if the socket is in a valid state.
         *
         **/
        bool Attempt(const BaseAddress &addrDest, std::uint32_t nTimeout = 3000) override;


        /** Available
         *
         *  Poll the socket to check for available data
         *
         *  @return the total bytes available for read
         *
         **/
        std::int32_t Available() const override;


        /** Close
         *
         *  Clear resources associated with socket and return to invalid state.
         *
         **/
        void Close() override;


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
        std::int32_t Read(std::vector<std::uint8_t>& vData, size_t nBytes) override;


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
        std::int32_t Read(std::vector<std::int8_t>& vchData, size_t nBytes) override;


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
        std::int32_t Write(const std::vector<std::uint8_t>& vData, size_t nBytes) override;


        /** Flush
         *
         *  Flushes data out of the overflow buffer
         *
         *  @return the total bytes that were written
         *
         **/
        std::int32_t Flush() override;


        /** Timeout
        *
        *  Determines if nTime seconds have elapsed since last Read / Write.
        *
        *  @param[in] nTime The time in seconds.
        *  @param[in] nFlags Flags to determine if checking reading or writing timeouts.
        *
        **/
        bool Timeout(std::uint32_t nTime, std::uint8_t nFlags = ALL) const override;


        /** Buffered
         *
         *  Get the amount of data buffered.
         *
         **/
        std::uint64_t Buffered() const override; 


        /** IsNull
         *
         *  Checks if is in null state.
         *
         **/
        bool IsNull() const override;


        /** Errors
         *
         *  Checks for any flags in the Error Handle.
         *
         **/
        bool Errors() const override;


        /** Error
         *
         *  Give the message (c-string) of the error in the socket.
         *
         **/
        const char* Error() const override;


        /** SetSSL
         *
         *  Creates or destroys the SSL object depending on the flag set.
         *
         *  @param[in] fSSL_ The flag to set SSL on or off.
         *
         **/
         void SetSSL(bool fSSL) override;


         /** IsSSL
          *
          * Determines if socket is using SSL encryption.
          *
          **/
          bool IsSSL() const override;


    private:

        /** error_code
         *
         *  Returns the error of socket if any
         *
         *  @return error code of the socket
         *
         **/
        std::int32_t error_code() const;

    };

}

#endif
