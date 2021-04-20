/* mem.c : memory manager */
#include <i386.h>
#include <xeroskernel.h>

// Globals
struct memHeader *memSlot; // Head of the free memory list
extern long freemem; // Start of memory space
extern char	*maxaddr; // End of memory space

// Prototypes
unsigned long header_size(void);
unsigned long data_size(struct memHeader *node);
unsigned long node_size(struct memHeader *node);
unsigned long node_end(struct memHeader *node);

unsigned long align(unsigned long size);
struct memHeader *create_node(unsigned long address, unsigned long end, struct memHeader *next, struct memHeader *prev);
struct memHeader *insert_node(struct memHeader *before, struct memHeader *middle, struct memHeader *after);
void coalesce(struct memHeader *node);
void *allocate(unsigned long size, struct memHeader* node);
int is_aligned(unsigned long address);
int is_sanity_check_valid(struct memHeader *node);

//////////////////////////////////////////////
// Helpers for calculating memory addresses //
//////////////////////////////////////////////

// Returns the bytesize of a memory header
unsigned long header_size() { return (unsigned long) sizeof(struct memHeader); }

// Takes a node pointer and returns the bytesize of it's data component
unsigned long data_size(struct memHeader *node) { return node -> size; }

// Takes a node pointer and returns the full bytesize of that node (header included)
unsigned long node_size(struct memHeader *node) { return header_size() + data_size(node); }

// Takes a node pointer and returns the byte_address of the end of the node
unsigned long node_end(struct memHeader *node) { return (unsigned long) node + node_size(node); }

// Align a byte address to the next 16-byte segment
unsigned long align(unsigned long size) {
    unsigned long block_size = 16;
    unsigned long block_count = (size / block_size) + ((size % block_size) ? 1 : 0);
    return block_count * block_size;
}
// Align a byte address to the previous 16-byte segment
unsigned long align_down(unsigned long size) {
    unsigned long block_size = 16;
    unsigned long block_count = size / block_size;
    return block_count * block_size;
}

/////////////////////////////////////////////
// Helpers for validating memory addresses //
/////////////////////////////////////////////

// Return 1 if memory address is 16-byte aligned
int is_aligned(unsigned long address) {
    return address == align(address);
}

// Return 1 if node passes sanity check
int is_sanity_check_valid(struct memHeader *node) {
    unsigned long address = (unsigned long) node;
    return (unsigned long) (node -> sanityCheck) == address + header_size();
}

// Helper for creating nodes
struct memHeader *create_node(unsigned long start, unsigned long end, struct memHeader *next, struct memHeader *prev) {
    struct memHeader *node = (struct memHeader *)align(start);
    node -> size = align_down(end) - align(start) - header_size();
    node -> next = next;
    node -> prev = prev;
    node -> sanityCheck = (char *)(align(start) + header_size());
    return node;
}

// Helper for inserting nodes
struct memHeader *insert_node(struct memHeader *before, struct memHeader *middle, struct memHeader *after){
    if (before != NULL) { 
        before -> next = middle;

        if (node_end(before) > (unsigned long) middle) {
            kprintf("WARNING: Attempt to insert %d before %d\n", node_end(before), middle);
        }
    }
    middle -> prev = before;
    middle -> next = after;
    if (after != NULL) {
        after -> prev = middle;
        if (node_end(middle) > (unsigned long) after) {
            kprintf("WARNING: Attempt to insert %d before %d\n", node_end(middle), after);
        }
    }
    return middle;
}

///////////////////////////////////////////////////
// Helpers for operating on the free memory list //
///////////////////////////////////////////////////

// Allocate size amount of memory from node, maintaining free memory list integrity and returning the dataStart address for consumption
// node is gauranteed to be a valid node with sufficient size
void *allocate(unsigned long size, struct memHeader *node) {
    if (data_size(node) == size) { // requested size occupies entire node
        if (node -> prev == NULL && node -> next == NULL) { // node is the only node in free memory list
            memSlot = NULL;
        } else if (node -> prev != NULL && node -> next != NULL) { // node is between two other nodes
            (node -> prev) -> next = node -> next;
            (node -> next) -> prev = node -> prev;
        } else if (node -> prev == NULL && node -> next != NULL) { // node is the first node and there is a node after it
            struct memHeader *next_node = node -> next;
            next_node -> prev = NULL;
            memSlot = next_node;
        } else if (node -> prev != NULL && node -> next == NULL) { // node is the last node and there is a node before it
            struct memHeader *prev_node = node -> prev;
            prev_node -> next = NULL;
        }
    } else { // requested size doesn't occupy entire node
        unsigned long node_boundary = align((unsigned long) (node -> dataStart) + size);
        unsigned long new_node_address_start = node_boundary;
        unsigned long new_node_address_end = node_end(node);

        struct memHeader *new_node = create_node(
            new_node_address_start, // start address
            new_node_address_end,   // end address
            node -> next,             // next
            node -> prev              // prev
        );
        node -> size = align(size);

        // if (!is_in_memory_space(new_node_address_start)) {
        //     kprintf("WARNING: Updated Node Start Invalid\n");
        // }
        // if ( !is_in_memory_space(node_end((struct memHeader *) new_node_address_start)) ) {
        //     kprintf("WARNING: Updated Node End Invalid\n");
        // }
        // if ( !is_in_memory_space((unsigned long) node) ) {
        //     kprintf("WARNING: Allocated Node Start Invalid\n");
        // }
        // if (!is_in_memory_space(node_end(node))) {
        //     kprintf("WARNING: Allocated Node End Invalid\n");
        // }

        if (node -> prev == NULL) { // If node is the first Node in the list
            memSlot = new_node;
        } else {
            (node -> prev) -> next = new_node;
        }
    }
    // kmem_print();

    return node -> dataStart;
}

// Coalesce node with the next node if possible
// Will recursively coalesce more nodes if possible
void coalesce(struct memHeader *node) {
    if (node -> next != NULL && (unsigned long) (node -> next) == node_end(node)) {
        if ((node -> next) -> next != NULL) {
            ((node -> next) -> next) -> prev = node;
        }
        node -> size = node -> size + node_size(node -> next);
        node -> next = (node -> next) -> next;
        coalesce(node);
    }
    return;
}

//////////////////
// External API //
//////////////////

// Return 1 if memory address is within memory manager space
extern int is_in_memory_space(unsigned long address) {
    return (address >= freemem && address <= HOLESTART) || (address >= HOLEEND && address <= (unsigned long) maxaddr);
}

// Oberserver function to help test the memory manager
extern unsigned long kmem_level(void) {
    unsigned long memory_available = 0;
    struct memHeader *currentNode = memSlot;
    while (currentNode != NULL) {
        memory_available += (currentNode -> size + header_size());
        currentNode = currentNode -> next;
    }
    return memory_available;
}

// Observer function to visualize memory
extern void kmem_print(void) {
    struct memHeader *currentNode = memSlot;
    unsigned int node_index = 0;
    while (currentNode != NULL)
    {
        kprintf("Node #%d, span: [%d, %d], size: %d, sanity: %d\n\n",
            node_index,
            (unsigned long) currentNode,
            node_end(currentNode),
            currentNode -> size,
            (unsigned long) (currentNode -> sanityCheck));
        currentNode = currentNode -> next;
        node_index += 1;
    }
    return;
}

// Initialize the free memory list
extern void kmeminit(void) {
    // kprintf("Initializing memory manager\n\n");
    // kprintf("freemem: %d, HOLESTART: %d, HOLEEND %d, maxaddr: %d\n\n", freemem, HOLESTART, HOLEEND, (unsigned long ) maxaddr);

    struct memHeader *firstNode = create_node(
        freemem,                            // start address
        HOLESTART,                          // end address
        (struct memHeader *) HOLEEND,        // next
        NULL                                // prev
    );
    struct memHeader *secondNode = create_node(
        HOLEEND,                            // start address
        (unsigned long) (maxaddr),          // end address
        NULL,                               // next
        firstNode                           // prev
    );
    memSlot = firstNode;
    // Sanity Test
    if (!(HOLESTART <= HOLEEND)) {
        kprintf("ERROR: BIOS Space invalid\n\n");
    }
    if (!(freemem <= HOLESTART)) {
        kprintf("ERROR: First free space\n\n");
    }
    if (!(HOLEEND <= (unsigned long) maxaddr)) {
        kprintf("ERROR: Second free space invalid\n\n");
    }

    // Initial Integrity Test
    if (!(freemem <= (unsigned long) firstNode)) {
        kprintf("ERROR: Free list overlaps kernel space\n\n");
    }
    if (!( node_end(firstNode) <= HOLESTART)) {
        kprintf("ERROR: Free list (first node) overlaps BIOS space\n\n"); // violated
    }
    if (!(HOLEEND <= (unsigned long) secondNode)) {
        kprintf("ERROR: Free list (second node) overlaps BIOS space\n\n"); // violated
    }
    if (!(node_end(secondNode) <= (unsigned long) maxaddr)) { // violated
        kprintf("ERROR: Free list exceeds system memory\n\n");
    }

    if (!((unsigned long) (firstNode) - freemem < 16)){
        kprintf("ERROR: Free list (first node) starting boundary too high\n\n");
    }
    if (!(HOLESTART - node_end(firstNode) < 16)) {
        kprintf("ERROR: Free list (first node) ending boundary too low\n\n");
    }

    if (!((unsigned long)(secondNode) - HOLEEND < 16)) {
        kprintf("ERROR: Free list (second node) starting boundary too high\n\n");
    }
    if (!((unsigned long) maxaddr - node_end(secondNode) < 16)) {
        kprintf("ERROR: Free list (second node) ending boundary too low\n\n");
    }

    // kprintf("Memory manager initialized\n\n");
    // kmem_print();

    return;
}

// Allocates size amount of memory if possible
// Returns the address of the allocated memory if the allocation is possible
// Returns 0 if the memory manager cannot find sufficient contiguous space
extern void *kmalloc(size_t size) {
  if (size <= 0) { return 0; }
  unsigned long requested_size = (unsigned long) size;
  struct memHeader* currentNode = memSlot;
  while (currentNode != NULL) {
    if (data_size(currentNode) >= requested_size) { break; }
    currentNode = currentNode -> next;
  }
  unsigned long memory_level_before = kmem_level();
  void *address = (currentNode == NULL) ? 0 : allocate(requested_size, currentNode);
  unsigned long memory_level_after = kmem_level();
  if (memory_level_before < memory_level_after) {
      kprintf("ERROR: Memory level increased after kmalloc\n");
  }
  return address;
}

// Deallocates the memory at ptr, coalescing any memory in the free memory list if possible
// Returns 0 if the address given is invalid
// Returns 1 if the deallocation was successful
extern int kfree(void *ptr) {
    // kprintf("Calculating effective node address\n");
    // kmem_print();
    unsigned long memory_level_before = kmem_level();
    unsigned long address = ((unsigned long) ptr - header_size());
    if (!is_in_memory_space(address) || !is_in_memory_space((unsigned long) ptr)) {
        // kprintf("WARNING: Attempt to deallocate out-of-range memory %d\n\n", address);
        return 0;
    }
    if (!is_aligned(address)) {
        // kprintf("WARNING: Attempt to deallocate misaligned memory %d\n\n", address);
        return 0;
    }

    // kprintf("Casting address to struct\n");
    struct memHeader *node = (struct memHeader *) address;
    if (!is_sanity_check_valid(node)) {
        // kprintf("WARNING: Sanity check failed at %d\n Expected %d but got %d \n\n", address, (unsigned long) ptr, (unsigned long) node -> sanityCheck);
        return 0;
    }
    if (memSlot == NULL) {
        // kprintf("Free list is empty\n");
        memSlot = node;
        unsigned long memory_level_after = kmem_level();
        if (memory_level_before > memory_level_after) {
            kprintf("ERROR: Memory level decreased after kfree\n");
        }
        return 1;
    }

    if (node_end(node) <= (unsigned long) memSlot) { // node is before the whole free list
        // kprintf("Node to free is before free list\n");
        node -> next = memSlot;
        memSlot -> prev = node;
        memSlot = node;
        coalesce(node);
        unsigned long memory_level_after = kmem_level();
        if (memory_level_before > memory_level_after) {
            kprintf("ERROR: Memory level decreased after kfree\n");
        }
        return 1;
    }

    // kprintf("Begin forward search for relative node position\n");
    struct memHeader *currentNode = memSlot;
    while (currentNode != NULL) {
        if (address < node_end(currentNode)) { break; } // address is before or part of current node
        if (currentNode -> next == NULL) { // current node is the last node in the free list
            // kprintf("Insert A\n");
            insert_node(currentNode, node, NULL);
            // kprintf("coalesce A\n");
            coalesce(currentNode);
            unsigned long memory_level_after = kmem_level();
            if (memory_level_before > memory_level_after) {
                kprintf("ERROR: Memory level decreased after kfree\n");
            }
            return 1;
        }
        if (node_end(currentNode -> next) <= address) { // there exists a node between current and node
            // kprintf("Moving further down free list %d %d\n", currentNode, currentNode -> next);
            currentNode = currentNode -> next;
        } else if (node_end(node) <= (unsigned long) (currentNode -> next)) { // node is between current node and next
            // kprintf("insert B\n");
            insert_node(currentNode, node, currentNode -> next);
            // kprintf("coalesce B\n");
            coalesce(currentNode);
            unsigned long memory_level_after = kmem_level();
            if (memory_level_before > memory_level_after) {
                kprintf("ERROR: Memory level decreased after kfree\n");
            }
            return 1;
        } else { break; } // node is part of next
    }
    // kprintf("WARNING: Attempt to deallocate free memory %d\n\n", address);
    return 0;
}
