/*!
 * \file      rtc-board.c
 *
 * \brief     Target board RTC timer and low power modes management
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech - STMicroelectronics
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    MCD Application Team (C)( STMicroelectronics International )
 */
#include <math.h>
#include <time.h>
// #include "stm32l1xx.h"
#include "utilities.h"
// #include "delay.h"
#include "board.h"
#include "timer.h"
#include "systime.h"
// #include "gpio.h"
// #include "lpm-board.h"
#include "rtc-board.h"

#include "stm32l1xx_hal_rtc.h"

#include <time.h>
#include <sys/time.h>


#include "esp_timer.h"

// MCU Wake Up Time
#define MIN_ALARM_DELAY                             1 // in ticks

// sub-second number of bits
#define N_PREDIV_S                                  10

// Synchronous prediv
#define PREDIV_S                                    ( ( 1 << N_PREDIV_S ) - 1 )

// Asynchronous prediv
#define PREDIV_A                                    ( 1 << ( 15 - N_PREDIV_S ) ) - 1

// Sub-second mask definition
#define ALARM_SUBSECOND_MASK                        ( N_PREDIV_S << RTC_ALRMASSR_MASKSS_Pos )

// RTC Time base in us
#define USEC_NUMBER                                 1000000
#define MSEC_NUMBER                                 ( USEC_NUMBER / 1000 )

#define COMMON_FACTOR                               3
#define CONV_NUMER                                  ( MSEC_NUMBER >> COMMON_FACTOR )
#define CONV_DENOM                                  ( 1 << ( N_PREDIV_S - COMMON_FACTOR ) )

/*!
 * \brief Days, Hours, Minutes and seconds
 */
#define DAYS_IN_LEAP_YEAR                           ( ( uint32_t )  366U )
#define DAYS_IN_YEAR                                ( ( uint32_t )  365U )
#define SECONDS_IN_1DAY                             ( ( uint32_t )86400U )
#define SECONDS_IN_1HOUR                            ( ( uint32_t ) 3600U )
#define SECONDS_IN_1MINUTE                          ( ( uint32_t )   60U )
#define MINUTES_IN_1HOUR                            ( ( uint32_t )   60U )
#define HOURS_IN_1DAY                               ( ( uint32_t )   24U )

/*!
 * \brief Correction factors
 */
#define  DAYS_IN_MONTH_CORRECTION_NORM              ( ( uint32_t )0x99AAA0 )
#define  DAYS_IN_MONTH_CORRECTION_LEAP              ( ( uint32_t )0x445550 )

/*!
 * \brief Calculates ceiling( X / N )
 */
#define DIVC( X, N )                                ( ( ( X ) + ( N ) -1 ) / ( N ) )

/*!
 * RTC timer context 
 */
typedef struct
{
    uint32_t        Time;         // Reference time
    RTC_TimeTypeDef CalendarTime; // Reference time in calendar format
    RTC_DateTypeDef CalendarDate; // Reference date in calendar format
}RtcTimerContext_t;

/*!
 * \brief Indicates if the RTC is already Initialized or not
 */
static bool RtcInitialized = false;

/*!
 * \brief Indicates if the RTC Wake Up Time is calibrated or not
 */
static bool McuWakeUpTimeInitialized = false;

/*!
 * \brief Compensates MCU wakeup time
 */
static int16_t McuWakeUpTimeCal = 0;

/*!
 * Keep the value of the RTC timer when the RTC alarm is set
 * Set with the \ref RtcSetTimerContext function
 * Value is kept as a Reference to calculate alarm
 */
static RtcTimerContext_t RtcTimerContext;


static uint64_t RtcGetCalendarValue( RTC_DateTypeDef* date, RTC_TimeTypeDef* time )
{
    uint64_t calendarValue = 0;
    uint32_t firstRead;
    uint32_t correction;
    uint32_t seconds;

    struct timeval ts_now;
    gettimeofday(&ts_now, NULL);
    calendarValue = (ts_now.tv_sec)*1000 + ((ts_now.tv_usec)/1000);

   

    return( calendarValue );
}

esp_timer_handle_t periodic_timer;
esp_timer_handle_t oneshot_timer;
static void periodic_timer_callback(void* arg)
{

}

static void oneshot_timer_callback(void* arg)
{
   RTC_Alarm_IRQHandler();
}

static void _rtos_time_init(){
    /* Create two timers:
     * 1. a periodic timer which will run every 0.5s, and print a message
     * 2. a one-shot timer which will fire after 5s, and re-start periodic
     *    timer with period of 1s.
     */

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };


    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    /* The timer has been created but is not running yet */

    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = &oneshot_timer_callback,
            /* argument specified here will be passed to timer callback function */
            .arg = (void*) periodic_timer,
            .name = "one-shot"
    };

    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));

}

void RtcInit( void )
{
    _rtos_time_init();
}

/*!
 * \brief Sets the RTC timer reference, sets also the RTC_DateStruct and RTC_TimeStruct
 *
 * \param none
 * \retval timerValue In ticks
 */
uint32_t RtcSetTimerContext( void )
{
    RtcTimerContext.Time = ( uint32_t )RtcGetCalendarValue( &RtcTimerContext.CalendarDate, &RtcTimerContext.CalendarTime );
    return ( uint32_t )RtcTimerContext.Time;

}

/*!
 * \brief Gets the RTC timer reference
 *
 * \param none
 * \retval timerValue In ticks
 */
uint32_t RtcGetTimerContext( void )
{
    return RtcTimerContext.Time;
}

/*!
 * \brief returns the wake up time in ticks
 *
 * \retval wake up time in ticks
 */
uint32_t RtcGetMinimumTimeout( void )
{
    return( MIN_ALARM_DELAY );
}

/*!
 * \brief converts time in ms to time in ticks
 *
 * \param[IN] milliseconds Time in milliseconds
 * \retval returns time in timer ticks
 */
uint32_t RtcMs2Tick( uint32_t milliseconds )
{
#if 0    
    return ( uint32_t )( ( ( ( uint64_t )milliseconds ) * CONV_DENOM ) / CONV_NUMER );
#endif
    return     milliseconds;
}

/*!
 * \brief converts time in ticks to time in ms
 *
 * \param[IN] time in timer ticks
 * \retval returns time in milliseconds
 */
uint32_t RtcTick2Ms( uint32_t tick )
{
#if 0    
    uint32_t seconds = tick >> N_PREDIV_S;

    tick = tick & PREDIV_S;
    return ( ( seconds * 1000 ) + ( ( tick * 1000 ) >> N_PREDIV_S ) );
#endif
    return tick ;   
}

/*!
 * \brief a delay of delay ms by polling RTC
 *
 * \param[IN] delay in ms
 */
void RtcDelayMs( uint32_t delay )
{
    uint64_t delayTicks = 0;
    uint64_t refTicks = RtcGetTimerValue( );
    
    delayTicks = RtcMs2Tick( delay );

    // Wait delay ms
    while( ( ( RtcGetTimerValue( ) - refTicks ) ) < delayTicks )
    {
       
    }
}

/*!
 * \brief Sets the alarm
 *
 * \note The alarm is set at now (read in this function) + timeout
 *
 * \param timeout Duration of the Timer ticks
 */
void RtcSetAlarm( uint32_t timeout )
{
    RtcStartAlarm( timeout );  
}

void RtcStopAlarm( void )
{
    esp_timer_stop(oneshot_timer);
}

void RtcStartAlarm( uint32_t timeout )
{
    printf("xxxxRtcStartAlarm[%ld]\n",timeout*1000*1000 );
    esp_timer_start_once(oneshot_timer, timeout*1000*1000);
}

uint32_t RtcGetTimerValue( void )
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    uint32_t calendarValue = ( uint32_t )RtcGetCalendarValue( &date, &time );

    return( calendarValue );
}

uint32_t RtcGetTimerElapsedTime( void )
{
  RTC_TimeTypeDef time;
  RTC_DateTypeDef date;
  
  uint32_t calendarValue = ( uint32_t )RtcGetCalendarValue( &date, &time );

  return( ( uint32_t )( calendarValue - RtcTimerContext.Time ) );
}

void RtcSetMcuWakeUpTime( void )
{
 
}

int16_t RtcGetMcuWakeUpTime( void )
{
    return 0;
}



uint32_t RtcGetCalendarTime( uint16_t *milliseconds )
{
    RTC_TimeTypeDef time ;
    RTC_DateTypeDef date;
    uint32_t ticks;

    uint64_t calendarValue = RtcGetCalendarValue( &date, &time );



    ticks =  ( uint32_t )calendarValue & PREDIV_S;

    *milliseconds = RtcTick2Ms( ticks );

    return calendarValue;

}

/*!
 * \brief RTC IRQ Handler of the RTC Alarm
 */
void RTC_Alarm_IRQHandler( void )
{
      TimerIrqHandler( );
}



void RtcBkupWrite( uint32_t data0, uint32_t data1 )
{

}

void RtcBkupRead( uint32_t *data0, uint32_t *data1 )
{

}

void RtcProcess( void )
{

}

TimerTime_t RtcTempCompensation( TimerTime_t period, float temperature )
{
    float k = RTC_TEMP_COEFFICIENT;
    float kDev = RTC_TEMP_DEV_COEFFICIENT;
    float t = RTC_TEMP_TURNOVER;
    float tDev = RTC_TEMP_DEV_TURNOVER;
    float interim = 0.0;
    float ppm = 0.0;

    if( k < 0.0 )
    {
        ppm = ( k - kDev );
    }
    else
    {
        ppm = ( k + kDev );
    }
    interim = ( temperature - ( t - tDev ) );
    ppm *=  interim * interim;

    // Calculate the drift in time
    interim = ( ( float ) period * ppm ) / 1e6;
    // Calculate the resulting time period
    interim += period;
    interim = floor( interim );

    if( interim < 0.0 )
    {
        interim = ( float )period;
    }

    // Calculate the resulting period
    return ( TimerTime_t ) interim;
}
