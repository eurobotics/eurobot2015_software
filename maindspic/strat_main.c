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
#include <dac_mc.h>
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


#include <rdline.h>
#include <parse.h>

#include "../common/i2c_commands.h"
#include "i2c_protocol.h"
#include "main.h"
#include "strat.h"
#include "strat_base.h"
#include "strat_avoid.h"
#include "strat_utils.h"
#include "sensor.h"
#include "actuator.h"
#include "beacon.h"
#include "cmdline.h"
#include "bt_protocol.h"


#define ERROUT(e) do {\
		err = e;			 \
		goto end;		 \
	} while(0)


/* Add here the main strategy, the intelligence of robot */


/*
 *Check if the zone is available. 
 * @Param zone_num checked zone
 * @Return zone | -1.
 */
 

int8_t strat_is_valid_zone(uint8_t zone_num)
{
//#define OPP_WAS_IN_ZONE_TIMES	

	//static uint16_t opp_times[ZONES_MAX];
	//static microseconds opp_time_us = 0;

	/* discard current zone */
	if(strat_infos.current_zone == zone_num)
	{
		//printf_P("zone num: %d. current_zone.\n");
		return -1;	
	}

	/* discard if opp is in zone */
	if(opponents_are_in_area(COLOR_X(strat_infos.zones[zone_num].x_up), strat_infos.zones[zone_num].y_up,
								  COLOR_X(strat_infos.zones[zone_num].x_down),	strat_infos.zones[zone_num].y_down)) {		
		return -1;
	}

	/* discard avoid and checked zones */
	if(strat_infos.zones[zone_num].flags & ZONE_AVOID)
	{
		//printf_P("zone num: %d. avoid.\n");
		return -1;	
	}
		
	return zone_num;
}
void set_next_sec_strategy(){
	strat_infos.current_sec_strategy++;
	switch(strat_infos.current_sec_strategy){
		case 1:
			set_strat_sec_1();
			break;
		case 2:
			set_strat_sec_2();
			break;
		case 3:
			set_strat_sec_3();
			strat_infos.current_sec_strategy = 0;
			break;
		default:
			break;
	}
	
}
void set_strat_sec_1(void){
	printf_P(PSTR("strat_sec_1"));
	strat_infos.zones[ZONE_MY_CLAP_3].prio = 100;
	strat_infos.zones[ZONE_MY_CINEMA_DOWN].prio = 90;
	strat_infos.zones[ZONE_MY_CINEMA_UP].prio = 80;
	strat_infos.zones[ZONE_MY_STAIRS].prio = 70;
}
void set_strat_sec_2(void){
	printf_P(PSTR("strat_sec_2"));
	strat_infos.zones[ZONE_MY_CINEMA_UP].prio = 100;
	strat_infos.zones[ZONE_MY_CLAP_3].prio = 90;
	strat_infos.zones[ZONE_MY_CINEMA_DOWN].prio = 80;	
	strat_infos.zones[ZONE_MY_STAIRS].prio = 70;
}
void set_strat_sec_3(void){
	printf_P(PSTR("strat_sec_3"));
	strat_infos.zones[ZONE_MY_CINEMA_DOWN].prio = 100;
	strat_infos.zones[ZONE_MY_CLAP_3].prio = 90;
	strat_infos.zones[ZONE_MY_CINEMA_UP].prio = 80;
	strat_infos.zones[ZONE_MY_STAIRS].prio = 70;
}

/* return new work zone, -1 if any zone is found */
int8_t strat_get_new_zone(void)
{
	uint8_t prio_max = 0;
	int8_t zone_num = -1, valid_zone= -1 ;
	int8_t i=0;
	/* evaluate zones */
	for(i=0; i < ZONES_MAX; i++) 
	{
		/* compare current priority */
		if(strat_infos.zones[i].prio >= prio_max && (strat_infos.zones[i].flags != ZONE_CHECKED)) {
			/* check if is a valid zone */
			prio_max = strat_infos.zones[i].prio;
			zone_num = i;
		}
		if( i == ZONES_MAX-1 ){
			valid_zone = strat_is_valid_zone(zone_num);
			switch(valid_zone){
				case -1:
					//strategy has to change
					set_next_sec_strategy(); 
					i=0;
					break;
				default:
					break;
			}
		}

	}
	if(zone_num == -1){
		printf_P(PSTR("No more zones available\r\n"));
	}
	return zone_num;
}

/* return END_TRAJ if zone is reached, err otherwise */
uint8_t strat_goto_zone(uint8_t zone_num)
{
	int8_t err=0;
	
#define BASKET_OFFSET_SIDE 175
#define BEGIN_LINE_Y 	450
#define BEGIN_FRESCO_X	1295
#define PROTECT_H1_X 400
#define PROTECT_H1_Y 1500

	
	/* update strat_infos */
	strat_infos.current_zone=-1;
	strat_infos.goto_zone=zone_num;
	
	/* Secondary robot */
	if(strat_infos.zones[zone_num].robot==SEC_ROBOT)
	{
		bt_robot_2nd_goto_and_avoid(COLOR_X(strat_infos.zones[zone_num].init_x), 
												strat_infos.zones[zone_num].init_y);
												
		bt_robot_2nd_wait_end();
		err = END_TRAJ;
		
	}else{
		err = goto_and_avoid (COLOR_X(strat_infos.zones[zone_num].init_x), 
											strat_infos.zones[zone_num].init_y,  
											TRAJ_FLAGS_STD, TRAJ_FLAGS_NO_NEAR);
		
	}
	
	/* update strat_infos */
	strat_infos.last_zone=strat_infos.current_zone;
	strat_infos.goto_zone=-1;
	
	if (!TRAJ_SUCCESS(err)) {
		strat_infos.current_zone=-1;
	}
	else{
		strat_infos.current_zone=zone_num;
	}
end:
    /* TODO XXX if error put arm in safe position */
	return err;
}


/* return END_TRAJ if the work is done, err otherwise */
uint8_t strat_work_on_zone(uint8_t zone_num)
{
	uint8_t err = END_TRAJ;
	
	printf_P(PSTR("Working on zone\r\n"));
	strat_infos.zones[zone_num].prio = ZONE_PRIO_0;
	strat_infos.zones[zone_num].flags |= ZONE_CHECKED;
		
	#if 0
#ifdef HOST_VERSION
	//printf_P(PSTR("strat_work_on_zone %d %s: press a key\r\n"),zone_num,numzone2name[zone_num]);
	//while(!cmdline_keypressed());
#endif

    /* XXX if before the tree harvesting was interrupted by opponent */    
    if (strat_infos.tree_harvesting_interrumped) {
        strat_infos.tree_harvesting_interrumped = 0;
        i2c_slavedspic_wait_ready();
        i2c_slavedspic_mode_harvest_fruits (I2C_SLAVEDSPIC_MODE_HARVEST_FRUITS_END);
    }
    
	/* Secondary robot */
	if(strat_infos.zones[zone_num].robot==SEC_ROBOT){
		
	} else{
	
	}
	#endif
	return err;
}

/* debug state machines step to step */
void state_debug_wait_key_pressed(void)
{
	if (!strat_infos.debug_step)
		return;

	printf_P(PSTR("press a key\r\n"));
	while(!cmdline_keypressed());
}


void recalculate_priorities(void)
{
	#if 0
	uint8_t zone_num;
	
	for(zone_num=0; zone_num<ZONES_MAX; zone_num++)
	{
		/* 1. opp checked zone AFTER us */
		if((strat_infos.zones[zone_num].flags & ZONE_CHECKED_OPP)&&
		(strat_infos.zones[zone_num].flags & ZONE_CHECKED))
		{
				//TODO
		}
		
		/* 2. checked zone */
		else if(strat_infos.zones[zone_num].flags & ZONE_CHECKED)
		{
				//TODO
		}
		
		/* 3. Points we can get now in this zone */
		switch(strat_infos.zones[zone_num].type)
		{
			case ZONE_TYPE_HEART:
				if(strat_infos.zones[zone_num].flags & ZONE_CHECKED_OPP)
					strat_zones_points[zone_num]=4;
					
				/* The robot has fires inside */
				strat_zones_points[zone_num]+=(strat_infos.fires_inside*2);
				break;
				
			case ZONE_TYPE_TREE:
				/* If visited by the opponent: no points */
				if(strat_infos.zones[zone_num].flags & ZONE_CHECKED_OPP)
					strat_zones_points[zone_num]=0;
				break;
				
			case ZONE_TYPE_FIRE:
				if(strat_infos.zones[zone_num].flags & ZONE_CHECKED_OPP)
					strat_zones_points[zone_num]=0;
				break;
				
			case ZONE_TYPE_TORCH:
				if(strat_infos.zones[zone_num].flags & ZONE_CHECKED_OPP)
					strat_zones_points[zone_num]=0;
				break;
				
			case ZONE_TYPE_M_TORCH:
				if(strat_infos.zones[zone_num].flags & ZONE_CHECKED_OPP)
					strat_zones_points[zone_num]=0;
				break;
				
				
				
			case ZONE_TYPE_BASKET:
				/* Save fruits on basket */
				strat_zones_points[zone_num]=strat_infos.harvested_trees*3;
				
				/* Opponent has given us toxic fruits */
				// XXX Remove toxic fruits available???
				if(((mainboard.our_color==I2C_COLOR_YELLOW) && (zone_num==ZONE_BASKET_2)) ||
				((mainboard.our_color==I2C_COLOR_RED) && (zone_num==ZONE_BASKET_1)))
				{
					if(strat_infos.zones[zone_num].flags & ZONE_CHECKED_OPP)
					{
						if(strat_infos.zones[ZONE_TREE_1].flags & ZONE_CHECKED_OPP)
							strat_zones_points[zone_num]+=2;
						if(strat_infos.zones[ZONE_TREE_2].flags & ZONE_CHECKED_OPP)
							strat_zones_points[zone_num]+=2;
						if(strat_infos.zones[ZONE_TREE_3].flags & ZONE_CHECKED_OPP)
							strat_zones_points[zone_num]+=2;
						if(strat_infos.zones[ZONE_TREE_4].flags & ZONE_CHECKED_OPP)
							strat_zones_points[zone_num]+=2;
					}
				}
				break;
				
			/* Remain the same */
			case ZONE_TYPE_FRESCO:
			case ZONE_TYPE_MAMOOTH:
			case ZONE_TYPE_HOME:
				break;
				
			default:
				break;
		}
		
		/* 4. Distance from robot to the zone */
		// TODO
		//Give points to closest zones
		
		
		
		/* Recalculate priorities depending on strategy TODO*/
		/***********************************************************/
		/* Defensive: protect our points */
		
		/* Risky: try to get the maximum points without protecting */
		
		/* Moderate: combine defense and risk */
		
		/* Offensive: make the opponent lose points */
		/***********************************************************/
	}
	#endif
}


/* smart play */
uint8_t strat_smart(void)
{
	int8_t zone_num;
	uint8_t err;
	
	/* Tasks secondary robot */
	
	/* get new zone */
	
	zone_num = strat_get_new_zone();
	printf_P(PSTR("Zone: %d. Priority: %d\r\n"),zone_num,strat_infos.zones[zone_num].prio);
		
	if(zone_num == -1) {
		//printf_P(PSTR("No zone is found\r\n"));
		return END_TRAJ;
	}else{
		/* goto zone */
		//printf_P(PSTR("Going to zone %s.\r\n"),numzone2name[zone_num]);
		strat_infos.goto_zone = zone_num;
		strat_dump_infos(__FUNCTION__);
		
		err = strat_goto_zone(zone_num);
		
		if (!TRAJ_SUCCESS(err)) {
			//printf_P(PSTR("Can't reach zone %d.\r\n"), zone_num);
			
			printf_P(PSTR("err\r\n"));
			return END_TRAJ;
		}
		
		printf_P(PSTR("strat_work_on_zone\r\n"));
		/* work on zone */
		strat_infos.last_zone = strat_infos.current_zone;
		strat_infos.current_zone = strat_infos.goto_zone;
		strat_dump_infos(__FUNCTION__);

		err = strat_work_on_zone(zone_num);
		if (!TRAJ_SUCCESS(err)) {
			//printf_P(PSTR("Work on zone %s fails.\r\n"), numzone2name[zone_num]);
		}
		else
		{
			// Switch off devices, go back to normal state if anything was deployed
		}

		// mark the zone as checked 
		strat_infos.zones[zone_num].flags |= ZONE_CHECKED;

		return END_TRAJ;
	}
}


void strat_opp_tracking (void) 
{
#if 0
#define MAX_TIME_BETWEEN_VISITS_MS	4000
#define TIME_MS_TREE				1500
#define TIME_MS_HEART				1500
#define TIME_MS_BASKET				1000
#define UPDATE_ZONES_PERIOD_MS		25	
	
	uint8_t flags;
	uint8_t zone_opp;
	
    /* check if there are opponents in every zone */
    for(zone_opp = 0; zone_opp <  ZONES_MAX-1; zone_opp++)
    {
	   
	if(opponents_are_in_area(COLOR_X(strat_infos.zones[zone_opp].x_up), strat_infos.zones[zone_opp].y_up,
                                     COLOR_X(strat_infos.zones[zone_opp].x_down), strat_infos.zones[zone_opp].y_down)){
			
			if(!(strat_infos.zones[zone_opp].flags & (ZONE_CHECKED_OPP)))
			{
				IRQ_LOCK(flags);
				strat_infos.zones[zone_opp].last_time_opp_here=time_get_us2();
				IRQ_UNLOCK(flags);
				if((time_get_us2() - strat_infos.zones[zone_opp].last_time_opp_here) < MAX_TIME_BETWEEN_VISITS_MS*1000L)
				{
					/* Opponent continues in the same zone: */
					/* update zone time */ 
					IRQ_LOCK(flags);
					strat_infos.zones[zone_opp].opp_time_zone_us += UPDATE_ZONES_PERIOD_MS*1000L;
					IRQ_UNLOCK(flags);

					/* Mark zone as checked and sum points */
					switch(strat_infos.zones[zone_opp].type)
					{
						case ZONE_TYPE_TREE:
							if(strat_infos.zones[zone_opp].opp_time_zone_us>=TIME_MS_TREE*1000L)
							{
								strat_infos.zones[zone_opp].flags |= ZONE_CHECKED_OPP;
								strat_infos.opp_harvested_trees++;
								printf_P("opp_harvested_trees=%d\n",strat_infos.opp_harvested_trees);
								printf_P("OPP approximated score: %d\n", strat_infos.opp_score);
							}
							break;
						case ZONE_TYPE_BASKET:
							if(((mainboard.our_color==I2C_COLOR_YELLOW) && (zone_opp==ZONE_BASKET_1)) ||
							((mainboard.our_color==I2C_COLOR_RED) && (zone_opp==ZONE_BASKET_2)))
							{
								if(strat_infos.zones[zone_opp].opp_time_zone_us>=TIME_MS_BASKET*1000L)
								{
									if(strat_infos.opp_harvested_trees!=0)
									{
										strat_infos.opp_score += strat_infos.opp_harvested_trees * 3;
										strat_infos.opp_harvested_trees=0;
										printf_P("opp_harvested_trees=%d\n",strat_infos.opp_harvested_trees);
										printf_P("OPP approximated score: %d\n", strat_infos.opp_score);
									}
								}
							}
							break;
						case ZONE_TYPE_HEART:
							if(strat_infos.zones[zone_opp].opp_time_zone_us>= TIME_MS_HEART*1000L)
							{
								strat_infos.zones[zone_opp].flags |= ZONE_CHECKED_OPP;
								strat_infos.opp_score += 4;
								printf_P("OPP approximated score: %d\n", strat_infos.opp_score);
							}
							break;
						default:
							break;
					}
				}
							
				/* Zone has changed */
				else
				{
					/* reset zone time */
					IRQ_LOCK(flags);
					strat_infos.zones[zone_opp].opp_time_zone_us = 0;
					IRQ_UNLOCK(flags);
				}
			}
		}
	}
#endif
}


void strat_homologation(void)
{
	uint8_t err;
	uint8_t i=0;
	#define ZONES_SEQUENCE_LENGTH 6
	uint8_t zones_sequence[ZONES_SEQUENCE_LENGTH];
	
	/* Secondary robot */
	//
	
	time_wait_ms(2000);
	trajectory_d_rel(&mainboard.traj,250);
	err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);
	
	for(i=0; i<ZONES_SEQUENCE_LENGTH; i++)
	{
		/* goto zone */
		//printf_P(PSTR("Going to zone %s.\r\n"),numzone2name[zones_sequence[i]]);
		strat_dump_infos(__FUNCTION__);
		strat_infos.current_zone=-1;
		strat_infos.goto_zone=i;

		strat_goto_zone (zones_sequence[i]);
		err = wait_traj_end(TRAJ_FLAGS_STD);
		if (!TRAJ_SUCCESS(err)) {
			strat_infos.current_zone=-1;
			printf_P(PSTR("Can't reach zone %s.\r\n"), numzone2name[zones_sequence[i]]);
		}
		else{
			strat_infos.current_zone=i;
		}

		strat_infos.last_zone=strat_infos.current_zone;
		strat_infos.goto_zone=-1;


		/* work on zone */
		strat_dump_infos(__FUNCTION__);
		err = strat_work_on_zone(zones_sequence[i]);
		if (!TRAJ_SUCCESS(err)) {
			printf_P(PSTR("Work on zone %s fails.\r\n"), numzone2name[zones_sequence[i]]);
		}

		/* mark the zone as checked */
		strat_infos.zones[i].flags |= ZONE_CHECKED;
	}
}

