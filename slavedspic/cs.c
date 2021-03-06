/*  
 *  Copyright Robotics Association of Coslada, Eurobotics Engineering (2012)
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Revision : $Id$
 *
 *  Javier Bali�as Santos <javier@arc-robots.org>
 */

#include <stdio.h>
#include <string.h>

#include <aversive.h>
#include <aversive/error.h>

#include <ax12.h>
#include <encoders_dspic.h>
#include <pwm_mc.h>
#include <dac_mc.h>
#include <pwm_servo.h>
#include <timer.h>
#include <scheduler.h>
#include <clock_time.h>

#include <pid.h>
#include <quadramp.h>
#include <control_system_manager.h>
#include <blocking_detection_manager.h>

#include <parse.h>
#include <rdline.h>

#include "main.h"
#include "actuator.h"

/* called every 5 ms */
void do_cs(__attribute__((unused)) void *dummy) 
{
	/* read encoders */
	if (slavedspic.flags & DO_ENCODERS) {
		encoders_dspic_manage(NULL);
	}
	/* control system */
	if (slavedspic.flags & DO_CS) {
		if (slavedspic.stands_exchanger.on)
			cs_manage(&slavedspic.stands_exchanger.cs);
	}
	if (slavedspic.flags & DO_BD) {
		bd_manage_from_cs(&slavedspic.stands_exchanger.bd, &slavedspic.stands_exchanger.cs);
	}
	if (slavedspic.flags & DO_POWER)
		BRAKE_OFF();
	else
		BRAKE_ON();
}

void dump_cs_debug(const char *name, struct cs *cs)
{
	DEBUG(E_USER_CS, "%s cons=% .5ld fcons=% .5ld err=% .5ld "
	      "in=% .5ld out=% .5ld", 
	      name, cs_get_consign(cs), cs_get_filtered_consign(cs),
	      cs_get_error(cs), cs_get_filtered_feedback(cs),
	      cs_get_out(cs));
}

void dump_cs(const char *name, struct cs *cs)
{
	printf_P(PSTR("%s cons=% .5ld fcons=% .5ld err=% .5ld "
		      "in=% .5ld out=% .5ld\r\n"), 
		 name, cs_get_consign(cs), cs_get_filtered_consign(cs),
		 cs_get_error(cs), cs_get_filtered_feedback(cs),
		 cs_get_out(cs));
}

void dump_pid(const char *name, struct pid_filter *pid)
{
	printf_P(PSTR("%s P=% .8ld I=% .8ld D=% .8ld out=% .8ld\r\n"),
		 name,
		 pid_get_value_in(pid) * pid_get_gain_P(pid),
		 pid_get_value_I(pid) * pid_get_gain_I(pid),
		 pid_get_value_D(pid) * pid_get_gain_D(pid),
		 pid_get_value_out(pid));
}

void pwm_mc_set_or_disable(void *pwm, int32_t value)
{
#define PWM_DISABLE_VALUE_TH 100
	int32_t __value;

	/* absolute value */
	__value = value;
	if (__value < 0)
		__value = -__value;

	if (__value < PWM_DISABLE_VALUE_TH) {
		/* breaked */
		pwm_mc_init(&gen.pwm_mc_mod2_ch1, 19000, CH1_COMP&PDIS1H&PDIS1L);
		_LATB12  = 0;	
		_LATB13  = 0;
	}
	else {
		pwm_mc_init(&gen.pwm_mc_mod2_ch1, 19000, CH1_COMP&PEN1H&PEN1L);
		pwm_mc_set (pwm, value);
	}
}

void slavedspic_cs_init(void)
{
	/* ---- CS */
	/* PID */
	pid_init(&slavedspic.stands_exchanger.pid);
	pid_set_gains(&slavedspic.stands_exchanger.pid, 70, 0, 50);
	pid_set_maximums(&slavedspic.stands_exchanger.pid, 0, 3000, 3300);
	pid_set_out_shift(&slavedspic.stands_exchanger.pid, 6);
	pid_set_derivate_filter(&slavedspic.stands_exchanger.pid, 1);

	/* QUADRAMP */
	quadramp_init(&slavedspic.stands_exchanger.qr);
	quadramp_set_1st_order_vars(&slavedspic.stands_exchanger.qr, 75, 75);
	quadramp_set_2nd_order_vars(&slavedspic.stands_exchanger.qr, 0, 0);

	/* CS */
	cs_init(&slavedspic.stands_exchanger.cs);
	cs_set_consign_filter(&slavedspic.stands_exchanger.cs, quadramp_do_filter, &slavedspic.stands_exchanger.qr);
	cs_set_correct_filter(&slavedspic.stands_exchanger.cs, pid_do_filter, &slavedspic.stands_exchanger.pid);
	cs_set_process_in(&slavedspic.stands_exchanger.cs, pwm_mc_set_or_disable, PWM_MC_STANDS_EXCHANGER_MOTOR);
	cs_set_process_out(&slavedspic.stands_exchanger.cs, encoders_dspic_get_value, STANDS_EXCHANGER_ENCODER);
	cs_set_consign(&slavedspic.stands_exchanger.cs, 0);

	/* Blocking detection */
	/* detect blocking based on motor current.
	 * triggers the blocking if:
	 *   - the current in the motor is a above a threshold
	 *     during n tests
	 *   - the speed is below the threshold (if specified)
	 *
	 * We suppose that i = k1.V - k2.w
	 * (V is the voltage applied on the motor, and w the current speed
	 * of the motor)
	 */

	bd_init(&slavedspic.stands_exchanger.bd);
	bd_set_speed_threshold(&slavedspic.stands_exchanger.bd, 50);						/* speed */
	bd_set_current_thresholds(&slavedspic.stands_exchanger.bd, 800, 10000, 1000000, 10); /* k1, k2, i, cpt */


	/* set on!! */
	slavedspic.stands_exchanger.on = 1;
}
