#include <iostream>
#include <list>
#include <typeinfo>
#include <cstdlib>
#include "gc_details.h"
#include "gc_iterator.h"
/*
    Pointer implements a pointer type that uses
    garbage collection to release unused memory.
    A Pointer must only be used to point to memory
    that was dynamically allocated using new.
    When used to refer to an allocated array,
    specify the array size.
*/
template <class T, int size = 0>
class Pointer{
private:
    // refContainer maintains the garbage collection list.
    static std::list<PtrDetails<T> > refContainer;
    // addr points to the allocated memory to which
    // this Pointer pointer currently points.
    T *addr;
    /*  isArray is true if this Pointer points
        to an allocated array. It is false
        otherwise.
    */
    bool isArray;
    // true if pointing to array
    // If this Pointer is pointing to an allocated
    // array, then arraySize contains its size.
    unsigned arraySize; // size of the array
    static bool first; // true when first Pointer is created
    // Return an iterator to pointer details in refContainer.
    typename std::list<PtrDetails<T> >::iterator findPtrInfo(T *ptr);
public:
    // Define an iterator type for Pointer<T>.
    typedef Iter<T> GCiterator;
    // Empty constructor
    // NOTE: templates aren't able to have prototypes with default arguments
    // this is why constructor is designed like this:
    Pointer(){
        Pointer(NULL);
    }
    Pointer(T*);
    // Copy constructor.
    Pointer(const Pointer &);
    // Destructor for Pointer.
    ~Pointer();
    // Collect garbage. Returns true if at least
    // one object was freed.
    static bool collect();
    // Overload assignment of pointer to Pointer.
    T *operator=(T *t);
    // Overload assignment of Pointer to Pointer.
    Pointer &operator=(Pointer &rv);
    // Return a reference to the object pointed
    // to by this Pointer.
    T &operator*(){
        return *addr;
    }
    // Return the address being pointed to.
    T *operator->() { return addr; }
    // Return a reference to the object at the
    // index specified by i.
    T &operator[](int i){ return addr[i];}
    // Conversion function to T *.
    operator T *() { return addr; }
    // Return an Iter to the start of the allocated memory.
    Iter<T> begin(){
        int _size;
        if (isArray)
            _size = arraySize;
        else
            _size = 1;
        return Iter<T>(addr, addr, addr + _size);
    }
    // Return an Iter to one past the end of an allocated array.
    Iter<T> end(){
        int _size;
        if (isArray)
            _size = arraySize;
        else
            _size = 1;
        return Iter<T>(addr + _size, addr, addr + _size);
    }
    // Return the size of refContainer for this type of Pointer.
    static int refContainerSize() { return refContainer.size(); }
    // A utility function that displays refContainer.
    static void showlist();
    // Clear refContainer when program exits.
    static void shutdown();
};

// STATIC INITIALIZATION
// Creates storage for the static variables
template <class T, int size>
std::list<PtrDetails<T> > Pointer<T, size>::refContainer;
template <class T, int size>
bool Pointer<T, size>::first = true;

// Constructor for both initialized and uninitialized objects. -> see class interface
template<class T,int size>
Pointer<T,size>::Pointer(T *t): addr(t), isArray(size > 1), arraySize(size) {
    // Register shutdown() as an exit function.
    if (first)
        atexit(shutdown);
    first = false;

    if (Pointer<T, size>::findPtrInfo(t) == Pointer<T, size>::refContainer.end()) {
        PtrDetails<T> newDetails;
        newDetails.refcount = 1;
        newDetails.memPtr = t;
        newDetails.isArray = this->isArray;
        newDetails.arraySize = this->arraySize;
    
        Pointer<T, size>::refContainer.push_back(newDetails);
    }
    else {
        PtrDetails<T> &details = *Pointer<T, size>::findPtrInfo(t);
        details.refcount++;
    }
}

// Copy constructor.
template< class T, int size>
Pointer<T,size>::Pointer(const Pointer &ob): Pointer(ob.addr) {
}

// Destructor for Pointer.
template <class T, int size>
Pointer<T, size>::~Pointer(){
    PtrDetails<T> &details = *Pointer<T, size>::findPtrInfo(this->addr);
    details.refcount--;

    if (details.refcount == 0) {
        this->collect();
    }
}

// Collect garbage. Returns true if at least
// one object was freed.
template <class T, int size>
bool Pointer<T, size>::collect(){
    int collected = 0;

    typename std::list<PtrDetails<T> >::iterator i;
    for (i = refContainer.begin(); i != refContainer.end(); i++) {
        if (i->refcount == 0) {
            if (i->isArray) delete[] i->memPtr;
            else delete i->memPtr;
            
            //Remove and then step to the next item
            Pointer<T, size>::refContainer.erase(i++);
            
            collected++;
        }
    }
    
    return collected > 0;
}

// Overload assignment of pointer to Pointer.
template <class T, int size>
T *Pointer<T, size>::operator=(T *t){

    PtrDetails<T>& oldDetails = *Pointer<T, size>::findPtrInfo(this->addr);
    oldDetails.refcount--;

    this->addr = t;
    if (Pointer<T, size>::findPtrInfo(t) == Pointer<T, size>::refContainer.end()) {
        PtrDetails<T> newDetails;
        newDetails.refcount = 1;
        newDetails.memPtr = t;
        newDetails.isArray = this->isArray;
        newDetails.arraySize = this->arraySize;
    
        Pointer<T, size>::refContainer.push_back(newDetails);
    }
    else {
        PtrDetails<T>& newDetails = *Pointer<T, size>::findPtrInfo(t);
        newDetails.refcount++;
    }

    if (oldDetails.refcount == 0) {
        this->collect();
    }

    return t;
}
// Overload assignment of Pointer to Pointer.
template <class T, int size>
Pointer<T, size> &Pointer<T, size>::operator=(Pointer &rv){

    PtrDetails<T>& oldDetails = *Pointer<T, size>::findPtrInfo(this->addr);
    oldDetails.refcount--;

    this->addr = rv->addr;
    PtrDetails<T>& newDetails = *Pointer<T, size>::findPtrInfo(rv.addr);
    newDetails.refcount++;
    
    if (oldDetails.refcount == 0) {
        this->collect();
    }

    return rv;
}

// A utility function that displays refContainer.
template <class T, int size>
void Pointer<T, size>::showlist(){
    typename std::list<PtrDetails<T> >::iterator p;
    std::cout << "refContainer<" << typeid(T).name() << ", " << size << ">:\n";
    std::cout << "memPtr refcount value\n ";
    if (refContainer.begin() == refContainer.end())
    {
        std::cout << " Container is empty!\n\n ";
    }
    for (p = refContainer.begin(); p != refContainer.end(); p++)
    {
        std::cout << "[" << (void *)p->memPtr << "]"
             << " " << p->refcount << " ";
        if (p->memPtr)
            std::cout << " " << *p->memPtr;
        else
            std::cout << "---";
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
// Find a pointer in refContainer.
template <class T, int size>
typename std::list<PtrDetails<T> >::iterator
Pointer<T, size>::findPtrInfo(T *ptr){
    typename std::list<PtrDetails<T> >::iterator p;
    // Find ptr in refContainer.
    for (p = refContainer.begin(); p != refContainer.end(); p++)
        if (p->memPtr == ptr)
            return p;
    return p;
}
// Clear refContainer when program exits.
template <class T, int size>
void Pointer<T, size>::shutdown(){
    if (refContainerSize() == 0)
        return; // list is empty
    typename std::list<PtrDetails<T> >::iterator p;
    for (p = refContainer.begin(); p != refContainer.end(); p++)
    {
        // Set all reference counts to zero
        p->refcount = 0;
    }
    collect();
}
