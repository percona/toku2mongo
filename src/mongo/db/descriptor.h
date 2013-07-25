/** @file index_descriptor.h */

/**
*    Copyright (C) 2013 Tokutek Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "mongo/pch.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/storage/key.h"

namespace mongo {

    // A Descriptor contains the necessary information for comparing
    // and generating index keys and values.
    //
    // Descriptors are stored on disk as a ydb dictionary's DESCRIPTOR (excuse the screaming typedef).
    //
    // It provides the API glue for the ydb's environment-wide comparison
    // function as well as the generate rows for put/del/update functions.

    class Descriptor {
    public:
        // For creating a brand new descriptor.
        Descriptor(const BSONObj &keyPattern,
                   const bool hashed = false,
                   const int hashSeed = 0,
                   const bool sparse = false,
                   const bool clustering = false);
        // For interpretting a memory buffer as a descriptor.
        Descriptor(const char *data, const size_t size);

        static size_t serializedSize(const BSONObj &keyPattern);

        void assertEqual(const Descriptor &rhs) const;

        DBT dbt() const;

        int compareKeys(const storage::Key &key1, const storage::Key &key2) const;

        void generateKeys(const BSONObj &obj, BSONObjSet &keys) const;

        bool clustering() const {
            const Header &h(*reinterpret_cast<const Header *>(_data));
            return h.clustering;
        }


    private:
#pragma pack(1)
        // Descriptor format:
        //   [
        //     4 byte ordering, 1 byte for version, 1 byte for hashed,
        //     1 byte for sparse, 1 byte for clustering,
        //     4 bytes num fields, 4 bytes hash key, array of 4 byte offsets,
        //     arena of null-terminated field names
        //   ]
        //
        // TODO: This is gross. The header for each index d escriptor should be
        // generated by some implementation of IndexDetails (note: nothing is
        // virtualized in IndexDetails just yet...) instead of just all thrown
        // together in one format here.
        struct Header {
            Header(const Ordering &o, char h, char s, char c, int hs, uint32_t n)
                : ordering(o), version(0), hashed(h), sparse(s), clustering(c),
                  hashSeed(hs), numFields(n) {
            }

            void checkVersion() const {
                if (version != CURRENT_VERSION) {
                    problem() << "Detected unsupported dictionary descriptor version: " << version << "."
                              << "To use this data with this version of TokuMX, restart with the version "
                              << "that created this data and mongodump/restore into the newer version. You "
                              << "could also just synchronize a secondary replica member using the newer "
                              << "version (preferred)."
                              << endl << endl
                              << "The assertion failure you are about to see is intentional."
                              << endl;
                }
                verify(version == CURRENT_VERSION);
            }

            Ordering ordering;
            char version;
            char hashed;
            char sparse;
            char clustering;
            int hashSeed;
            uint32_t numFields;

        private:
            enum Version {
                VERSION_0 = 0,
                NEXT_VERSION = 1
            };
            static const Version CURRENT_VERSION = (Version) (((int) NEXT_VERSION) - 1);
        };

        static const int FixedSize = sizeof(Header);
        BOOST_STATIC_ASSERT(FixedSize == 16);
#pragma pack()

        const char *_data;
        const size_t _size;
        scoped_ptr<char> _dataOwned;
    };

} // namespace mongo

