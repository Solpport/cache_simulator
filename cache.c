/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

// **Note** this is a preprocessor macro. This is not the same as a function.
// Powers of 2 have exactly one 1 and the rest 0's, and 0 isn't a power of 2.
#define is_power_of_2(val) (val && !(val & (val - 1)))

/*
 * Accesses 1 word of memory.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads and 1 for writes.
 * write_data is a word, and is only valid if write_flag is 1.
 * If write flag is 1, mem_access does: state.mem[addr] = write_data.
 * The return of mem_access is state.mem[addr].
 */
extern int mem_access(int addr, int write_flag, int write_data);

/*
 * Returns the number of times mem_access has been called.
 */
extern int get_num_mem_accesses(void);

// Use this when calling printAction. Do not modify the enumerated type below.
enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

/* You may add or remove variables from these structs */
typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int tag;
    int valid;
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
    // Stats for end-of-run
    int hits;
    int misses;
    int writebacks;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache(void);

// Helper functions for cache addressing
static int get_tag(int addr)
{
    return addr / (cache.blockSize * cache.numSets);
}

static int get_set_index(int addr)
{
    return (addr / cache.blockSize) % cache.numSets;
}

static int get_block_offset(int addr)
{
    return addr % cache.blockSize;
}

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{
    if (blockSize <= 0 || numSets <= 0 || blocksPerSet <= 0)
    {
        printf("error: input parameters must be positive numbers\n");
        exit(1);
    }
    if (blocksPerSet * numSets > MAX_CACHE_SIZE)
    {
        printf("error: cache must be no larger than %d blocks\n", MAX_CACHE_SIZE);
        exit(1);
    }
    if (blockSize > MAX_BLOCK_SIZE)
    {
        printf("error: blocks must be no larger than %d words\n", MAX_BLOCK_SIZE);
        exit(1);
    }
    if (!is_power_of_2(blockSize))
    {
        printf("warning: blockSize %d is not a power of 2\n", blockSize);
    }
    if (!is_power_of_2(numSets))
    {
        printf("warning: numSets %d is not a power of 2\n", numSets);
    }
    printf("Simulating a cache with %d total lines; each line has %d words\n",
           numSets * blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n",
           blocksPerSet, numSets);
    // Initialize cache parameters
    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;
    // Initialize statistics
    cache.hits = 0;
    cache.misses = 0;
    cache.writebacks = 0;
    // Initialize all cache blocks
    for (int i = 0; i < numSets * blocksPerSet; i++)
    {
        cache.blocks[i].valid = 0;
        cache.blocks[i].dirty = 0;
        cache.blocks[i].lruLabel = 0;
        cache.blocks[i].tag = 0;
    }
}

// Find the least recently used block in a set
static int find_lru_block(int set_index)
{
    int set_start = set_index * cache.blocksPerSet;
    int lru_block = set_start;
    int highest_lru = cache.blocks[set_start].lruLabel;
    for (int i = 1; i < cache.blocksPerSet; i++)
    {
        int block = set_start + i;
        if (cache.blocks[block].valid && cache.blocks[block].lruLabel > highest_lru)
        {
            highest_lru = cache.blocks[block].lruLabel;
            lru_block = block;
        }
    }
    return lru_block;
}

// Update LRU labels for all blocks in a set (Ver 1's approach)
static void update_lru(int set_index, int accessed_block)
{
    int set_start = set_index * cache.blocksPerSet;
    for (int i = 0; i < cache.blocksPerSet; i++)
    {
        int block = set_start + i;
        if (block != accessed_block && cache.blocks[block].valid)
        {
            cache.blocks[block].lruLabel++;
        }
    }
    cache.blocks[accessed_block].lruLabel = 0;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data)
{
    int set_index = get_set_index(addr);
    int tag = get_tag(addr);
    int block_offset = get_block_offset(addr);
    int set_start = set_index * cache.blocksPerSet;
    int found_block = -1;

    // Look for the block in the cache
    for (int i = 0; i < cache.blocksPerSet; i++)
    {
        int block = set_start + i;
        if (cache.blocks[block].valid && cache.blocks[block].tag == tag)
        {
            found_block = block;
            cache.hits++;
            break;
        }
    }

    // Cache miss
    if (found_block == -1)
    {
        cache.misses++;

        // Find a block to use (either empty or LRU)
        found_block = -1;
        for (int i = 0; i < cache.blocksPerSet; i++)
        {
            if (!cache.blocks[set_start + i].valid)
            {
                found_block = set_start + i;
                break;
            }
        }
        // If no empty block found, use LRU
        if (found_block == -1)
        {
            found_block = find_lru_block(set_index);
        }

        // If block is dirty, write it back to memory
        if (cache.blocks[found_block].valid && cache.blocks[found_block].dirty)
        {
            int old_addr = (cache.blocks[found_block].tag * cache.numSets + set_index) * cache.blockSize;
            cache.writebacks++;
            printAction(old_addr, cache.blockSize, cacheToMemory);
            for (int i = 0; i < cache.blockSize; i++)
            {
                mem_access(old_addr + i, 1, cache.blocks[found_block].data[i]);
            }
        }
        else if (cache.blocks[found_block].valid)
        {
            // If block is valid but not dirty, we still need to evict it
            int old_addr = (cache.blocks[found_block].tag * cache.numSets + set_index) * cache.blockSize;
            printAction(old_addr, cache.blockSize, cacheToNowhere);
        }

        // Read the new block from memory
        int base_addr = (addr / cache.blockSize) * cache.blockSize;
        printAction(base_addr, cache.blockSize, memoryToCache);
        for (int i = 0; i < cache.blockSize; i++)
        {
            cache.blocks[found_block].data[i] = mem_access(base_addr + i, 0, 0);
        }

        cache.blocks[found_block].valid = 1;
        cache.blocks[found_block].dirty = 0;
        cache.blocks[found_block].tag = tag;
    }

    // Update LRU (using Ver 1's approach)
    update_lru(set_index, found_block);

    // Handle the actual access
    if (write_flag)
    {
        printAction(addr, 1, processorToCache);
        cache.blocks[found_block].data[block_offset] = write_data;
        cache.blocks[found_block].dirty = 1;
        return 0;
    }
    else
    {
        printAction(addr, 1, cacheToProcessor);
        return cache.blocks[found_block].data[block_offset];
    }
}

/*
 * print end of run statistics like in the spec. **This is not required**,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(void)
{
    printf("End of run statistics:\n");
    printf("hits %d, misses %d, writebacks %d\n",
           cache.hits, cache.misses, cache.writebacks);

    int dirtyBlocks = 0;
    for (int i = 0; i < cache.numSets * cache.blocksPerSet; i++)
    {
        if (cache.blocks[i].valid && cache.blocks[i].dirty)
        {
            dirtyBlocks++;
        }
    }
    printf("%d dirty cache blocks left\n", dirtyBlocks);
}

/*
 * Log the specifics of each cache action.
 *
 * DO NOT modify the content below.
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);
    if (type == cacheToProcessor)
    {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache)
    {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache)
    {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory)
    {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere)
    {
        printf("from the cache to nowhere\n");
    }
    else
    {
        printf("Error: unrecognized action\n");
        exit(1);
    }
}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache(void)
{
    int blockIdx;
    int decimalDigitsForWaysInSet = (cache.blocksPerSet == 1) ? 1 : (int)ceil(log10((double)cache.blocksPerSet));
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set)
    {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block)
        {
            blockIdx = set * cache.blocksPerSet + block;
            if (cache.blocks[set * cache.blocksPerSet + block].valid)
            {
                printf("\t\t[ %0*i ] : ( V:T | D:%c | LRU:%-*i | T:%i )\n\t\t%*s{",
                       decimalDigitsForWaysInSet, block,
                       (cache.blocks[blockIdx].dirty) ? 'T' : 'F',
                       decimalDigitsForWaysInSet, cache.blocks[blockIdx].lruLabel,
                       cache.blocks[blockIdx].tag,
                       7 + decimalDigitsForWaysInSet, "");
                for (int index = 0; index < cache.blockSize; ++index)
                {
                    printf(" 0x%08X", cache.blocks[blockIdx].data[index]);
                }
                printf(" }\n");
            }
            else
            {
                printf("\t\t[ %0*i ] : (V:F)\n\t\t%*s{  }\n", decimalDigitsForWaysInSet, block, 7 + decimalDigitsForWaysInSet, "");
            }
        }
    }
    printf("end cache\n");
}
