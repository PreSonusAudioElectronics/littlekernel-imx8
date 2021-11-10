/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CORE_AARCH64_GCC_H
#define __CORE_AARCH64_GCC_H

#define CORE_SYSREG(op1, crn, crm, op2) s3_##op1 ##_##crn ##_##crm ##_##op2

#define MPIDR_EL1        CORE_SYSREG(0, c0, c0, 5)
#define TTBR0_EL1        CORE_SYSREG(0, c2, c0, 0)
#define TTBR1_EL1        CORE_SYSREG(0, c2, c0, 1)
#define TCR_EL1          CORE_SYSREG(0, c2, c0, 2)
#define SCTLR_EL1        CORE_SYSREG(0, c1, c0, 0)
#define MAIR_EL1         CORE_SYSREG(0, c10, c2, 0)

#define ICC_IAR1_EL1     CORE_SYSREG(0, c12, c12, 0)
#define ICC_EOIR1_EL1    CORE_SYSREG(0, c12, c12, 1)
#define ICC_HPPIR1_EL1   CORE_SYSREG(0, c12, c12, 2)
#define ICC_BPR1_EL1     CORE_SYSREG(0, c12, c12, 3)
#define ICC_DIR_EL1      CORE_SYSREG(0, c12, c11, 1)
#define ICC_PMR_EL1      CORE_SYSREG(0, c4, c6, 0)
#define ICC_RPR_EL1      CORE_SYSREG(0, c12, c11, 3)
#define ICC_CTLR_EL1     CORE_SYSREG(0, c12, c12, 4)
#define ICC_SRE_EL1      CORE_SYSREG(0, c12, c12, 5)
#define ICC_SRE_EL2      CORE_SYSREG(4, c12, c9, 5)
#define ICC_IGRPEN1_EL1  CORE_SYSREG(0, c12, c12, 7)
#define ICC_AP1R0_EL1    CORE_SYSREG(0, c12, c9, 0)
#define ICC_AP1R1_EL1    CORE_SYSREG(0, c12, c9, 1)
#define ICC_AP1R2_EL1    CORE_SYSREG(0, c12, c9, 2)
#define ICC_AP1R3_EL1    CORE_SYSREG(0, c12, c9, 3)

#define __CORE_stringify_1(x...)     #x
#define __CORE_stringify(x...)       __CORE_stringify_1(x)

#define __MSR(sysreg, val) \
  asm volatile ("msr " __CORE_stringify(sysreg)", %0\n" : : "r"((uint64_t)(val)))

#define __MRS(sysreg, val) \
  asm volatile ("mrs  %0, " __CORE_stringify(sysreg)"\n" : "=r"((val)))

#define __DSB_SY() \
  asm volatile ("dsb sy")

#define __ISB() \
  asm volatile ("isb")

#define __NOP() \
  asm volatile ("nop")

#define __B() \
  asm volatile ("b .")

#define __TLBI() \
  asm volatile("dsb ishst\n"     \
               "tlbi vmalle1\n"  \
               "dsb sy\n"        \
               "isb\n")


FORCEDINLINE __STATIC_INLINE uint32_t __get_MPIDR_EL1(void)
{
    uint32_t result;
    __MRS(MPIDR_EL1, result);
    return result;
}

/* backward compatibility */
#define MPIDR_GetCoreID() (__get_MPIDR_EL1() & 0xff)

#endif /* __CORE_AARCH64_GCC_H */
