/**
 * @file m_cfifo.h
 * @brief Public API for circular FIFO buffer management with optional cascading support.
 *
 * This header defines the external interface for operating circular byte-based FIFO
 * buffers. The module supports:
 * - single-buffer FIFO operations (push, pop, clear, status queries)
 * - cascading multiple FIFO instances for extended storage
 * - optional dummy-byte behavior when no buffer is assigned
 *
 * The FIFO must be initialized using @ref m_cfifo_InitBuffer, and may optionally
 * be configured with a backing data buffer via @ref m_cfifo_ConfigBuffer before use.
 *
 * Thread safety:
 * - The module is designed for external semaphore protection.
 * - Public API functions are expected to be wrapped by the caller if required.
 *
 * @note Internal helper functions are declared only in the implementation file.
 *
 * @author Martin Langbein
 * @date 2025-11-23
 * @copyright GPLv2
 */

#ifndef M_CFIFO_H_
#define M_CFIFO_H_


#include <stdbool.h>
#include <inttypes.h>

//*****************************************************************************
// Global Defines
//*****************************************************************************


//*****************************************************************************
// Global Types
//*****************************************************************************

/**
 * @brief Direction selector for traversing cascaded FIFO buffers.
 *
 * This enumeration is used by multi-buffer operations such as
 * @ref m_cfifo_All_Clear, @ref m_cfifo_All_SetFull,
 * @ref m_cfifo_All_Pop, and @ref m_cfifo_All_IsEmpty
 * to determine which adjacent FIFO to access next.
 *
 * - `M_CFIFO_UP`   → move to `next` FIFO in the chain
 * - `M_CFIFO_DOWN` → move to `prev` FIFO in the chain
 */
typedef enum
{
  M_CFIFO_UP,
  M_CFIFO_DOWN
}m_cfifo_tDirection;


/**
 * @brief Control structure for a circular FIFO byte buffer.
 *
 * This structure stores all runtime state needed for FIFO operation,
 * including buffer pointers, usage tracking, and optional links to
 * adjacent FIFOs for cascading support.
 *
 * Usage requirements:
 * - Must be initialized using @ref m_cfifo_InitBuffer before use.
 * - A working data buffer may be assigned with @ref m_cfifo_ConfigBuffer.
 * - If no buffer is configured, pop operations return `dummy_byte`.
 *
 * The FIFO implements circular wrapping for both read and write indices.
 */
typedef struct _cfifo
{
  struct _cfifo* prev;
  struct _cfifo* next;

  uint8_t* buffer;
  uint16_t buffer_size;
  uint16_t used_count;
  uint16_t rdPtr;
  uint16_t wrPtr;
  
  uint8_t dummy_byte;
}m_cfifo_tCFifo;


//*****************************************************************************
// Global Variable Declarations (Extern)
//*****************************************************************************


//*****************************************************************************
// Global Function Prototypes
//*****************************************************************************

/**
 * @brief Initializes a CFIFO instance to a known default state.
 *
 * This function resets the FIFO structure pointers and sets the dummy byte
 * to zero. After initialization, the FIFO is unconfigured and must be
 * assigned a valid memory buffer using @ref m_cfifo_ConfigBuffer before use.
 *
 * @param cfifo Pointer to the FIFO instance to initialize.
 */
void m_cfifo_InitBuffer(m_cfifo_tCFifo* cfifo);


/**
 * @brief Links the given FIFO as the next FIFO in a cascade chain.
 *
 * Cascading allows operations like push, pop, clear, and state queries
 * to propagate through multiple FIFO buffers automatically.
 *
 * @warning This does not configure or initialize the buffer contents.
 *
 * @param cfifo       Pointer to the current FIFO.
 * @param cfifo_next  Pointer to the next FIFO to attach.
 */
void m_cfifo_CascadeAsNextBuffer(m_cfifo_tCFifo* cfifo, m_cfifo_tCFifo* cfifo_next);


/**
 * @brief Assigns a data buffer and size to the FIFO.
 *
 * This function sets the storage buffer for the FIFO. After configuration,
 * the FIFO is set to a full state, meaning all positions are marked as used.
 *
 * @note Passing NULL as buffer disables storage and enables dummy-byte output.
 *
 * @param cfifo        Pointer to the FIFO instance.
 * @param buffer       Pointer to a memory area for FIFO data.
 * @param buffer_size  Size of the buffer in bytes.
 */
void m_cfifo_ConfigBuffer(m_cfifo_tCFifo* cfifo, const void* buffer, uint16_t buffer_size);


/**
 * @brief Sets the dummy byte used when no real buffer is configured.
 *
 * When popping from a FIFO without an assigned buffer, the dummy byte
 * will be returned instead of real data.
 *
 * @param cfifo Pointer to the FIFO instance.
 * @param data  Dummy byte to return on pop operations.
 */
void m_cfifo_SetDummyByte(m_cfifo_tCFifo* cfifo, uint8_t data);


/**
 * @brief Pushes a single byte into this FIFO.
 *
 * Writes the given byte to the FIFO if space is available.
 *
 * @param cfifo Pointer to the FIFO instance.
 * @param data  Byte value to push.
 *
 * @retval true  Data successfully pushed.
 * @retval false FIFO is full or unconfigured.
 */
bool m_cfifo_This_Push(m_cfifo_tCFifo* cfifo, uint8_t data);


/**
 * @brief Attempts to push a byte into this FIFO or any cascaded successor.
 *
 * The function walks through chained FIFOs (via @ref next) until a push
 * succeeds or no more buffers are available.
 *
 * @param cfifo Pointer to the first FIFO in the cascade.
 * @param data  Byte value to push.
 *
 * @retval true  Data successfully pushed into at least one FIFO.
 * @retval false All FIFOs were full or invalid.
 */
bool m_cfifo_All_Push(m_cfifo_tCFifo* cfifo, uint8_t data);


/**
 * @brief Pops a single byte from this FIFO.
 *
 * Removes and returns the oldest byte in the FIFO.
 *
 * @param cfifo Pointer to the FIFO instance.
 * @param data  Output pointer to receive the popped byte (may be NULL).
 *
 * @retval true  Data successfully popped.
 * @retval false FIFO is empty.
 */
bool m_cfifo_This_Pop(m_cfifo_tCFifo* cfifo, uint8_t* data);


/**
 * @brief Pops a byte from this FIFO or any cascaded successor.
 *
 * Attempts to pop from the current FIFO; if empty, continues
 * through linked FIFOs until successful or no more remain.
 *
 * @param cfifo Pointer to the first FIFO in the cascade.
 * @param data  Output pointer to receive the popped byte (may be NULL).
 *
 * @retval true  Data successfully popped from at least one FIFO.
 * @retval false All FIFOs were empty.
 */
bool m_cfifo_All_Pop(m_cfifo_tCFifo* cfifo, uint8_t* data);


/**
 * @brief Clears all stored data in this FIFO.
 *
 * Resets read/write pointers and usage count to zero.
 *
 * @param cfifo Pointer to the FIFO instance.
 */
void m_cfifo_This_Clear(m_cfifo_tCFifo* cfifo);


/**
 * @brief Clears one or more FIFOs in a cascade chain.
 *
 * Traverses adjacent FIFOs according to the specified direction and clears
 * each one until no further buffers are linked.
 *
 * @param cfifo     Pointer to the starting FIFO.
 * @param direction Direction of traversal (UP = next, DOWN = previous).
 */
void m_cfifo_All_Clear(m_cfifo_tCFifo* cfifo, m_cfifo_tDirection direction);


/**
 * @brief Marks this FIFO as full.
 *
 * Sets used count to buffer capacity without modifying stored data.
 * Useful for initializing prefilled buffers.
 *
 * @param cfifo Pointer to the FIFO instance.
 */
void m_cfifo_This_SetFull(m_cfifo_tCFifo* cfifo);


/**
 * @brief Marks all FIFOs in a cascade chain as full.
 *
 * Applies @ref m_cfifo_This_SetFull repeatedly in the given direction.
 *
 * @param cfifo     Pointer to the starting FIFO.
 * @param direction Direction of traversal.
 */
void m_cfifo_All_SetFull(m_cfifo_tCFifo* cfifo, m_cfifo_tDirection direction);


/**
 * @brief Returns the configured size of this FIFO.
 *
 * @param cfifo Pointer to the FIFO instance.
 * @return Buffer size in bytes.
 */
uint16_t m_cfifo_This_GetSize(m_cfifo_tCFifo* cfifo);


/**
 * @brief Returns the total size of all cascaded FIFOs.
 *
 * Adds the configured sizes of each FIFO reachable through @ref next.
 *
 * @param cfifo Pointer to the first FIFO in the cascade.
 * @return Total size in bytes.
 */
uint32_t m_cfifo_All_GetSize(m_cfifo_tCFifo* cfifo);


/**
 * @brief Returns the number of used elements in this FIFO.
 *
 * @param cfifo Pointer to the FIFO instance.
 * @return Number of stored bytes.
 */
uint16_t m_cfifo_This_GetUsage(m_cfifo_tCFifo* cfifo);


/**
 * @brief Returns total usage across cascaded FIFOs.
 *
 * @param cfifo Pointer to the first FIFO in the cascade.
 * @return Sum of used bytes across all buffers.
 */
uint32_t m_cfifo_All_GetUsage(m_cfifo_tCFifo* cfifo);


/**
 * @brief Checks whether this FIFO is empty.
 *
 * @param cfifo Pointer to the FIFO instance.
 *
 * @retval true  FIFO contains no data.
 * @retval false FIFO has at least one byte stored.
 */
bool m_cfifo_This_IsEmpty(m_cfifo_tCFifo* cfifo);


/**
 * @brief Checks whether all cascaded FIFOs are empty.
 *
 * The function returns true only if every FIFO in the traversal chain
 * contains zero stored elements.
 *
 * @param cfifo Pointer to the first FIFO in the cascade.
 *
 * @retval true  All FIFOs are empty.
 * @retval false At least one FIFO contains data.
 */
bool m_cfifo_All_IsEmpty(m_cfifo_tCFifo* cfifo);


/**
 * @brief Checks whether this FIFO is full.
 *
 * @param cfifo Pointer to the FIFO instance.
 *
 * @retval true  FIFO has no remaining space.
 * @retval false At least one byte can still be stored.
 */
bool m_cfifo_This_IsFull(m_cfifo_tCFifo* cfifo);


/**
 * @brief Checks whether all cascaded FIFOs are full.
 *
 * Returns true only if every FIFO in the chain reports full status.
 *
 * @param cfifo Pointer to the first FIFO in the cascade.
 *
 * @retval true  All FIFOs are full.
 * @retval false At least one FIFO has free space.
 */
bool m_cfifo_All_IsFull(m_cfifo_tCFifo* cfifo);


#endif /* M_CFIFO_H_ */