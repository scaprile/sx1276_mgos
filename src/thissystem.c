#include <string.h>

#include "thissystem.h"

#include "mgos.h"
#include "mgos_system.h"
#include "mgos_time.h"
#include "mgos_timers.h"


void TimerInit( TimerEvent_t *obj, void ( *callback )( void *context ) )
{
    obj->id = MGOS_INVALID_TIMER_ID;    // mOS won't assign an id until we start the timer
    obj->Callback = callback;
    obj->Context = NULL;                // function argument is actually not used by the function itself
}

// a mOS timer callback callable function that redirects to driver callbacks
static void tmr_cb_trampoline(void *userdata)
{
TimerEvent_t *obj = (TimerEvent_t *)userdata;

    obj->id = MGOS_INVALID_TIMER_ID;    // clear expired timer id
    (*obj->Callback)(obj->Context);     // and jump to driver function
}

/** \brief one shot timer
\warn mOS might actually return MGOS_INVALID_TIMER_ID for an id in case of low memory conditions.
In such a case the timer will never start and consequently never expire, so the system might misbehave erratically
*/
void TimerStart( TimerEvent_t *obj, uint32_t value )
{
    obj->id = mgos_set_timer(value, false, tmr_cb_trampoline, obj); // this allocates timer memory and id
}

void TimerStop( TimerEvent_t *obj )
{
mgos_timer_id id = obj->id;

    if(id == MGOS_INVALID_TIMER_ID)     // verify we actually have started a timer (dynamic ids)
        return;
    obj->id = MGOS_INVALID_TIMER_ID;    // and clear already used ids
    mgos_clear_timer(id);               // this frees timer memory and invalidates id
}


TimerTime_t TimerGetCurrentTime( void )
{
    return mgos_uptime_micros();
}

TimerTime_t TimerGetElapsedTime( TimerTime_t past )
{
    return mgos_uptime_micros() - past;
}

void DelayMs( uint32_t ms )
{
    mgos_msleep(ms);
}


void MemCpy( void * dest, void * source, int length )
{
    memcpy(dest, source, length);
}
