/****************************************************************
**	OrangeBot  Project
*****************************************************************************
**        /
**       /
**      /
** ______ \
**         \
**          \
*****************************************************************************
**
*****************************************************************************
**	Author: 			Orso Eric
**	Creation Date:
**	Last Edit Date:
**	Revision:			1
**	Version:			0.1 ALFA
****************************************************************************/

/****************************************************************************
**	HYSTORY VERSION
*****************************************************************************
**
****************************************************************************/

/****************************************************************************
**	DESCRIPTION
*****************************************************************************
**
****************************************************************************/

/****************************************************************************
**	KNOWN BUG
*****************************************************************************
**
****************************************************************************/

/****************************************************************************
**	INCLUDE
****************************************************************************/

//type definition using the bit width and signedness
#include <stdint.h>
//General purpose macros
#include "at_utils.h"
//AT4809 PORT macros definitions
#include "at4809_port.h"
//from number to string
#include "at_string.h"

#include "global.h"

/****************************************************************************
**	NAMESPACES
****************************************************************************/

/****************************************************************************
**	GLOBAL VARIABILE
****************************************************************************/

/****************************************************************************
**	FUNCTION
****************************************************************************/

/***************************************************************************/
//!	@brief function
//!	error | Error_code
/***************************************************************************/
//! @param err_code | (Error_code) caller report the error code experienced by the system
//! @return void |
//! @details
//!	Error code handler function
/***************************************************************************/

void report_error( Error_code err_code )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//counter
	uint8_t t;
	//length of number string
	uint8_t ret_len;
	//numeric code
	uint8_t msg[MAX_DIGIT8+2];

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//error code
	uint8_t u8_err_code = (uint8_t)err_code;

	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//Construct numeric string
	ret_len = u8_to_str( u8_err_code, msg );
	//Error command
	AT_BUF_PUSH( rpi_tx_buf, 'E' );
	AT_BUF_PUSH( rpi_tx_buf, 'R' );
	AT_BUF_PUSH( rpi_tx_buf, 'R' );
	//Send numeric string
	for (t = 0;t < ret_len;t++)
	{
		//Send number
		AT_BUF_PUSH( rpi_tx_buf, msg[t] );
	}
	//Send terminator
	AT_BUF_PUSH( rpi_tx_buf, '\0' );
	
	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------
	
	return;
}	//End Function: error | Error_code

/***************************************************************************/
//!	function
//!	send_msg_ctrl_mode
/***************************************************************************/
//! @return void |
//! @brief Inform that control mode has been switched
//! @details
/***************************************************************************/

void send_msg_ctrl_mode( void )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//Fetch current control mode
	Control_mode ctrl_mode_tmp = g_control_mode;

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	AT_BUF_PUSH( rpi_tx_buf, 'C' );
	AT_BUF_PUSH( rpi_tx_buf, 'T' );
	AT_BUF_PUSH( rpi_tx_buf, 'R' );
	AT_BUF_PUSH( rpi_tx_buf, 'L' );
	AT_BUF_PUSH( rpi_tx_buf, ':' );
	
	switch (ctrl_mode_tmp)
	{
		case CONTROL_STOP:
		{
			AT_BUF_PUSH( rpi_tx_buf, 'O' );
			AT_BUF_PUSH( rpi_tx_buf, 'F' );
			AT_BUF_PUSH( rpi_tx_buf, 'F' );
			break;
		}
		case CONTROL_PWM:
		{
			AT_BUF_PUSH( rpi_tx_buf, 'P' );
			AT_BUF_PUSH( rpi_tx_buf, 'W' );
			AT_BUF_PUSH( rpi_tx_buf, 'M' );
			break;
		}
		case CONTROL_SPD:
		{
			AT_BUF_PUSH( rpi_tx_buf, 'S' );
			AT_BUF_PUSH( rpi_tx_buf, 'P' );
			AT_BUF_PUSH( rpi_tx_buf, 'D' );
			break;
		}
		case CONTROL_POS:
		{
			AT_BUF_PUSH( rpi_tx_buf, 'P' );
			AT_BUF_PUSH( rpi_tx_buf, 'O' );
			AT_BUF_PUSH( rpi_tx_buf, 'S' );
			break;
		}
		default:
		{
			AT_BUF_PUSH( rpi_tx_buf, 'E' );
			AT_BUF_PUSH( rpi_tx_buf, 'R' );
			AT_BUF_PUSH( rpi_tx_buf, 'R' );
			break;
		}
	}

	AT_BUF_PUSH( rpi_tx_buf, '\0' );

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------
	
	return;
}	//End function: send_msg_ctrl_mode | void

/***************************************************************************/
//!	function
//!	send_pwm
/***************************************************************************/
//! @param x |
//! @return void |
//! @brief
//! @details
/***************************************************************************/

void send_data( uint8_t data_len, int16_t *data )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//counter
	uint8_t t, ti;

	int16_t s16_tmp;

	uint8_t str_tmp[ MAX_DIGIT16 +2 ];
	uint8_t str_len;
	
	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------
	
	AT_BUF_PUSH( rpi_tx_buf, 'P' );
	AT_BUF_PUSH( rpi_tx_buf, 'W' );
	AT_BUF_PUSH( rpi_tx_buf, 'M' );
	AT_BUF_PUSH( rpi_tx_buf, '_' );
	AT_BUF_PUSH( rpi_tx_buf, 'D' );
	AT_BUF_PUSH( rpi_tx_buf, 'U' );
	AT_BUF_PUSH( rpi_tx_buf, 'A' );
	AT_BUF_PUSH( rpi_tx_buf, 'L' );
	
	for (t = 0; t < data_len;t++)
	{
		//Convert from PWM to number
		s16_tmp = data[t];
		//Convert to string
		str_len = s16_to_str( s16_tmp, str_tmp );
		
		//Send string
		for (ti = 0;ti < str_len;ti++)
		{
			
			AT_BUF_PUSH( rpi_tx_buf, str_tmp[ti] );
			
		}
		//If not last argument
		if (t != data_len-1)
		{
			//Add argument separator
			AT_BUF_PUSH( rpi_tx_buf, ':' );	
		}
		
	}
	AT_BUF_PUSH( rpi_tx_buf, '\0' );
	
	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------
	
	return;
}	//End function: send_pwm