/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/base_address.h>
#include <LLP/templates/data.h>

#include <LLP/templates/socket.h>

#include <LLP/types/tritium.h>
#include <LLP/types/time.h>
#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>
#include <LLP/types/p2p.h>

#include <Util/include/hex.h>


namespace LLP
{

    /** Default Constructor **/
    template <class ProtocolType>
    DataThread<ProtocolType>::DataThread(uint32_t nID, bool ffDDOSIn,
                                         uint32_t rScore, uint32_t cScore,
                                         uint32_t nTimeout, bool fMeter)
    : SLOT_MUTEX      ( )
    , fDDOS           (ffDDOSIn)
    , fMETER          (fMeter)
    , fDestruct       (false)
    , nIncoming       (0)
    , nOutbound       (0)
    , ID              (nID)
    , TIMEOUT         (nTimeout)
    , DDOS_rSCORE     (rScore)
    , DDOS_cSCORE     (cScore)
    , CONNECTIONS     (memory::atomic_ptr< std::vector<memory::atomic_ptr<ProtocolType>> >(new std::vector<memory::atomic_ptr<ProtocolType>>()))
    , RELAY           (memory::atomic_ptr< std::queue<std::pair<typename ProtocolType::message_t, DataStream>> >(new std::queue<std::pair<typename ProtocolType::message_t, DataStream>>()))
    , CONDITION       ( )
    , DATA_THREAD     (std::bind(&DataThread::Thread, this))
    , FLUSH_CONDITION ( )
    , FLUSH_THREAD    (std::bind(&DataThread::Flush, this))
    {
    }


    /** Default Destructor **/
    template <class ProtocolType>
    DataThread<ProtocolType>::~DataThread()
    {
        fDestruct = true;
        CONDITION.notify_all();
        if(DATA_THREAD.joinable())
            DATA_THREAD.join();

        FLUSH_CONDITION.notify_all();
        if(FLUSH_THREAD.joinable())
            FLUSH_THREAD.join();

        CONNECTIONS.free();
        RELAY.free();
    }


    /*  Disconnects all connections by issuing a DISCONNECT::FORCE event message
     *  and then removes the connection from this data thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::DisconnectAll()
    {
        /* Iterate through connections to remove. When call on destruct, simply remove the connection. Otherwise,
         * force a disconnect event. This will inform address manager so it knows to attempt new connections.
         */
        for(uint32_t nIndex = 0; nIndex < CONNECTIONS->size(); ++nIndex)
        {
            if(!fDestruct.load())
                disconnect_remove_event(nIndex, DISCONNECT::FORCE);
            else
                remove(nIndex);
        }
    }


    /*  Thread that handles all the Reading / Writing of Data from Sockets.
     *  Creates a Packet QUEUE on this connection to be processed by an
     *  LLP Messaging Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::Thread()
    {
        /* Cache sleep time if applicable. */
        uint32_t nSleep = config::GetArg("-llpsleep", 0);

        /* The mutex for the condition. */
        std::mutex CONDITION_MUTEX;

        /* This mirrors CONNECTIONS with pollfd settings for passing to poll methods.
         * Windows throws SOCKET_ERROR intermittently if pass CONNECTIONS directly.
         */
        std::vector<pollfd> POLLFDS;

        /* The main connection handler loop. */
        while(!fDestruct.load() && !config::fShutdown.load())
        {
            /* Check for data thread sleep (helps with cpu usage). */
            if(nSleep > 0)
                runtime::sleep(nSleep);

            /* Keep data threads waiting for work.
             * Will wait until have one or more connections, DataThread is disposed, or system shutdown
             * While loop catches potential for spurious wakeups. Also has the effect of skipping the wait() call after connections established.
             */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK,
            [this]
            {
                return fDestruct.load()
                || config::fShutdown.load()
                || nIncoming.load() > 0
                || nOutbound.load() > 0;
            });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
                return;

            /* Wrapped mutex lock. */
            uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());

            /* Check the pollfd's size. */
            if(POLLFDS.size() != nSize)
                POLLFDS.resize(nSize);

            /* Initialize the revents for all connection pollfd structures.
             * One connection must be live, so verify that and skip if none
             */
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Set the proper POLLIN flags. */
                    POLLFDS.at(nIndex).events  = POLLIN;// | POLLRDHUP;
                    POLLFDS.at(nIndex).revents = 0; //reset return events

                    /* Set to invalid socket if connection is inactive. */
                    if(!CONNECTIONS->at(nIndex))
                    {
                        POLLFDS.at(nIndex).fd = INVALID_SOCKET;

                        continue;
                    }

                    /* Set the correct file descriptor. */
                    POLLFDS.at(nIndex).fd = CONNECTIONS->at(nIndex)->POLL.fd;
                }
                catch(const std::exception& e)
                {
                    debug::error(FUNCTION, e.what());
                }
            }

            /* Poll the sockets. */
#ifdef WIN32
            int32_t nPoll = WSAPoll((pollfd*)&POLLFDS[0], nSize, 100);
#else
            int32_t nPoll = poll((pollfd*)&POLLFDS[0], nSize, 100);
#endif

            /* Check poll for available sockets. */
            if(nPoll < 0)
            {
                runtime::sleep(1);
                continue;
            }


            /* Check all connections for data and packets. */
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Load the atomic pointer raw data. */
                    ProtocolType* CONNECTION = CONNECTIONS->at(nIndex).load();

                    /* Skip over Inactive Connections. */
                    if(!CONNECTION || !CONNECTION->Connected())
                        continue;

                    /* Disconnect if there was a polling error */
                    if(POLLFDS.at(nIndex).revents & POLLERR)
                    {
                         disconnect_remove_event(nIndex, DISCONNECT::POLL_ERROR);
                         continue;
                    }

                    /* Disconnect if the socket was disconnected by peer (need for Windows) */
                    if(POLLFDS.at(nIndex).revents & POLLHUP)
                    {
                        disconnect_remove_event(nIndex, DISCONNECT::PEER);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any read/write Errors. */
                    if(CONNECTION->Errors())
                    {
                        disconnect_remove_event(nIndex, DISCONNECT::ERRORS);
                        continue;
                    }

                    /* Remove Connection if it has Timed out or had any Errors. */
                    if(CONNECTION->Timeout(TIMEOUT * 1000, Socket::READ))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT::TIMEOUT);
                        continue;
                    }

                    /* Disconnect if pollin signaled with no data (This happens on Linux). */
                    if((POLLFDS.at(nIndex).revents & POLLIN)
                    && CONNECTION->Available() == 0 && !CONNECTION->IsSSL())
                    {
                        disconnect_remove_event(nIndex, DISCONNECT::POLL_EMPTY);
                        continue;
                    }

                    /* Disconnect if buffer is full and remote host isn't reading at all. */
                    if(CONNECTION->Buffered()
                    && CONNECTION->Timeout(15000, Socket::WRITE))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT::TIMEOUT_WRITE);
                        continue;
                    }

                    /* Check that write buffers aren't overflowed. */
                    if(CONNECTION->Buffered() > config::GetArg("-maxsendbuffer", MAX_SEND_BUFFER))
                    {
                        disconnect_remove_event(nIndex, DISCONNECT::BUFFER);
                        continue;
                    }

                    /* Handle any DDOS Filters. */
                    if(fDDOS && CONNECTION->DDOS)
                    {
                        /* Ban a node if it has too many Requests per Second. **/
                        if(CONNECTION->DDOS->rSCORE.Score() > DDOS_rSCORE
                        || CONNECTION->DDOS->cSCORE.Score() > DDOS_cSCORE)
                            CONNECTION->DDOS->Ban();

                        /* Remove a connection if it was banned by DDOS Protection. */
                        if(CONNECTION->DDOS->Banned())
                        {
                            debug::log(0, ProtocolType::Name(), " BANNED: ", CONNECTION->GetAddress().ToString());
                            disconnect_remove_event(nIndex, DISCONNECT::DDOS);
                            continue;
                        }
                    }

                    /* Generic event for Connection. */
                    CONNECTION->Event(EVENTS::GENERIC);

                    /* Work on Reading a Packet. **/
                    CONNECTION->ReadPacket();

                    /* If a Packet was received successfully, increment request count [and DDOS count if enabled]. */
                    if(CONNECTION->PacketComplete())
                    {
                        /* Debug dump of message type. */
                        if(config::nVerbose.load() >= 4)
                            debug::log(4, FUNCTION, "Received Message (", CONNECTION->INCOMING.GetBytes().size(), " bytes)");

                        /* Debug dump of packet data. */
                        if(config::nVerbose.load() >= 5)
                            PrintHex(CONNECTION->INCOMING.GetBytes());

                        /* Handle Meters and DDOS. */
                        if(fMETER)
                            ++ProtocolType::REQUESTS;

                        /* Increment rScore. */
                        if(fDDOS && CONNECTION->DDOS)
                            CONNECTION->DDOS->rSCORE += 1;

                        /* Packet Process return value of False will flag Data Thread to Disconnect. */
                        if(!CONNECTION->ProcessPacket())
                        {
                            disconnect_remove_event(nIndex, DISCONNECT::FORCE);
                            continue;
                        }

                        /* Run procssed event for connection triggers. */
                        CONNECTION->Event(EVENTS::PROCESSED);
                        CONNECTION->ResetPacket();
                    }
                }
                catch(const std::exception& e)
                {
                    debug::error(FUNCTION, "Data Connection: ", e.what());
                    disconnect_remove_event(nIndex, DISCONNECT::ERRORS);
                }
            }
        }
    }


    /*  Thread that handles all the Reading / Writing of Data from Sockets.
     *  Creates a Packet QUEUE on this connection to be processed by an
     *  LLP Messaging Thread. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::Flush()
    {
        /* The mutex for the condition. */
        std::mutex CONDITION_MUTEX;

        /* The main connection handler loop. */
        while(!fDestruct.load() && !config::fShutdown.load())
        {
            /* Keep data threads waiting for work.
             * Will wait until have one or more connections, DataThread is disposed, or system shutdown
             * While loop catches potential for spurious wakeups. Also has the effect of skipping the wait() call after connections established.
             */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            FLUSH_CONDITION.wait(CONDITION_LOCK,
                [this]{

                    /* Break on shutdown or destructor. */
                    if(fDestruct.load() || config::fShutdown.load())
                        return true;

                    /* Check for data in the queue. */
                    if(!RELAY->empty())
                        return true;

                    /* Check for buffered connection. */
                    for(uint32_t nIndex = 0; nIndex < CONNECTIONS->size(); ++nIndex)
                    {
                        try
                        {
                            /* Check for buffered connection. */
                            if(CONNECTIONS->at(nIndex)->Buffered())
                                return true;
                        }
                        catch(const std::exception& e) { }
                    }

                    return false;
                });

            /* Check for close. */
            if(fDestruct.load() || config::fShutdown.load())
                return;

            /* Pair to store the relay from the queue. */
            std::pair<typename ProtocolType::message_t, DataStream> qRelay =
                std::make_pair(typename ProtocolType::message_t(), DataStream(SER_NETWORK, MIN_PROTO_VERSION));

            /* Grab data from queue. */
            if(!RELAY->empty())
            {
                /* Make a copy of the relay data. */
                qRelay = RELAY->front();
                RELAY->pop();
            }

            /* Check all connections for data and packets. */
            uint32_t nSize = CONNECTIONS->size();
            for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
            {
                try
                {
                    /* Reset stream read position. */
                    qRelay.second.Reset();

                    /* Get atomic pointer to reduce locking around CONNECTIONS scope. */
                    memory::atomic_ptr<ProtocolType>& CONNECTION = CONNECTIONS->at(nIndex);

                    /* Relay if there are active subscriptions. */
                    const DataStream ssRelay = CONNECTION->RelayFilter(qRelay.first, qRelay.second);
                    if(ssRelay.size() != 0)
                    {
                        /* Build the sender packet. */
                        typename ProtocolType::packet_t PACKET = typename ProtocolType::packet_t(qRelay.first);
                        PACKET.SetData(ssRelay);

                        /* Write packet to socket. */
                        CONNECTION->WritePacket(PACKET);
                    }

                    /* Attempt to flush data when buffer is available. */
                    if(CONNECTION->Buffered() && CONNECTION->Flush() < 0)
                        runtime::sleep(std::min(5u, CONNECTION->nConsecutiveErrors.load() / 1000)); //we want to sleep when we have periodic failures
                }
                catch(const std::exception& e) { }
            }
        }
    }


    /* Tell the data thread an event has occured and notify each connection. */
    template<class ProtocolType>
    void DataThread<ProtocolType>::NotifyEvent()
    {
        /* Loop through each connection. */
        uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            try { CONNECTIONS->at(nIndex)->NotifyEvent(); }
            catch(const std::exception& e) { }
        }
    }


    /* Get the number of active connection pointers from data threads. */
    template <class ProtocolType>
    uint32_t DataThread<ProtocolType>::GetConnectionCount(const uint8_t nFlags)
    {
        /* Check for incoming connections. */
        uint32_t nConnections = 0;
        if(nFlags & FLAGS::INCOMING)
            nConnections += nIncoming.load();

        /* Check for outgoing connections. */
        if(nFlags & FLAGS::OUTGOING)
            nConnections += nOutbound.load();

        return nConnections;
    }


    /* Fires off a Disconnect event with the given disconnect reason and also removes the data thread connection. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::disconnect_remove_event(uint32_t nIndex, uint8_t nReason)
    {
        ProtocolType* raw = CONNECTIONS->at(nIndex).load(); //we use raw pointer here because event could contain switch node that will cause deadlocks
        raw->Event(EVENTS::DISCONNECT, nReason);

        remove(nIndex);
    }


    /* Removes given connection from current Data Thread. This happens on timeout/error, graceful close, or disconnect command. */
    template <class ProtocolType>
    void DataThread<ProtocolType>::remove(uint32_t nIndex)
    {
        LOCK(SLOT_MUTEX);

        /* Check for inbound socket. */
        if(CONNECTIONS->at(nIndex)->Incoming())
            --nIncoming;
        else
            --nOutbound;

        /* Free the memory. */
        CONNECTIONS->at(nIndex).free();
        CONDITION.notify_all();
    }


    /* Returns the index of a component of the CONNECTIONS vector that has been flagged Disconnected */
    template <class ProtocolType>
    uint32_t DataThread<ProtocolType>::find_slot()
    {
        /* Loop through each connection. */
        uint32_t nSize = static_cast<uint32_t>(CONNECTIONS->size());
        for(uint32_t nIndex = 0; nIndex < nSize; ++nIndex)
        {
            if(!CONNECTIONS->at(nIndex))
                return nIndex;
        }

        return nSize;
    }


    /* Explicity instantiate all template instances needed for compiler. */
    template class DataThread<TritiumNode>;
    template class DataThread<TimeNode>;
    template class DataThread<APINode>;
    template class DataThread<RPCNode>;
    template class DataThread<Miner>;
    template class DataThread<P2PNode>;
}
