#pragma once

#include <stdint.h>
#include <climits>
#include <stdio.h>

typedef unsigned ID;

#define MAX_OBJECTS 64*1024
#define INDEX_MASK 0xffff
#define NEW_OBJECT_ID_ADD 0x10000
#define INDEX_FROM_ID(x) x & INDEX_MASK

struct Index
{
    ID id;
    unsigned short index;
    unsigned short next;
};

template<class T, size_t N>
struct PackedArray
{
    unsigned _num_objects;
    T _objects[N];
    Index _indices[N];
    unsigned short _freelist_enqueue;
    unsigned short _freelist_dequeue;
    
    PackedArray()
    {
        _num_objects = 0;
        for (unsigned i=0; i<N; ++i)
        {
            _indices[i].id = i;
            _indices[i].next = i+1;
        }
        _freelist_dequeue = 0;
        _freelist_enqueue = N-1;
    }
    
    inline bool has(ID id)
    {
        Index &in = _indices[id & INDEX_MASK];
        return in.id == id && in.index != USHRT_MAX;
    }
    
    inline T& lookup(ID id)
    {
        return _objects[_indices[id & INDEX_MASK].index];
    }

	inline bool set(ID id, T object)
	{
		if (!has(id))
			return false;
		else
		{
			_objects[_indices[id & INDEX_MASK].index] = object;
			return true;
		}
	}
    
    inline ID add()
    {
        Index &in = _indices[_freelist_dequeue];
        _freelist_dequeue = in.next;
        in.id += NEW_OBJECT_ID_ADD;
        in.index = _num_objects++;
        return in.id;
    }

	T* array()
	{
		return &_objects[0];
	}

	T& operator[] (int i)
	{
		return _objects[i];
	}
    
    inline uint32_t size()
    {
        return _num_objects;
    }
    
    inline void remove(ID id)
    {
        Index &in = _indices[id & INDEX_MASK];
        
        T &o = _objects[in.index];
        o = _objects[--_num_objects];
        _indices[id & INDEX_MASK].index = in.index;
        
        in.index = USHRT_MAX;
        _indices[_freelist_enqueue].next = id & INDEX_MASK;
        _freelist_enqueue = id & INDEX_MASK;
    }
};
