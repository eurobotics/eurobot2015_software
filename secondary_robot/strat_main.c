/*  
 *  Copyright Robotics Association of Coslada, Eurobotics Engineering (2011)
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
 *  Javier Bali�as Santos <javier@arc-robots.org> and Silvia Santano
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <aversive/pgmspace.h>
#include <aversive/queue.h>
#include <aversive/wait.h>
#include <aversive/error.h>

#include <uart.h>
#include <pwm_mc.h>
#include <pwm_servo.h>
#include <clock_time.h>

#include <pid.h>
#include <quadramp.h>
#include <control_system_manager.h>
#include <trajectory_manager.h>
#include <trajectory_manager_utils.h>
//#include <trajectory_manager_core.h>
#include <vect_base.h>
#include <lines.h>
#include <polygon.h>
#include <obstacle_avoidance.h>
#include <blocking_detection_manager.h>
#include <robot_system.h>
#include <position_manager.h>
#include <scheduler.h>


#include <rdline.h>
#include <parse.h>

#include "../common/i2c_commands.h"
#include "i2c_protocol.h"
#include "main.h"
#include "strat.h"
#include "strat_base.h"
#include "../maindspic/strat_avoid.h"
#include "../maindspic/strat_utils.h"
#include "sensor.h"
#include "actuator.h"
#include "beacon.h"
#include "cmdline.h"


#define ERROUT(e) do {\
		err = e;			 \
		goto end;		 \
	} while(0)

/* Add here the main strategic, the inteligence of robot */


/* auto possition depending on color */
void strat_auto_position (void)
{
#define AUTOPOS_SPEED_FAST 	1000
#define BASKET_WIDTH		300

	uint8_t err;
	uint16_t old_spdd, old_spda;

	/* save & set speeds */
	interrupt_traj_reset();
	strat_get_speed(&old_spdd, &old_spda);
	strat_set_speed(AUTOPOS_SPEED_FAST, AUTOPOS_SPEED_FAST);

	/* goto blocking to y axis */
	trajectory_d_rel(&mainboard.traj, -200);
	err = wait_traj_end(END_INTR|END_TRAJ|END_BLOCKING);
	if (err == END_INTR)
		goto intr;
	wait_ms(100);

	/* set y */
	strat_reset_pos(DO_NOT_SET_POS, BASKET_WIDTH + ROBOT_CENTER_TO_BACK, 90);

	/* prepare to x axis */
	trajectory_d_rel(&mainboard.traj, 45);
	err = wait_traj_end(END_INTR|END_TRAJ);
	if (err == END_INTR)
		goto intr;

	trajectory_a_rel(&mainboard.traj, COLOR_A_REL(-90));
	err = wait_traj_end(END_INTR|END_TRAJ);
	if (err == END_INTR)
		goto intr;


	/* goto blocking to x axis */
	trajectory_d_rel(&mainboard.traj, -700);
	err = wait_traj_end(END_INTR|END_TRAJ|END_BLOCKING);
	if (err == END_INTR)
		goto intr;
	wait_ms(100);
	
	/* set x and angle */
	strat_reset_pos(COLOR_X(ROBOT_CENTER_TO_BACK), DO_NOT_SET_POS, COLOR_A_ABS(0));
	
	/* goto start position */
	trajectory_d_rel(&mainboard.traj, 175);
	err = wait_traj_end(END_INTR|END_TRAJ);
	if (err == END_INTR)
		goto intr;
	wait_ms(100);
	
	/* restore speeds */	
	strat_set_speed(old_spdd, old_spda);
	return;

intr:
	strat_hardstop();
	strat_set_speed(old_spdd, old_spda);
}


/* TODO MAMOOTH!!!! */
uint8_t strat_patrol_fresco_mamooth(uint8_t balls_mamooth_1, uint8_t balls_mamooth_2)
{
	#define BEGIN_FRESCO_X	1295
	#define REFERENCE_DISTANCE_TO_ROBOT 800
	static uint8_t fresco_done =0;
	static uint8_t mamooth_done=0;
	int16_t d1,d2;
	int16_t a1,a2;
	int16_t opp1_x, opp1_y, opp2_x, opp2_y;
	
	get_opponent1_da(&d1,&a1);
	get_opponent2_da(&d2,&a2);
	
	if(d2>REFERENCE_DISTANCE_TO_ROBOT && d1>REFERENCE_DISTANCE_TO_ROBOT && fresco_done!=1)
	{
		fresco_done=strat_paint_fresco();
		return (fresco_done);
	}
	else if(d2>REFERENCE_DISTANCE_TO_ROBOT && d1>REFERENCE_DISTANCE_TO_ROBOT && mamooth_done!=1){
		mamooth_done=strat_shoot_mamooth(balls_mamooth_1,balls_mamooth_2);
		return (mamooth_done);
	}
	
	else
	{
		get_opponent1_xy(&opp1_x, &opp1_y);
		get_opponent2_xy(&opp2_x, &opp2_y);
		if(((opp1_y<300) || (opp2_y<300)) && ((fresco_done!=1)||(mamooth_done!=1)))
		{
			if(fresco_done!=1)
			{
				fresco_done=strat_paint_fresco();
				return (fresco_done);
			}
			else if(mamooth_done!=1)
			{
				mamooth_done=strat_shoot_mamooth(balls_mamooth_1,balls_mamooth_2);
				return (mamooth_done);
			}
		}
			
		else{
			return strat_patrol_between(COLOR_X(BEGIN_FRESCO_X),300,COLOR_X(BEGIN_FRESCO_X),900);
		}
	}
}


uint8_t strat_paint_fresco(void)
{
	static uint8_t state = 0;
	uint16_t old_spdd, old_spda, temp_spdd, temp_spda;
	uint8_t err = 0;
#define BEGIN_LINE_Y 	450
#define BEGIN_FRESCO_X	1295

	switch (state)
	{
		/* go in front of fresco */
		case 0:
			//printf_P("fresco case 0");
			trajectory_goto_xy_abs (&mainboard.traj,  COLOR_X(BEGIN_FRESCO_X), BEGIN_LINE_Y);
			err = wait_traj_end(TRAJ_FLAGS_STD);
			if (!TRAJ_SUCCESS(err))
					ERROUT(err);

			state ++;	
			return 0;		
			break;

		/* turn to fresco 1 */
		case 1:
			//printf_P("fresco case 1");
			trajectory_a_abs (&mainboard.traj, 90);
			err = wait_traj_end(TRAJ_FLAGS_STD);
			if (!TRAJ_SUCCESS(err))
					ERROUT(err);

			state ++;	
			return 0;		
			break;

		/* paint fresco 2*/
		case 2:
			//printf_P("fresco case 2");
			sensor_obstacle_enable();
			if (sensor_get (S_OBS_REAR_L) || sensor_get (S_OBS_REAR_R))
				ERROUT(END_OBSTACLE);
				
			trajectory_goto_backward_xy_abs (&mainboard.traj, 
										COLOR_X(BEGIN_FRESCO_X), ROBOT_CENTER_TO_BACK + 100);
			err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);
			if (!TRAJ_SUCCESS(err))
					ERROUT(err);

			/* go backwards */
			strat_get_speed(&old_spdd, &old_spda);
			strat_set_speed(SPEED_DIST_VERY_SLOW, SPEED_ANGLE_FAST);
			trajectory_d_rel(&mainboard.traj, -200);
			err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);

			strat_set_speed(old_spdd, old_spda);

			trajectory_goto_forward_xy_abs (&mainboard.traj, 
										COLOR_X(BEGIN_FRESCO_X), ROBOT_CENTER_TO_BACK + 100);
			err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);
			if (!TRAJ_SUCCESS(err))
					ERROUT(err);
					
				
			state ++;		
			return 0;	
			break;

		/* leave fresco */
		case 3:	
			//printf_P("fresco case 3");
			if(opponent2_is_infront()==0 && opponent2_is_infront()==0)
			{
				trajectory_goto_forward_xy_abs (&mainboard.traj, COLOR_X(BEGIN_FRESCO_X),600);
				err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);
				if (!TRAJ_SUCCESS(err))
						ERROUT(err);
						
				state++;
				printf_P("fresco done");
				return 1;
			}
			
			else
				return 0;
			break;
			
		default:
			break;
	}
end:
	return err;
}


uint8_t strat_patrol_between(int16_t x1, int16_t y1,int16_t x2, int16_t y2)
{	
	#define REFERENCE_DISTANCE_TO_ROBOT 800
	int16_t opp1_x, opp1_y, opp2_x, opp2_y, reference_x;
	uint16_t old_spdd, old_spda, temp_spdd, temp_spda;
	uint8_t err = 0;
	int16_t d,d2;
	int16_t a,a2;

	/* save speed */
	strat_get_speed (&old_spdd, &old_spda);
  	strat_limit_speed_disable ();
	strat_set_speed(SPEED_DIST_SLOW, SPEED_ANGLE_VERY_SLOW);
	
	/* get robot coordinates */
	get_opponent1_xy(&opp1_x, &opp1_y);
	get_opponent2_xy(&opp2_x, &opp2_y);
	
	get_opponent1_da(&d,&a);
	get_opponent2_da(&d2,&a2);
	
		if(d<(REFERENCE_DISTANCE_TO_ROBOT))
		{
			if((opp1_y_is_more_than(y1)&&!opp1_y_is_more_than(y2))||(!opp1_y_is_more_than(y1)&&opp1_y_is_more_than(y2)))
			{
				trajectory_goto_xy_abs(&mainboard.traj,(x1+x2)/2,opp1_y);
			}
		}
		
		if(d2<(REFERENCE_DISTANCE_TO_ROBOT))
		{
			if((opp2_y_is_more_than(y1)&&!opp2_y_is_more_than(y2))||(!opp2_y_is_more_than(y1)&&opp2_y_is_more_than(y2)))
			{
				trajectory_goto_xy_abs(&mainboard.traj,(x1+x2)/2,opp2_y); 
			}
		}
	
	err = test_traj_end(TRAJ_FLAGS_NO_NEAR);
	
	if (!TRAJ_SUCCESS(err))
		ERROUT(err);
	
end:
	strat_set_speed(old_spdd, old_spda);	
	strat_limit_speed_enable();
	return err;
}

uint8_t strat_shoot_mamooth(uint8_t balls_mamooth_1, uint8_t balls_mamooth_2)
{
    uint8_t err = 0;

#define BEGIN_LINE_Y 	450
#define BEGIN_MAMOOTH_X	750
#define SERVO_SHOOT_POS_UP 80
#define SERVO_SHOOT_POS_DOWN 300

	if(mainboard.our_color == I2C_COLOR_RED)
	{
		uint8_t aux=balls_mamooth_1;
		balls_mamooth_1=balls_mamooth_2;
		balls_mamooth_2=aux;
	}

	if(balls_mamooth_1 > 0)
	{
		trajectory_goto_forward_xy_abs (&mainboard.traj, COLOR_X(BEGIN_MAMOOTH_X), BEGIN_LINE_Y);
		err = wait_traj_end(TRAJ_FLAGS_NO_NEAR);
		if (!TRAJ_SUCCESS(err))
				ERROUT(err);
	
		trajectory_a_abs (&mainboard.traj, -90);
		err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);
		if (!TRAJ_SUCCESS(err))
				ERROUT(err);
				
		/* TODO: on slavedspic: possibility to select the number of balls to be thrown */
		#ifndef HOST_VERSION
			pwm_servo_set(&gen.pwm_servo_oc3, SERVO_SHOOT_POS_UP);
			pwm_servo_set(&gen.pwm_servo_oc4, SERVO_SHOOT_POS_DOWN);

		#endif
		if(balls_mamooth_2 == 0)
			return 1;
	}
	
	if(balls_mamooth_2 > 0)
	{
		trajectory_goto_forward_xy_abs (&mainboard.traj, COLOR_X(3000-BEGIN_MAMOOTH_X), BEGIN_LINE_Y);
		err = wait_traj_end(TRAJ_FLAGS_NO_NEAR);
		if (!TRAJ_SUCCESS(err))
			ERROUT(err);
	
		trajectory_a_abs (&mainboard.traj, -90);
		err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);
		if (!TRAJ_SUCCESS(err))
				ERROUT(err);
				
		#ifndef HOST_VERSION
			pwm_servo_set(&gen.pwm_servo_oc3, SERVO_SHOOT_POS_UP);
			pwm_servo_set(&gen.pwm_servo_oc4, SERVO_SHOOT_POS_DOWN);
		#endif
		
		return 1;
	}
	
	return 0;
end:	
	return err;
}




