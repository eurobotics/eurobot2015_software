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
 *  Javier Baliñas Santos <javier@arc-robots.org> and Silvia Santano
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
#include "../common/bt_commands.h"

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

/******************* BT STATUS MANAGE FUNCTIONS ******************************/

/* set bt cmd id and args checksum */
/*void bt_status_set_cmd_id_and_checksum (uint8_t cmd_id, uint8_t args_sumatory) {
	uint8_t flags;
	IRQ_LOCK (flags);
	robot_2nd.cmd_id = cmd_id;
	robot_2nd.cmd_args_checksum = (uint8_t)(cmd_id + args_sumatory);
	robot_2nd.cmd_ret = 0;
	IRQ_UNLOCK (flags);
}*/

/* set bt cmd id and args checksum */
void bt_status_set_cmd_ret (uint8_t ret) {
	uint8_t flags;
	IRQ_LOCK (flags);
	robot_2nd.cmd_ret = ret;
	IRQ_UNLOCK (flags);
}

inline void bt_status_set_cmd_ack (uint8_t val) {
 	bt_status_set_cmd_ret (val);
}

/* send data in norma mode, any protocol is used */
static void uart_send_buffer (uint8_t *buff, uint16_t length)
{
  uint16_t i;

	for(i=0; i<length; i++){
#ifndef HOST_VERSION
		uart_send(CMDLINE_UART, buff[i]);
#else
		robotsim_uart_send_BT (buff[i]);
#endif
	}
}

void bt_send_status (void)
{
	struct bt_robot_2nd_status_ans ans;
    uint8_t flags;
	//static uint16_t i=0;

	/* send status info */
	IRQ_LOCK(flags);

	/* robot possition */
	ans.x = position_get_x_s16(&mainboard.pos);
	ans.y = position_get_y_s16(&mainboard.pos);
	ans.a_abs = position_get_a_deg_s16(&mainboard.pos);

	/* opponent 1 */
	ans.opponent1_x = beaconboard.opponent1_x;
	ans.opponent1_y = beaconboard.opponent1_y;

	/* opponent 2 */
	ans.opponent2_x = beaconboard.opponent2_x;
	ans.opponent2_y = beaconboard.opponent2_y;

	ans.done_flags = robot_2nd.done_flags;
	ans.color = mainboard.our_color;

	/* XXX cmd feedback: must be filld by cmds */
	//ans.cmd_id = robot_2nd.cmd_id;
	ans.cmd_ret = robot_2nd.cmd_ret;
	//ans.cmd_args_checksum = robot_2nd.cmd_args_checksum;
	IRQ_UNLOCK(flags);

//#define DEBUG_STATUS
#ifdef DEBUG_STATUS
	/* fill answer structure */
	ans.x = ans.opponent_x = ans.opponent2_x = i++;
	ans.y = ans.opponent_y = ans.opponent2_y = i + 1000;
	ans.a_abs = i + 2000;
#endif

	ans.checksum = bt_checksum ((uint8_t *)&ans, sizeof (ans)-sizeof(ans.checksum));

	/* send answer */
	uint8_t sync_header[] = BT_ROBOT_2ND_SYNC_HEADER;
	uart_send_buffer (sync_header, sizeof(sync_header));
	uart_send_buffer ((uint8_t*) &ans, sizeof(ans));
}



/******************* BT PROTOCOL COMMANDS *************************************/

void bt_strat_exit (void)
{
	/* set ACK */
	bt_status_set_cmd_ack (0);

	strat_exit();
}

void bt_auto_position (void)
{
	/* implicit checksum in cmdline because arguments are ascii tokens
	   pe. "color yellow" or "color red" */

	/* set ACK */
	bt_status_set_cmd_ret (0);

	/* set bt_task */
	strat_bt_task_rqst (BT_AUTO_POSITION, 0,0,0,0,0);
}

void bt_set_color (uint8_t color)
{
	/* implicit checksum in cmdline because arguments are acii tokens
	   pe. "color yellow" or "color red" */

	/* set ACK */
	bt_status_set_cmd_ret (0);

	/* execute command, can be a variable assigment, or a signe o periodic
       schedule event */
	mainboard.our_color = color;
}

void bt_strat_init (void)
{
	/* implicit checksum in cmdline because arguments are acii tokens
	   pe. "color yellow" or "color red" */

	/* set ACK */
	bt_status_set_cmd_ret (0);

	strat_init();
}

void bt_trajectory_goto_forward_xy_abs (int16_t x, int16_t y, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* execute */
		trajectory_goto_forward_xy_abs (&mainboard.traj, x, y);

		/* set bt_task */
		strat_bt_task_rqst (BT_GOTO, x,y, 0,0,0);
	}
	else {
		/* set ACK */
		bt_status_set_cmd_ack (END_ERROR);

	}
}
void bt_trajectory_goto_backward_xy_abs (int16_t x, int16_t y, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* execute */
		trajectory_goto_backward_xy_abs (&mainboard.traj, x, y);

		/* set bt_task */
		strat_bt_task_rqst (BT_GOTO, 0,0,0,0,0);
	}
	else {
		/* set ACK */
		bt_status_set_cmd_ack (END_ERROR);

	}
}

void bt_trajectory_goto_xy_abs (int16_t x, int16_t y, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* execute */
		trajectory_goto_xy_abs (&mainboard.traj, x, y);

		/* set bt_task */
		strat_bt_task_rqst (BT_GOTO, x,y, 0,0,0);
	}
	else {
		/* set ACK */
		bt_status_set_cmd_ack (END_ERROR);

	}
}

void bt_trajectory_goto_xy_rel(int16_t x, int16_t y, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* execute */
		trajectory_goto_xy_rel (&mainboard.traj, x, y);

		/* set bt_task */
		strat_bt_task_rqst (BT_GOTO, x,y, 0,0,0);
	}
	else {
		/* set ACK */
		bt_status_set_cmd_ack (END_ERROR);

	}
}



void bt_goto_and_avoid (int16_t x, int16_t y, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* set bt_task */
		strat_bt_task_rqst (BT_GOTO_AVOID, x,y, 0,0,0);
	}
	else {
		/* set ACK */
		bt_status_set_cmd_ack (END_ERROR);
	}
}
void bt_goto_and_avoid_forward (int16_t x, int16_t y, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* set bt_task */
		strat_bt_task_rqst (BT_GOTO_AVOID_FW, x,y, 0,0,0);
	}
	else {
		/* set NACK */
		bt_status_set_cmd_ack (END_ERROR);

	}
}

void bt_goto_and_avoid_backward (int16_t x, int16_t y, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* set bt_task */
		strat_bt_task_rqst (BT_GOTO_AVOID_BW, x,y, 0,0,0);
	}
	else {
		/* set ACK */
		bt_status_set_cmd_ack (END_ERROR);

	}
}

void bt_task_pick_cup(int16_t x, int16_t y, uint8_t side, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y+side) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* set bt_task */
		strat_bt_task_rqst (BT_TASK_PICK_CUP, x,y, side,0,0);
	}
	else {
		/* set ACK */
		bt_status_set_cmd_ack (END_ERROR);

	}
}

void bt_task_carpet(void)
{
	/* set ACK */
	bt_status_set_cmd_ack (0);

	/* set bt_task */
	strat_bt_task_rqst (BT_TASK_CARPET, 0,0,0,0,0);
}

void bt_task_stairs(void)
{
	/* set ACK */
	bt_status_set_cmd_ack (0);

	/* set bt_task */
	strat_bt_task_rqst (BT_TASK_STAIRS, 0,0,0,0,0);
}

void bt_task_bring_cup(int16_t x, int16_t y, uint8_t side, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y+side) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* set bt_task */
		strat_bt_task_rqst (BT_TASK_BRING_CUP, x,y, side,0,0);
	}
	else {
		/* set ACK */
		bt_status_set_cmd_ack (END_ERROR);

	}
}

void bt_task_clap(int16_t x, int16_t y, int16_t args_checksum)
{
	/* check args checksum */
	if ((x+y) == args_checksum) {

		/* set ACK */
		bt_status_set_cmd_ack (0);

		/* set bt_task */
		strat_bt_task_rqst (BT_TASK_CLAP, x,y, 0,0,0);
	}
	else {
		/* set ACK */
		bt_status_set_cmd_ack (END_ERROR);

	}
}




