/*
 * Copyright (c) 2016 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any adapted
 * or modified version may be disclosed or distributed to third parties
 * in any manner, medium, or form, in whole or in part, without the prior
 * written consent of Cadence Design Systems Inc.  This software and its
 * derivatives are to be executed solely on products incorporating a Cadence
 * Design Systems processor.
 */

#ifndef __XMEM_H__
#define __XMEM_H__

/*! @file */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <xtensa/config/core-isa.h>

/*! Minimum size of a memory manager object in bytes. */
#define XMEM_MGR_SIZE  (96)

#ifndef __XMEM_INTERNAL_H__
/* Override this by including <xtensa/system/xmp_subsystem.h> on an
 * MP-subsystem, where XMP_MAX_DCACHE_LINESIZE is the maximum cache line
 * size across all cores in the subsystem
 */
#ifndef XMP_MAX_DCACHE_LINESIZE
#if XCHAL_DCACHE_SIZE > 0
#define XMEM_MAX_DCACHE_LINESIZE  XCHAL_DCACHE_LINESIZE
#else
#define XMEM_MAX_DCACHE_LINESIZE  4
#endif
#else
#define XMEM_MAX_DCACHE_LINESIZE  XMP_MAX_DCACHE_LINESIZE
#endif
struct xmem_mgr_struct
{
  char _[((XMEM_MGR_SIZE + XMEM_MAX_DCACHE_LINESIZE - 1) / XMEM_MAX_DCACHE_LINESIZE) * XMEM_MAX_DCACHE_LINESIZE];
}  __attribute__ ((aligned(XMEM_MAX_DCACHE_LINESIZE)));
#endif

/*! Minimum size of a heap3 memory manager block in bytes */
#define XMEM_HEAP3_BLOCK_STRUCT_SIZE  (16)

/*!
 * Type to represent the generic memory manager object.
 * The minimum size of an object of this type is
 * \ref XMEM_MGR_SIZE. For a cached subsystem, the size is rounded
 * and aligned to the maximum dcache line size across all cores in the
 * subsystem. */
typedef struct xmem_mgr_struct  xmem_mgr_t;

/*!
 * Type used to represent the return value of the xmem API calls.
 */
typedef enum
{
  /*! No Error */
  XMEM_OK                      = 0,
  /*! User provided pool for the heap2 memory manager
   * is not aligned to the required block size. */
  XMEM_ERROR_POOL_NOT_ALIGNED  = -1,
  /*! User provided pool does not have the required size. */
  XMEM_ERROR_POOL_SIZE         = -2,
  /*! User provided pool is NULL. */
  XMEM_ERROR_POOL_NULL         = -3,
  /*! The block size for heap2 memory manager is not a power of 2. */
  XMEM_ERROR_BLOCK_SIZE        = -4,
  /*! Unsupported memory manager type. */
  XMEM_ERROR_UNKNOWN_HEAP_TYPE = -5,
  /*! Failed to allocate the requested buffer. */
  XMEM_ERROR_ALLOC_FAILED      = -6,
  /*! Alignment of the request buffer has to be a power of 2. */
  XMEM_ERROR_ILLEGAL_ALIGN     = -7,
  /*! Requested allocation from an uninitialized memory manager object. */
  XMEM_ERROR_UNINITIALIZED     = -8,
  /*! Encountered an internal error */
  XMEM_ERROR_INTERNAL          = -100
} xmem_status_t;

/*! Initialize the heap1 memory manager.
 *
 * \param m      Pointer to generic memory manager object.
 * \param buffer User provided pool for allocation.
 * \param size   Size of the pool.
 * \return       XMEM_OK if successful, else returns XMEM_ERROR_INTERNAL or
 *               XMEM_ERROR_POOL_NULL.
 */
xmem_status_t
xmem_heap1_init(xmem_mgr_t *m, void *buffer, size_t size);

/*!
 * Initialize the heap2 memory manager.
 *
 * \param m          Pointer to generic memory manager object.
 * \param buf        User provided pool for allocation.
 * \param size       Size of the pool.
 * \param block_size Size of the heap2 memory manager block.
 * \param header     Optional buffer used to store the header information
 *                   required by the heap2 memory manager. If NULL, the
 *                   headers are allocated out of the user provided pool.
 * \return           XMEM_OK if successful, else returns one of
 *                   - XMEM_ERROR_POOL_NULL
 *                   - XMEM_ERROR_INTERNAL
 *                   - XMEM_ERROR_BLOCK_SIZE
 *                   - XMEM_ERROR_POOL_SIZE
 *                   - XMEM_ERROR_POOL_NOT_ALIGNED
 */
xmem_status_t
xmem_heap2_init(xmem_mgr_t *m,
                void *buf,
                size_t size,
                size_t block_size,
                void *header);

/*!
 * Macro returns the number of bytes required to hold the header information
 * for the heap2 memory manager. The header includes the two bitvectors that
 * are used to track the free blocks and the number of allocated blocks.
 * The caller can use this macro to reserve bytes for the header that may be
 * passed optionally to \ref xmem_heap2_init.
 * This macros takes as parameters the size of the pool where the data is
 * allocated and \a log2 of the block size.
 * The \a buffer_size is expected to be a multiple of the block size.
 * Use this macro to compute the header size on non cache-based subsystems.
 */
#define XMEM_HEAP2_HEADER_SIZE(buffer_size, log2_block_size) \
  (((((buffer_size) >> (log2_block_size)) + 31) >> 5) * 2 * 4)

/*!
 * Same as the macro \ref XMEM_HEAP2_HEADER_SIZE, but the computed size is
 * aligned to the maximum data cache line size across all the cores in the
 * subsystem. Use this macro to compute the header size on cache-based
 * subsystems.
 */
#define XMEM_HEAP2_CACHE_ALIGNED_HEADER_SIZE(buffer_size, log2_block_size) \
  (((XMEM_HEAP2_HEADER_SIZE((buffer_size), (log2_block_size)) +            \
     XMEM_MAX_DCACHE_LINESIZE - 1) / XMEM_MAX_DCACHE_LINESIZE) *           \
   XMEM_MAX_DCACHE_LINESIZE)

/*!
 * Macro to returns the number of bytes required to hold the header information
 * for the heap3 memory manager. The header includes an array of block
 * structures that are used to track the free and
 * allocated buffers in the pool and a bitvector to track
 * available block structures.
 * The caller can use this macro to reserve bytes for the header that may be
 * passed optionally to \ref xmem_heap3_init.
 * The macro takes as parameter the number of blocks avaiable for the heap
 * manager to track the allocate/free buffers in the pool.
 * Use this macro to compute the header size on non cache-based subsystems.
 */
#define XMEM_HEAP3_HEADER_SIZE(num_blocks) \
  (XMEM_HEAP3_BLOCK_STRUCT_SIZE * (num_blocks) + (((num_blocks) + 31) >> 5) * 4)

/*!
 * Same as the macro \ref XMEM_HEAP3_HEADER_SIZE but the computed size
 * is aligned to the maximum data cache line size across all the cores
 * in the subsystem. Use this macro to compute the header size on cache-based
 * subsystems.
 */
#define XMEM_HEAP3_CACHE_ALIGNED_HEADER_SIZE(num_blocks)         \
  (((XMEM_HEAP3_HEADER_SIZE((num_blocks)) +                      \
     XMEM_MAX_DCACHE_LINESIZE - 1) / XMEM_MAX_DCACHE_LINESIZE) * \
   XMEM_MAX_DCACHE_LINESIZE)

/*!
 * Initialize the heap3 memory manager.
 *
 * \param m          Pointer to generic memory manager object.
 * \param buf        User provided pool for allocation.
 * \param size       Size of the pool.
 * \param num_blocks Number of blocks available to track the allocation/free
 *                   status of the user buffers. This is used only if the
 *                   \a header is set, else a default of 32 blocks are reserved
 *                   for tracking the user allocations.
 * \param header     Optional buffer used to store the header information.
 *                   required by the heap3 memory manager.
 *                   If NULL, the headers are allocated out of the user
 *                   provided pool.
 * \return           XMEM_OK if successful, else returns one of
 *                   - XMEM_ERROR_POOL_NULL
 *                   - XMEM_ERROR_POOL_SIZE
 *                   - XMEM_ERROR_INTERNAL
 */
xmem_status_t
xmem_heap3_init(xmem_mgr_t *m,
                void *buf,
                size_t size,
                uint32_t num_blocks,
                void *header);

/*!
 * Allocate a buffer of given input size and alignment using the
 * memory manager.
 *
 * \param mgr      Pointer to the generic memory manager object.
 * \param size     Size of the buffer to allocate.
 * \param align    Requested alignment of the buffer.
 * \param err_code Error status.
 * \return         Pointer to the allocated buffer. If the allocation succeeds,
 *                 the \a err_code is set to XMEM_OK else one of the following
 *                 error codes
 *                 - XMEM_ERROR_ALLOC_FAILED
 *                 - XMEM_ERROR_ILLEGAL_ALIGN
 *                 - XMEM_ERROR_UNKNOWN_HEAP_TYPE
 *                 is returned in \a err_code.
 */
extern void *
xmem_alloc(xmem_mgr_t *mgr,
           size_t size,
           uint32_t align,
           xmem_status_t *err_code);

/*!
 * Free an allocated buffer using the memory manager.
 *
 * \param mgr Pointer to the generic memory manager object.
 * \param p   Pointer to buffer to be freed.
 */
extern void
xmem_free(xmem_mgr_t *mgr, void *p);

/*!
 * Print heap usage statistics. Only avaiable in debug builds.
 *
 * \param mgr Pointer to generic memory manager object.
 */
extern void
xmem_print_stats(xmem_mgr_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* __XMEM_H__ */
