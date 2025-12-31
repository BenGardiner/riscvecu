.section .text.entry
.global _start

_start:
    # Set stack pointer to top of RAM (128K at 0x80000000)
    # The `la` pseudo-instruction might be misinterpreted by the 64-bit toolchain.
    # We use the explicit `lui` instruction for 32-bit address loading.
    lui sp, 0x80020
    call main

loop:
    j loop

# Minimal implementation of memcpy and memset for bare metal
.global memcpy
.type memcpy, @function
memcpy:
    # a0 = dest, a1 = src, a2 = n
    mv t0, a0        # Save dest to return it
1:
    beqz a2, 2f      # If n == 0, done
    lb t1, 0(a1)     # Load byte from src
    sb t1, 0(a0)     # Store byte to dest
    addi a0, a0, 1   # Increment dest
    addi a1, a1, 1   # Increment src
    addi a2, a2, -1  # Decrement n
    j 1b
2:
    mv a0, t0        # Return original dest
    ret

.global memset
.type memset, @function
memset:
    # a0 = dest, a1 = val, a2 = n
    mv t0, a0        # Save dest to return it
1:
    beqz a2, 2f      # If n == 0, done
    sb a1, 0(a0)     # Store val to dest
    addi a0, a0, 1   # Increment dest
    addi a2, a2, -1  # Decrement n
    j 1b
2:
    mv a0, t0        # Return original dest
    ret
