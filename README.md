
# m_cfifo – Circular FIFO Buffer Library with Cascading Support

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

## Overview

`m_cfifo` is a lightweight and efficient **circular FIFO (First-In-First-Out) buffer library** implemented in C. It is designed for embedded systems and real-time applications where efficient byte storage and retrieval are required.  

The library supports **single FIFO buffers** as well as **cascaded buffers**, allowing multiple FIFO instances to be chained together for extended storage capacity.  

It also supports **dummy-byte behavior** for unconfigured buffers, providing safe fallback values during pop operations.  

This library is **licensed under GPLv2**.

---

## Features

- Single FIFO operations:
  - Push a byte (`m_cfifo_This_Push`)
  - Pop a byte (`m_cfifo_This_Pop`)
  - Clear the FIFO (`m_cfifo_This_Clear`)
  - Query usage and size
  - Check full/empty status
- Cascaded FIFO operations:
  - Push/pop across multiple buffers (`m_cfifo_All_Push`, `m_cfifo_All_Pop`)
  - Clear or mark full across linked buffers (`m_cfifo_All_Clear`, `m_cfifo_All_SetFull`)
  - Query combined usage and size (`m_cfifo_All_GetUsage`, `m_cfifo_All_GetSize`)
  - Check full/empty status across all buffers (`m_cfifo_All_IsFull`, `m_cfifo_All_IsEmpty`)
- Thread safety:
  - Designed for external semaphore protection (optional)
- Circular buffer design:
  - Read/write indices wrap automatically using modulo arithmetic
- Configurable:
  - Optional dummy byte returned when no buffer is assigned
- Minimal memory footprint

---

## Data Structures

### `m_cfifo_tCFifo`

Represents a single FIFO buffer instance.

```c
typedef struct _cfifo {
    struct _cfifo* prev;       // Pointer to previous FIFO in cascade
    struct _cfifo* next;       // Pointer to next FIFO in cascade
    uint8_t* buffer;           // Data storage buffer
    uint16_t buffer_size;      // Size of the buffer in bytes
    uint16_t used_count;       // Number of bytes currently stored
    uint16_t rdPtr;            // Read index
    uint16_t wrPtr;            // Write index
    uint8_t dummy_byte;        // Dummy byte returned if buffer is NULL
} m_cfifo_tCFifo;
```

### `m_cfifo_tDirection`

Enumeration to select traversal direction for cascaded operations.

```c
typedef enum {
    M_CFIFO_UP,   // Traverse toward next FIFO
    M_CFIFO_DOWN  // Traverse toward previous FIFO
} m_cfifo_tDirection;
```

---

## Initialization

All FIFO instances **must** be initialized before use:

```c
m_cfifo_tCFifo fifo;
m_cfifo_InitBuffer(&fifo);           // Initialize FIFO structure
m_cfifo_ConfigBuffer(&fifo, buffer, buffer_size); // Assign backing buffer
m_cfifo_SetDummyByte(&fifo, 0x00);  // Optional dummy byte
```

Cascading multiple buffers:

```c
m_cfifo_CascadeAsNextBuffer(&fifo1, &fifo2);
```

---

## Single FIFO Operations

```c
// Push a byte
bool success = m_cfifo_This_Push(&fifo, data);

// Pop a byte
uint8_t value;
bool has_data = m_cfifo_This_Pop(&fifo, &value);

// Clear FIFO
m_cfifo_This_Clear(&fifo);

// Set FIFO as full
m_cfifo_This_SetFull(&fifo);

// Query size and usage
uint16_t size = m_cfifo_This_GetSize(&fifo);
uint16_t used = m_cfifo_This_GetUsage(&fifo);

// Check if empty/full
bool empty = m_cfifo_This_IsEmpty(&fifo);
bool full  = m_cfifo_This_IsFull(&fifo);
```

---

## Cascaded FIFO Operations

```c
// Push across all linked buffers
bool success = m_cfifo_All_Push(&fifo1, data);

// Pop across all linked buffers
uint8_t value;
bool has_data = m_cfifo_All_Pop(&fifo1, &value);

// Clear all linked buffers (UP or DOWN)
m_cfifo_All_Clear(&fifo1, M_CFIFO_UP);

// Mark all linked buffers as full
m_cfifo_All_SetFull(&fifo1, M_CFIFO_UP);

// Query total usage and size
uint32_t total_used = m_cfifo_All_GetUsage(&fifo1);
uint32_t total_size = m_cfifo_All_GetSize(&fifo1);

// Check if all linked buffers are empty/full
bool all_empty = m_cfifo_All_IsEmpty(&fifo1);
bool all_full  = m_cfifo_All_IsFull(&fifo1);
```

---

## Design Notes

- Read/write indices wrap automatically (`rdPtr`, `wrPtr`) using modulo arithmetic.
- If no buffer is assigned, pop operations return the configured **dummy byte**.
- Cascading allows multi-buffer storage by linking multiple `m_cfifo_tCFifo` instances.
- Internal functions (e.g., `m_cfifo_This_PushInternal`) are **static** and should not be called outside the module.
- Semaphore protection points are indicated in comments; actual protection must be implemented externally if needed.

---

## Internal Functions (for reference)

- `m_cfifo_This_PushInternal` – Inserts a byte without semaphore protection.
- `m_cfifo_This_PopInternal` – Removes a byte without semaphore protection.
- `m_cfifo_This_ClearInternal` – Resets FIFO state.
- `m_cfifo_This_SetFullInternal` – Marks FIFO as full.
- `m_cfifo_This_GetSizeInternal` – Returns buffer capacity.
- `m_cfifo_This_GetUsageInternal` – Returns number of stored bytes.
- `m_cfifo_This_IsEmptyInternal` / `m_cfifo_This_IsFullInternal` – Check FIFO state.
- `m_cfifo_IncRdPtr` / `m_cfifo_IncWrPtr` – Increment read/write pointers with wrap-around.
- `m_cfifo_GetAdjacentFifo` – Returns the next or previous FIFO in cascade.

---

## License

This project is licensed under the **GNU General Public License v2 (GPL-2.0)**.  

See [GPL v2 License](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html) for full details.

---

## Author

**Martin Langbein**  
Date: 2025-11-23  

---

## Example Usage

```c
#include "m_cfifo.h"

#define BUFFER_SIZE 16
uint8_t buffer1[BUFFER_SIZE];
uint8_t buffer2[BUFFER_SIZE];

int main(void) {
    m_cfifo_tCFifo fifo1, fifo2;

    // Initialize buffers
    m_cfifo_InitBuffer(&fifo1);
    m_cfifo_InitBuffer(&fifo2);

    // Configure backing storage
    m_cfifo_ConfigBuffer(&fifo1, buffer1, BUFFER_SIZE);
    m_cfifo_ConfigBuffer(&fifo2, buffer2, BUFFER_SIZE);

    // Set dummy bytes
    m_cfifo_SetDummyByte(&fifo1, 0xFF);
    m_cfifo_SetDummyByte(&fifo2, 0xFF);

    // Cascade buffers
    m_cfifo_CascadeAsNextBuffer(&fifo1, &fifo2);

    // Push data
    m_cfifo_All_Push(&fifo1, 0x42);

    // Pop data
    uint8_t value;
    m_cfifo_All_Pop(&fifo1, &value);

    return 0;
}
```

---

## Conclusion

`m_cfifo` provides a **robust and flexible circular FIFO solution** for embedded C projects, with support for cascading multiple buffers, dummy-byte fallback, and modular API design. It is lightweight, thread-aware, and GPLv2 licensed.
