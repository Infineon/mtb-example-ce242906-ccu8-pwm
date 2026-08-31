/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the PSOC™ Control C1 MCU: CCU8 PWM Example
*              for ModusToolbox.
*
* Related Document: See README.md
*
******************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*****************************************************************************/
#include <stdio.h>
#include "cybsp.h"
#include "cy_utils.h"
#include "cy_retarget_io.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define PWM_COMPAREVAL_UPDATE_STEP                      (2500U)
#define PWM_INTERRUPT_MAX_COUNT                         (50U)
#define PWM_PERIOD                                      (65000U)


#define SYMMETRIC_PWM_SLICE_PERIOD_MATCH_EVENT_IRQN     CCU8_MODULE_SR1_IRQN
#define SYMMETRIC_PWM_SLICE_PERIOD_MATCH_EVENT_HANDLER  CCU8_MODULE_SR1_INTERRUPT_HANDLER



#define SYMMETRIC_PWM_SHADOW_TRANSFER_MASK              CY_CCU8_SHADOW_TRANSFER_SLICE_0 | \
                                                        CY_CCU8_SHADOW_TRANSFER_DITHER_SLICE_0 | \
                                                        CY_CCU8_SHADOW_TRANSFER_PRESCALER_SLICE_0
#define ASYMMETRIC_PWM_SHADOW_TRANSFER_MASK             CY_CCU8_SHADOW_TRANSFER_SLICE_1 | \
                                                        CY_CCU8_SHADOW_TRANSFER_DITHER_SLICE_1 | \
                                                        CY_CCU8_SHADOW_TRANSFER_PRESCALER_SLICE_1
#define SYMMETRIC_PWM_PERIOD_MATCH_IRQn_PRIORITY        (3U)


/* Define macro to enable/disable printing of debug messages */
#define ENABLE_DEBUG_PRINT              (0)

/* Define macro to set the loop count before printing debug messages */
#if ENABLE_DEBUG_PRINT
#define DEBUG_LOOP_COUNT_MAX                (1U)
#endif

/*******************************************************************************
* Defines
*******************************************************************************/
static volatile bool interrupt_handler_flag = false;

/*******************************************************************************
* Global Data
*******************************************************************************/
/* Interrupt counter */
volatile uint32_t g_num_period_interrupts;

void SYMMETRIC_PWM_SLICE_PERIOD_MATCH_EVENT_HANDLER(void)
{
    g_num_period_interrupts++;
    Cy_CCU8_SLICE_ClearEvent(SYMMETRIC_PWM_SLICE_HW, CY_CCU8_SLICE_IRQ_ID_PERIOD_MATCH);
    interrupt_handler_flag = true;
}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function. It configures 2 of the CCU8 slices to generate PWM
* output. One CCU8 slice is configured to generate symmetric PWM output and the
* second one is configured to generate asymmetric PWM output. Period match
* interrupt for symmetric PWM slice is enabled. Compare values for both
* symmetric and asymmetric PWM slices are updated after every 50 period match
* interrupts of symmetric PWM slice. User LEDs available on PSOC™ control C1 evaluation kit 
* is configured to output PWM signals.
* Output:
* Brightness of USER LEDs vary continuously from LOW to HIGH.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/

int main(void)
{
    cy_rslt_t result;
    /* Compare value for both symmetric and asymmetric PWMs */
    uint16_t pwm_compareVal = 0U;

    #if ENABLE_DEBUG_PRINT
    /* Initialize the current loop count to zero */
    static uint32_t debug_loop_count = 0;
    #endif

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_HW);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    #if ENABLE_DEBUG_PRINT
    printf("Initialization done\r\n");
    #endif

    /* Set interrupt priority */
    NVIC_SetPriority(SYMMETRIC_PWM_SLICE_PERIOD_MATCH_EVENT_IRQN, SYMMETRIC_PWM_PERIOD_MATCH_IRQn_PRIORITY);


    /* Select interrupt source */
    Cy_SCU_SetInterruptControl(SYMMETRIC_PWM_SLICE_PERIOD_MATCH_EVENT_IRQN, (Cy_SCU_IRQCTRL_t)((SYMMETRIC_PWM_SLICE_PERIOD_MATCH_EVENT_IRQN << 8) | 1U));

    /* Enable interrupt */
    NVIC_EnableIRQ(SYMMETRIC_PWM_SLICE_PERIOD_MATCH_EVENT_IRQN);


    for (;;)
    {
        #if ENABLE_DEBUG_PRINT
        debug_loop_count++;
        #endif

        /* Wait for 50 symmetric PWM period match events */
        while(g_num_period_interrupts < PWM_INTERRUPT_MAX_COUNT);

        #if ENABLE_DEBUG_PRINT
        if (interrupt_handler_flag && (debug_loop_count == DEBUG_LOOP_COUNT_MAX))
        {
            /* Print message after the loop has run DEBUG_LOOP_COUNT_MAX times and SYMMETRIC_PWM_SLICE_PERIOD_MATCH_EVENT_HANDLER has occurred */
            printf("50 symmetric PWM period match events passed\r\n");
        }
        #endif

        /* Reset interrupt counter */
        g_num_period_interrupts = 0;

        /* Update compare value for symmetric PWM */
        Cy_CCU8_SLICE_SetTimerCompareMatchChannel1(SYMMETRIC_PWM_SLICE_HW, pwm_compareVal);
        Cy_CCU8_EnableShadowTransfer(CCU8_MODULE_HW, SYMMETRIC_PWM_SHADOW_TRANSFER_MASK);

        #if ENABLE_DEBUG_PRINT
        if (debug_loop_count == DEBUG_LOOP_COUNT_MAX)
        {
            printf("Updated compare value for symmetric PWM\r\n");
        }
        #endif

        /* Update compare value for asymmetric PWM channels */
        Cy_CCU8_SLICE_SetTimerCompareMatchChannel1(ASYMMETRIC_PWM_SLICE_HW, pwm_compareVal);
        Cy_CCU8_SLICE_SetTimerCompareMatchChannel2(ASYMMETRIC_PWM_SLICE_HW, pwm_compareVal);
        Cy_CCU8_EnableShadowTransfer(CCU8_MODULE_HW, ASYMMETRIC_PWM_SHADOW_TRANSFER_MASK);

        #if ENABLE_DEBUG_PRINT
        if (debug_loop_count == DEBUG_LOOP_COUNT_MAX)
        {
            printf("Updated compare value for asymmetric PWM channels\r\n");
        }
        #endif

        /* Increment PWM compare value */
        pwm_compareVal += PWM_COMPAREVAL_UPDATE_STEP;
        if (pwm_compareVal > PWM_PERIOD)
        {
            pwm_compareVal = 0U;
        }
    }
}

/* [] END OF FILE */
