#include <stdint.h>
#include <stdbool.h>

#include "mgos_timers.h"


typedef struct TimerEvent_s
{
    void ( *Callback )( void* context ); //! Timer IRQ callback function
    void *Context;                       //! User defined data object pointer to pass back (not actually used)
    mgos_timer_id id;   // needed to stop the timer
}TimerEvent_t;

/*!
 * \brief Timer time variable definition
 */
#define TimerTime_t int64_t // int64_t mgos_uptime_micros(void);
#ifndef TimerTime_t
typedef uint32_t TimerTime_t;
#define TIMERTIME_T_MAX                             ( ( uint32_t )~0 )
#endif

/*!
 * \brief Initializes the timer object
 *
 * \remark TimerSetValue function must be called before starting the timer.
 *         this function initializes timestamp and reload value at 0.
 *
 * \param [IN] obj          Structure containing the timer object parameters
 * \param [IN] callback     Function callback called at the end of the timeout
 */
void TimerInit( TimerEvent_t *obj, void ( *callback )( void *context ) );

/*!
 * \brief Starts and adds the timer object to the list of timer events
 *
 * \param [IN] obj Structure containing the timer object parameters
 * \param [IN] value New timer timeout value
 */
void TimerStart( TimerEvent_t *obj, uint32_t value );

/*!
 * \brief Stops and removes the timer object from the list of timer events
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void TimerStop( TimerEvent_t *obj );

/*!
 * \brief Resets the timer object
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void TimerReset( TimerEvent_t *obj );

/*!
 * \brief Read the current time
 *
 * \retval time returns current time
 */
TimerTime_t TimerGetCurrentTime( void );

/*!
 * \brief Return the Time elapsed since a fix moment in Time
 *
 * \remark TimerGetElapsedTime will return 0 for argument 0.
 *
 * \param [IN] past         fix moment in Time
 * \retval time             returns elapsed time
 */
TimerTime_t TimerGetElapsedTime( TimerTime_t past );

/*! 
 * Blocking delay of "ms" milliseconds
 */
void DelayMs( uint32_t ms );


void MemCpy( void * dest, void * source, int length );
