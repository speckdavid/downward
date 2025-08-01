#ifndef ALGORITHMS_SEGMENTED_VECTOR_H
#define ALGORITHMS_SEGMENTED_VECTOR_H

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

/*
  SegmentedVector is a vector-like class with the following advantages over
  vector:
    1. Resizing has no memory spike. (*)
    2. Should work more nicely with fragmented memory because data is
       partitioned into fixed-size chunks of size SEGMENT_BYTES.
    3. Overallocation is only additive (by SEGMENT_BYTES), not multiplicative
       as in vector. (*)
    4. References stay stable forever, so there is no need to be careful about
       invalidating references upon growing the vector.

  (*) Assumes that the size of the "segments" vector can be neglected, which is
  true if SEGMENT_BYTES isn't chosen too small. For example, with 1 GB of data
  and SEGMENT_BYTES = 8192, we can have 131072 segments.

  The main disadvantage to vector is that there is an additional indirection
  for each lookup, but we hope that the first lookup will usually hit the cache.
  The implementation is basically identical to that of deque (at least the
  g++ version), but with the advantage that we can control SEGMENT_BYTES. A
  test on all optimal planning instances with several planner configurations
  showed a modest advantage over deque.

  The class can also be used as a simple "memory pool" to reduce allocation
  costs (time and memory) when allocating many objects of the same type.

  SegmentedArrayVector is a similar class that can be used for compactly
  storing many fixed-size arrays. It's essentially a variant of SegmentedVector
  where the size of the stored data is only known at runtime, not at compile
  time. Note that we do not support 0-length arrays (checked with an assertion).
*/

/*
  There is currently a significant amount of duplication between the
  two classes. We decided to live with this for the time being,
  but this could certainly be made prettier.
*/

/*
  For documentation on classes relevant to storing and working with registered
  states see the file state_registry.h.
*/

namespace segmented_vector {
template<class Entry, class Allocator = std::allocator<Entry>>
class SegmentedVector {
    using EntryAllocator =
        typename std::allocator_traits<Allocator>::template rebind_alloc<Entry>;
    static const size_t SEGMENT_BYTES = 8192;

    static const size_t SEGMENT_ELEMENTS = (SEGMENT_BYTES / sizeof(Entry)) >= 1
                                               ? (SEGMENT_BYTES / sizeof(Entry))
                                               : 1;

    EntryAllocator entry_allocator;

    std::vector<Entry *> segments;
    size_t the_size;

    size_t get_segment(size_t index) const {
        return index / SEGMENT_ELEMENTS;
    }

    size_t get_offset(size_t index) const {
        return index % SEGMENT_ELEMENTS;
    }

    void add_segment() {
        Entry *new_segment = std::allocator_traits<EntryAllocator>::allocate(
            entry_allocator, SEGMENT_ELEMENTS);
        segments.push_back(new_segment);
    }

    SegmentedVector(const SegmentedVector<Entry> &) = delete;
    SegmentedVector &operator=(const SegmentedVector<Entry> &) = delete;
public:
    SegmentedVector() : the_size(0) {
    }

    SegmentedVector(const EntryAllocator &allocator_)
        : entry_allocator(allocator_), the_size(0) {
    }

    ~SegmentedVector() {
        for (size_t i = 0; i < the_size; ++i) {
            std::allocator_traits<EntryAllocator>::destroy(
                entry_allocator, &operator[](i));
        }
        for (size_t segment = 0; segment < segments.size(); ++segment) {
            std::allocator_traits<EntryAllocator>::deallocate(
                entry_allocator, segments[segment], SEGMENT_ELEMENTS);
        }
    }

    Entry &operator[](size_t index) {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment][offset];
    }

    const Entry &operator[](size_t index) const {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment][offset];
    }

    size_t size() const {
        return the_size;
    }

    void push_back(const Entry &entry) {
        size_t segment = get_segment(the_size);
        size_t offset = get_offset(the_size);
        if (segment == segments.size()) {
            assert(offset == 0);
            // Must add a new segment.
            add_segment();
        }
        std::allocator_traits<EntryAllocator>::construct(
            entry_allocator, segments[segment] + offset, entry);
        ++the_size;
    }

    void pop_back() {
        std::allocator_traits<EntryAllocator>::destroy(
            entry_allocator, &operator[](the_size - 1));
        --the_size;
        /*
          If the removed element was the last in its segment, the segment
          is not removed (memory is not deallocated). This way a subsequent
          push_back does not have to allocate the memory again.
        */
    }

    void resize(size_t new_size, Entry entry = Entry()) {
        while (new_size < the_size) {
            pop_back();
        }
        while (new_size > the_size) {
            push_back(entry);
        }
    }
};

template<class Element, class Allocator = std::allocator<Element>>
class SegmentedArrayVector {
    using ElementAllocator = typename std::allocator_traits<
        Allocator>::template rebind_alloc<Element>;
    static const size_t SEGMENT_BYTES = 8192;

    const size_t elements_per_array;
    const size_t arrays_per_segment;
    const size_t elements_per_segment;

    ElementAllocator element_allocator;

    std::vector<Element *> segments;
    size_t the_size;

    size_t get_segment(size_t index) const {
        return index / arrays_per_segment;
    }

    size_t get_offset(size_t index) const {
        return (index % arrays_per_segment) * elements_per_array;
    }

    void add_segment() {
        Element *new_segment =
            std::allocator_traits<ElementAllocator>::allocate(
                element_allocator, elements_per_segment);
        segments.push_back(new_segment);
    }

    SegmentedArrayVector(const SegmentedArrayVector<Element> &) = delete;
    SegmentedArrayVector &operator=(const SegmentedArrayVector<Element> &) =
        delete;
public:
    SegmentedArrayVector(size_t elements_per_array_)
        : elements_per_array(
              (assert(elements_per_array_ > 0), elements_per_array_)),
          arrays_per_segment(std::max(
              SEGMENT_BYTES / (elements_per_array * sizeof(Element)),
              size_t(1))),
          elements_per_segment(elements_per_array * arrays_per_segment),
          the_size(0) {
    }

    SegmentedArrayVector(
        size_t elements_per_array_, const ElementAllocator &allocator_)
        : element_allocator(allocator_),
          elements_per_array(
              (assert(elements_per_array_ > 0), elements_per_array_)),
          arrays_per_segment(std::max(
              SEGMENT_BYTES / (elements_per_array * sizeof(Element)),
              size_t(1))),
          elements_per_segment(elements_per_array * arrays_per_segment),
          the_size(0) {
    }

    ~SegmentedArrayVector() {
        for (size_t i = 0; i < the_size; ++i) {
            for (size_t offset = 0; offset < elements_per_array; ++offset) {
                std::allocator_traits<ElementAllocator>::destroy(
                    element_allocator, operator[](i) + offset);
            }
        }
        for (size_t i = 0; i < segments.size(); ++i) {
            std::allocator_traits<ElementAllocator>::deallocate(
                element_allocator, segments[i], elements_per_segment);
        }
    }

    Element *operator[](size_t index) {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment] + offset;
    }

    const Element *operator[](size_t index) const {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment] + offset;
    }

    size_t size() const {
        return the_size;
    }

    void push_back(const Element *entry) {
        size_t segment = get_segment(the_size);
        size_t offset = get_offset(the_size);
        if (segment == segments.size()) {
            assert(offset == 0);
            // Must add a new segment.
            add_segment();
        }
        Element *dest = segments[segment] + offset;
        for (size_t i = 0; i < elements_per_array; ++i)
            std::allocator_traits<ElementAllocator>::construct(
                element_allocator, dest++, *entry++);
        ++the_size;
    }

    void pop_back() {
        for (size_t offset = 0; offset < elements_per_array; ++offset) {
            std::allocator_traits<ElementAllocator>::destroy(
                element_allocator, operator[](the_size - 1) + offset);
        }
        --the_size;
        /*
          If the removed element was the last in its segment, the segment
          is not removed (memory is not deallocated). This way a subsequent
          push_back does not have to allocate the memory again.
        */
    }

    void resize(size_t new_size, const Element *entry) {
        while (new_size < the_size) {
            pop_back();
        }
        while (new_size > the_size) {
            push_back(entry);
        }
    }
};
}

#endif
