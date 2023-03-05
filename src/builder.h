#pragma once

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <cfloat>

#include "qassert.h"
#include "endian.h"
#include "string_data.h"

namespace quadis {

    const int BufferMaxSize = 64 * 1024 * 1024;

    struct PackedDouble {
        double d;
    }; // PACKED_DECL;

    class TrivialAllocator { 
    public:
        void* Malloc(size_t sz) { return malloc(sz); }
        void* Realloc(void *p, size_t sz) { return realloc(p, sz); }
        void Free(void *p) { free(p); }
    };

    template<class Allocator>
    class _BufBuilder {

        // non-copyable, non-assignable
        _BufBuilder( const _BufBuilder& );
        _BufBuilder& operator=( const _BufBuilder& );
        Allocator al;

    public:
        _BufBuilder(int initsize = 512) : size(initsize) {
            if ( size > 0 ) {
                data = (char *) al.Malloc(size);
                if( data == 0 )
                    msgasserted(10000, "out of memory BufBuilder");
            }
            else {
                data = 0;
            }
            l = 0;
        }
        ~_BufBuilder() { kill(); }

        void kill() {
            if ( data ) {
                al.Free(data);
                data = 0;
            }
        }

        /** leave room for some stuff later
            @return point to region that was skipped.  pointer may change later (on realloc), so for immediate use only
        */
        char* skip(int n) { return grow(n); }

        /* note this may be deallocated (realloced) if you keep writing. */
        char* buf() { return data; }
        const char* buf() const { return data; }

        /** @return length of current string */
        int len() const { return l; }
        void setlen( int newLen ) { l = newLen; }

        /** @return size of the buffer */
        int getSize() const { return size; }

        /* returns the pre-grow write position */
        inline char* grow(int by) {
            int oldlen = l;
            int newLen = l + by;
            if ( newLen > size ) {
                grow_reallocate(newLen);
            }
            l = newLen;
            return data + oldlen;
        }

        void reset() { l = 0;}
        void reset( int maxSize ) {
            l = 0;
            if ( maxSize && size > maxSize ) {
                al.Free(data);
                data = (char*)al.Malloc(maxSize);
                if ( data == 0 )
                    msgasserted( 15913 , "out of memory BufBuilder::reset" );
                size = maxSize;
            }
        }

        void appendUChar(unsigned char j) {
            *((unsigned char*)grow(sizeof(unsigned char))) = j;
        }
        void appendChar(char j) {
            *((char*)grow(sizeof(char))) = j;
        }
        void appendNum(char j) {
            *((char*)grow(sizeof(char))) = j;
        }
        void appendNum(short j) {
            *((short*)grow(sizeof(short))) = endian_short(j);
        }
        void appendNum(int j) {
            *((int*)grow(sizeof(int))) = endian_int(j);
        }
        void appendNum(unsigned j) {
            *((unsigned*)grow(sizeof(unsigned))) = endian(j);
        }
        void appendNum(bool j) {
            *((char*)grow(sizeof(char))) = j ? 1 : 0;
        }
        void appendNum(double j) {
            (reinterpret_cast<PackedDouble*>(grow(sizeof(double))))->d = endian_d(j);
        }
        void appendNum(long long j) {
            *((long long*)grow(sizeof(long long))) = endian_ll(j);
        }
        void appendNum(unsigned long long j) {
            *((unsigned long long*)grow(sizeof(unsigned long long))) = endian_ll(j);
        }

        void appendBuf(const void *src, size_t len) {
            memcpy(grow((int) len), src, len);
        }

        template<class T>
        void appendStruct(const T& s) {
            appendBuf(&s, sizeof(T));
        }

        void appendStr(const StringData &str , bool includeEndingNull = true ) {
            const int len = str.size() + ( includeEndingNull ? 1 : 0 );
            str.copyTo( grow(len), includeEndingNull );
        }

    private:
        /* "slow" portion of 'grow()'  */
        void NOINLINE_DECL grow_reallocate(int newLen) {
            int a = 64;
            while( a < newLen ) 
                a = a * 2;
            printf("a:%d BufferMaxSize:%d\n",a,BufferMaxSize);
            if ( a > BufferMaxSize ) {
                std::stringstream ss;
                ss << "BufBuilder attempted to grow() to " << a << " bytes, past the 64MB limit.";
                msgasserted(13548, ss.str().c_str());
            }
            data = (char *) al.Realloc(data, a);
            if ( data == NULL )
                msgasserted( 16070 , "out of memory BufBuilder::grow_reallocate" );
            size = a;
        }

        char *data;
        int l;
        int size;
    };
    
    typedef _BufBuilder<TrivialAllocator> BufBuilder;
}

