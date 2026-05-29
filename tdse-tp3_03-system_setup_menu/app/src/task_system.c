/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"

#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"
#include "task_display_attribute.h"
#include "task_display_interface.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"
#include <stdio.h>

/********************** macros and definitions *******************************/
#define DEL_SYS_MIN			0ul
#define DEL_SYS_MED			250ul
#define DEL_SYS_MAX			500ul

/* Modes to excite Task System */
typedef enum task_system_mode {NORMAL, SETUP, MODE_QTY} task_system_mode_t;

#define SYSTEM_DTA_QTY	MODE_QTY

/********************** internal data declaration ****************************/
task_system_dta_t task_system_dta_list[SYSTEM_DTA_QTY];

/********************** internal functions declaration ***********************/
void task_system_normal_statechart(void);
void task_system_setup_statechart(void);

void task_system_set_mode(task_system_mode_t);

/********************** internal data definition *****************************/
const char *p_task_system 		= "Task System (System Statechart)";
const char *p_task_system_ 		= "Non-Blocking Code";
const char *p_task_system__ 	= "(Update by Time Code, period = 1mS)";

/********************** external data declaration ****************************/
task_system_mode_t g_task_system_mode;

/********************** external functions definition ************************/
void task_system_init(void *parameters)
{
	uint32_t index;
	task_system_dta_t 	*p_task_system_dta;
	task_system_st_t	state;
	task_system_ev_t	event;
	bool b_event;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", GET_NAME(task_system_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_system), p_task_system);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_system), p_task_system_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_system), p_task_system__);

	init_event_task_system();

	task_system_set_mode(NORMAL);

	for (index = 0; SYSTEM_DTA_QTY > index; index++)
	{
		/* Update Task System Data Pointer */
		p_task_system_dta = &task_system_dta_list[index];

		/* Init & Print out: Task execution FSM */
		state = ST_SYS_IDLE;
		p_task_system_dta->state = state;

		event = EV_SYS_IDLE;
		p_task_system_dta->event = event;

		b_event = false;
		p_task_system_dta->flag = b_event;

		//p_task_system_dta->editar_valor = 0;

		LOGGER_INFO(" ");
		LOGGER_INFO("   %s = %lu   %s = %lu   %s = %s",
					GET_NAME(state), (uint32_t)state,
					GET_NAME(event), (uint32_t)event,
					GET_NAME(b_event), (b_event ? "true" : "false"));
	}

	put_event_task_display(0, 0, "task_system_mode");
	put_event_task_display(0, 1, " NORMAL         ");

	task_system_set_mode(NORMAL);
}

void task_system_update(void *parameters)
{
	/* Run Task Statechart */
	switch (g_task_system_mode)
	{
		case NORMAL:

			task_system_normal_statechart();

			break;

		case SETUP:

			task_system_setup_statechart();

			break;

		default:

			task_system_set_mode(NORMAL);

			break;
		}
}

void task_system_normal_statechart(void)
{
	task_system_dta_t *p_task_system_dta;

	/* Update Task System Data Pointer */
	p_task_system_dta = &task_system_dta_list[NORMAL];

	if (true == any_event_task_system())
	{
		p_task_system_dta->flag = true;
		p_task_system_dta->event = get_event_task_system();
	}

	switch (p_task_system_dta->state)
	{
		case ST_SYS_IDLE:

			if ((true == p_task_system_dta->flag) && (EV_SYS_BTN_A == p_task_system_dta->event))
			{
				p_task_system_dta->flag = false;
				put_event_task_actuator(EV_LED_ACTIVE, ID_LED_A);
				p_task_system_dta->state = ST_SYS_ACTIVE;

				put_event_task_display(0, 0, "task_system_mode");
				put_event_task_display(0, 1, " SETUP          ");

				task_system_set_mode(SETUP);
			}

			break;

		case ST_SYS_ACTIVE:

			if ((true == p_task_system_dta->flag) && (EV_SYS_IDLE == p_task_system_dta->event))
			{
				p_task_system_dta->flag = false;
				put_event_task_actuator(EV_LED_IDLE, ID_LED_A);
				p_task_system_dta->state = ST_SYS_IDLE;
			}

			break;

		default:

			p_task_system_dta->tick  = DEL_SYS_MIN;
			p_task_system_dta->state = ST_SYS_IDLE;
			p_task_system_dta->event = EV_SYS_IDLE;
			p_task_system_dta->flag = false;

			break;
	}
}

void task_system_setup_statechart(void)
{
	task_system_dta_t *p_task_system_dta;

	/* Update Task System Data Pointer */
	p_task_system_dta = &task_system_dta_list[SETUP];
	char str_buffer[17]; // Búfer de texto para imprimir en el LCD
	if (true == any_event_task_system())
	{
		p_task_system_dta->flag = true;
		p_task_system_dta->event = get_event_task_system();
	}

	switch (p_task_system_dta->state)
	{
		case ST_SYS_IDLE:
				p_task_system_dta->state = ST_SYS_MAIN;

				put_event_task_display(0, 0, "---MAIN MENU----");
				put_event_task_display(0, 1, " ENTER         ");

			break;

		case ST_SYS_MAIN:
			if ((true == p_task_system_dta->flag) && (EV_SYS_ENTER == p_task_system_dta -> event))
			{
				p_task_system_dta -> flag = false;
				p_task_system_dta -> selec_motor = 1;
				p_task_system_dta -> state = ST_SYS_MENU_1;

				put_event_task_display(0, 0, "Menu 1: Motores");
				put_event_task_display(0, 1, " > Motor 1        ");

			}
			else if ((true == p_task_system_dta -> flag) && (EV_SYS_ESCAPE == p_task_system_dta -> event))
			{
			p_task_system_dta->flag = false;
			task_system_set_mode(NORMAL);
			p_task_system_dta->state = ST_SYS_IDLE; // Resetear para la próxima vez

			put_event_task_display(0, 0, "task_system_mode");
			put_event_task_display(0, 1, " NORMAL         ");
			}
			break;

		case ST_SYS_MENU_1:
			if (p_task_system_dta -> flag == true )
			{
				p_task_system_dta -> flag = false;
			 if (EV_SYS_ESCAPE == p_task_system_dta -> event)
			 {
				p_task_system_dta -> state = ST_SYS_MAIN;
				put_event_task_display(0, 0, "---MAIN MENU---");
				put_event_task_display(0, 1, " ENTER         ");
			 }
			 else if (EV_SYS_ENTER == p_task_system_dta -> event)
			 {
				p_task_system_dta -> state = ST_SYS_MENU_2;
				p_task_system_dta -> selec_param = 1;

				put_event_task_display(0, 0, "Menu 2: Parametros");
				put_event_task_display(0, 1, " > Power         ");
			 }
			 else if (EV_SYS_NEXT == p_task_system_dta -> event)
			 {
				 if (p_task_system_dta  -> selec_motor == 1)
				 {
					 p_task_system_dta -> selec_motor = 2;

					 put_event_task_display(0, 1, " > Motor 2        ");
				 }
				 else if (p_task_system_dta  -> selec_motor == 2)
				 {
					 p_task_system_dta -> selec_motor = 1;

					 put_event_task_display(0, 1, " > Motor 1        ");
				 }
			 }
			} break;


		case ST_SYS_MENU_2:
			if ( p_task_system_dta -> flag == true )
			{
				p_task_system_dta -> flag = false;
				if (p_task_system_dta -> event == EV_SYS_ESCAPE )
				{
					p_task_system_dta -> state = ST_SYS_MENU_1;
					put_event_task_display(0, 0, "Menu 1: Motores");
					if (p_task_system_dta -> selec_motor == 1 )
					{
						put_event_task_display(0, 1, " > Motor 1        ");
					} else if (p_task_system_dta -> selec_motor == 2 )
					{
						put_event_task_display(0, 1, " > Motor 2        ");
					}
				} else if (p_task_system_dta -> event == EV_SYS_ENTER)
				{
					p_task_system_dta -> state = ST_SYS_MENU_3;


					put_event_task_display(0, 0, "Menu 3: Editar");
					put_event_task_display(0, 1, " > Valor: 0         ");
				} else if ( p_task_system_dta -> event == EV_SYS_NEXT)
				{
					if (p_task_system_dta -> selec_param == 1)
					{
						p_task_system_dta -> selec_param = 2;

						put_event_task_display(0, 1, " > Speed        ");
					} else if (p_task_system_dta-> selec_param  == 2)
					{
						p_task_system_dta -> selec_param = 3;

						put_event_task_display(0, 1, " > Spin        ");
					} else if (p_task_system_dta -> selec_param == 3)
					{
						p_task_system_dta -> selec_param = 1;

						put_event_task_display(0, 1, " > Power        ");
					}
				}
			} break;


		case ST_SYS_MENU_3:
			if (p_task_system_dta -> flag == true)
		{
			p_task_system_dta->flag = false;

			if (EV_SYS_ESCAPE == p_task_system_dta->event)
			{
				p_task_system_dta->state = ST_SYS_MENU_2;
				put_event_task_display(0, 0, "Menu 2: Parametros  ");

				if (1 == p_task_system_dta->selec_param)
				{
					put_event_task_display(0, 1, "> Power         ");
				}
				else if (2 == p_task_system_dta->selec_param)
				{
					put_event_task_display(0, 1, "> Speed         ");
				}
				else if (3 == p_task_system_dta->selec_param)
				{
					put_event_task_display(0, 1, "> Spin          ");
				}
			}
			else if (EV_SYS_ENTER == p_task_system_dta->event)
			{
				p_task_system_dta->state = ST_SYS_MENU_2;
				put_event_task_display(0, 0, "Guardado!          ");
			}
			else if (EV_SYS_NEXT == p_task_system_dta->event)
			{
			if (1 == p_task_system_dta->selec_param)
				{
					if (0 == p_task_system_dta->editar_valor) {
						p_task_system_dta->editar_valor = 1;
					} else {
						p_task_system_dta->editar_valor = 0;
					}
					if (1 == p_task_system_dta->editar_valor) {
						put_event_task_display(0, 1, " > Power: ON    ");
					} else {
						put_event_task_display(0, 1, " > Power: OFF   ");
						}
					}
			else if (p_task_system_dta -> selec_param == 2){
				p_task_system_dta->editar_valor++;
				if (p_task_system_dta->editar_valor > 9) p_task_system_dta->editar_valor = 0;{

				snprintf(str_buffer, sizeof(str_buffer), "> Valor: %-7lu", p_task_system_dta->editar_valor);
				put_event_task_display(0, 1, str_buffer);
					}
				}
			else if (p_task_system_dta -> selec_param == 3 ) {
				if ( p_task_system_dta -> editar_valor == 0 ) {
					p_task_system_dta -> editar_valor = 1;
				} else {
					p_task_system_dta->editar_valor = 0;
					}
				if (p_task_system_dta->editar_valor == 1) {
					put_event_task_display(0, 1, " > Spin: RIGHT  ");
				} else {
					put_event_task_display(0, 1, " > Spin: LEFT   ");
					}
				}
			}
		}
		break;


		default:

			p_task_system_dta->tick  = DEL_SYS_MIN;
			p_task_system_dta->state = ST_SYS_IDLE;
			p_task_system_dta->event = EV_SYS_IDLE;
			p_task_system_dta->flag = false;

			break;
	}
}

void task_system_set_mode(task_system_mode_t task_system_mode)
{
	g_task_system_mode = task_system_mode;
}

/********************** end of file ******************************************/
