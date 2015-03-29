/*
 *  Copyright Droids Corporation (2008)
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
 *  Olivier MATZ <zer0@droids-corp.org> 
 */

/*   *  Copyright Robotics Association of Coslada, Eurobotics Engineering (2011) *  Javier Bali�as Santos <javier@arc-robots.org> * *  Code ported to family of microcontrollers dsPIC from *  commands_gen.c,v 1.4 2009/05/27 20:04:07 zer0 Exp  */

#include <stdio.h>
#include <string.h>

#include <aversive/pgmspace.h>
#include <aversive/wait.h>
#include <aversive/error.h>

#include <ax12.h>
#include <uart.h>
#include <clock_time.h>
#include <scheduler.h>
#include <rdline.h>
#include <parse.h>
#include <parse_string.h>
#include <parse_num.h>

//#include <diagnostic.h>

#include "../common/i2c_commands.h"
#include "cmdline.h"
#include "sensor.h"
#include "state.h"
#include "main.h"

/**********************************************************/
/* Reset */

/* this structure is filled when cmd_reset is parsed successfully */
struct cmd_reset_result {
	fixed_string_t arg0;
};

/* function called when cmd_reset is parsed successfully */
static void cmd_reset_parsed(__attribute__((unused)) void *parsed_result,
			     __attribute__((unused)) void *data)
{
	asm("Reset");
}

prog_char str_reset_arg0[] = "reset";
parse_pgm_token_string_t cmd_reset_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_reset_result, arg0, str_reset_arg0);

prog_char help_reset[] = "Reset the board";
parse_pgm_inst_t cmd_reset = {
	.f = cmd_reset_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_reset,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_reset_arg0, 
		NULL,
	},
};

///**********************************************************/
///* Bootloader */
//
///* this structure is filled when cmd_bootloader is parsed successfully */
//struct cmd_bootloader_result {
//	fixed_string_t arg0;
//};
//
///* function called when cmd_bootloader is parsed successfully */
//static void cmd_bootloader_parsed(__attribute__((unused)) void *parsed_result,
//				  __attribute__((unused)) void *data)
//{
//	bootloader();
//}
//
//prog_char str_bootloader_arg0[] = "bootloader";
//parse_pgm_token_string_t cmd_bootloader_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_bootloader_result, arg0, str_bootloader_arg0);
//
//prog_char help_bootloader[] = "Launch the bootloader";
//parse_pgm_inst_t cmd_bootloader = {
//	.f = cmd_bootloader_parsed,  /* function to call */
//	.data = NULL,      /* 2nd arg of func */
//	.help_str = help_bootloader,
//	.tokens = {        /* token list, NULL terminated */
//		(prog_void *)&cmd_bootloader_arg0, 
//		NULL,
//	},
//};

/**********************************************************/
/* Encoders tests */

/* this structure is filled when cmd_encoders is parsed successfully */
struct cmd_encoders_result {
	fixed_string_t arg0;
	fixed_string_t arg1;
};

/* function called when cmd_encoders is parsed successfully */
static void cmd_encoders_parsed(void *parsed_result, __attribute__((unused)) void *data)
{
	struct cmd_encoders_result *res = parsed_result;

	if (!strcmp_P(res->arg1, PSTR("reset"))) {
		encoders_dspic_set_value((void *)1, 0);
		return;
	}

	/* show */
	while(!cmdline_keypressed()) {
		printf_P(PSTR("% .8ld\r\n"), 
			 encoders_dspic_get_value((void *)1));
		wait_ms(100);
	}
}

prog_char str_encoders_arg0[] = "encoders";
parse_pgm_token_string_t cmd_encoders_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_encoders_result, arg0, str_encoders_arg0);
prog_char str_encoders_arg1[] = "show#reset";
parse_pgm_token_string_t cmd_encoders_arg1 = TOKEN_STRING_INITIALIZER(struct cmd_encoders_result, arg1, str_encoders_arg1);

prog_char help_encoders[] = "Show encoders values";
parse_pgm_inst_t cmd_encoders = {
	.f = cmd_encoders_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_encoders,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_encoders_arg0, 
		(prog_void *)&cmd_encoders_arg1, 
		NULL,
	},
};

/**********************************************************/
/* Scheduler show */

/* this structure is filled when cmd_scheduler is parsed successfully */
struct cmd_scheduler_result {
	fixed_string_t arg0;
	fixed_string_t arg1;
};

/* function called when cmd_scheduler is parsed successfully */
static void cmd_scheduler_parsed( __attribute__((unused)) void *parsed_result,
				  __attribute__((unused)) void *data)
{
	scheduler_dump_events();
}

prog_char str_scheduler_arg0[] = "scheduler";
parse_pgm_token_string_t cmd_scheduler_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_scheduler_result, arg0, str_scheduler_arg0);
prog_char str_scheduler_arg1[] = "show";
parse_pgm_token_string_t cmd_scheduler_arg1 = TOKEN_STRING_INITIALIZER(struct cmd_scheduler_result, arg1, str_scheduler_arg1);

prog_char help_scheduler[] = "Show scheduler events";
parse_pgm_inst_t cmd_scheduler = {
	.f = cmd_scheduler_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_scheduler,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_scheduler_arg0, 
		(prog_void *)&cmd_scheduler_arg1, 
		NULL,
	},
};

/**********************************************************/
/* pwm_mc tests */

/* this structure is filled when cmd_pwm is parsed successfully */
struct cmd_pwm_mc_result {
	fixed_string_t arg0;
	fixed_string_t arg1;
	int16_t arg2;
};

/* function called when cmd_pwm is parsed successfully */
static void cmd_pwm_mc_parsed(void * parsed_result, __attribute__((unused)) void *data)
{
	void * pwm_mc_ptr = NULL;
	struct cmd_pwm_mc_result * res = parsed_result;

	if (!strcmp_P(res->arg1, PSTR("mod1_ch2")))
		pwm_mc_ptr = &gen.pwm_mc_mod1_ch2;
	else if (!strcmp_P(res->arg1, PSTR("mod2_ch1")))
		pwm_mc_ptr = &gen.pwm_mc_mod2_ch1;

	if (pwm_mc_ptr)
		pwm_mc_set(pwm_mc_ptr, res->arg2);

	printf_P(PSTR("done\r\n"));
}

prog_char str_pwm_mc_arg0[] = "pwm_mc";
parse_pgm_token_string_t cmd_pwm_mc_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_pwm_mc_result, arg0, str_pwm_mc_arg0);
prog_char str_pwm_mc_arg1[] = "mod1_ch2#mod2_ch1";
parse_pgm_token_string_t cmd_pwm_mc_arg1 = TOKEN_STRING_INITIALIZER(struct cmd_pwm_mc_result, arg1, str_pwm_mc_arg1);
parse_pgm_token_num_t cmd_pwm_mc_arg2 = TOKEN_NUM_INITIALIZER(struct cmd_pwm_mc_result, arg2, INT16);

prog_char help_pwm_mc[] = "Set pwm_mc values [-3333 ; 3333]";
parse_pgm_inst_t cmd_pwm_mc = {
	.f = cmd_pwm_mc_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_pwm_mc,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_pwm_mc_arg0, 
		(prog_void *)&cmd_pwm_mc_arg1, 
		(prog_void *)&cmd_pwm_mc_arg2, 
		NULL,
	},
};

/**********************************************************/
/* pwm_servo tests */

/* this structure is filled when cmd_pwm is parsed successfully */
struct cmd_pwm_servo_result {
	fixed_string_t arg0;
	fixed_string_t arg1;
	uint16_t arg2;
};

/* function called when cmd_pwm is parsed successfully */
static void cmd_pwm_servo_parsed(void *parsed_result, void *show_range)
{
	void * pwm_servo_ptr = NULL;
	struct cmd_pwm_servo_result * res = parsed_result;

	if(show_range){
		if (!strcmp_P(res->arg1, PSTR("oc1")))
			printf("oc1 range = [0 ; %d]\r\n", gen.pwm_servo_oc1.range);
		else if (!strcmp_P(res->arg1, PSTR("oc2")))
			printf("oc2 range = [0 ; %d]\r\n", gen.pwm_servo_oc2.range);
		else if (!strcmp_P(res->arg1, PSTR("oc3")))
			printf("oc3 range = [0 ; %d]\r\n", gen.pwm_servo_oc3.range);
#ifdef EUROBOT_2012_BOARD
		else if (!strcmp_P(res->arg1, PSTR("oc4")))
			printf("oc4 range = [0 ; %d]\r\n", gen.pwm_servo_oc4.range);
#endif
	}
	else{
		if (!strcmp_P(res->arg1, PSTR("oc1")))
			pwm_servo_ptr = &gen.pwm_servo_oc1;
		else if (!strcmp_P(res->arg1, PSTR("oc2")))
			pwm_servo_ptr = &gen.pwm_servo_oc2;
		else if (!strcmp_P(res->arg1, PSTR("oc3")))
			pwm_servo_ptr = &gen.pwm_servo_oc3;
#ifdef EUROBOT_2012_BOARD
		else if (!strcmp_P(res->arg1, PSTR("oc4")))
			pwm_servo_ptr = &gen.pwm_servo_oc4;
#endif	

		if (pwm_servo_ptr)
			pwm_servo_set(pwm_servo_ptr, res->arg2);
	
		printf("done\r\n");
	}
}

prog_char str_pwm_servo_arg0[] = "pwm_servo";
parse_pgm_token_string_t cmd_pwm_servo_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_pwm_servo_result, arg0, str_pwm_servo_arg0);
prog_char str_pwm_servo_arg1[] = "oc1#oc2#oc3#oc4";
parse_pgm_token_string_t cmd_pwm_servo_arg1 = TOKEN_STRING_INITIALIZER(struct cmd_pwm_servo_result, arg1, str_pwm_servo_arg1);
parse_pgm_token_num_t cmd_pwm_servo_arg2 = TOKEN_NUM_INITIALIZER(struct cmd_pwm_servo_result, arg2, INT16);

prog_char help_pwm_servo[] = "Set pwm_servo values";
parse_pgm_inst_t cmd_pwm_servo = {
	.f = cmd_pwm_servo_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_pwm_servo,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_pwm_servo_arg0, 
		(prog_void *)&cmd_pwm_servo_arg1, 
		(prog_void *)&cmd_pwm_servo_arg2, 
		NULL,
	},
};

/* show range */

/* this structure is filled when cmd_pwm_servo is parsed successfully */
struct cmd_pwm_servo_show_range_result {
	fixed_string_t arg0;
	fixed_string_t show_range;
	fixed_string_t arg1;
};
prog_char str_pwm_servo_show_range_arg[] = "show_range";
parse_pgm_token_string_t cmd_pwm_servo_show_range_arg = TOKEN_STRING_INITIALIZER(struct cmd_pwm_servo_show_range_result, show_range, str_pwm_servo_show_range_arg);

prog_char help_pwm_servo_show_range[] = "Show range values of pwm_servo channel";
parse_pgm_inst_t cmd_pwm_servo_show_range = {
	.f = cmd_pwm_servo_parsed,  /* function to call */
	.data = (void *)1,      /* 2nd arg of func */
	.help_str = help_pwm_servo_show_range,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_pwm_servo_arg0, 
		(prog_void *)&cmd_pwm_servo_show_range_arg,
		(prog_void *)&cmd_pwm_servo_arg1, 
		NULL,
	},
};

#if 0
/**********************************************************/
/* DAC MC tests */

/* this structure is filled when cmd_dac_mc is parsed successfully */
struct cmd_dac_mc_result {
	fixed_string_t arg0;
	fixed_string_t arg1;
	int32_t arg2;
};

/* function called when cmd_dac_mc is parsed successfully */
static void cmd_dac_mc_parsed(void * parsed_result, __attribute__((unused)) void *data)
{
	void * dac_mc_ptr = NULL;
	struct cmd_dac_mc_result * res = parsed_result;
	
	dac_mc_ptr = &gen.dac_mc_left;
	
	if (dac_mc_ptr)
		dac_mc_set(dac_mc_ptr, res->arg2);

	printf_P(PSTR("done\r\n"));
}

prog_char str_dac_mc_arg0[] = "dac_mc";
parse_pgm_token_string_t cmd_dac_mc_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_dac_mc_result, arg0, str_dac_mc_arg0);
//prog_char str_dac_mc_arg1[] = "left";
//parse_pgm_token_string_t cmd_dac_mc_arg1 = TOKEN_STRING_INITIALIZER(struct cmd_dac_mc_result, arg1, str_dac_mc_arg1);
parse_pgm_token_num_t cmd_dac_mc_arg2 = TOKEN_NUM_INITIALIZER(struct cmd_dac_mc_result, arg2, INT32);

prog_char help_dac_mc[] = "Set dac_mc values [-65535 ; 65535]";
parse_pgm_inst_t cmd_dac_mc = {
	.f = cmd_dac_mc_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_dac_mc,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_dac_mc_arg0, 
		//(prog_void *)&cmd_dac_mc_arg1, 
		(prog_void *)&cmd_dac_mc_arg2, 
		NULL,
	},
};
#endif


/**********************************************************/
/* Sensors tests */

/* this structure is filled when cmd_sensor is parsed successfully */
struct cmd_sensor_result {
	fixed_string_t arg0;
	fixed_string_t arg1;
};

/* function called when cmd_sensor is parsed successfully */
static void cmd_sensor_parsed(void *parsed_result, __attribute__((unused)) void *data)
{
	struct cmd_sensor_result *res = parsed_result;
	uint8_t i, loop = 0;
	uint16_t sensors, sensors_old = 0;

	if (!strcmp_P(res->arg1, PSTR("loop_show")))
		loop = 1;
	
	do {
		sensors = sensor_get_all();
		
		if (sensors == sensors_old)
		continue;

		sensors_old = sensors;

		printf_P(PSTR("SENSOR values: "));
		for (i=0; i<SENSOR_MAX; i++) {
			printf_P(PSTR("%d "), sensor_get(i));
		}
		printf_P(PSTR("\r\n"));
		//wait_ms(100);
	} while (loop && !cmdline_keypressed());
}

prog_char str_sensor_arg0[] = "sensor";
parse_pgm_token_string_t cmd_sensor_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_sensor_result, arg0, str_sensor_arg0);
prog_char str_sensor_arg1[] = "show#loop_show";
parse_pgm_token_string_t cmd_sensor_arg1 = TOKEN_STRING_INITIALIZER(struct cmd_sensor_result, arg1, str_sensor_arg1);

prog_char help_sensor[] = "Show sensor values";
parse_pgm_inst_t cmd_sensor = {
	.f = cmd_sensor_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_sensor,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_sensor_arg0, 
		(prog_void *)&cmd_sensor_arg1, 
		NULL,
	},
};

/**********************************************************/
/* Rele and solenoid outputs */

/* this structure is filled when cmd_rele is parsed successfully */
struct cmd_rele_result {
	fixed_string_t arg0;
	fixed_string_t arg1;
};

/* function called when cmd_rele is parsed successfully */
static void cmd_rele_parsed(void *parsed_result, __attribute__((unused)) void *data)
{
	struct cmd_rele_result *res = parsed_result;

	if (!strcmp_P(res->arg1, "on"))
		RELE_OUT_PIN = 0;
	else if (!strcmp_P(res->arg1, "off"))
		RELE_OUT_PIN = 1;

	/* show */
	printf("Rele is %s\n\r", RELE_OUT_PIN? "off":"on");
}

prog_char str_rele_arg0[] = "rele";
parse_pgm_token_string_t cmd_rele_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_rele_result, arg0, str_rele_arg0);
prog_char str_rele_arg1[] = "on#off#show";
parse_pgm_token_string_t cmd_rele_arg1 = TOKEN_STRING_INITIALIZER(struct cmd_rele_result, arg1, str_rele_arg1);

prog_char help_rele[] = "Set rele on/off";
parse_pgm_inst_t cmd_rele = {
	.f = cmd_rele_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_rele,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_rele_arg0, 
		(prog_void *)&cmd_rele_arg1, 
		NULL,
	},
};


/**********************************************************/
/* Log */

/* this structure is filled when cmd_log is parsed successfully */
struct cmd_log_result {
	fixed_string_t arg0;
	fixed_string_t arg1;
	uint8_t arg2;
	fixed_string_t arg3;
};

/* keep it sync with string choice */
static const prog_char uart_log[] = "uart";
static const prog_char i2c_log[] = "i2c";
static const prog_char i2cproto_log[] = "i2cproto";
static const prog_char sensor_log[] = "sensor";
static const prog_char state_log[] = "state";
static const prog_char ax12_log[] = "ax12";
static const prog_char cs_log[] = "cs";

struct log_name_and_num {
	const prog_char * name;
	uint8_t num;
};

static const struct log_name_and_num log_name_and_num[] = {
	{ uart_log, E_UART },
	{ i2c_log, E_I2C },
	{ i2cproto_log, E_USER_I2C_PROTO },
	{ sensor_log, E_USER_SENSOR },
	{ state_log, E_USER_ST_MACH },
	{ ax12_log, E_USER_AX12 },
	{ cs_log, E_USER_CS },
};

static uint8_t
log_name2num(const char * s)
{
	uint8_t i;
	
	for (i=0; i<sizeof(log_name_and_num)/sizeof(struct log_name_and_num); i++) {
		if (!strcmp_P(s, log_name_and_num[i].name)) {
			return log_name_and_num[i].num;
		}
	}
	return 0;
}

const prog_char *
log_num2name(uint8_t num)
{
	uint8_t i;
	
	for (i=0; i<sizeof(log_name_and_num)/sizeof(struct log_name_and_num); i++) {
		if (num ==  log_name_and_num[i].num) {
			return log_name_and_num[i].name;
		}
	}
	return NULL;
}

/* function called when cmd_log is parsed successfully */
static void cmd_log_do_show(void)
{
	uint8_t i, empty=1;
	const prog_char * name;

	printf_P(PSTR("log level is %d\r\n"), gen.log_level);
	for (i=0; i<NB_LOGS; i++) {
		name = log_num2name(gen.logs[i]);
		if (name) {
			printf_P(PSTR("log type %s is on\r\n"), name);
			empty = 0;
		}
	}
	if (empty)
		printf_P(PSTR("no log configured\r\n"));
}

/* function called when cmd_log is parsed successfully */
static void cmd_log_parsed(void * parsed_result, __attribute__((unused)) void *data)
{
	struct cmd_log_result *res = (struct cmd_log_result *) parsed_result;

	if (!strcmp_P(res->arg1, PSTR("level"))) {
		gen.log_level = res->arg2;
	}

	/* else it is a show */
	cmd_log_do_show();
}

prog_char str_log_arg0[] = "log";
parse_pgm_token_string_t cmd_log_arg0 = TOKEN_STRING_INITIALIZER(struct cmd_log_result, arg0, str_log_arg0);
prog_char str_log_arg1[] = "level";
parse_pgm_token_string_t cmd_log_arg1 = TOKEN_STRING_INITIALIZER(struct cmd_log_result, arg1, str_log_arg1);
parse_pgm_token_num_t cmd_log_arg2 = TOKEN_NUM_INITIALIZER(struct cmd_log_result, arg2, INT32);

prog_char help_log[] = "Set log options: level (0 -> 5)";
parse_pgm_inst_t cmd_log = {
	.f = cmd_log_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_log,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_log_arg0, 
		(prog_void *)&cmd_log_arg1, 
		(prog_void *)&cmd_log_arg2, 
		NULL,
	},
};

prog_char str_log_arg1_show[] = "show";
parse_pgm_token_string_t cmd_log_arg1_show = TOKEN_STRING_INITIALIZER(struct cmd_log_result, arg1, str_log_arg1_show);

prog_char help_log_show[] = "Show configured logs";
parse_pgm_inst_t cmd_log_show = {
	.f = cmd_log_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_log_show,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_log_arg0, 
		(prog_void *)&cmd_log_arg1_show, 
		NULL,
	},
};

/* this structure is filled when cmd_log is parsed successfully */
struct cmd_log_type_result {
	fixed_string_t arg0;
	fixed_string_t arg1;
	fixed_string_t arg2;
	fixed_string_t arg3;
};

/* function called when cmd_log is parsed successfully */
static void cmd_log_type_parsed(void * parsed_result, __attribute__((unused)) void *data)
{
	struct cmd_log_type_result *res = (struct cmd_log_type_result *) parsed_result;
	uint8_t lognum;
	uint8_t i;
	
	lognum = log_name2num(res->arg2);
	if (lognum == 0) {
		printf_P(PSTR("Cannot find log num\r\n"));
		return;
	}

	if (!strcmp_P(res->arg3, PSTR("on"))) {
		for (i=0; i<NB_LOGS; i++) {
			if (gen.logs[i] == lognum) {
				printf_P(PSTR("Already on\r\n"));
				return;
			}
		}
		for (i=0; i<NB_LOGS; i++) {
			if (gen.logs[i] == 0) {
				gen.logs[i] = lognum;
				break;
			}
		}
		if (i==NB_LOGS) {
			printf_P(PSTR("no more room\r\n"));
		}
	}
	else if (!strcmp_P(res->arg3, PSTR("off"))) {
		for (i=0; i<NB_LOGS; i++) {
			if (gen.logs[i] == lognum) {
				gen.logs[i] = 0;
				break;
			}
		}
		if (i==NB_LOGS) {
			printf_P(PSTR("already off\r\n"));
		}
	}
	cmd_log_do_show();
}

prog_char str_log_arg1_type[] = "type";
parse_pgm_token_string_t cmd_log_arg1_type = TOKEN_STRING_INITIALIZER(struct cmd_log_type_result, arg1, str_log_arg1_type);
/* keep it sync with log_name_and_num above */
prog_char str_log_arg2_type[] = "uart#i2c#i2cproto#sensor#state#ax12";
parse_pgm_token_string_t cmd_log_arg2_type = TOKEN_STRING_INITIALIZER(struct cmd_log_type_result, arg2, str_log_arg2_type);
prog_char str_log_arg3[] = "on#off";
parse_pgm_token_string_t cmd_log_arg3 = TOKEN_STRING_INITIALIZER(struct cmd_log_type_result, arg3, str_log_arg3);

prog_char help_log_type[] = "Set log type";
parse_pgm_inst_t cmd_log_type = {
	.f = cmd_log_type_parsed,  /* function to call */
	.data = NULL,      /* 2nd arg of func */
	.help_str = help_log_type,
	.tokens = {        /* token list, NULL terminated */
		(prog_void *)&cmd_log_arg0,
		(prog_void *)&cmd_log_arg1_type,
		(prog_void *)&cmd_log_arg2_type,
		(prog_void *)&cmd_log_arg3,
		NULL,
	},
};

