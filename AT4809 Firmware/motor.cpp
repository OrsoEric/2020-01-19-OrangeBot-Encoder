/****************************************************************************
**	INCLUDE
****************************************************************************/

//type definition using the bit width and signedness
#include <stdint.h>
//define the ISR routune, ISR vector, and the sei() cli() function
#include <avr/interrupt.h>
//name all the register and bit
#include <avr/io.h>
//General purpose macros
#include "at_utils.h"
//AT4809 PORT macros definitions
#include "at4809_port.h"
//Program wide definitions
#include "global.h"
//This class handles slope on multiple PWM channels
#include "ctrl_pwm.h"

/****************************************************************************
**	NAMESPACES
****************************************************************************/

/****************************************************************************
**	GLOBAL VARS
****************************************************************************/

//Actual and target control system mode
Control_mode g_control_mode					= CONTROL_STOP;
Control_mode g_control_mode_target			= CONTROL_STOP;
//Slew rate limiter controller for the PWM channels
Orangebot::Ctrl_pwm g_vnh7040_pwm_ctrl;

/****************************************************************************
**	FUNCTION
****************************************************************************/

/***************************************************************************/
//!	@brief function
//!	init_ctrl_system | void
/***************************************************************************/
//! @param x |
//! @return void |
//! @details
/***************************************************************************/

bool init_ctrl_system( void )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//Initialize 
	g_vnh7040_pwm_ctrl = Orangebot::Ctrl_pwm( MAX_VNH7040_PWM, MAX_VNH7040_PWM_SLOPE );

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------
	
	return false; //OK
}	//End function: init_ctrl_system | void

/***************************************************************************/
//!	@brief function
//!	control_system
/***************************************************************************/
//! @return bool | true: failed to execute control system
//! @brief select and execute correct control system
//! @details
/***************************************************************************/

bool control_system( void )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//Counter
	uint8_t t;

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

		//----------------------------------------------------------------
		//	CONTROL MODE
		//----------------------------------------------------------------
		
	//If: switch of control mode
	if (g_control_mode != g_control_mode_target)
	{
		//Control mode is the one desired by the user
		g_control_mode = g_control_mode_target;
		//Signal switch to main
		send_msg_ctrl_mode();
	}
	//otherwise control mode is already correct
	else
	{
		//Do nothing
	}
	
		//----------------------------------------------------------------
		//	EXECUTE CONTROL SYSTEM
		//----------------------------------------------------------------

	//Update position and speed encoder readings
	if (process_enc() == true)
	{
		return true;  //FAIL
	}

	//! Execute a step in the right control mode
	//If: control system is in STOP state
	if (g_control_mode == CONTROL_STOP)
	{
		//Forcefully reset the PWM controller
		g_vnh7040_pwm_ctrl.reset();
		//For: every VNH7040 motor controller
		for (t = 0;t < NUM_VNH7040;t++)
		{
			//Set driver to stopped
			set_vnh7040_pwm( t, 0 );
		} //End for: every VNH7040 motor controller
	}	//End If: control system is in STOP state
	//If: control system is open loop PWM
	else if (g_control_mode == CONTROL_PWM)
	{
		//Execute slew rate limiter
		g_vnh7040_pwm_ctrl.update();
		//For: every VNH7040 motor controller
		for (t = 0;t < NUM_VNH7040;t++)
		{
			//Apply computed PWM settings to the motors
			set_vnh7040_pwm( t, g_vnh7040_pwm_ctrl.pwm(t) );
		} //End for: every VNH7040 motor controller
	}	//End If: control system is open loop PWM
	//If: control system is closed loop speed
	else if (g_control_mode == CONTROL_SPD)
	{
				

	}	//End If: control system is closed loop speed
	//if: undefined control system
	else
	{
		//Signal error
		report_error( ERR_CODE_UNDEFINED_CONTROL_SYSTEM );
		//Reset control mode
		g_control_mode_target = CONTROL_STOP;
	}	//End if: undefined control system

	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------
	
	return false; //OK
}	//End function: control_system | void

/****************************************************************************
//!	@brief function
**  set_vnh7040_pwm
****************************************************************************/
//! @param index	| uint8_t	| index of the motor to be controlled. 0 to 3
//! @param pwm		| int16_t	| Speed of the motor
//! @return bool	| false = OK | true = a wrong VNH7040 index was given
//! @brief Set direction and pwm setting of the VNH7040 controlled motor
//! @details
//!	HAL Hardware Abstraction Layer API for the VNH7040 driver
//! Set direction and pwm setting of the VNH7040 controlled motor
/***************************************************************************/

bool set_vnh7040_pwm( uint8_t index, int16_t pwm )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------

	//temp direction
	uint8_t f_dir;
	//Temp 8bit pwm
	uint8_t vnh7040_pwm;

	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------
	
		///STEP1: Compute direction
	//if: reverse
	if (pwm < 0)
	{
		//reverse
		f_dir = uint8_t(0xff);
		//correct sign
		pwm = -pwm;
	}
	//if: forward
	else
	{
		//forward
		f_dir = uint8_t(0x00);
	}
	
		///STEP2: Apply VNH7040 Hardware PWM limits
	//If: PWM setting is too low
	if (pwm <= MIN_VNH7040_PWM)
	{
		//Set PWM to zero. It would not result to motion anyway
		pwm = 0;
	}
	//If: PWM exceed maximum
	else if (pwm >= MAX_VNH7040_PWM)
	{
		//Clip PWM to maximum
		pwm = MAX_VNH7040_PWM;
	}
	//If: PWM is correct
	else
	{
		//do nothing
	}
	//convert from S16 to U8
	vnh7040_pwm = pwm;
	
		///STEP3: Compute reverse direction according to layout
	//Compute direction. false = forward. true = reverse
	//	dir	
	// 0	0	0 forward
	// 0	1	1 backward
	// 1	0	1 backward
	// 1	1	0 forward
	f_dir = ( ( (f_dir ^ LAYOUT_VNH7040_REVERSE) & MASK(index) ) != 0 );
	
	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//switch: VNH7040 driver index
	switch (index)
	{
		case 0:
		{
			//Set INA, INB and SEL0 to select the direction of rotation of the motor
			SET_MASKED_BIT( PORTA.OUT, 0x30, ((f_dir)?(0x10):(0x20)) );
			//Set the PWM value of the right PWM channel of TCA0
			TCB0.CCMPH = vnh7040_pwm;
			
			break;
		}
		case 1:
		{
			//Set INA, INB and SEL0 to select the direction of rotation of the motor
			SET_MASKED_BIT( PORTA.OUT, 0xc0, ((f_dir)?(0x40):(0x80)) );
			//Set the PWM value of the right PWM channel of TCA0
			TCB1.CCMPH = vnh7040_pwm;
			
			break;
		}
		case 2:
		{
			//Set INA, INB and SEL0 to select the direction of rotation of the motor
			SET_MASKED_BIT( PORTB.OUT, 0x0c, ((f_dir)?(0x04):(0x08)) );
			//Set the PWM value of the right PWM channel of TCA0
			TCB2.CCMPH = vnh7040_pwm;
			
			break;
		}
		case 3:
		{
			//Set INA, INB and SEL0 to select the direction of rotation of the motor
			SET_MASKED_BIT( PORTD.OUT, 0xc0, ((f_dir)?(0x40):(0x80)) );
			//Set the PWM value of the right PWM channel of TCA0
			TCB3.CCMPH = vnh7040_pwm;
			
			break;
		}
		//Driver index does not exist
		default:
		{
			//a wrong VNH7040 index was given
			return true;
			
			break;
		}
	} //end switch: VNH7040 driver index
	
	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------
	
	return false;
}	//End: set_vnh7040_pwm

/***************************************************************************/
//!	@brief function
//!	set_platform_pwm | int16_t | int16_t
/***************************************************************************/
//! @param right | int16_t | pwm to apply to the right side wheel(s)
//! @param left | int16_t | pwm to apply to the left side wheel(s)
//! @return bool | false = OK | true = a wrong VNH7040 index was given
//! @brief Control PWM of the platform to move according to the layout
//! @details
//!	HAL Hardware Abstraction Layer API for the VNH7040 driver
//! Control PWM of the platform to move according to the layout
/***************************************************************************/

bool set_platform_pwm( int16_t right, int16_t left )
{
	//----------------------------------------------------------------
	//	VARS
	//----------------------------------------------------------------
	
	//int16_t data[2];
	
	//----------------------------------------------------------------
	//	INIT
	//----------------------------------------------------------------

	//----------------------------------------------------------------
	//	BODY
	//----------------------------------------------------------------

	//The PWM references are set as target for the PWM slope controller
	g_vnh7040_pwm_ctrl.target( 0 ) = right;
	g_vnh7040_pwm_ctrl.target( 1 ) = left;
	//Debug: send actual PWM back as response to PWM message
	//data[0] = g_vnh7040_pwm_ctrl.pwm( 0 );
	//data[1] = g_vnh7040_pwm_ctrl.pwm( 1 );
	//Debug send message
	//send_data( 2, data );
	
	//----------------------------------------------------------------
	//	RETURN
	//----------------------------------------------------------------
	
	return false; //OK
}	//End function:
