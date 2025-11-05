#ifndef __TTASK_H__
#define __TTASK_H__

/** System Timing Definitions *************************************************************************************** */
// Configure task counter length 16/32 bits
#define COUNTER_BITS (32)

/** Maximum Tick Value */
#if COUNTER_BITS == 16
#define TICK_MAX 0xFFFF
#elif COUNTER_BITS == 32
#define TICK_MAX 0xFFFFFFFF
#else
#error "Invalid value for COUNTER_BITS"
#endif

/** Milliseconds per tick, modify according to actual value */
#define TICK_RATE_MS 10

/** Define system tick count variable */
#define DefSystemTickCount() unsigned int systemTickCount = 0;
extern unsigned int systemTickCount;

/** Get system tick count */
#define GetSystemTickCount() (systemTickCount)

/** Increment system tick count */
#define IncSystemTickCount() systemTickCount += 1;

/** Calculate time span between start and end considering counter overflow */
#define GetSystemTickSpan(_start, _end) (((_end) >= (_start)) ? ((_end) - (_start)) : (TICK_MAX - (_start) + (_end)))

/** Task Control Block */
typedef struct
{
#if COUNTER_BITS == 16
    unsigned short tick; // 16-bit length counter
#elif COUNTER_BITS == 32
    unsigned int tick; // 32-bit length counter
#else
#error "Invalid value for tcb.tick"
#endif
    unsigned char ctrl; // Task control
    unsigned char line; // Task yield position
} task_control_block_t;

/** Task Return Codes */
#define _TASK_RET_YIELD 1 // Yield return
#define _TASK_RET_EXIT  0 // Exit return

/** Task Control Bits */
#define _TASK_CTRL_SUSPEND (1 << 0) // Task suspend control bit
#define _TASK_CTRL_WAIT    (1 << 1) // Task wait control bit
#define _TASK_CTRL_NOTIFY  (1 << 2) // Task notify control bit
#define _TASK_CTRL_STOP    (1 << 3) // Task has been stopped externally
#define _TASK_CTRL_RUNNING (1 << 4) // Task is marked as running

/** Task Function Definitions and Declarations */

/** Define Task */
#define TaskDefine(task)                                            \
    task_control_block_t tcb_##task = {.ctrl = _TASK_CTRL_RUNNING}; \
    unsigned char        task(task_control_block_t *tcb)

/** Declare Task */
#define TaskDeclare(task)                   \
    extern task_control_block_t tcb_##task; \
    extern unsigned char        task(task_control_block_t *tcb);

/** Task Start */
#define TTS            \
    switch (tcb->line) \
    {                  \
    default:

/** Task End */
#define TTE                           \
    }                                 \
    tcb->line = 0;                    \
    tcb->tick = TICK_MAX;             \
    tcb->ctrl &= ~_TASK_CTRL_RUNNING; \
    tcb->ctrl |= _TASK_CTRL_STOP;     \
    return _TASK_RET_EXIT;

/** Task Control */

/** Stop Task Completely */
#define TaskStop(task)                          \
    do                                          \
    {                                           \
        tcb_##task.ctrl |= _TASK_CTRL_STOP;     \
        tcb_##task.ctrl &= ~_TASK_CTRL_RUNNING; \
        tcb_##task.tick = 0;                    \
        tcb_##task.line = 0;                    \
    } while (0);

/** Start (or Restart) Task */
#define TaskStart(task)                        \
    do                                         \
    {                                          \
        tcb_##task.ctrl &= ~_TASK_CTRL_STOP;   \
        tcb_##task.ctrl |= _TASK_CTRL_RUNNING; \
        tcb_##task.tick = 0;                   \
        tcb_##task.line = 0;                   \
    } while (0);

/** Check if Task is Running */
#define TaskIsRunning(task) \
    (!(tcb_##task.ctrl & _TASK_CTRL_STOP) && (tcb_##task.ctrl & _TASK_CTRL_RUNNING))

/** Run Task in Main Loop */
#define TaskRun(task)                                                    \
    do                                                                   \
    {                                                                    \
        if (!(tcb_##task.ctrl & _TASK_CTRL_SUSPEND) &&                   \
            !(tcb_##task.ctrl & _TASK_CTRL_STOP) &&                      \
            (tcb_##task.ctrl & _TASK_CTRL_RUNNING) &&                    \
            (tcb_##task.tick == 0 || tcb_##task.ctrl & _TASK_CTRL_WAIT)) \
        {                                                                \
            task(&tcb_##task);                                           \
        }                                                                \
    } while (0);

/** Tick Task in Timer Interrupt */
#define TaskTick(task)                                         \
    do                                                         \
    {                                                          \
        if (!(tcb_##task.ctrl & _TASK_CTRL_SUSPEND) &&         \
            tcb_##task.tick > 0 && tcb_##task.tick < TICK_MAX) \
        {                                                      \
            tcb_##task.tick -= 1;                              \
        }                                                      \
    } while (0);

/** Reset Task */
#define TaskReset(task)      \
    do                       \
    {                        \
        tcb_##task.tick = 0; \
        tcb_##task.line = 0; \
    } while (0);

/** Reset Current Task and Yield */
#define TaskResetThis()         \
    do                          \
    {                           \
        tcb->tick = 0;          \
        tcb->line = 0;          \
        return _TASK_RET_YIELD; \
    } while (0);

/** Delay Task for Ticks */
#define TaskDelay(ticks)                   \
    do                                     \
    {                                      \
        tcb->tick = (ticks);               \
        tcb->line = (__LINE__ % 0xFF) + 1; \
        return _TASK_RET_YIELD;            \
    case (__LINE__ % 0xFF) + 1:;           \
    } while (0);

/** Abort Task Delay */
#define TaskAbortDelay(task) \
    do                       \
    {                        \
        tcb_##task.tick = 0; \
    } while (0);

/** Wait for Condition or Timeout */
#define TaskWait(condition, timeout)              \
    do                                            \
    {                                             \
        tcb->line = (__LINE__ % 0xFF) + 1;        \
        tcb->tick = (timeout);                    \
        tcb->ctrl |= _TASK_CTRL_WAIT;             \
        do                                        \
        {                                         \
            return _TASK_RET_YIELD;               \
        case (__LINE__ % 0xFF) + 1:;              \
        } while (tcb->tick != 0 && !(condition)); \
        tcb->ctrl &= ~_TASK_CTRL_WAIT;            \
    } while (0);

/** Wait for Another Task to Complete or Timeout */
#define TaskWaitSync(task, timeout)                                             \
    do                                                                          \
    {                                                                           \
        TaskReset(task);                                                        \
        tcb->line = (__LINE__ % 0xFF) + 1;                                      \
        tcb->tick = (timeout);                                                  \
        tcb->ctrl |= _TASK_CTRL_WAIT;                                           \
        do                                                                      \
        {                                                                       \
            return _TASK_RET_YIELD;                                             \
        case (__LINE__ % 0xFF) + 1:;                                            \
            if (tcb->tick == 0) break;                                          \
            if ((tcb_##task.ctrl & _TASK_CTRL_SUSPEND) ||                       \
                (tcb_##task.tick != 0 && !(tcb_##task.ctrl & _TASK_CTRL_WAIT))) \
            {                                                                   \
                return _TASK_RET_YIELD;                                         \
            }                                                                   \
        } while (_TASK_RET_EXIT != task(&tcb_##task));                          \
        tcb->ctrl &= ~_TASK_CTRL_WAIT;                                          \
    } while (0);

/** Wait for Notification or Timeout */
#define TaskWaitNotify(timeout)                            \
    do                                                     \
    {                                                      \
        TaskWait(tcb->ctrl &_TASK_CTRL_NOTIFY, (timeout)); \
        tcb->ctrl &= ~_TASK_CTRL_NOTIFY;                   \
    } while (0);

/** Send Notification to Task */
#define TaskSendNotify(task)                  \
    do                                        \
    {                                         \
        tcb_##task.ctrl |= _TASK_CTRL_NOTIFY; \
    } while (0);

/** Clear Task Notification */
#define TaskClearNotify(task)                  \
    do                                         \
    {                                          \
        tcb_##task.ctrl &= ~_TASK_CTRL_NOTIFY; \
    } while (0);

/** Check if Task Timeout Expired */
#define TaskTimeoutExpired() (tcb->tick == 0)

/** Suspend Task */
#define TaskSuspend(task)                      \
    do                                         \
    {                                          \
        tcb_##task.ctrl |= _TASK_CTRL_SUSPEND; \
    } while (0);

/** Suspend Current Task and Yield */
#define TaskSuspendThis()                \
    do                                   \
    {                                    \
        tcb->ctrl |= _TASK_CTRL_SUSPEND; \
        return _TASK_RET_YIELD;          \
    } while (0);

/** Resume Task */
#define TaskResume(task)                        \
    do                                          \
    {                                           \
        tcb_##task.ctrl &= ~_TASK_CTRL_SUSPEND; \
    } while (0);

/** Exit Task */
#define TaskExit()             \
    do                         \
    {                          \
        tcb->tick = TICK_MAX;  \
        tcb->line = 0;         \
        return _TASK_RET_EXIT; \
    } while (0);

/** Event Group Definitions */

// Event group is a 32-bit variable
typedef uint64_t EventGroup_t;

/** Initialize Event Group */
#define EventGroupInit(eventGroup) (eventGroup) = 0;

/** Check if Event Group has Events */
#define EventGroupHasEvent(eventGroup) (eventGroup != 0)

/** Set Event Group Bits */
#define EventGroupSetBits(eventGroup, bitsToSet) (eventGroup) |= (bitsToSet);

/** Get Event Group Bits */
#define EventGroupGetBits(eventGroup, bitsToGet) ((eventGroup) & (bitsToGet))

/** Clear Event Group Bits */
#define EventGroupClearBits(eventGroup, bitsToClear) (eventGroup) &= ~(bitsToClear);

/** Clear All Event Group Bits */
#define EventGroupClearAllBits(eventGroup) eventGroup = 0;

/** Check if Event Group has Specific Bits */
#define EventGroupCheckBits(eventGroup, bitsToCheck) \
    (((eventGroup) & (bitsToCheck)) != 0)

/** Check if Event Group has All Specific Bits */
#define EventGroupCheckAllBits(eventGroup, bitsToCheck) \
    (((eventGroup) & (bitsToCheck)) == (bitsToCheck))

/** Wait for Specific Bits in Event Group or Timeout */
#define EventGroupWaitBits(eventGroup, bitsToWaitFor, timeout) \
    TaskWait(EventGroupCheckBits((eventGroup), (bitsToWaitFor)), (timeout))

/** Wait for All Specific Bits in Event Group or Timeout */
#define EventGroupWaitAllBits(eventGroup, bitsToWaitFor, timeout) \
    TaskWait(EventGroupCheckAllBits((eventGroup), (bitsToWaitFor)), (timeout))

/** Event Group and Task Declarations */

extern EventGroup_t event_group_system;

#define EVT_BTN_PRESS             (1ULL << 0)
#define EVT_BTN_PRESS_2T          (1ULL << 1)
#define EVT_BTN_PRESS_2S          (1ULL << 2)
#define EVT_UI_UP_CO2             (1ULL << 3)
#define EVT_NUS_TX_RDY            (1ULL << 4)
#define EVT_CO2_CALIB_TIMING      (1ULL << 5)
#define EVT_CO2_UP_ALARM          (1ULL << 6)
#define EVT_BAT_UPDATE            (1ULL << 7)
#define EVT_CO2_UPDATE_ONCE       (1ULL << 8)
#define EVT_CO2_UP_HIS            (1ULL << 9)
#define EVT_TIMEBASE_UP           (1ULL << 10)
#define EVT_REQUEST_HISTORY       (1ULL << 11)
#define EVT_NUS_TAKEN             (1ULL << 12)
#define EVT_SHOW_PASSKEY          (1ULL << 13)
#define EVT_CHANCEL_SHOW_PASSKEY  (1ULL << 14)
#define EVT_SCREEN_FORCE_ON       (1ULL << 15)
#define EVT_CHARGING              (1ULL << 16)
#define EVT_TIME_UPDATE           (1ULL << 17)
#define EVT_SCREEN_ON_ONETIME     (1ULL << 18)
#define EVT_FIND_DEVICE           (1ULL << 19)
#define EVT_CO2_CALIB_MODE_CHANGE (1ULL << 20)
#define EVT_POPULATE_FAKE_DATA    (1ULL << 21)
#define EVT_FAST_CONN             (1ULL << 22)
#define EVT_UI_UP_BLE             (1ULL << 23)
#define EVT_CO2_CALIB_START       (1ULL << 24)
#define EVT_UI_BLINK              (1ULL << 25)
#define EVT_CO2_CALIB_DONE        (1ULL << 26)
#define EVT_BAT_LOW               (1ULL << 27)
#define EVT_BTN_DOWN              (1ULL << 28)
#define EVT_UI_OFF_SCREEN         (1ULL << 29)
#define EVT_BATTER_ADC_EN         (1ULL << 30)
#define EVT_CO2_SENSOR_ERROR      (1ULL << 31)
#define EVT_DEVICE_SLEEP          (1ULL << 32)
#define EVT_CO2_FACTORY_RESET     (1ULL << 33)
#define EVT_STOP_ADVERTISING      (1ULL << 34)
#define EVT_TOGGLE_UI_MODE        (1ULL << 35)
#define EVT_BAT_LOW_WARNING       (1ULL << 36)
#define EVT_UI_GRAPH_UPDATE       (1ULL << 37)
#define EVT_GET_SENSOR_DETAILS    (1ULL << 38)
#define EVT_FLIGHT_MODE_UPDATE    (1ULL << 39)

#define EVT_UI_UPDATE             (EVT_UI_BLINK | EVT_UI_UP_CO2 | EVT_BAT_UPDATE | EVT_TIME_UPDATE | EVT_UI_UP_BLE | EVT_UI_OFF_SCREEN | EVT_CO2_CALIB_MODE_CHANGE | EVT_TOGGLE_UI_MODE | EVT_BAT_LOW | EVT_BAT_LOW_WARNING | EVT_UI_GRAPH_UPDATE | EVT_FLIGHT_MODE_UPDATE)
#define EVT_CO2_UPDATE            (EVT_CO2_UPDATE_ONCE | EVT_CO2_CALIB_DONE | EVT_CO2_CALIB_START | EVT_BAT_LOW | EVT_BAT_LOW_WARNING | EVT_CO2_FACTORY_RESET | EVT_GET_SENSOR_DETAILS)
#define EVT_CO2_MEASUREMENT_BREAK (EVT_BAT_LOW | EVT_BAT_LOW_WARNING | EVT_CO2_CALIB_START | EVT_CO2_CALIB_DONE | EVT_CO2_FACTORY_RESET | EVT_CO2_CALIB_MODE_CHANGE | EVT_GET_SENSOR_DETAILS)

TaskDeclare(task_protocol);
TaskDeclare(task_history_recover);
TaskDeclare(task_history_storage);
TaskDeclare(task_history_upload);
TaskDeclare(task_co2_read);
TaskDeclare(task_co2_calibrate);
TaskDeclare(task_co2_alarm);
TaskDeclare(task_ui);
TaskDeclare(task_batery);
TaskDeclare(task_populate_fake_records);

// power on task that runs once at startup
TaskDeclare(task_power_on);

#define DEVICE_NAME       "AirSpot" /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME "MinTec"  /**< Manufacturer. Will be passed to Device Information Service. */
#define HARDWARE_REVISION "1.1.0"
#define SOFTWARE_REVISION "1.5.0"
#define FIRMWARE_REVISION "s112_nrf52_7.2.0"

#endif // __TTASK_H__
