/*!
 * \file mimx8mn_irq.h
 * \author D. Anderson
 * \brief Separated IRQ number declarations out from
 * MIMX8MN6_ca53.h to fix conflicting declarations
 * 
 * Copyright (c) 2022 Presonus Audio Electronics
 * 
 */

#ifndef _mimx8mn_irq_h
#define _mimx8mn_irq_h

/* ----------------------------------------------------------------------------
   -- Interrupt vector numbers
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Interrupt_vector_numbers Interrupt vector numbers
 * @{
 */

/** Interrupt Number Definitions */
#define NUMBER_OF_INT_VECTORS 144 /**< Number of interrupts in the Vector table */

typedef enum IRQn
{
    /* Auxiliary constants */
    NotAvail_IRQn = -128, /**< Not available device specific interrupt */

    /* Core interrupts */
    NonMaskableInt_IRQn   = -14, /**< Non Maskable Interrupt */
    HardFault_IRQn        = -13, /**< Cortex-M7 SV Hard Fault Interrupt */
    MemoryManagement_IRQn = -12, /**< Cortex-M7 Memory Management Interrupt */
    BusFault_IRQn         = -11, /**< Cortex-M7 Bus Fault Interrupt */
    UsageFault_IRQn       = -10, /**< Cortex-M7 Usage Fault Interrupt */
    SVCall_IRQn           = -5,  /**< Cortex-M7 SV Call Interrupt */
    DebugMonitor_IRQn     = -4,  /**< Cortex-M7 Debug Monitor Interrupt */
    PendSV_IRQn           = -2,  /**< Cortex-M7 Pend SV Interrupt */
    SysTick_IRQn          = -1,  /**< Cortex-M7 System Tick Interrupt */

    /* Device specific interrupts */
    GPR_IRQ_IRQn           = 0,  /**< GPR Interrupt. Used to notify cores on exception condition while boot. */
    DAP_IRQn               = 1,  /**< DAP Interrupt */
    SDMA1_IRQn             = 2,  /**< AND of all 48 SDMA1 interrupts (events) from all the channels */
    GPU3D_IRQn             = 3,  /**< GPU3D Interrupt */
    SNVS_IRQn              = 4,  /**< ON-OFF button press shorter than 5 seconds (pulse event) */
    LCDIF_IRQn             = 5,  /**< LCDIF Interrupt */
    SPDIF1_IRQn            = 6,  /**< SPDIF1 RZX/TX Interrupt */
    Reserved23_IRQn        = 7,  /**< Reserved Interrupt */
    Reserved24_IRQn        = 8,  /**< Reserved Interrupt */
    QOS_IRQn               = 9,  /**< QOS interrupt */
    WDOG3_IRQn             = 10, /**< Watchdog Timer reset */
    HS_CP1_IRQn            = 11, /**< HS Interrupt Request */
    APBHDMA_IRQn           = 12, /**< GPMI operation channel 0-3 description complete interrupt */
    Reserved29_IRQn        = 13, /**< Reserved */
    BCH_IRQn               = 14, /**< BCH operation complete interrupt */
    GPMI_IRQn              = 15, /**< GPMI operation TIMEOUT ERROR interrupt */
    ISI_CH0_IRQn           = 16, /**< ISI Camera Channel 0 Interrupt */
    MIPI_CSI1_IRQn         = 17, /**< MIPI CSI Interrupt */
    MIPI_DSI_IRQn          = 18, /**< MIPI DSI Interrupt */
    SNVS_Consolidated_IRQn = 19, /**< SRTC Consolidated Interrupt. Non TZ. */
    SNVS_Security_IRQn     = 20, /**< SRTC Security Interrupt. TZ. */
    CSU_IRQn =
        21, /**< CSU Interrupt Request. Indicates to the processor that one or more alarm inputs were asserted. */
    USDHC1_IRQn          = 22, /**< uSDHC1 Enhanced SDHC Interrupt Request */
    USDHC2_IRQn          = 23, /**< uSDHC2 Enhanced SDHC Interrupt Request */
    USDHC3_IRQn          = 24, /**< uSDHC3 Enhanced SDHC Interrupt Request */
    Reserved41_IRQn      = 25, /**< Reserved Interrupt */
    UART1_IRQn           = 26, /**< UART-1 ORed interrupt */
    UART2_IRQn           = 27, /**< UART-2 ORed interrupt */
    UART3_IRQn           = 28, /**< UART-3 ORed interrupt */
    UART4_IRQn           = 29, /**< UART-4 ORed interrupt */
    Reserved46_IRQn      = 30, /**< Reserved Interrupt */
    ECSPI1_IRQn          = 31, /**< ECSPI1 interrupt request line to the core. */
    ECSPI2_IRQn          = 32, /**< ECSPI2 interrupt request line to the core. */
    ECSPI3_IRQn          = 33, /**< ECSPI3 interrupt request line to the core. */
    SDMA3_IRQn           = 34, /**< AND of all 48 SDMA3 interrupts (events) from all the channels */
    I2C1_IRQn            = 35, /**< I2C-1 Interrupt */
    I2C2_IRQn            = 36, /**< I2C-2 Interrupt */
    I2C3_IRQn            = 37, /**< I2C-3 Interrupt */
    I2C4_IRQn            = 38, /**< I2C-4 Interrupt */
    RDC_IRQn             = 39, /**< RDC interrupt */
    USB1_IRQn            = 40, /**< USB1 Interrupt */
    Reserved57_IRQn      = 41, /**< Reserved Interrupt */
    ISI_CH1_IRQn         = 42, /**< ISI Camera Channel 1 Interrupt */
    ISI_CH2_IRQn         = 43, /**< ISI Camera Channel 2 Interrupt */
    PDM_HWVAD_EVENT_IRQn = 44, /**< Digital Microphone interface voice activity detector event interrupt */
    PDM_HWVAD_ERROR_IRQn = 45, /**< Digital Microphone interface voice activity detector error interrupt */
    GPT6_IRQn = 46, /**< OR of GPT Rollover interrupt line, Input Capture 1 and 2 lines, Output Compare 1, 2, and 3
                       Interrupt lines */
    SCTR_IRQ0_IRQn   = 47, /**< System Counter Interrupt 0 */
    SCTR_IRQ1_IRQn   = 48, /**< System Counter Interrupt 1 */
    TEMPMON_LOW_IRQn = 49, /**< TempSensor (Temperature low alarm). */
    I2S3_IRQn        = 50, /**< SAI3 Receive / Transmit Interrupt */
    GPT5_IRQn = 51, /**< OR of GPT Rollover interrupt line, Input Capture 1 and 2 lines, Output Compare 1, 2, and 3
                       Interrupt lines */
    GPT4_IRQn = 52, /**< OR of GPT Rollover interrupt line, Input Capture 1 and 2 lines, Output Compare 1, 2, and 3
                       Interrupt lines */
    GPT3_IRQn = 53, /**< OR of GPT Rollover interrupt line, Input Capture 1 and 2 lines, Output Compare 1, 2, and 3
                       Interrupt lines */
    GPT2_IRQn = 54, /**< OR of GPT Rollover interrupt line, Input Capture 1 and 2 lines, Output Compare 1, 2, and 3
                       Interrupt lines */
    GPT1_IRQn = 55, /**< OR of GPT Rollover interrupt line, Input Capture 1 and 2 lines, Output Compare 1, 2, and 3
                       Interrupt lines */
    GPIO1_INT7_IRQn           = 56, /**< Active HIGH Interrupt from INT7 from GPIO */
    GPIO1_INT6_IRQn           = 57, /**< Active HIGH Interrupt from INT6 from GPIO */
    GPIO1_INT5_IRQn           = 58, /**< Active HIGH Interrupt from INT5 from GPIO */
    GPIO1_INT4_IRQn           = 59, /**< Active HIGH Interrupt from INT4 from GPIO */
    GPIO1_INT3_IRQn           = 60, /**< Active HIGH Interrupt from INT3 from GPIO */
    GPIO1_INT2_IRQn           = 61, /**< Active HIGH Interrupt from INT2 from GPIO */
    GPIO1_INT1_IRQn           = 62, /**< Active HIGH Interrupt from INT1 from GPIO */
    GPIO1_INT0_IRQn           = 63, /**< Active HIGH Interrupt from INT0 from GPIO */
    GPIO1_Combined_0_15_IRQn  = 64, /**< Combined interrupt indication for GPIO1 signal 0 throughout 15 */
    GPIO1_Combined_16_31_IRQn = 65, /**< Combined interrupt indication for GPIO1 signal 16 throughout 31 */
    GPIO2_Combined_0_15_IRQn  = 66, /**< Combined interrupt indication for GPIO2 signal 0 throughout 15 */
    GPIO2_Combined_16_31_IRQn = 67, /**< Combined interrupt indication for GPIO2 signal 16 throughout 31 */
    GPIO3_Combined_0_15_IRQn  = 68, /**< Combined interrupt indication for GPIO3 signal 0 throughout 15 */
    GPIO3_Combined_16_31_IRQn = 69, /**< Combined interrupt indication for GPIO3 signal 16 throughout 31 */
    GPIO4_Combined_0_15_IRQn  = 70, /**< Combined interrupt indication for GPIO4 signal 0 throughout 15 */
    GPIO4_Combined_16_31_IRQn = 71, /**< Combined interrupt indication for GPIO4 signal 16 throughout 31 */
    GPIO5_Combined_0_15_IRQn  = 72, /**< Combined interrupt indication for GPIO5 signal 0 throughout 15 */
    GPIO5_Combined_16_31_IRQn = 73, /**< Combined interrupt indication for GPIO5 signal 16 throughout 31 */
    Reserved90_IRQn           = 74, /**< Reserved interrupt */
    Reserved91_IRQn           = 75, /**< Reserved interrupt */
    Reserved92_IRQn           = 76, /**< Reserved interrupt */
    Reserved93_IRQn           = 77, /**< Reserved interrupt */
    WDOG1_IRQn                = 78, /**< Watchdog Timer reset */
    WDOG2_IRQn                = 79, /**< Watchdog Timer reset */
    Reserved96_IRQn           = 80, /**< Reserved interrupt */
    PWM1_IRQn = 81, /**< Cumulative interrupt line. OR of Rollover Interrupt line, Compare Interrupt line and FIFO
                       Waterlevel crossing interrupt line. */
    PWM2_IRQn = 82, /**< Cumulative interrupt line. OR of Rollover Interrupt line, Compare Interrupt line and FIFO
                       Waterlevel crossing interrupt line. */
    PWM3_IRQn = 83, /**< Cumulative interrupt line. OR of Rollover Interrupt line, Compare Interrupt line and FIFO
                       Waterlevel crossing interrupt line. */
    PWM4_IRQn = 84, /**< Cumulative interrupt line. OR of Rollover Interrupt line, Compare Interrupt line and FIFO
                       Waterlevel crossing interrupt line. */
    CCM_IRQ1_IRQn               = 85,  /**< CCM Interrupt Request 1 */
    CCM_IRQ2_IRQn               = 86,  /**< CCM Interrupt Request 2 */
    GPC_IRQn                    = 87,  /**< GPC Interrupt Request 1 */
    MU_A53_IRQn                 = 88,  /**< Interrupt to A53 */
    SRC_IRQn                    = 89,  /**< SRC interrupt request */
    I2S56_IRQn                  = 90,  /**< SAI5/6 Receive / Transmit Interrupt */
    RTIC_IRQn                   = 91,  /**< RTIC Interrupt */
    CPU_PerformanceUnit_IRQn    = 92,  /**< Performance Unit Interrupts from Cheetah (interrnally: PMUIRQ[n] */
    CPU_CTI_Trigger_IRQn        = 93,  /**< CTI trigger outputs (internal: nCTIIRQ[n] */
    SRC_Combined_IRQn           = 94,  /**< Combined CPU wdog interrupts (4x) out of SRC. */
    Reserved111_IRQn            = 95,  /**< Reserved Interrupt */
    I2S2_IRQn                   = 96,  /**< SAI2 Receive / Transmit Interrupt */
    MU_M7_IRQn                  = 97,  /**< Interrupt to M7 */
    DDR_PerformanceMonitor_IRQn = 98,  /**< ddr Interrupt for performance monitor */
    DDR_IRQn                    = 99,  /**< ddr Interrupt */
    Reserved116_IRQn            = 100, /**< Reserved interrupt */
    CPU_Error_AXI_IRQn   = 101, /**< CPU Error indicator for AXI transaction with a write response error condition */
    CPU_Error_L2RAM_IRQn = 102, /**< CPU Error indicator for L2 RAM double-bit ECC error */
    SDMA2_IRQn           = 103, /**< AND of all 48 SDMA2 interrupts (events) from all the channels */
    SJC_IRQn             = 104, /**< Interrupt triggered by SJC register */
    CAAM_IRQ0_IRQn       = 105, /**< CAAM interrupt queue for JQ */
    CAAM_IRQ1_IRQn       = 106, /**< CAAM interrupt queue for JQ */
    QSPI_IRQn            = 107, /**< QSPI Interrupt */
    TZASC_IRQn           = 108, /**< TZASC (PL380) interrupt */
    PDM_EVENT_IRQn       = 109, /**< Digital Microphone interface interrupt */
    PDM_ERROR_IRQn       = 110, /**< Digital Microphone interface error interrupt */
    I2S7_IRQn            = 111, /**< SAI7 Receive / Transmit Interrupt */
    PERFMON1_IRQn        = 112, /**< General Interrupt */
    PERFMON2_IRQn        = 113, /**< General Interrupt */
    CAAM_IRQ2_IRQn       = 114, /**< CAAM interrupt queue for JQ */
    CAAM_ERROR_IRQn      = 115, /**< Recoverable error interrupt */
    HS_CP0_IRQn          = 116, /**< HS Interrupt Request */
    CM7_CTI_IRQn         = 117, /**< CTI trigger outputs from CM7 platform */
    ENET_MAC0_Rx_Tx_Done1_IRQn = 118, /**< MAC 0 Receive / Trasmit Frame / Buffer Done */
    ENET_MAC0_Rx_Tx_Done2_IRQn = 119, /**< MAC 0 Receive / Trasmit Frame / Buffer Done */
    ENET_IRQn                  = 120, /**< MAC 0 IRQ */
    ENET_1588_IRQn             = 121, /**< MAC 0 1588 Timer Interrupt - synchronous */
    ASRC_IRQn                  = 122, /**< ASRC Interrupt */
    Reserved139_IRQn           = 123, /**< Reserved Interrupt */
    Reserved140_IRQn           = 124, /**< Reserved Interrupt */
    Reserved141_IRQn           = 125, /**< Reserved Interrupt */
    ISI_CH3_IRQn               = 126, /**< ISI Camera Channel 3 Interrupt */
    Reserved143_IRQn           = 127  /**< Reserved Interrupt */
} IRQn_Type;

/*!
 * @}
 */ /* end of group Interrupt_vector_numbers */

#endif // _mimx8mn_irq_h