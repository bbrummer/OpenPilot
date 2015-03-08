/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_SERVO RC Servo Functions
 * @brief Code to do set RC servo output
 * @{
 *
 * @file       pios_servo.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      RC Servo routines (STM32 dependent)
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "pios.h"

#ifdef PIOS_INCLUDE_SERVO

#include "pios_servo_priv.h"
#include "pios_tim_priv.h"

/* Private Function Prototypes */

static const struct pios_servo_cfg *servo_cfg;

// determine if the related timer will work in synchronous (or OneShot/OneShot125) One Pulse mode.
static uint8_t pios_servo_bank_mode[PIOS_SERVO_BANKS] = { 0 };
// used to skip updates when pulse length is higher than update cycle
static uint16_t pios_servo_bank_next_update[PIOS_SERVO_BANKS] = { 0 };
static uint16_t pios_servo_bank_max_pulse[PIOS_SERVO_BANKS] = { 0 };
// timer associated to each bank
static TIM_TypeDef *pios_servo_bank_timer[PIOS_SERVO_BANKS] = { 0 };

// index of bank used for each pin
static uint8_t *pios_servo_pin_bank;

#define PIOS_SERVO_TIMER_CLOCK 1000000
#define PIOS_SERVO_SAFE_MARGIN 50
/**
 * Initialise Servos
 */
int32_t PIOS_Servo_Init(const struct pios_servo_cfg *cfg)
{
    uint32_t tim_id;

    if (PIOS_TIM_InitChannels(&tim_id, cfg->channels, cfg->num_channels, NULL, 0)) {
        return -1;
    }

    /* Store away the requested configuration */
    servo_cfg = cfg;
    pios_servo_pin_bank = pios_malloc(sizeof(uint8_t) * cfg->num_channels);

    uint8_t bank = 0;
    for (uint8_t i = 0; (i < servo_cfg->num_channels); i++) {
        const struct pios_tim_channel *chan = &servo_cfg->channels[i];
        bool new = true;
        /* See if any previous channels use that same timer */
        for (uint8_t j = 0; (j < i) && new; j++) {
            new &= chan->timer != servo_cfg->channels[j].timer;
        }

        if (new) {
            PIOS_Assert(bank < PIOS_SERVO_BANKS);
            for (uint8_t j = i; j < servo_cfg->num_channels; j++) {
                if (servo_cfg->channels[j].timer == chan->timer) {
                    pios_servo_pin_bank[j] = bank;
                }
            }
            pios_servo_bank_timer[bank] = chan->timer;

            TIM_ARRPreloadConfig(chan->timer, ENABLE);
            TIM_CtrlPWMOutputs(chan->timer, ENABLE);
            TIM_Cmd(chan->timer, DISABLE);

            bank++;
        }

        /* Set up for output compare function */
        switch (chan->timer_chan) {
        case TIM_Channel_1:
            TIM_OC1Init(chan->timer, &servo_cfg->tim_oc_init);
            TIM_OC1PreloadConfig(chan->timer, TIM_OCPreload_Enable);
            break;
        case TIM_Channel_2:
            TIM_OC2Init(chan->timer, &servo_cfg->tim_oc_init);
            TIM_OC2PreloadConfig(chan->timer, TIM_OCPreload_Enable);
            break;
        case TIM_Channel_3:
            TIM_OC3Init(chan->timer, &servo_cfg->tim_oc_init);
            TIM_OC3PreloadConfig(chan->timer, TIM_OCPreload_Enable);
            break;
        case TIM_Channel_4:
            TIM_OC4Init(chan->timer, &servo_cfg->tim_oc_init);
            TIM_OC4PreloadConfig(chan->timer, TIM_OCPreload_Enable);
            break;
        }
    }

    return 0;
}

void PIOS_Servo_SetBankMode(uint8_t bank, uint8_t mode)
{
    PIOS_Assert(bank < PIOS_SERVO_BANKS);
    pios_servo_bank_mode[bank] = mode;

    if (pios_servo_bank_timer[bank]) {
        for (uint8_t i = 0; (i < servo_cfg->num_channels); i++) {
            if (pios_servo_pin_bank[i] == bank) {
                const struct pios_tim_channel *chan = &servo_cfg->channels[i];
                /* Set up for output compare function */
                switch (chan->timer_chan) {
                case TIM_Channel_1:
                    TIM_OC1PolarityConfig(chan->timer, TIM_OCPolarity_High);
                    break;
                case TIM_Channel_2:
                    TIM_OC2PolarityConfig(chan->timer, TIM_OCPolarity_High);
                    break;
                case TIM_Channel_3:
                    TIM_OC3PolarityConfig(chan->timer, TIM_OCPolarity_High);
                    break;
                case TIM_Channel_4:
                    TIM_OC4PolarityConfig(chan->timer, TIM_OCPolarity_High);
                    break;
                }
            }
        }

        // Setup the timer accordingly
        TIM_SelectOnePulseMode(pios_servo_bank_timer[bank], TIM_OPMode_Repetitive);
        TIM_Cmd(pios_servo_bank_timer[bank], ENABLE);
    }
}


void PIOS_Servo_Update()
{
    for (uint8_t i = 0; (i < PIOS_SERVO_BANKS); i++) {
        const TIM_TypeDef *timer = pios_servo_bank_timer[i];
        if (timer && pios_servo_bank_mode[i] == PIOS_SERVO_BANK_MODE_SINGLE_PULSE) {
            // a pulse to be generated is longer than cycle period. skip this update.
            if (TIM_GetCounter((TIM_TypeDef *)timer) > (uint32_t)(pios_servo_bank_next_update[i] + PIOS_SERVO_SAFE_MARGIN)) {
                TIM_GenerateEvent((TIM_TypeDef *)timer, TIM_EventSource_Update);
                pios_servo_bank_next_update[i] = pios_servo_bank_max_pulse[i];
            }
        }
        pios_servo_bank_max_pulse[i] = 0;
    }
    for (uint8_t i = 0; (i < servo_cfg->num_channels); i++) {
        uint8_t bank = pios_servo_pin_bank[i];
        uint8_t mode = pios_servo_bank_mode[bank];
        if (mode == PIOS_SERVO_BANK_MODE_SINGLE_PULSE) {
            /* Update the position */
            const struct pios_tim_channel *chan = &servo_cfg->channels[i];

            switch (chan->timer_chan) {
            case TIM_Channel_1:
                TIM_SetCompare1(chan->timer, 0);
                break;
            case TIM_Channel_2:
                TIM_SetCompare2(chan->timer, 0);
                break;
            case TIM_Channel_3:
                TIM_SetCompare3(chan->timer, 0);
                break;
            case TIM_Channel_4:
                TIM_SetCompare4(chan->timer, 0);
                break;
            }
        }
    }
}
/**
 * Set the servo update rate (Max 500Hz)
 * \param[in] array of rates in Hz
 * \param[in] array of timer clocks in Hz
 * \param[in] maximum number of banks
 */
void PIOS_Servo_SetHz(const uint16_t *speeds, const uint32_t *clock, uint8_t banks)
{
    PIOS_Assert(banks <= PIOS_SERVO_BANKS);
    if (!servo_cfg) {
        return;
    }

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = servo_cfg->tim_base_init;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;

    for (uint8_t i = 0; i < banks && i < PIOS_SERVO_BANKS; i++) {
        const TIM_TypeDef *timer = pios_servo_bank_timer[i];
        if (timer) {
            uint32_t new_clock = PIOS_SERVO_TIMER_CLOCK;
            if (clock[i]) {
                new_clock = clock[i];
            }
            // Choose the correct prescaler value for the APB the timer is attached
            if (timer == TIM1 || timer == TIM8 || timer == TIM9 || timer == TIM10 || timer == TIM11) {
                TIM_TimeBaseStructure.TIM_Prescaler = (PIOS_PERIPHERAL_APB2_CLOCK / new_clock) - 1;
            } else {
                TIM_TimeBaseStructure.TIM_Prescaler = (PIOS_PERIPHERAL_APB1_CLOCK / new_clock) - 1;
            }
            TIM_TimeBaseStructure.TIM_Period = ((new_clock / speeds[i]) - 1);
            TIM_TimeBaseInit((TIM_TypeDef *)timer, &TIM_TimeBaseStructure);
        }
    }
}

/**
 * Set servo position
 * \param[in] Servo Servo number (0-7)
 * \param[in] Position Servo position in microseconds
 */
void PIOS_Servo_Set(uint8_t servo, uint16_t position)
{
    /* Make sure servo exists */
    if (!servo_cfg || servo >= servo_cfg->num_channels) {
        return;
    }


    /* Update the position */
    const struct pios_tim_channel *chan = &servo_cfg->channels[servo];
    uint16_t val    = position;
    uint16_t margin = chan->timer->ARR / 50; // Leave 2% of period as margin to prevent overlaps
    if (val > (chan->timer->ARR - margin)) {
        val = chan->timer->ARR - margin;
    }

    uint8_t bank = pios_servo_pin_bank[servo];
    if (pios_servo_bank_max_pulse[bank] < val) {
        pios_servo_bank_max_pulse[bank] = val;
    }
    switch (chan->timer_chan) {
    case TIM_Channel_1:
        TIM_SetCompare1(chan->timer, val);
        break;
    case TIM_Channel_2:
        TIM_SetCompare2(chan->timer, val);
        break;
    case TIM_Channel_3:
        TIM_SetCompare3(chan->timer, val);
        break;
    case TIM_Channel_4:
        TIM_SetCompare4(chan->timer, val);
        break;
    }
}

uint8_t PIOS_Servo_GetPinBank(uint8_t pin)
{
    if (pin < servo_cfg->num_channels) {
        return pios_servo_pin_bank[pin];
    } else {
        return 0;
    }
}

#endif /* PIOS_INCLUDE_SERVO */
