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


int8_t strat_is_valid_zone(int8_t zone_num)
{
//#define OPP_WAS_IN_ZONE_TIMES

	//static uint16_t opp_times[ZONES_MAX];
	//static microseconds opp_time_us = 0;
	if(zone_num < 0){
		return -1;
	}


	/* if the zone has 0 priority then must be avoided */
	if(strat_infos.zones[zone_num].prio == 0)
	{
		//DEBUG(E_USER_STRAT,"zone num: %d. avoid.");
		return -1;
	}


	/* discard current zone */
	if(strat_infos.current_zone == zone_num)
	{
		//DEBUG(E_USER_STRAT,"zone num: %d. current_zone.");
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
		//DEBUG(E_USER_STRAT,"zone num: %d. avoid.");
		return -1;
	}

	return zone_num;
}
void set_next_sec_strategy(void){
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
void set_next_main_strategy(void){
	//strat_infos.current_main_strategy++;
	switch(strat_infos.current_main_strategy){
		case 1:
			set_strat_main_1();
			break;
		/*case 2:
			set_strat_main_2();
			strat_infos.current_sec_strategy = 0;
			break;*/
		default:
			break;
	}

}
void set_strat_main_1(void){
	strat_infos.current_main_strategy = 1;

	strat_infos.zones[ZONE_MY_STAND_GROUP_1].prio = 100;
	strat_infos.zones[ZONE_POPCORNCUP_3].prio = 90;


	strat_infos.zones[ZONE_MY_CLAP_2].prio = 80;
	strat_infos.zones[ZONE_POPCORNCUP_2].prio = 70;
	strat_infos.zones[ZONE_MY_STAND_GROUP_2].prio = 60;
	strat_infos.zones[ZONE_MY_CLAP_1].prio = 50;


	strat_infos.zones[ZONE_MY_STAND_GROUP_3].prio = 40;
	strat_infos.zones[ZONE_MY_POPCORNMAC].prio = 30;
	strat_infos.zones[ZONE_MY_STAND_GROUP_4].prio = 20;


	strat_infos.zones[ZONE_MY_HOME].prio = 10;


}
void set_strat_main_2(void){
	strat_infos.current_main_strategy = 1;

	strat_infos.zones[ZONE_MY_STAND_GROUP_1].prio = 100;


	strat_infos.zones[ZONE_MY_CLAP_2].prio = 90;
	strat_infos.zones[ZONE_POPCORNCUP_2].prio = 80;
	strat_infos.zones[ZONE_MY_STAND_GROUP_2].prio = 70;
	strat_infos.zones[ZONE_MY_CLAP_1].prio = 60;


	strat_infos.zones[ZONE_MY_STAND_GROUP_3].prio = 50;
	strat_infos.zones[ZONE_MY_POPCORNMAC].prio = 40;
	strat_infos.zones[ZONE_MY_STAND_GROUP_4].prio = 30;


	strat_infos.zones[ZONE_MY_HOME].prio = 10;


}



void set_strat_sec_1(void){
	strat_infos.current_sec_strategy = 1;
	strat_infos.zones[ZONE_POPCORNCUP_1].prio = 100;

	strat_infos.zones[ZONE_MY_CLAP_3].prio = 90;
	//DEBUG(E_USER_STRAT,"strat_sec_1");
	strat_infos.zones[ZONE_MY_CINEMA_DOWN].prio = 80;
	strat_infos.zones[ZONE_MY_CINEMA_UP].prio = 70;
	strat_infos.zones[ZONE_MY_STAIRS].prio = 60;
}
void set_strat_sec_2(void){
	strat_infos.current_sec_strategy = 2;
	//DEBUG(E_USER_STRAT,"strat_sec_2");
	strat_infos.zones[ZONE_POPCORNCUP_1].prio = 100;
	strat_infos.zones[ZONE_MY_CINEMA_UP].prio = 90;
	strat_infos.zones[ZONE_MY_CLAP_3].prio = 80;
	strat_infos.zones[ZONE_MY_CINEMA_DOWN].prio = 70;
	strat_infos.zones[ZONE_MY_STAIRS].prio = 60;
}

void set_strat_sec_3(void){
	strat_infos.current_sec_strategy = 3;
	//DEBUG(E_USER_STRAT,"strat_sec_3");

	strat_infos.zones[ZONE_POPCORNCUP_1].prio = 100;
	strat_infos.zones[ZONE_MY_CINEMA_DOWN].prio = 90;
	strat_infos.zones[ZONE_MY_CLAP_3].prio = 80;
	strat_infos.zones[ZONE_MY_CINEMA_UP].prio = 70;
	strat_infos.zones[ZONE_MY_STAIRS].prio = 60;
}


/* return new work zone, -1 if any zone is found */
int8_t strat_get_new_zone(uint8_t robot)
{
	uint8_t prio_max = 0;
	int8_t zone_num = -1, valid_zone= -1 ;
	int8_t i=0;
	static uint8_t no_more_zones = 0;

	/* evaluate zones */
	for(i=0; i < ZONES_MAX; i++)
	{
		/* compare current priority */
		if(strat_infos.zones[i].prio >= prio_max
		&& (strat_infos.zones[i].flags != ZONE_CHECKED)
		&& (strat_infos.zones[i].robot == robot)) {
			/* check if is a valid zone */
			prio_max = strat_infos.zones[i].prio;
			zone_num = i;
		}
		if( i == ZONES_MAX-1 ){
			valid_zone = strat_is_valid_zone(zone_num);
			zone_num = valid_zone;
		}

	}
	if(zone_num == -1 && no_more_zones == 0){
		no_more_zones = 1;
		DEBUG(E_USER_STRAT,"No more zones available");
	}
	return zone_num;
}

/* return END_TRAJ if zone is reached, err otherwise */
uint8_t strat_goto_zone(uint8_t zone_num)
{
	int8_t err=0;


	/* update strat_infos */
	strat_infos.current_zone=-1;
	strat_infos.goto_zone=zone_num;


	/* return if NULL xy */
	if (strat_infos.zones[zone_num].init_x == NULL ||
		strat_infos.zones[zone_num].init_y == NULL) {
		trajectory_d_rel(&mainboard.traj, 500);
		WARNING (E_USER_STRAT, "WARNING, No where to go (xy is NULL)");
		ERROUT(END_TRAJ);
	}

	/* set boundinbox */
	if (zone_num == ZONE_POPCORNCUP_2 || zone_num == ZONE_MY_STAND_GROUP_2)
		strat_set_bounding_box(BOUNDINBOX_WITHOUT_PLATFORM );
	else
		strat_set_bounding_box(BOUNDINBOX_INCLUDES_PLAFORM );

	/* Secondary robot */
	if(strat_infos.zones[zone_num].robot==SEC_ROBOT)
	{
		bt_robot_2nd_goto_and_avoid(COLOR_X(strat_infos.zones[zone_num].init_x),
												strat_infos.zones[zone_num].init_y);
		return 0;

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
	return err;
}


/* return END_TRAJ if the work is done, err otherwise */
uint8_t strat_work_on_zone(uint8_t zone_num)
{
	uint8_t err = END_TRAJ;

	time_wait_ms(2000);
#if 0
#ifdef HOST_VERSION
	DEBUG(E_USER_STRAT,"%s %d %s: press a key", __FUNCTION__, zone_num,numzone2name[zone_num]);
#endif



	/* return if NULL xy */
	if (strat_infos.zones[zone_num].x == NULL ||
		strat_infos.zones[zone_num].y == NULL) {
		/*ERROR (E_USER_STRAT, "ERROR zone xy is NULL");
		ERROUT(END_ERROR);*/
		return err;
	}


	/* Secondary robot */
	if(strat_infos.zones[zone_num].robot==SEC_ROBOT)
	{
		/* TODO */
	}
	else
	{
		switch (zone_num)
		{
			case ZONE_MY_STAND_GROUP_1:

				/* TODO: call specific function for stand group 1 */
				err = strat_harvest_orphan_stands (COLOR_X(MY_STAND_4_X),
												   MY_STAND_4_Y,
												   COLOR_INVERT(SIDE_RIGHT),
									 			   COLOR_INVERT(SIDE_RIGHT),
												   0,
												   SPEED_DIST_SLOW, /* harvest speed */

			    if (!TRAJ_SUCCESS(err))
				   ERROUT(err);

				err = strat_harvest_orphan_stands (COLOR_X(MY_STAND_5_X),
												   MY_STAND_5_Y,
												   COLOR_INVERT(SIDE_LEFT),
									 			   COLOR_INVERT(SIDE_LEFT),
												   0,
												   SPEED_DIST_SLOW, /* harvest speed */
												   0);				/* flags */
			    if (!TRAJ_SUCCESS(err))
				   ERROUT(err);

				err = strat_harvest_orphan_stands (COLOR_X(MY_STAND_6_X),
												   MY_STAND_6_Y,
												   COLOR_INVERT(SIDE_LEFT),
									 			   COLOR_INVERT(SIDE_LEFT),
												   0,
												   SPEED_DIST_SLOW, /* harvest speed */
												   0);				/* flags */
				break;


			case ZONE_MY_STAND_GROUP_2:
				err = strat_harvest_orphan_stands (COLOR_X(strat_infos.zones[zone_num].x),
												   strat_infos.zones[zone_num].y,
												   COLOR_INVERT(SIDE_LEFT),         /* side target */
									 			   SIDE_ALL,                        /* storing sides */
												   COLOR_A_REL(-10),                /* blade angle */
												   SPEED_DIST_SLOW,                 /* harvest speed */
												   STANDS_HARVEST_BACK_INIT_POS);	/* flags */
				break;

			case ZONE_MY_STAND_GROUP_3:
				err = strat_harvest_orphan_stands (COLOR_X(strat_infos.zones[zone_num].x),
												   strat_infos.zones[zone_num].y,
												   COLOR_INVERT(SIDE_LEFT),         /* side target */
									 			   SIDE_ALL,                        /* storing sides */
												   COLOR_A_REL(-20),                /* blade angle */
												   SPEED_DIST_SLOW,                 /* harvest speed */
												   STANDS_HARVEST_BACK_INIT_POS);	/* flags */
				break;

			case ZONE_MY_STAND_GROUP_4:
				err = strat_harvest_orphan_stands (COLOR_X(strat_infos.zones[zone_num].x),
												   strat_infos.zones[zone_num].y,
												   COLOR_INVERT(SIDE_RIGHT),        /* side target */
									 			   COLOR_INVERT(SIDE_RIGHT),        /* storing sides */
												   0,                               /* blade angle */
												   SPEED_DIST_SLOW,                 /* harvest speed */
												   STANDS_HARVEST_BACK_INIT_POS |
                                                   STANDS_HARVEST_CALIB_X);	        /* flags */

				break;

			case ZONE_MY_HOME:
				err = strat_buit_and_release_spotlight (COLOR_X(strat_infos.zones[zone_num].x),
														strat_infos.zones[zone_num].y,
														COLOR_INVERT(SIDE_LEFT));
				break;


			case ZONE_MY_STAND_GROUP_2:
			case ZONE_MY_STAND_GROUP_3:
			case ZONE_MY_STAND_GROUP_4:
			case ZONE_MY_POPCORNMAC:
			case ZONE_OPP_POPCORNMAC:

			case ZONE_POPCORNCUP_1:
			case ZONE_POPCORNCUP_2:
			case ZONE_POPCORNCUP_3:

			case ZONE_MY_CINEMA_UP:
			case ZONE_MY_CINEMA_DOWN:

			case ZONE_MY_CLAP_1:
			case ZONE_MY_CLAP_2:
			case ZONE_MY_CLAP_3:

				DEBUG(E_USER_STRAT, "Working on zone... ");
				time_wait_ms(2000);
				DEBUG(E_USER_STRAT, "fishish!! ");
				ERROUT(END_TRAJ);

				break;

			default:
				ERROR (E_USER_STRAT, "ERROR %s zone %d not supported", zone_num);
				break;

		}
	}
#endif
end:
	return err;
}


/* debug state machines step to step */
void state_debug_wait_key_pressed(void)
{
	if (!strat_infos.debug_step)
		return;

	DEBUG(E_USER_STRAT,"press a key");
	while(!cmdline_keypressed());
}



/* smart play */
uint8_t strat_smart(void)
{
	int8_t zone_num;
	uint8_t err;

	/* get new zone */
	zone_num = strat_get_new_zone(MAIN_ROBOT);
	strat_set_sec_new_order(zone_num);
	DEBUG(E_USER_STRAT,"Zone: %d. Priority: %d",zone_num,strat_infos.zones[zone_num].prio);

	if(zone_num == -1) {
		DEBUG(E_USER_STRAT,"No zone is found");
		return END_TRAJ;
	}else{
		/* goto zone */
		DEBUG(E_USER_STRAT,"Going to zone %s.",numzone2name[zone_num]);
		strat_infos.goto_zone = zone_num;
		strat_dump_infos(__FUNCTION__);

		err = strat_goto_zone(zone_num);

		if (!TRAJ_SUCCESS(err)) {
			DEBUG(E_USER_STRAT,"Can't reach zone %d.", zone_num);
			//set_next_main_strategy();
			return END_TRAJ;
		}

    		DEBUG(E_USER_STRAT,"WORK ON ZONE");
		DEBUG(E_USER_STRAT,"strat_work_on_zone");
		/* work on zone */
		strat_infos.last_zone = strat_infos.current_zone;
		strat_infos.current_zone = strat_infos.goto_zone;
		strat_dump_infos(__FUNCTION__);
		err = strat_work_on_zone(zone_num);
		if (!TRAJ_SUCCESS(err)) {
		    DEBUG(E_USER_STRAT,"Work on zone %s fails.", numzone2name[zone_num]);
                    /* IMPORTANT: If in home, get out of it before continuing
                    if(strat_infos.zones[zone_num].type==ZONE_TYPE_HOME)
                    {
                        do{
                                err = goto_and_avoid (COLOR_X(strat_infos.zones[zone_num].init_x),  strat_infos.zones[zone_num].init_y,  TRAJ_FLAGS_STD, TRAJ_FLAGS_NO_NEAR);
                        }while(!TRAJ_SUCCESS(err));
                    } */
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
/*Function to set robot_2nd in waiting state*/
void strat_should_wait_new_order(void){
	switch(strat_infos.strat_smart_sec_task){

		case ZONE_POPCORNCUP_2:
			strat_infos.strat_smart_sec = WAIT_FOR_ORDER;
		break;

		default:
			strat_infos.strat_smart_sec = GET_NEW_TASK;
		break;
	}
}
/*Function to set robot_2nd in working state*/
void strat_set_sec_new_order(int8_t zone_num){
	switch(zone_num){

		case ZONE_MY_CLAP_2:
			strat_infos.strat_smart_sec = GET_NEW_TASK;
		break;
		case ZONE_MY_STAND_GROUP_1:
			strat_infos.strat_smart_sec = INIT_ROBOT_2ND;
		break;
		default:
		break;
	}
}

void strat_smart_robot_2nd(void)
{
	static microseconds us = 0;
	uint8_t received_ack;
	uint8_t err;

	switch (strat_infos.strat_smart_sec){

		case WAIT_FOR_ORDER:
			break;

		case INIT_ROBOT_2ND:
#define INIT_ROBOT_2ND_X 400
#define INIT_ROBOT_2ND_Y 1000
			bt_robot_2nd_goto_xy_abs(COLOR_X(INIT_ROBOT_2ND_X) , INIT_ROBOT_2ND_Y);
			strat_infos.strat_smart_sec = WAIT_FOR_ORDER;
			break;

		case GET_NEW_TASK:
			strat_infos.strat_smart_sec_task = strat_get_new_zone(SEC_ROBOT);
 			if(strat_infos.strat_smart_sec_task == -1 ){
				 //set_new strategy,
				break;
			}
			else {
				DEBUG (E_USER_STRAT, "ZONE: %d",strat_infos.strat_smart_sec_task);
				strat_goto_zone(strat_infos.strat_smart_sec_task);
				DEBUG (E_USER_STRAT, "Sent command - Going to Zone: %d, RET= %d\r\n",strat_infos.strat_smart_sec_task,robot_2nd.cmd_ret);
				strat_infos.strat_smart_sec = WAIT_ACK_GOTO;
				us = time_get_us2();
			}

			break;

		case WAIT_ACK_GOTO:
		    // Wait 200ms to avoid previous values
			if (time_get_us2() - us < 200000L)
				break;

			// Check ACK
			received_ack=bt_robot_2nd_is_ack_received ();

			// ACK
			if(received_ack)
			{
				DEBUG (E_USER_STRAT, "Received ACK - GOTO Zone: %d, RET= %d\r\n",strat_infos.strat_smart_sec_task,robot_2nd.cmd_ret);
				us = time_get_us2();
				strat_infos.strat_smart_sec = GO_TO_ZONE;
			}

			// NACK: Communication error. Repeat command
			else if (time_get_us2() - us > 1000000L)
				strat_infos.strat_smart_sec = GET_NEW_TASK;

			break;

		case GO_TO_ZONE:
			if (time_get_us2() - us < 200000L)
				break;

			if(bt_robot_2nd_is_ret_received()){
				DEBUG (E_USER_STRAT, "Finished - Going to Zone: %d, RET= %d\r\n",strat_infos.strat_smart_sec_task,robot_2nd.cmd_ret);
				// XXX evaluate ret value
				err=bt_robot_2nd_test_end();
				if (TRAJ_SUCCESS(err)) {
					//strat_work_on_zone(strat_infos.strat_smart_sec_task);
					strat_infos.strat_smart_sec = WAIT_ACK_WORK;
				}else{
					//set_next_sec_strategy();
					strat_infos.strat_smart_sec = GET_NEW_TASK;
				}
			}

			break;

		// Not implemented
		case WAIT_ACK_WORK:
			us = time_get_us2();
			strat_infos.strat_smart_sec = WORK_ON_ZONE;
			break;

		case WORK_ON_ZONE:
			/* XXX HACK */
			if (time_get_us2() - us < 2000000L)
				break;

			strat_infos.zones[strat_infos.strat_smart_sec_task].flags |= ZONE_CHECKED;
			strat_should_wait_new_order();

			break;


		default:
			strat_infos.strat_smart_sec = WAIT_FOR_ORDER;
			break;
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
								DEBUG(E_USER_STRAT,"opp_harvested_trees=%d",strat_infos.opp_harvested_trees);
								DEBUG(E_USER_STRAT,"OPP approximated score: %d", strat_infos.opp_score);
							}
							break;
						case ZONE_TYPE_BASKET:
							if(((mainboard.our_color==I2C_COLOR_YELLOW) && (zone_opp==ZONE_BASKET_1)) ||
							((mainboard.our_color==I2C_COLOR_GREEN) && (zone_opp==ZONE_BASKET_2)))
							{
								if(strat_infos.zones[zone_opp].opp_time_zone_us>=TIME_MS_BASKET*1000L)
								{
									if(strat_infos.opp_harvested_trees!=0)
									{
										strat_infos.opp_score += strat_infos.opp_harvested_trees * 3;
										strat_infos.opp_harvested_trees=0;
										DEBUG(E_USER_STRAT,"opp_harvested_trees=%d",strat_infos.opp_harvested_trees);
										DEBUG(E_USER_STRAT,"OPP approximated score: %d", strat_infos.opp_score);
									}
								}
							}
							break;
						case ZONE_TYPE_HEART:
							if(strat_infos.zones[zone_opp].opp_time_zone_us>= TIME_MS_HEART*1000L)
							{
								strat_infos.zones[zone_opp].flags |= ZONE_CHECKED_OPP;
								strat_infos.opp_score += 4;
								DEBUG(E_USER_STRAT,"OPP approximated score: %d", strat_infos.opp_score);
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
        #define ZONES_SEQUENCE_LENGTH 3
	uint8_t zones_sequence[ZONES_SEQUENCE_LENGTH]={ZONE_MY_STAND_GROUP_1,ZONE_MY_POPCORNMAC,ZONE_MY_HOME};

	/* Secondary robot */
	//

	time_wait_ms(2000);
	trajectory_d_rel(&mainboard.traj,250);
	err = wait_traj_end(TRAJ_FLAGS_SMALL_DIST);

	for(i=0; i<ZONES_SEQUENCE_LENGTH; i++)
	{
		/* goto zone */
		//DEBUG(E_USER_STRAT,"Going to zone %s.",numzone2name[zones_sequence[i]]);
		strat_dump_infos(__FUNCTION__);
		strat_infos.current_zone=-1;
		strat_infos.goto_zone=i;

		//strat_goto_zone (zones_sequence[i], robot);
		err = wait_traj_end(TRAJ_FLAGS_STD);
		if (!TRAJ_SUCCESS(err)) {
			strat_infos.current_zone=-1;
			//DEBUG(E_USER_STRAT,"Can't reach zone %s.", numzone2name[zones_sequence[i]]);
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
                    //DEBUG(E_USER_STRAT,"Work on zone %s fails.", numzone2name[zones_sequence[i]]);

                    /* IMPORTANT: If in home, get out of it before continuing */
                    if(strat_infos.zones[zones_sequence[i]].type==ZONE_TYPE_HOME)
                    {
                        do{
                                err = goto_and_avoid (COLOR_X(strat_infos.zones[zones_sequence[i]].init_x),  strat_infos.zones[zones_sequence[i]].init_y,  TRAJ_FLAGS_STD, TRAJ_FLAGS_NO_NEAR);
                        }while(!TRAJ_SUCCESS(err));
                    }
		}

		/* mark the zone as checked */
		strat_infos.zones[i].flags |= ZONE_CHECKED;
	}
}
