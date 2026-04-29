/* boot.s
 * Assembler startup file for the STM32
 * Ford Seidel  2019-01-31
 * Ronit Banerjee
 */

# The Cortex M4 is a thumb only processor
.cpu cortex-m4
.syntax unified
.section .ivt
.thumb

.global ivt
ivt:
@ TODO: add the stack and the reset handler addr here.
.word   __msp_stack_top     /* stack top address */
.word   _reset_             /* 1 Reset */
.word   _nmi_               /* 2 NMI */
.word   _hard_fault_        /* 3 Hard Fault */
.word   _mm_fault_          /* 4 MM Fault */
.word   _bus_fault_         /* 5 Bus Fault */
.word   _usage_fault_       /* 6 Usage Fault */
.word   spin                /* 7 RESERVED */
.word   spin                /* 8 RESERVED */
.word   spin                /* 9 RESERVED*/
.word   spin                /* 10 RESERVED */
.word   _svc_asm_handler_   /* 11 SV call */
.word   spin                /* 12 Debug reserved */
.word   spin                /* 13 RESERVED */
.word   _pend_sv_           /* 14 PendSV */
.word   systick_c_handler   /* 15 SysTick */
.word   spin                /* 16 IRQ0 Window Watchdog Interrupt */
.word   spin                /* 17 IRQ1 PVD */
.word   spin                /* 18 IRQ2 TAMPER */
.word   spin                /* 19 IRQ3 RTC   */
.word   spin                /* 20 IRQ4 FLASH   */
.word   spin                /* 21 IRQ5 RCC */
.word   encoder_irq_handler_left /* 22 IRQ6 EXTI0 */
.word   encoder_irq_handler_left /* 23 IRQ7 EXTI1  */
.word   exti2_irq_handler   /* 24 IRQ8 EXTI2   */
.word   exti3_irq_handler   /* 25 IRQ9 EXTI3 */
.word   exti4_irq_handler   /* 26 IRQ10 EXTI4 */
.word   spin                /* 27 IRQ11 DMA1_Channel1   */
.word   spin                /* 28 IRQ12 DMA1_Channel2   */
.word   spin                /* 29 IRQ13 DMA1_Channel3 */
.word   dma1_stream3_irq_handler /* 30 IRQ14 DMA1_Stream3 */
.word   spin                /* 31 IRQ15 DMA1_Channel5   */
.word   spin                /* 32 IRQ16 DMA1_Channel6   */
.word   spin                /* 33 IRQ17 DMA1_Channel7 */
.word   spin                /* 34 IRQ18 ADC1_2 */
.word   spin                /* 35 IRQ19 CAN1_TX   */
.word   spin                /* 36 IRQ20 CAN1_TX0   */
.word   spin                /* 37 IRQ21 CAN1_RX1 */
.word   spin                /* 38 IRQ22 CAN1_SCE */
.word   encoder_irq_handler_right /* 39 IRQ23 EXTI9_5   */
.word   spin                /* 40 IRQ24 TIM1_BRK   */
.word   spin                /* 41 IRQ25 TIM1_UP */
.word   spin                /* 42 IRQ26 TIM1_TRG_COM */
.word   spin                /* 43 IRQ27 TIM1_CC   */
.word   tim2_irq_handler    /* 44 IRQ28 TIM2   */
.word   tim3_irq_handler    /* 45 IRQ29 TIM3 */
.word   tim4_irq_handler    /* 46 IRQ30 TIM4 */
.word   spin                /* 47 IRQ31 I2C1_EV   */
.word   spin                /* 48 IRQ32 I2C1_ER   */
.word   spin                /* 49 IRQ33 I2C2_EV */
.word   spin                /* 50 IRQ34 I2C2_ER */
.word   spin                /* 51 IRQ35 SPI1   */
.word   spin                /* 52 IRQ36 SPI2   */
.word   spin                /* 53 IRQ37 USART1 */
.word   uart_irq_handler    /* 54 IRQ38 USART2 */
.word   spin                /* 55 IRQ39 USART3   */
.word   exti15_10_irq_handler /* 56 IRQ40 EXTI15_10   */
.word   spin                /* 57 IRQ41 RTCAlarm */
.word   spin                /* 58 IRQ42 OTG_FS_WKUP */
.word   spin                /* 59 IRQ43 RESERVED   */
.word   spin                /* 60 IRQ44 RESERVED   */
.word   spin                /* 61 IRQ45 RESERVED */
.word   spin                /* 62 IRQ46 RESERVED */
.word   spin                /* 63 IRQ47 RESERVED   */
.word   spin                /* 64 IRQ48 RESERVED   */
.word   spin                /* 65 IRQ49 RESERVED */
.word   tim5_irq_handler    /* 66 IRQ50 TIM5 */
.word   spin                /* 67 IRQ51 SPI3   */
.word   spin                /* 68 IRQ52 UART4   */
.word   spin                /* 69 IRQ53 UART5 */
.word   spin                /* 70 IRQ54 TIM6 */
.word   spin                /* 71 IRQ55 TIM7   */
.word   spin                /* 72 IRQ56 DMA2_Channel1   */
.word   spin                /* 73 IRQ57 DMA2_Channel2 */
.word   spin                /* 74 IRQ58 DMA2_Channel3 */
.word   spin                /* 75 IRQ59 DMA2_Channel4   */
.word   spin                /* 76 IRQ60 DMA2_Channel5   */
.word   spin                /* 77 IRQ61 ETH */
.word   spin                /* 78 IRQ62 ETH_WKUP */
.word   spin                /* 79 IRQ63 CAN2_TX   */
.word   spin                /* 80 IRQ64 CAN2_RX0  */
.word   spin                /* 81 IRQ65 CAN2_RX1 */
.word   spin                /* 82 IRQ66 CAN2_SCE */
.word   spin                /* 83 IRQ67 OTG_FS   */

.section .text

.thumb_func
spin:
  bkpt

.thumb_func
_nmi_ :
  bkpt

.thumb_func
_hard_fault_ :
  bkpt

.thumb_func
_mm_fault_:
  bkpt

.thumb_func
_bus_fault_ :
  bkpt

.thumb_func
_usage_fault_ :
  bkpt

.thumb_func
_pend_sv_ :
  @saving context for general registers
  mrs r1, psp
  mov r2, lr @ save exc_return
  push {r1, r2}
  push {r4-r11}
  
  mrs r0, msp
  
  bl pendsv_c_handler
  @ r0 contains the kernel stack frame that we want to go into now
  mov sp, r0

  pop {r4-r11}
  pop {r1, r2}
  mov lr, r2
  msr psp, r1

  @ stack is okay to be restored automatically
  bx lr

.thumb_func
_spi1_handler:
  bkpt

.thumb_func
.global _reset_
_reset_:
@ Write your boot loader here!
  ldr  r0, =_data_size
  cmp  r0, #0
  beq  done_with_data
  ldr  r0, =_k_data
  ldr  r1, =_u_edata
  ldr  r2, =_erodata
loop_for_data:
  cmp  r0, r1
  beq  done_with_data
  ldr  r3, [r2], #4
  str  r3, [r0], #4
  b    loop_for_data
done_with_data:
  ldr  r0, =_bss_size
  cmp  r0, #0
  beq  done_with_bss
  ldr  r0, =_k_bss
  ldr  r1, =_u_ebss
  movs r2, #0
loop_for_bss:
  cmp  r0, r1
  beq  done_with_bss
  str  r2, [r0], #4
  b    loop_for_bss
done_with_bss:
  bl kernel_main
  bkpt

.global _sys_tick_asm_
_sys_tick_asm_:
  bkpt

.thumb_func
.global _svc_asm_handler_
_svc_asm_handler_:
  mov r4, lr

  tst lr, #4
  ite eq
  mrseq r1, msp
  mrsne r1, psp
@ determine if we are in msp or psp
  mov r5, r1

  ldr r0, [r1, #24]

  mov r3, r0
  orrs r3, r3, #1
  str r3, [r1, #24]

  subs r0, r0, #2
  bl svc_c_handler

  str r0, [r5, #0]

@ restore it  
  mov lr, r4
  bx lr
