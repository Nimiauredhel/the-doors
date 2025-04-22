/*
 * isr_utils.c
 *
 *  Created on: Apr 22, 2025
 *      Author: mickey
 */

#include "isr_utils.h"

bool is_isr()
{
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0 ;
}
