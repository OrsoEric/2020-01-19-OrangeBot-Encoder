
/****************************************************************
**	INCLUDES
****************************************************************/

//type definition using the bit width and signedness
#include <stdint.h>

//General purpose macros
#include "at_utils.h"

#include "at_string.h"

#include "global.h"

//Universal Parser V4
#include "uniparser.h"

/****************************************************************
** GLOBAL VARIABLES
****************************************************************/

	///--------------------------------------------------------------------------
	///	PARSER
	///--------------------------------------------------------------------------

//Board Signature
uint8_t *g_board_sign = (uint8_t *)"OrangeBot-2020-01-19";
//communication timeout counter
uint8_t g_uart_timeout_cnt = 0;
//Communication timeout has been detected
bool g_f_timeout_detected = false;

/***************************************************************************/
//!	function
//!	init_parser_commands | Orangebot::Uniparser &
/***************************************************************************/
//! @param parser_tmp | Orangebot::Uniparser | Parser to which commands are to be atached
//! @return bool | false = OK | true = command was invalid and was not registered
//! @brief
//! @details
/***************************************************************************/

bool init_parser_commands( Orangebot::Uniparser &parser_tmp )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//Return flag
	bool f_ret = false;

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//! Register commands and handler for the universal parser class. A masterpiece :')
	//Register ping command. It's used to reset the communication timeout
	f_ret = parser_tmp.add_cmd( "P", (void *)&ping_handler );
	//Register the Find command. Board answers with board signature
	f_ret |= parser_tmp.add_cmd( "F", (void *)&send_signature_handler );
	//Direct platform PWM command
	f_ret |= parser_tmp.add_cmd( "PWMR%SL%S", (void *)&set_platform_pwm_handler );
	//Master asks for absolute encoder position
	f_ret |= parser_tmp.add_cmd( "ENC_ABS%u", (void *)&send_enc_pos_handler );
	//Master asks for encoder speed
	f_ret |= parser_tmp.add_cmd( "ENC_SPD", (void *)&send_enc_spd_handler );
	
	//If: Uniparser V4 failed to register a command
	if (f_ret == true)
	{
		//Signal a likely syntax error
		report_error( ERR_BAD_PARSER_DICTIONARY );
	}

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------
	
	return f_ret;
}	//End function: init_parser_commands | Orangebot::Uniparser &

/***************************************************************************/
//!	@brief ping command handler
//!	ping_handler | void
/***************************************************************************/
//! @return void
//!	@details
//! Handler for the ping command. Keep alive connection
/***************************************************************************/

void ping_handler( void )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------
	
	//Reset communication timeout handler
	g_uart_timeout_cnt = 0;
	
	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------

	return; //OK
}	//end handler: ping_handler | void

/***************************************************************************/
//!	@brief board signature handler
//!	signature_handler | void
/***************************************************************************/
//! @return void
//!	@details
//! Handler for the get board signature command. Send board signature via UART
/***************************************************************************/

void send_signature_handler( void )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//Counter
	uint8_t t;
	//Temp return
	uint8_t ret;
	
	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//Reset communication timeout handler
	g_uart_timeout_cnt = 0;
	//Length of the board signature string
	uint8_t board_sign_length = str_len( g_board_sign, MAX_SIGNATURE_LENGTH );
	//If: string is invalid
	if (board_sign_length > MAX_SIGNATURE_LENGTH)
	{
		report_error( Error_code::ERR_BAD_BOARD_SIGN );
		return; //FAIL
	}
	//Construct string length
	uint8_t str[MAX_STRING8];
	//Convert the string length to number
	ret = u8_to_str( board_sign_length, str );
	
	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//Send a signature message with string argument to the RPI 3B+
	AT_BUF_PUSH( rpi_tx_buf, 'F' );
	//For: number of number of signature characters
	for (t = 0;t < ret;t++)
	{
		//Push the number of characters	
		AT_BUF_PUSH( rpi_tx_buf, str[t] );
	}
	//Send message terminator
	AT_BUF_PUSH( rpi_tx_buf, (uint8_t)0x00 );
	//For: number of signature characters
	for (t = 0;t < board_sign_length;t++)
	{
		//Send the next signature byte
		AT_BUF_PUSH(rpi_tx_buf, g_board_sign[t]);
	}
	//Send string terminator
	AT_BUF_PUSH( rpi_tx_buf, (uint8_t)0x00 );

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------

	return; //OK
}	//end handler: signature_handler | void

/***************************************************************************/
//!	function
//!	set_platform_pwm_handler | int16_t, int16_t
/***************************************************************************/
//! @param x |
//! @return void |
//! @brief
//! @details
/***************************************************************************/

void set_platform_pwm_handler( int16_t right, int16_t left )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//Reset communication timeout handler
	g_uart_timeout_cnt = 0;

	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//Select PWM controls
	g_control_mode_target = CONTROL_PWM;
	//HAL API to control VNH7040 motors according to the platform layout
	set_platform_pwm( right, left );

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------
	
	return;
}	//End function: set_platform_pwm_handler | int16_t, int16_t

/***************************************************************************/
//!	@brief board signature handler
//!	send_enc_pos_handler | uint8_t
/***************************************************************************/
//! @param index | index of the encoder position to be sent
//! @return void
//!	@details
//! Handle request for absolute encoder position
/***************************************************************************/

void send_enc_pos_handler( uint8_t index )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//Counter
	uint8_t t;
	//return
	uint8_t ret;

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//If: invalid encoder channel
	if (index >= NUM_ENC)
	{
		report_error(Error_code::ERR_BAD_PARSER_RUNTIME_ARGUMENT);
		return;	//FAIL
	}
	//Reset communication timeout handler
	g_uart_timeout_cnt = 0;
	
	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//Temp string sized for an int32_t
	uint8_t str[MAX_STRING32];
	//Construct index string
	ret = u8_to_str( index, str );
	//Send a encoder absolute position message
	AT_BUF_PUSH( rpi_tx_buf, 'E' );
	AT_BUF_PUSH( rpi_tx_buf, 'N' );
	AT_BUF_PUSH( rpi_tx_buf, 'C' );
	AT_BUF_PUSH( rpi_tx_buf, '_' );
	AT_BUF_PUSH( rpi_tx_buf, 'A' );
	AT_BUF_PUSH( rpi_tx_buf, 'B' );
	AT_BUF_PUSH( rpi_tx_buf, 'S' );
	//For each string character
	for (t = 0;t < ret;t++)
	{
		//Push the number of characters
		AT_BUF_PUSH( rpi_tx_buf, str[t] );
	}
	//Send argument separator
	AT_BUF_PUSH( rpi_tx_buf, ':' );
	//Construct encoder position string
	ret = s32_to_str( g_enc_pos[ index ], str );
	//For each string character
	for (t = 0;t < ret;t++)
	{
		//Push the number of characters
		AT_BUF_PUSH( rpi_tx_buf, str[t] );
	}
	//Send terminator
	AT_BUF_PUSH( rpi_tx_buf, '\0' );

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------

	return; //OK
}	//end handler: send_enc_pos_handler | uint8_t

/***************************************************************************/
//!	@brief board signature handler
//!	send_enc_spd_handler | void
/***************************************************************************/
//! @param index | index of the encoder position to be sent
//! @return void
//!	@details
//! Handle request for all encoder speed
/***************************************************************************/

void send_enc_spd_handler( void )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//Counter
	uint8_t t, ti;
	//return
	uint8_t ret;
	//Temp string sized for an int16_t
	uint8_t str[MAX_STRING16];

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//Reset communication timeout handler
	g_uart_timeout_cnt = 0;
	
	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//Send a encoder absolute position message
	AT_BUF_PUSH( rpi_tx_buf, 'E' );
	AT_BUF_PUSH( rpi_tx_buf, 'N' );
	AT_BUF_PUSH( rpi_tx_buf, 'C' );
	AT_BUF_PUSH( rpi_tx_buf, '_' );
	AT_BUF_PUSH( rpi_tx_buf, 'S' );
	AT_BUF_PUSH( rpi_tx_buf, 'P' );
	AT_BUF_PUSH( rpi_tx_buf, 'D' );
	AT_BUF_PUSH( rpi_tx_buf, '_' );
	AT_BUF_PUSH( rpi_tx_buf, 'D' );
	AT_BUF_PUSH( rpi_tx_buf, 'U' );
	AT_BUF_PUSH( rpi_tx_buf, 'A' );
	AT_BUF_PUSH( rpi_tx_buf, 'L' );
	
	//For: each encoder channel
	for (t = 0;t < NUM_ENC;t++)
	{
		//Construct encoder position string
		ret = s16_to_str( g_enc_spd[ t ], str );
		//For each string character
		for (ti = 0;ti < ret;ti++)
		{
			//Push the number of characters
			AT_BUF_PUSH( rpi_tx_buf, str[ti] );
		}
		//If not last argument
		if (t < NUM_ENC -1)
		{
			//Send argument separator
			AT_BUF_PUSH( rpi_tx_buf, ':' );	
		}
		
	}
	//Send terminator
	AT_BUF_PUSH( rpi_tx_buf, '\0' );

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------

	return; //OK
}	//end handler: signature_handler | void