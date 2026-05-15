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
#include "task_display_attribute.h"
#include "task_display_interface.h"
#include "display.h"

/********************** macros and definitions *******************************/
#define DEL_DSP_MIN				0ul
#define DEL_DSP_MED				50ul
#define DEL_DSP_MAX				500ul

/********************** internal data declaration ****************************/
task_display_dta_t task_display_dta;

/********************** internal functions declaration ***********************/
void task_display_statechart(void);

/********************** internal data definition *****************************/
const char *p_task_display		= "Task Display (Display Statechart)";
const char *p_task_display_		= "Non-Blocking Code";
const char *p_task_display__	= "(Update by Time Code, period = 1mS)";

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
void task_display_init(void *parameters)
{
	task_display_dta_t 	*p_task_display_dta;
	task_display_st_t	state;
	task_display_ev_t	event;
	bool b_event;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", GET_NAME(task_display_init), HAL_GetTick());
	LOGGER_INFO("   %s is a %s", GET_NAME(task_display), p_task_display);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_display), p_task_display_);
	LOGGER_INFO("   %s is a %s", GET_NAME(task_display), p_task_display__);

	/* Update Task Display Data Pointer */
	p_task_display_dta = &task_display_dta;

	/* Init & Print out: Task execution FSM */
	state = ST_DSP_IDLE;
	p_task_display_dta->state = state;

	event = EV_DSP_IDLE;
	p_task_display_dta->event = event;

	b_event = false;
	p_task_display_dta->flag = b_event;

	LOGGER_INFO(" ");
	LOGGER_INFO("   %s = %lu   %s = %lu   %s = %s",
				GET_NAME(state), (uint32_t)state,
				GET_NAME(event), (uint32_t)event,
				GET_NAME(b_event), (b_event ? "true" : "false"));

	/* Init & Print out: LCD Display */
	displayInit( DISPLAY_CONNECTION_GPIO_4BITS );

	displayCharPositionWrite(0, 0);
    p_task_display_dta->row = 0;
	displayStringWrite(p_task_display_dta->ddram[p_task_display_dta->row]);

    displayCharPositionWrite(0, 1);
	p_task_display_dta->row = 1;
	displayStringWrite(p_task_display_dta->ddram[p_task_display_dta->row]);
}

void task_display_update(void *parameters)
{
	/* Run Task Statechart */
	task_display_statechart();
}

void task_display_statechart(void)
{
	task_display_dta_t *p_task_display_dta;

	/* Update Task Display Data Pointer */
	p_task_display_dta = &task_display_dta;

	switch (p_task_display_dta->state)
	{
		case ST_DSP_IDLE:

			if ((true == p_task_display_dta->flag) && (EV_DSP_UPDATE == p_task_display_dta->event))
			{
				p_task_display_dta->flag = false;
				p_task_display_dta->row = 0;
				p_task_display_dta->column = 0;
				p_task_display_dta->state = ST_DSP_SET_CURSOR_ROW_0;
			}

			break;

		//Posicionnamos en fila 0
		case ST_DSP_SET_CURSOR_ROW_0:
				displayCharPositionWrite(0, 0);
				p_task_display_dta->state = ST_DSP_WRITE_ROW_0;
				break;

		case ST_DSP_WRITE_ROW_0:
			// Enviamos 1 solo dato: El carácter actual que esta en la Fila 0
				displayDataWrite(p_task_display_dta->ddram[0][p_task_display_dta->column]);
				p_task_display_dta->column++; // Aca avanzamos a la siguiente letra

				// Si ya se escribieron las 16 columnas, pasa a configurar la Fila 1
				if (p_task_display_dta->column == COLUMNS)
				{
					p_task_display_dta->row = 1;
					p_task_display_dta->column = 0;
					p_task_display_dta->state = ST_DSP_SET_CURSOR_ROW_1;
				}
				// Si no ha llegado a 16, se queda en este mismo estado para el próximo milisegundo
				break;

		case ST_DSP_SET_CURSOR_ROW_1:
					// Envía 1 sola instrucción: Posicionar el cursor en la Fila 1
					displayCharPositionWrite(0, 1);
					p_task_display_dta->state = ST_DSP_WRITE_ROW_1;
					break;

		case ST_DSP_WRITE_ROW_1:
					// Se envía 1 solo dato: El carácter actual de la Fila 1
					displayDataWrite(p_task_display_dta->ddram[1][p_task_display_dta->column]);
					p_task_display_dta->column++;

					// Si ya se escribieron las 16 columnas de la Fila 1 ya terminó el trabajo
					if (p_task_display_dta->column == COLUMNS)
					{
						p_task_display_dta->event = EV_DSP_IDLE;
						p_task_display_dta->state = ST_DSP_IDLE; // Vuelve al estado idle
					}
					break;

					/*
		case ST_DSP_UPDATE:

			if ((true == p_task_display_dta->flag) && (EV_DSP_UPDATE == p_task_display_dta->event))
			{
				p_task_display_dta->flag = false;
				p_task_display_dta->state = ST_DSP_UPDATE;
				p_task_display_dta->row = 0;
				p_task_display_dta->column = 0;

				displayCharPositionWrite(0, 0);
			    p_task_display_dta->row = 0;
				displayStringWrite(p_task_display_dta->ddram[p_task_display_dta->row]);

			    displayCharPositionWrite(0, 1);
				p_task_display_dta->row = 1;
				displayStringWrite(p_task_display_dta->ddram[p_task_display_dta->row]);

				p_task_display_dta->state = ST_DSP_IDLE;
			}

			break;
		*/

		default:

			p_task_display_dta->tick  = DEL_DSP_MIN;
			p_task_display_dta->state = ST_DSP_IDLE;
			p_task_display_dta->event = EV_DSP_IDLE;
			p_task_display_dta->flag = false;

			break;
	}
}

/********************** end of file ******************************************/
