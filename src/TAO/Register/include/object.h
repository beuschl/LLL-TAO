/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "act the way you'd like to be and soon you'll be the way you act" - Leonard Cohen

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_OBJECT_H
#define NEXUS_TAO_REGISTER_INCLUDE_OBJECT_H

#include <TAO/Register/include/state.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/stream.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Object Register
         *
         *  Manages type specific fields and meta data formatting for states.
         *
         **/
        class Object : public State
        {
            /** Special system level memory for managing system states in protected portion of memory **/
            std::vector<uint8_t> vchSystem;

        public:

            /** Internal map for managing object data members and their binary positions. **/
            mutable std::map< std::string, std::pair<uint16_t, bool> > mapData; //internal map for data members


            /** Default constructor. **/
            Object()
            : State()
            , vchSystem(512, 0) //system memory by default is 512 bytes
            , mapData()
            {
            }


            /** Copy Constructor. **/
            Object(const Object& object)
            : State(object)
            , vchSystem(object.vchSystem)
            , mapData(object.mapData)
            {
            }


            IMPLEMENT_SERIALIZE
            (
                READWRITE(nVersion);
                READWRITE(nType);
                READWRITE(hashOwner);
                READWRITE(nTimestamp);
                READWRITE(vchState);
                READWRITE(vchSystem);

                //checksum hash not serialized on gethash
                if(!(nSerType & SER_GETHASH))
                    READWRITE(hashChecksum);
            )


            /** Parse
             *
             *  Parses out the data members of an object register.
             *
             **/
            bool Parse();


            /** Read
             *
             *  Read a value form the object register.
             *
             *  @param[in] strName The name of the value to read
             *  @param[in] vData The data to read from the object.
             *
             *  @return True if the read was successful.
             *
             **/
            template<typename Type>
            bool Read(const std::string& strName, Type& value)
            {
                /* Check that the name exists in the object. */
                if(!mapData.count(strName))
                    return false;

                /* Find the binary position of value. */
                nReadPos = mapData[strName].first;

                /* Deserialize the type specifier. */
                uint8_t nType;
                *this >> nType;

                /* Check for unsupported type enums. */
                if(nType == TYPES::UNSUPPORTED)
                    return debug::error(FUNCTION, "unsupported type");

                /* Check the expected type from read. */
                if(type(value) != nType)
                    return debug::error(FUNCTION, "type mismatch");

                /* Deserialize the value. */
                *this >> value;

                return true;
            }


            /** Write
             *
             *  Write into the object register a value of type bytes.
             *
             *  @param[in] strName The name of the value to write
             *  @param[in] value The data to write into the object.
             *
             *  @return True if the write was successful.
             *
             **/
            template<typename Type>
            bool Write(const std::string& strName, const Type& value)
            {
                /* Check that the name exists in the object. */
                if(!mapData.count(strName))
                    return false;

                /* Check that the value is mutable (writes allowed). */
                if(!mapData[strName].second)
                    return debug::error(FUNCTION, "cannot set value for READONLY data member");

                /* Find the binary position of value. */
                nReadPos = mapData[strName].first;

                /* Deserialize the type specifier. */
                uint8_t nType;
                *this >> nType;

                /* Check for unsupported types. */
                if(nType == TYPES::UNSUPPORTED)
                    return debug::error(FUNCTION, "unsupported type");

                /* Check the type to helper templates. */
                if(type(value) != nType)
                    return debug::error(FUNCTION, "type mismatch");

                /* Get the expected size. */
                if(nReadPos + sizeof(value) >= vchState.size())
                    return debug::error(FUNCTION, "performing an over-write");

                /* CnTypey the bytes into the object. */
                std::copy((uint8_t*)&value, (uint8_t*)&value + sizeof(value), (uint8_t*)&vchState[nReadPos]);

                return true;
            }


            /** Write
             *
             *  Write into the object register a value of type bytes.
             *
             *  @param[in] strName The name of the value to write
             *  @param[in] strValue The data to write into the object.
             *
             *  @return True if the write was successful.
             *
             **/
            bool Write(const std::string& strName, const std::string& strValue);



            /** Write
             *
             *  Write into the object register a value of type bytes.
             *
             *  @param[in] strName The name of the value to write
             *  @param[in] vData The data to write into the object.
             *
             *  @return True if the write was successful.
             *
             **/
            bool Write(const std::string& strName, const std::vector<uint8_t>& vData);


        private:

            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint8_t n) const;

            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint16_t n) const;


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint32_t n) const;


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint64_t& n) const;


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint256_t& n) const;


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint512_t& n) const;


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint1024_t& n) const;


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const std::string& n) const;


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const std::vector<uint8_t>& n) const;


            /** type
             *
             *  Helper function to determine unsupported types that failed tempalte deduction.
             *
             **/
            template<typename Type>
            uint8_t type(const Type& n) const
            {
                return TYPES::UNSUPPORTED;
            }
        };

    }
}

#endif