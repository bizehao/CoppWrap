/*
            Copyright Oliver Kowalke 2009.
            Copyright Thomas Sailer 2013.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
            http://www.boost.org/LICENSE_1_0.txt)
*/

/*************************************************************************************
* ---------------------------------------------------------------------------------- *
* |     0   |     1   |     2    |     3   |     4   |     5   |     6   |     7   | *
* ---------------------------------------------------------------------------------- *
* |    0x0  |    0x4  |    0x8   |    0xc  |   0x10  |   0x14  |   0x18  |   0x1c  | *
* ---------------------------------------------------------------------------------- *
* |                          SEE registers (XMM6-XMM15)                            | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |     8   |    9    |    10    |    11   |    12   |    13   |    14   |    15   | *
* ---------------------------------------------------------------------------------- *
* |   0x20  |  0x24   |   0x28   |   0x2c  |   0x30  |   0x34  |   0x38  |   0x3c  | *
* ---------------------------------------------------------------------------------- *
* |                          SEE registers (XMM6-XMM15)                            | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |    16   |    17   |    18   |    19    |    20   |    21   |    22   |    23   | *
* ---------------------------------------------------------------------------------- *
* |   0xe40  |   0x44 |   0x48  |   0x4c   |   0x50  |   0x54  |   0x58  |   0x5c  | *
* ---------------------------------------------------------------------------------- *
* |                          SEE registers (XMM6-XMM15)                            | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |    24   |   25    |    26    |   27    |    28   |    29   |    30   |    31   | *
* ---------------------------------------------------------------------------------- *
* |   0x60  |   0x64  |   0x68   |   0x6c  |   0x70  |   0x74  |   0x78  |   0x7c  | *
* ---------------------------------------------------------------------------------- *
* |                          SEE registers (XMM6-XMM15)                            | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |    32   |   32    |    33    |   34    |    35   |    36   |    37   |    38   | *
* ---------------------------------------------------------------------------------- *
* |   0x80  |   0x84  |   0x88   |   0x8c  |   0x90  |   0x94  |   0x98  |   0x9c  | *
* ---------------------------------------------------------------------------------- *
* |                          SEE registers (XMM6-XMM15)                            | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |    39   |   40    |    41    |   42    |    43   |    44   |    45   |    46   | *
* ---------------------------------------------------------------------------------- *
* |   0xa0  |   0xa4  |   0xa8   |   0xac  |   0xb0  |   0xb4  |   0xb8  |   0xbc  | *
* ---------------------------------------------------------------------------------- *
* | fc_mxcsr|fc_x87_cw|     <alignment>    |       fbr_strg    |      fc_dealloc   | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |    47   |   48    |    49    |   50    |    51   |    52   |    53   |    54   | *
* ---------------------------------------------------------------------------------- *
* |   0xc0  |   0xc4  |   0xc8   |   0xcc  |   0xd0  |   0xd4  |   0xd8  |   0xdc  | *
* ---------------------------------------------------------------------------------- *
* |        limit      |         base       |         R12       |         R13       | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |    55   |   56    |    57    |   58    |    59   |    60   |    61   |    62   | *
* ---------------------------------------------------------------------------------- *
* |   0xe0  |   0xe4  |   0xe8   |   0xec  |   0xf0  |   0xf4  |   0xf8  |   0xfc  | *
* ---------------------------------------------------------------------------------- *
* |        R14        |         R15        |         RDI       |        RSI        | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |    63   |   64    |    65    |   66    |    67   |    68   |    69   |    70   | *
* ---------------------------------------------------------------------------------- *
* |  0x100  |  0x104  |  0x108   |  0x10c  |  0x110  |  0x114  |  0x118  |  0x11c  | *
* ---------------------------------------------------------------------------------- *
* |        RBX        |         RBP        |       hidden      |        RIP        | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |    71   |   72    |    73    |   74    |    75   |    76   |    77   |    78   | *
* ---------------------------------------------------------------------------------- *
* |  0x120  |  0x124  |  0x128   |  0x12c  |  0x130  |  0x134  |  0x138  |  0x13c  | *
* ---------------------------------------------------------------------------------- *
* |                                   parameter area                               | *
* ---------------------------------------------------------------------------------- *
* ---------------------------------------------------------------------------------- *
* |    79   |   80    |    81    |   82    |    83   |    84   |    85   |    86   | *
* ---------------------------------------------------------------------------------- *
* |  0x140  |  0x144  |  0x148   |  0x14c  |  0x150  |  0x154  |  0x158  |  0x15c  | *
* ---------------------------------------------------------------------------------- *
* |       FCTX        |        DATA        |                                       | *
* ---------------------------------------------------------------------------------- *
**************************************************************************************/

.file	"ontop_x86_64_ms_pe_gas.asm"
.text
.p2align 4,,15
.globl	copp_ontop_fcontext_v2
.def	copp_ontop_fcontext_v2;	.scl	2;	.type	32;	.endef
.seh_proc	copp_ontop_fcontext_v2
copp_ontop_fcontext_v2:
.seh_endprologue

    leaq  -0x118(%rsp), %rsp /* prepare stack */

#if !defined(LIBCOPP_FCONTEXT_USE_TSX)
    /* save XMM storage */
    movaps  %xmm6, 0x0(%rsp)
    movaps  %xmm7, 0x10(%rsp)
    movaps  %xmm8, 0x20(%rsp)
    movaps  %xmm9, 0x30(%rsp)
    movaps  %xmm10, 0x40(%rsp)
    movaps  %xmm11, 0x50(%rsp)
    movaps  %xmm12, 0x60(%rsp)
    movaps  %xmm13, 0x70(%rsp)
    movaps  %xmm14, 0x80(%rsp)
    movaps  %xmm15, 0x90(%rsp)
    stmxcsr  0xa0(%rsp)  /* save MMX control- and status-word */
    fnstcw   0xa4(%rsp)  /* save x87 control-word */
#endif

    /* load NT_TIB */
    movq  %gs:(0x30), %r10
    /* save fiber local storage */
    movq  0x20(%r10), %rax
    movq  %rax, 0xb0(%rsp)
    /* save current deallocation stack */
    movq  0x1478(%r10), %rax
    movq  %rax, 0xb8(%rsp)
    /* save current stack limit */
    movq  0x10(%r10), %rax
    movq  %rax, 0xc0(%rsp)
    /* save current stack base */
    movq  0x08(%r10), %rax
    movq  %rax, 0xc8(%rsp)

    movq  %r12, 0xd0(%rsp)  /* save R12 */
    movq  %r13, 0xd8(%rsp)  /* save R13 */
    movq  %r14, 0xe0(%rsp)  /* save R14 */
    movq  %r15, 0xe8(%rsp)  /* save R15 */
    movq  %rdi, 0xf0(%rsp)  /* save RDI */
    movq  %rsi, 0xf8(%rsp)  /* save RSI */
    movq  %rbx, 0x100(%rsp)  /* save RBX */
    movq  %rbp, 0x108(%rsp)  /* save RBP */

    movq  %rcx, 0x110(%rsp)  /* save hidden address of transport_t */

    /* preserve RSP (pointing to context-data) in RCX */
    movq  %rsp, %rcx

    /* restore RSP (pointing to context-data) from RDX */
    movq  %rdx, %rsp

#if !defined(LIBCOPP_FCONTEXT_USE_TSX)
    /* restore XMM storage */
    movaps  0x0(%rsp), %xmm6
    movaps  0x10(%rsp), %xmm7
    movaps  0x20(%rsp), %xmm8
    movaps  0x30(%rsp), %xmm9
    movaps  0x40(%rsp), %xmm10
    movaps  0x50(%rsp), %xmm11
    movaps  0x60(%rsp), %xmm12
    movaps  0x70(%rsp), %xmm13
    movaps  0x80(%rsp), %xmm14
    movaps  0x90(%rsp), %xmm15
 	ldmxcsr 0xa0(%rsp) /* restore MMX control- and status-word */
 	fldcw   0xa4(%rsp) /* restore x87 control-word */
#endif

    /* load NT_TIB */
    movq  %gs:(0x30), %r10
    /* restore fiber local storage */
    movq  0xb0(%rsp), %rax
    movq  %rax, 0x20(%r10)
    /* restore current deallocation stack */
    movq  0xb8(%rsp), %rax
    movq  %rax, 0x1478(%r10)
    /* restore current stack limit */
    movq  0xc0(%rsp), %rax
    movq  %rax, 0x10(%r10)
    /* restore current stack base */
    movq  0xc8(%rsp), %rax
    movq  %rax, 0x08(%r10)

    movq  0xd0(%rsp),  %r12  /* restore R12 */
    movq  0xd8(%rsp),  %r13  /* restore R13 */
    movq  0xe0(%rsp),  %r14  /* restore R14 */
    movq  0xe8(%rsp),  %r15  /* restore R15 */
    movq  0xf0(%rsp),  %rdi  /* restore RDI */
    movq  0xf8(%rsp),  %rsi  /* restore RSI */
    movq  0x100(%rsp), %rbx  /* restore RBX */
    movq  0x108(%rsp), %rbp  /* restore RBP */

    movq  0x110(%rsp), %rax  /* restore hidden address of transport_t */

    leaq  0x118(%rsp), %rsp /* prepare stack */

    /* keep return-address on stack */

    /* transport_t returned in RAX */
    /* return parent fcontext_t */
    movq  %rcx, 0x0(%rax)
    /* return data */
    movq  %r8, 0x8(%rax)

    /* transport_t as 1.arg of context-function */
    /* RCX contains address of returned (hidden) transfer_t */
    movq  %rax, %rcx
    /* RDX contains address of passed transfer_t */
    movq  %rax, %rdx

    /* indirect jump to context */
    jmp  *%r9
.seh_endproc

.section .drectve
.ascii " -export:\"copp_ontop_fcontext_v2\""
