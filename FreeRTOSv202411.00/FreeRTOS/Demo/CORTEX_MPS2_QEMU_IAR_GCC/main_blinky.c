#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdbool.h>



#define P_ACTIVE_W 0.066 /* watts while CPU active (J/s) */
#define MEM_ENERGY_J_PER_KB 1e-5 /* J per kB memory access (illustrative) */

#define taskers_dag_input_power 0.0001
#define malardalen_like_input_power 0.0003
#define rtbench_mixed_input_power 0.0001
#define synthetic_small_input_power 0.0002
#define tacle_bench_like_input_power 0.0001



#define POWER_IN_SYSTEM             0.0001


typedef struct {
    char name[32];
    uint32_t release_ms;
    uint32_t C_ms;
    uint32_t period_ms;
    uint32_t deadline_ms;
    uint32_t M_kB;
    double alpha;
    double beta;
    double gamma;
    bool mandatory_task;
} TaskConfig_t;


// static TaskConfig_t taskConfigs[] = {         //bench_battery_energy_pressure
//     {"sens_fast", 0, 3, 12, 8, 120, 0.6, 0.04, 0.9, true},
//     {"filt_mid", 0, 5, 20, 12, 200, 0.7, 0.05, 1.1, true},
//     {"ctrl_tight", 0, 2, 10, 6, 40, 0.5, 0.03, 0.6, true},
//     {"radio_tx", 5, 6, 30, 16, 260, 0.8, 0.07, 1.3, true},
//     {"logger", 7, 4, 25, 10, 90, 0.4, 0.02, 0.4, true},
// };

// static TaskConfig_t taskConfigs[] = {         //bench_cpu_overload_mixed_deadlines
//     {"decA", 0, 4, 10, 7, 30, 0.6, 0.03, 0.6, true},
//     {"decB", 0, 5, 12, 8, 35, 0.6, 0.03, 0.6, true},
//     {"encA", 0, 6, 15, 9, 40, 0.7, 0.04, 0.7, true},
//     {"mix", 0, 3, 8, 6, 18, 0.5, 0.02, 0.4, true},
//     {"net", 2, 5, 14, 8, 45, 0.7, 0.04, 0.7, true},
// };

// static TaskConfig_t taskConfigs[] = {         //bench_deadline_stress_not_overload
//     {"crit_ctrl", 0, 2, 20, 7, 18, 0.30, 0.01, 0.10, true},
//     {"crit_sense", 0, 3, 25, 9, 22, 0.35, 0.01, 0.12, true},
//     {"crit_fuse", 5, 4, 40, 12, 28, 0.40, 0.02, 0.16, true},

//     {"trap_edf_poison", 0, 30, 180, 6, 120000, 0.99, 0.15, 3.00, true},
//     {"trap_rm_poison", 0, 20, 12, 70, 90000, 0.98, 0.14, 2.80, true},
// };

// static TaskConfig_t taskConfigs[] = {         //bench_harvest_scarce_energy_gap
//     {"crit_ctrl", 0, 3, 20, 9, 16, 0.30, 0.01, 0.10, true},
//     {"crit_sense", 0, 4, 25, 11, 18, 0.35, 0.01, 0.12, true},
//     {"crit_fuse", 0, 5, 30, 14, 20, 0.40, 0.02, 0.16, true},

//     {"trap_urgent_hiE", 0, 1, 500, 5, 50000, 0.95, 0.12, 2.30, true},
//     {"trap_rm_hiE", 0, 1, 12, 70, 30000, 0.98, 0.13, 2.50, true},
// };

// static TaskConfig_t taskConfigs[] = {         //bench_harvest_bursty_mixed
//     {"crit_ctrl", 0, 3, 20, 9, 16, 0.30, 0.01, 0.10, true},
//     {"crit_sense", 1, 4, 25, 11, 18, 0.35, 0.01, 0.12, true},
//     {"crit_fuse", 2, 5, 30, 14, 20, 0.40, 0.02, 0.16, true},

//     {"trap_burst_hiE1", 0, 1, 400, 5, 60000, 0.98, 0.13, 2.60, true},
//     {"trap_burst_hiE2", 200, 1, 400, 5, 60000, 0.98, 0.13, 2.60, true},
// };





// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
//     {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
//     {"update_control", 6, 4, 20, 20, 30, 0.5, 0.02, 0.4, true},
//     {"log_results", 8, 6, 25, 25, 25, 0.4, 0.01, 0.2, true},
//     {"send_actuators", 12, 7, 25, 25, 50, 0.7, 0.03, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1", 0, 7, 15, 15, 40, 0.6, 0.02, 0.4, true},
//     {"jfdctint", 0, 5, 10, 10, 30, 0.7, 0.03, 0.5, true},
//     {"adpcm", 2, 6, 20, 20, 25, 0.5, 0.02, 0.3, true},
//     {"cnt", 1, 3, 10, 10, 10, 0.4, 0.01, 0.2, true},
//     {"compress", 3, 8, 25, 25, 50, 0.8, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1", 0, 2, 5, 5, 30, 0.6, 0.02, 0.5, true},
//     {"T2", 1, 3, 8, 8, 10, 0.5, 0.03, 0.4, true},
//     {"T3", 2, 4, 10, 10, 80, 0.7, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 15, 15, 25, 0.5, 0.02, 0.3, true},
//     {"crc", 0, 5, 20, 20, 30, 0.6, 0.02, 0.4, true},
//     {"ludcmp", 2, 10, 30, 30, 60, 0.7, 0.03, 0.5, true},
//     {"qsort", 1, 8, 20, 20, 50, 0.8, 0.02, 0.6, true},
//     {"matmult", 3, 12, 25, 25, 70, 0.9, 0.03, 0.7, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
//     {"sensor_fusion", 0, 6, 20, 20, 35, 0.7, 0.03, 0.4, true},
//     {"fft_task", 5, 12, 50, 50, 60, 0.5, 0.02, 0.6, true},
//     {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
//     {"path_planning", 10, 15, 50, 50, 70, 0.8, 0.02, 0.7, true}
// };
















// static const TaskConfig_t taskConfigs[] = { // TaskDef bench_singlecore_overload[] = {
//     {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
//     {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
//     {"update_control", 6, 4, 20, 6, 30, 0.5, 0.02, 0.4, true},
//     {"log_results", 8, 5, 25, 25, 25, 0.4, 0.01, 0.2, true},
//     {"send_actuators", 12, 3, 25, 8, 50, 0.7, 0.03, 0.6, true}};

// static const TaskConfig_t taskConfigs[] = { // static TaskDef bench_sof_realtime[] = { 
//     {"fft1", 0, 4, 15, 15, 40, 0.6, 0.02, 0.4, true},
//     {"jfdctint", 0, 3, 10, 10, 30, 0.7, 0.03, 0.5, true},
//     {"adpcm", 2, 4, 20, 20, 25, 0.5, 0.02, 0.3, true},
//     {"cnt", 1, 2, 10, 10, 10, 0.4, 0.01, 0.2, true},
//     {"compress", 3, 4, 25, 7, 50, 0.8, 0.02, 0.6, true}};

// static const TaskConfig_t taskConfigs[] = { // static TaskDef bench_bounded_utilization[] = {
//     {"T1", 0, 4, 7, 7, 30, 0.6, 0.02, 0.5, true},
//     {"T2", 1, 3, 9, 9, 10, 0.5, 0.03, 0.4, true},
//     {"T3", 2, 4, 30, 6, 80, 0.7, 0.02, 0.6, true}};

// static const TaskConfig_t taskConfigs[] = { // static TaskDef bench_cpuIntensive_Starvation[] = {
//     {"bs", 0, 6, 15, 15, 25, 0.5, 0.02, 0.3, true},
//     {"crc", 0, 4, 20, 20, 30, 0.6, 0.02, 0.4, true},
//     {"ludcmp", 2, 6, 30, 30, 60, 0.7, 0.03, 0.5, true},
//     {"qsort", 1, 4, 20, 20, 50, 0.8, 0.02, 0.6, true},
//     {"matmult", 3, 6, 50, 10, 70, 0.9, 0.03, 0.7, true}};

// static const TaskConfig_t taskConfigs[] = { // static TaskDef bench_realtime_multirate[] = {
//     {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
//     {"sensor_fusion", 0, 7, 20, 20, 35, 0.7, 0.03, 0.4, true},
//     {"fft_task", 5, 8, 50, 50, 60, 0.5, 0.02, 0.6, true},
//     {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
//     {"path_planning", 10, 6, 60, 12, 70, 0.8, 0.02, 0.7, true}};

// static const TaskConfig_t taskConfigs[] = { // TaskDef bench_MixedCriticalityControl[] = {
//     {"IMU_Read", 0, 1, 5, 3, 8, 0.8, 0.01, 0.7, true},
//     {"Safety_Chk", 0, 1, 10, 5, 16, 0.8, 0.01, 0.7, true},

//     {"Motor_A", 0, 2, 12, 6, 16, 0.7, 0.02, 0.5, true},
//     {"Motor_B", 0, 2, 12, 6, 16, 0.7, 0.02, 0.5, true},
//     {"Motor_C", 0, 2, 12, 6, 16, 0.7, 0.02, 0.5, true},
//     {"Motor_D", 0, 2, 12, 6, 16, 0.7, 0.02, 0.5, true},

//     {"IK_Solver", 1, 6, 25, 12, 64, 0.9, 0.04, 0.7, true},

//     {"Path_Plan", 5, 12, 120, 60, 512, 0.3, 0.08, 0.3, true},

//     {"Obj_Detect", 0, 20, 200, 100, 1024, 0.4, 0.10, 0.4, true},

//     {"Telemetry", 10, 4, 60, 60, 64, 0.2, 0.06, 0.2, true},
//     {"UI_Refresh", 15, 3, 50, 50, 256, 0.2, 0.06, 0.2, true},

//     {"Bat_Mon", 0, 1, 100, 100, 8, 0.2, 0.01, 0.15, true},
// };

// static const TaskConfig_t taskConfigs[] = { // TaskDef bench_HighFootprint_PeriodicMixedBenchmark[] = {
//     {"Mem_Task_1", 0, 6, 25, 12, 1024, 0.7, 0.03, 0.5, true},
//     {"Mem_Task_2", 2, 6, 30, 15, 2048, 0.7, 0.03, 0.5, true},
//     {"Mem_Task_3", 4, 10, 50, 25, 4096, 0.8, 0.04, 0.6, true},

//     {"Small_1", 0, 1, 10, 5, 16, 0.6, 0.02, 0.4, true},
//     {"Small_2", 0, 1, 10, 5, 16, 0.6, 0.02, 0.4, true},
//     {"Small_3", 0, 1, 10, 5, 16, 0.6, 0.02, 0.4, true},
//     {"Small_4", 0, 1, 10, 5, 16, 0.6, 0.02, 0.4, true},
//     {"Small_5", 0, 1, 10, 5, 16, 0.6, 0.02, 0.4, true},
 
//     {"Bg_Fill", 0, 2, 60, 60, 512, 0.2, 0.05, 0.2, true},
//     {"Bg_Sweep", 0, 2, 60, 60, 512, 0.2, 0.05, 0.2, true},
// };

// static const TaskConfig_t taskConfigs[] = { // TaskDef bench_harmonic_periods[] = {
//     {"T10", 0, 3, 10, 5, 32, 0.6, 0.02, 0.4, true},
//     {"T20", 0, 5, 20, 10, 32, 0.6, 0.02, 0.4, true},
//     {"T40", 0, 7, 40, 20, 32, 0.6, 0.02, 0.4, true},
//     {"T80", 0, 10, 80, 40, 32, 0.6, 0.02, 0.4, true},
//     {"T160", 0, 16, 160, 80, 32, 0.6, 0.02, 0.4, true},
//     {"T320", 0, 24, 320, 160, 32, 0.6, 0.02, 0.4, true},
// };

// static const TaskConfig_t taskConfigs[] = { // TaskDef bench_non_harmonic_jitter_sporadic[] = {
//     {"T_7", 0, 2, 7, 3, 16, 0.7, 0.02, 0.5, true},
//     {"T_11", 0, 3, 11, 5, 16, 0.7, 0.02, 0.5, true},
//     {"T_13", 0, 4, 13, 6, 16, 0.7, 0.02, 0.5, true},
//     {"T_17", 0, 5, 17, 8, 16, 0.7, 0.02, 0.5, true},
//     {"T_19", 0, 6, 19, 9, 16, 0.7, 0.02, 0.5, true},

//     {"Sporadic", 5, 10, 1000, 40, 64, 0.2, 0.08, 0.2, true},
//     {"Bg", 0, 1, 60, 60, 8, 0.2, 0.01, 0.1, true},
// };

// static const TaskConfig_t taskConfigs[] = { // TaskDef bench_MixedCriticality_HardSoftTaskInteraction[] = {
//     {"Crit_1", 0, 2, 6, 3, 16, 0.9, 0.02, 0.7, true},
//     {"Crit_2", 0, 3, 12, 6, 16, 0.9, 0.02, 0.7, true},
//     {"Crit_3", 1, 2, 10, 5, 16, 0.9, 0.02, 0.7, true},

//     {"Loose_1", 0, 5, 60, 50, 64, 0.3, 0.03, 0.2, true},
//     {"Loose_2", 0, 5, 60, 50, 64, 0.3, 0.03, 0.2, true},

//     {"Sporadic", 10, 10, 1000, 40, 128, 0.2, 0.08, 0.2, true},

//     {"Bg_1", 0, 1, 30, 30, 8, 0.2, 0.01, 0.1, true},
//     {"Bg_2", 0, 1, 30, 30, 8, 0.2, 0.01, 0.1, true},
//     {"Bg_3", 0, 1, 30, 30, 8, 0.2, 0.01, 0.1, true},
// };

// static const TaskConfig_t taskConfigs[] = { // TaskDef bench_high_leakage[] = {
//     {"Leak_A", 0, 5, 25, 12, 32, 0.2, 0.50, 0.2, true},
//     {"Leak_B", 0, 5, 25, 12, 32, 0.2, 0.50, 0.2, true},

//     {"Active_A", 0, 2, 10, 5, 16, 0.9, 0.01, 0.8, true},
//     {"Active_B", 0, 2, 10, 5, 16, 0.9, 0.01, 0.8, true},

//     {"Sporadic", 5, 8, 500, 30, 64, 0.3, 0.10, 0.3, true},
//     {"Bg", 0, 1, 60, 60, 8, 0.2, 0.01, 0.1, true},
// };

// static const TaskConfig_t taskConfigs[] = { // TaskDef bench_sporadic_overload[] = {
//     {"P1", 0, 5, 20, 20, 32, 0.9, 0.02, 0.5, true},
//     {"P2", 0, 5, 20, 20, 32, 0.9, 0.02, 0.5, true},
//     {"P3", 0, 5, 20, 20, 32, 0.7, 0.02, 0.5, true},
//     {"P4", 0, 5, 20, 20, 32, 0.7, 0.02, 0.5, true},
//     {"P5", 0, 5, 20, 20, 32, 0.7, 0.02, 0.5, true},
//     {"P6", 0, 5, 20, 20, 32, 0.7, 0.02, 0.5, true},

//     {"S_Over_H1", 10, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},
//     {"S_Over_H2", 10, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},
//     {"S_Over_H3", 10, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},
//     {"S_Over_H4", 10, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},
//     {"S_Over_H5", 30, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},

//     {"S_Over_L1", 20, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
//     {"S_Over_L2", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
//     {"S_Over_L3", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
//     {"S_Over_L4", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
//     {"S_Over_L5", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
//     {"S_Over_L6", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
//     //

//     {"Bg", 0, 2, 100, 100, 8, 0.1, 0.01, 0.1, true},
// };

static const TaskConfig_t taskConfigs[] = { // TaskDef bench_ultimate_stress[] = {
    {"T01", 0, 2, 10, 5, 16, 0.9, 0.02, 0.7, true},
    {"T02", 0, 2, 10, 5, 16, 0.9, 0.02, 0.7, true},
    {"T03", 0, 2, 10, 5, 16, 0.9, 0.02, 0.7, true},
    {"T04", 0, 2, 10, 5, 16, 0.9, 0.02, 0.7, true},

    {"T05", 1, 4, 20, 10, 32, 0.9, 0.02, 0.7, true},
    {"T06", 1, 4, 20, 10, 32, 0.9, 0.02, 0.7, true},
    {"T07", 1, 4, 20, 10, 32, 0.9, 0.02, 0.7, true},
    {"T08", 1, 4, 20, 10, 32, 0.9, 0.02, 0.7, true},

    {"T09", 2, 8, 40, 20, 64, 0.8, 0.03, 0.6, true},
    {"T10", 2, 8, 40, 20, 64, 0.8, 0.03, 0.6, true},
    {"T11", 2, 8, 40, 20, 64, 0.8, 0.03, 0.6, true},
    {"T12", 2, 8, 40, 20, 64, 0.8, 0.03, 0.6, true},

    {"S01", 5, 10, 1000, 50, 128, 0.3, 0.08, 0.3, true},
    {"S02", 15, 10, 1000, 60, 128, 0.3, 0.08, 0.3, true},
    {"S03", 25, 10, 1000, 70, 128, 0.3, 0.08, 0.3, true},
    {"S04", 35, 10, 1000, 80, 128, 0.3, 0.08, 0.3, true},

    {"M01", 0, 1, 5, 3, 8, 0.7, 0.01, 0.5, true},
    {"M02", 0, 1, 5, 3, 8, 0.7, 0.01, 0.5, true},
    {"M03", 0, 1, 5, 3, 8, 0.7, 0.01, 0.5, true},
    {"M04", 0, 1, 5, 3, 8, 0.7, 0.01, 0.5, true},

    {"B01", 0, 5, 100, 100, 256, 0.2, 0.04, 0.1, true},
    {"B02", 0, 5, 100, 100, 256, 0.2, 0.04, 0.1, true},
    {"B03", 0, 5, 100, 100, 256, 0.2, 0.04, 0.1, true},
    {"B04", 0, 5, 100, 100, 256, 0.2, 0.04, 0.1, true},

    {"Last", 0, 1, 200, 200, 512, 0.1, 0.02, 0.05, true},
};





void printAllTaskInfo(void) {
    TaskStatus_t *pxTaskStatusArray;
    UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime;
    const char *const pcTaskState[] = { "Running", "Ready", "Blocked", "Suspended", "Deleted", "Invalid" };

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if (pxTaskStatusArray != NULL) {
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

        printf("\r\n--- Task List (%u tasks) ---\r\n", (unsigned int)uxArraySize);
        for (x = 0; x < uxArraySize; x++) {
            printf("Name: %-12s | State: %-10s | Priority: %u\r\n",
                   pxTaskStatusArray[x].pcTaskName,
                   pcTaskState[pxTaskStatusArray[x].eCurrentState],
                   (unsigned int)pxTaskStatusArray[x].uxCurrentPriority);
        }
        vPortFree(pxTaskStatusArray);
    }
}



#if ( configUSE_EEVDF_SCHEDULER == 1 )



#define mainTASK_PRIORITY (tskIDLE_PRIORITY + 1)
int iter = 0;


static inline double compute_energy_j(const TaskConfig_t *d)
{
    if (!d)
        return 0.0;
    double t_s = ((double)d->C_ms) / 1000.0;
    double e = P_ACTIVE_W * t_s + MEM_ENERGY_J_PER_KB * (double)d->M_kB;

    double model = d->alpha * (double)d->C_ms + d->beta * (double)d->M_kB + d->gamma;
    e *= (1.0 + model * 0.001);

    printf("Task Name: %s, Power: %d uj\r\n", d->name, (unsigned int)(e*1000*1000));

    return e;
}

#define NUM_TASKS (sizeof(taskConfigs) / sizeof(TaskConfig_t))

static TaskHandle_t taskHandles[NUM_TASKS];
static volatile uint32_t deadlineMisses[NUM_TASKS] = {0};
static volatile uint32_t totalDeadlineMisses = 0;
static volatile TickType_t absoluteDeadline[NUM_TASKS] = {0};
static volatile BaseType_t taskActive[NUM_TASKS] = {0};

typedef struct {
    uint32_t taskIndex;
    TickType_t startTime;
    TickType_t deadline;
} TaskContext_t;

static void prvWorkTask(void *pvParameters) {
    TaskContext_t *ctx = (TaskContext_t *)pvParameters;
    uint32_t taskIndex = ctx->taskIndex;
    const TaskConfig_t *config = &taskConfigs[taskIndex];
    TickType_t nextWakeTime;
    TickType_t instanceStartTime;
    TickType_t instanceCompleteTime;
    volatile uint32_t workCounter;
    
    vTaskDelay(pdMS_TO_TICKS(config->release_ms));
    
    nextWakeTime = xTaskGetTickCount();
    
    for (;;) {
        // printf("Task %s executing at tick %u\r\n", config->name, (unsigned int)instanceStartTime);

        instanceStartTime = xTaskGetTickCount();
        
        if (taskActive[taskIndex]) {    //TODO
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        
        taskActive[taskIndex] = pdTRUE;
        absoluteDeadline[taskIndex] = instanceStartTime + pdMS_TO_TICKS(config->deadline_ms);
        
        workCounter = 0;
        
        // printf("TaskName: %s, workEndTime: %u, CurrentTime: %d\r\n",config->name, (unsigned long)workEndTime, (unsigned long)instanceStartTime);

        vTaskResetEEVDFBudget(taskHandles[taskIndex]);
        while (vTaskGetEEVDFBudget(taskHandles[taskIndex]) < config->C_ms) {
            workCounter++;
            __asm volatile ("nop");
            // if(taskIndex == 1)
            //     printf("TaskName: %s, budget: %u, CurrentTime: %d\r\n",config->name, (unsigned long)vTaskGetEEVDFBudget(taskHandles[taskIndex]), (unsigned long)config->C_ms);           
        }
        
        instanceCompleteTime = xTaskGetTickCount();
        taskActive[taskIndex] = pdFALSE;
        
        if (instanceCompleteTime > absoluteDeadline[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        vTaskDelayUntil(&nextWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}

static void prvMonitorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t lastPrintTime = xTaskGetTickCount();
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));

        for (uint32_t i = 0; i < NUM_TASKS; i++) {
            
            if (absoluteDeadline[i] >= 0) {

                TickType_t deadline = absoluteDeadline[i] + pdMS_TO_TICKS(taskConfigs[i].period_ms);
                TickType_t currentTime = xTaskGetTickCount();
                // printf("absDeadLine: %u, deadline: %u, currentTime: %u\r\n", absoluteDeadline[i], deadline, currentTime);

                if (currentTime >= deadline) {
                    // printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n");
                    deadlineMisses[i]++;
                    totalDeadlineMisses++;
                    absoluteDeadline[i] = deadline;
                }
            }
        }
        
        if ((xTaskGetTickCount() - lastPrintTime) >= pdMS_TO_TICKS(1000)) {
            lastPrintTime = xTaskGetTickCount();
            printf("-----Iter i: %d, Deadline Misses: %u\r\n", iter++, (unsigned long)totalDeadlineMisses);
        }
    }
}

void main_blinky(void) {
    static TaskContext_t contexts[NUM_TASKS];
    
    for (uint32_t i = 0; i < NUM_TASKS; i++) {
        contexts[i].taskIndex = i;
        contexts[i].startTime = 0;
        contexts[i].deadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        xTaskCreate(prvWorkTask,
                    taskConfigs[i].name,
                    configMINIMAL_STACK_SIZE * 2,
                    &contexts[i],
                    mainTASK_PRIORITY,
                    &taskHandles[i]);
        
        UBaseType_t weight = (UBaseType_t)(5);
        TickType_t request = pdMS_TO_TICKS(taskConfigs[i].C_ms);
        double power = (double)(compute_energy_j(&taskConfigs[i]));
        
        vTaskSetEEVDFWeight(taskHandles[i], weight);
        vTaskSetEEVDFRequest(taskHandles[i], request);
        vTaskSetEEVDFPowerConsumption(taskHandles[i], power);
    }

    vTaskSetEnergyParameters(configSYSTEM_POWERON_WATERMARK, POWER_IN_SYSTEM);
    
    xTaskCreate(prvMonitorTask,
                "Monitor",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainTASK_PRIORITY + 1,
                NULL);
    
    vTaskStartScheduler();
    
    for (;;) {}
}



#endif


























#if ( configUSE_EDF_SCHEDULER == 1 )

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define mainTASK_PRIORITY (tskIDLE_PRIORITY + 1)

// typedef struct {
//     char name[10];
//     uint32_t release_ms;
//     uint32_t C_ms;
//     uint32_t period_ms;
//     uint32_t deadline_ms;
//     uint32_t M_kB;
//     double alpha;
//     double beta;
//     double gamma;
// } TaskConfig_t;

int iter = 0;

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 20, 20, 25, 0.5, 0.02, 0.3},
//     {"crc", 0, 5, 25, 25, 30, 0.6, 0.02, 0.4},
//     {"ludcmp", 2, 10, 40, 40, 60, 0.7, 0.03, 0.5},
//     {"qsort", 1, 8, 35, 35, 50, 0.8, 0.02, 0.6},
//     {"matmult", 3, 12, 45, 45, 70, 0.9, 0.03, 0.7}
// };
// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control",0,10,50,50,40,0.6,0.02,0.5},
//     {"sensor_fusion",0,6,30,30,35,0.7,0.03,0.4},
//     {"fft_task",5,12,60,60,60,0.5,0.02,0.6},
//     {"filter",0,5,25,25,20,0.4,0.01,0.3},
//     {"path_planning",10,15,70,70,70,0.8,0.02,0.7}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1",0,2,8,8,30,0.6,0.02,0.5},
//     {"T2",1,3,10,10,10,0.5,0.03,0.4},
//     {"T3",2,4,12,12,80,0.7,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1",0,7,25,25,40,0.6,0.02,0.4},
//     {"jfdctint",0,5,20,20,30,0.7,0.03,0.5},
//     {"adpcm",2,6,30,30,25,0.5,0.02,0.3},
//     {"cnt",1,3,15,15,10,0.4,0.01,0.2},
//     {"compress",3,8,35,35,50,0.8,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors",0,3,15,15,20,0.4,0.01,0.3},
//     {"process_data",2,5,25,25,40,0.6,0.02,0.5},
//     {"update_control",6,4,30,30,30,0.5,0.02,0.4},
//     {"log_results",8,6,40,40,25,0.4,0.01,0.2},
//     {"send_actuators",12,7,35,35,50,0.7,0.03,0.6},
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
//     {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
//     {"update_control", 6, 4, 20, 20, 30, 0.5, 0.02, 0.4, true},
//     {"log_results", 8, 6, 25, 25, 25, 0.4, 0.01, 0.2, true},
//     {"send_actuators", 12, 7, 25, 25, 50, 0.7, 0.03, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1", 0, 7, 15, 15, 40, 0.6, 0.02, 0.4, true},
//     {"jfdctint", 0, 5, 10, 10, 30, 0.7, 0.03, 0.5, true},
//     {"adpcm", 2, 6, 20, 20, 25, 0.5, 0.02, 0.3, true},
//     {"cnt", 1, 3, 10, 10, 10, 0.4, 0.01, 0.2, true},
//     {"compress", 3, 8, 25, 25, 50, 0.8, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1", 0, 2, 5, 5, 30, 0.6, 0.02, 0.5, true},
//     {"T2", 1, 3, 8, 8, 10, 0.5, 0.03, 0.4, true},
//     {"T3", 2, 4, 10, 10, 80, 0.7, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 15, 15, 25, 0.5, 0.02, 0.3, true},
//     {"crc", 0, 5, 20, 20, 30, 0.6, 0.02, 0.4, true},
//     {"ludcmp", 2, 10, 30, 30, 60, 0.7, 0.03, 0.5, true},
//     {"qsort", 1, 8, 20, 20, 50, 0.8, 0.02, 0.6, true},
//     {"matmult", 3, 12, 25, 25, 70, 0.9, 0.03, 0.7, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
//     {"sensor_fusion", 0, 6, 20, 20, 35, 0.7, 0.03, 0.4, true},
//     {"fft_task", 5, 12, 50, 50, 60, 0.5, 0.02, 0.6, true},
//     {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
//     {"path_planning", 10, 15, 50, 50, 70, 0.8, 0.02, 0.7, true}
// };




static inline double compute_energy_j(const TaskConfig_t *d)
{
    if (!d)
        return 0.0;
    double t_s = ((double)d->C_ms) / 1000.0;
    double e = P_ACTIVE_W * t_s + MEM_ENERGY_J_PER_KB * (double)d->M_kB;

    double model = d->alpha * (double)d->C_ms + d->beta * (double)d->M_kB + d->gamma;
    e *= (1.0 + model * 0.001);

    printf("Task Name: %s, Power: %d uj\r\n", d->name, (unsigned int)(e*1000*1000));

    return e;
}

#define NUM_TASKS (sizeof(taskConfigs) / sizeof(TaskConfig_t))

static TaskHandle_t taskHandles[NUM_TASKS];
static volatile uint32_t deadlineMisses[NUM_TASKS] = {0};
static volatile uint32_t totalDeadlineMisses = 0;
static volatile TickType_t absoluteDeadline[NUM_TASKS] = {0};
static volatile BaseType_t taskActive[NUM_TASKS] = {0};

typedef struct {
    uint32_t taskIndex;
    TickType_t startTime;
    TickType_t deadline;
} TaskContext_t;

static void prvWorkTask(void *pvParameters) {
    TaskContext_t *ctx = (TaskContext_t *)pvParameters;
    uint32_t taskIndex = ctx->taskIndex;
    const TaskConfig_t *config = &taskConfigs[taskIndex];
    TickType_t nextWakeTime;
    TickType_t instanceStartTime;
    TickType_t instanceCompleteTime;
    volatile uint32_t workCounter;
    
    vTaskDelay(pdMS_TO_TICKS(config->release_ms));
    
    nextWakeTime = xTaskGetTickCount();
    
    for (;;) {
        instanceStartTime = xTaskGetTickCount();
        
        if (taskActive[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        
        taskActive[taskIndex] = pdTRUE;
        absoluteDeadline[taskIndex] = instanceStartTime + pdMS_TO_TICKS(config->deadline_ms);
        
        vTaskSetEDFDeadline(taskHandles[taskIndex], absoluteDeadline[taskIndex]);
        
        workCounter = 0;

        // printf("Task %s executing at tick %u\r\n", config->name, (unsigned int)instanceStartTime);


        vTaskResetEDFBudget(taskHandles[taskIndex]);
        while (vTaskGetEDFBudget(taskHandles[taskIndex]) < config->C_ms) {
            workCounter++;
            __asm volatile ("nop");
        }
        
        instanceCompleteTime = xTaskGetTickCount();
        taskActive[taskIndex] = pdFALSE;
        
        if (instanceCompleteTime > absoluteDeadline[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        vTaskDelayUntil(&nextWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}

static void prvMonitorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t lastPrintTime = xTaskGetTickCount();
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));

        for (uint32_t i = 0; i < NUM_TASKS; i++) {
            
            if (absoluteDeadline[i] >= 0) {

                TickType_t deadline = absoluteDeadline[i] + pdMS_TO_TICKS(taskConfigs[i].period_ms);
                TickType_t currentTime = xTaskGetTickCount();

                if (currentTime >= deadline) {
                    deadlineMisses[i]++;
                    totalDeadlineMisses++;
                    absoluteDeadline[i] = deadline;
                }
            }
        }
        
        if ((xTaskGetTickCount() - lastPrintTime) >= pdMS_TO_TICKS(1000)) {
            lastPrintTime = xTaskGetTickCount();
            printf("-----Iter i: %d, Deadline Misses: %u\r\n", iter++, (unsigned long)totalDeadlineMisses);
        }
    }
}

void main_blinky(void) {
    static TaskContext_t contexts[NUM_TASKS];
    
    for (uint32_t i = 0; i < NUM_TASKS; i++) {
        contexts[i].taskIndex = i;
        contexts[i].startTime = 0;
        contexts[i].deadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        xTaskCreate(prvWorkTask,
                    taskConfigs[i].name,
                    configMINIMAL_STACK_SIZE * 2,
                    &contexts[i],
                    mainTASK_PRIORITY,
                    &taskHandles[i]);

        double power = (double)(compute_energy_j(&taskConfigs[i]));
        TickType_t request = pdMS_TO_TICKS(taskConfigs[i].C_ms);
        TickType_t initialDeadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        vTaskSetEDFDeadline(taskHandles[i], initialDeadline);
        vTaskSetEDFPowerConsumption(taskHandles[i], power);
        vTaskSetEDFRequest(taskHandles[i], request);

    }

    vTaskSetEnergyParameters(configSYSTEM_POWERON_WATERMARK, POWER_IN_SYSTEM);    
    xTaskCreate(prvMonitorTask,
                "Monitor",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainTASK_PRIORITY + 1,
                NULL);
    
    vTaskStartScheduler();
    
    for (;;) {}
}

#endif













#if ( configUSE_LAZY_SCHEDULER == 1 )

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define mainTASK_PRIORITY (tskIDLE_PRIORITY + 1)

// typedef struct {
//     char name[10];
//     uint32_t release_ms;
//     uint32_t C_ms;
//     uint32_t period_ms;
//     uint32_t deadline_ms;
//     uint32_t M_kB;
//     double alpha;
//     double beta;
//     double gamma;
// } TaskConfig_t;

int iter = 0;

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 20, 20, 25, 0.5, 0.02, 0.3},
//     {"crc", 0, 5, 25, 25, 30, 0.6, 0.02, 0.4},
//     {"ludcmp", 2, 10, 40, 40, 60, 0.7, 0.03, 0.5},
//     {"qsort", 1, 8, 35, 35, 50, 0.8, 0.02, 0.6},
//     {"matmult", 3, 12, 45, 45, 70, 0.9, 0.03, 0.7}
// };
// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control",0,10,50,50,40,0.6,0.02,0.5},
//     {"sensor_fusion",0,6,30,30,35,0.7,0.03,0.4},
//     {"fft_task",5,12,60,60,60,0.5,0.02,0.6},
//     {"filter",0,5,25,25,20,0.4,0.01,0.3},
//     {"path_planning",10,15,70,70,70,0.8,0.02,0.7}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1",0,2,8,8,30,0.6,0.02,0.5},
//     {"T2",1,3,10,10,10,0.5,0.03,0.4},
//     {"T3",2,4,12,12,80,0.7,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1",0,7,25,25,40,0.6,0.02,0.4},
//     {"jfdctint",0,5,20,20,30,0.7,0.03,0.5},
//     {"adpcm",2,6,30,30,25,0.5,0.02,0.3},
//     {"cnt",1,3,15,15,10,0.4,0.01,0.2},
//     {"compress",3,8,35,35,50,0.8,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors",0,3,15,15,20,0.4,0.01,0.3},
//     {"process_data",2,5,25,25,40,0.6,0.02,0.5},
//     {"update_control",6,4,30,30,30,0.5,0.02,0.4},
//     {"log_results",8,6,40,40,25,0.4,0.01,0.2},
//     {"send_actuators",12,7,35,35,50,0.7,0.03,0.6},
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
//     {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
//     {"update_control", 6, 4, 20, 20, 30, 0.5, 0.02, 0.4, true},
//     {"log_results", 8, 6, 25, 25, 25, 0.4, 0.01, 0.2, true},
//     {"send_actuators", 12, 7, 25, 25, 50, 0.7, 0.03, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1", 0, 7, 15, 15, 40, 0.6, 0.02, 0.4, true},
//     {"jfdctint", 0, 5, 10, 10, 30, 0.7, 0.03, 0.5, true},
//     {"adpcm", 2, 6, 20, 20, 25, 0.5, 0.02, 0.3, true},
//     {"cnt", 1, 3, 10, 10, 10, 0.4, 0.01, 0.2, true},
//     {"compress", 3, 8, 25, 25, 50, 0.8, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1", 0, 2, 5, 5, 30, 0.6, 0.02, 0.5, true},
//     {"T2", 1, 3, 8, 8, 10, 0.5, 0.03, 0.4, true},
//     {"T3", 2, 4, 10, 10, 80, 0.7, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 15, 15, 25, 0.5, 0.02, 0.3, true},
//     {"crc", 0, 5, 20, 20, 30, 0.6, 0.02, 0.4, true},
//     {"ludcmp", 2, 10, 30, 30, 60, 0.7, 0.03, 0.5, true},
//     {"qsort", 1, 8, 20, 20, 50, 0.8, 0.02, 0.6, true},
//     {"matmult", 3, 12, 25, 25, 70, 0.9, 0.03, 0.7, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
//     {"sensor_fusion", 0, 6, 20, 20, 35, 0.7, 0.03, 0.4, true},
//     {"fft_task", 5, 12, 50, 50, 60, 0.5, 0.02, 0.6, true},
//     {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
//     {"path_planning", 10, 15, 50, 50, 70, 0.8, 0.02, 0.7, true}
// };




static inline double compute_energy_j(const TaskConfig_t *d)
{
    if (!d)
        return 0.0;
    double t_s = ((double)d->C_ms) / 1000.0;
    double e = P_ACTIVE_W * t_s + MEM_ENERGY_J_PER_KB * (double)d->M_kB;

    double model = d->alpha * (double)d->C_ms + d->beta * (double)d->M_kB + d->gamma;
    e *= (1.0 + model * 0.001);

    printf("Task Name: %s, Power: %d uj\r\n", d->name, (unsigned int)(e*1000*1000));

    return e;
}

#define NUM_TASKS (sizeof(taskConfigs) / sizeof(TaskConfig_t))

static TaskHandle_t taskHandles[NUM_TASKS];
static volatile uint32_t deadlineMisses[NUM_TASKS] = {0};
static volatile uint32_t totalDeadlineMisses = 0;
static volatile TickType_t absoluteDeadline[NUM_TASKS] = {0};
static volatile BaseType_t taskActive[NUM_TASKS] = {0};

typedef struct {
    uint32_t taskIndex;
    TickType_t startTime;
    TickType_t deadline;
} TaskContext_t;

static void prvWorkTask(void *pvParameters) {
    TaskContext_t *ctx = (TaskContext_t *)pvParameters;
    uint32_t taskIndex = ctx->taskIndex;
    const TaskConfig_t *config = &taskConfigs[taskIndex];
    TickType_t nextWakeTime;
    TickType_t instanceStartTime;
    TickType_t instanceCompleteTime;
    volatile uint32_t workCounter;
    
    vTaskDelay(pdMS_TO_TICKS(config->release_ms));
    
    nextWakeTime = xTaskGetTickCount();
    
    for (;;) {
        instanceStartTime = xTaskGetTickCount();
        
        if (taskActive[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        
        taskActive[taskIndex] = pdTRUE;
        absoluteDeadline[taskIndex] = instanceStartTime + pdMS_TO_TICKS(config->deadline_ms);
        
        vTaskSetLazyDeadline(taskHandles[taskIndex], absoluteDeadline[taskIndex]);
        
        workCounter = 0;

        // printf("Task %s executing at tick %u\r\n", config->name, (unsigned int)instanceStartTime);


        vTaskResetLazyBudget(taskHandles[taskIndex]);
        while (vTaskGetLazyBudget(taskHandles[taskIndex]) < config->C_ms) {
            workCounter++;
            __asm volatile ("nop");
        }
        
        instanceCompleteTime = xTaskGetTickCount();
        taskActive[taskIndex] = pdFALSE;
        
        if (instanceCompleteTime > absoluteDeadline[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        vTaskDelayUntil(&nextWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}

static void prvMonitorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t lastPrintTime = xTaskGetTickCount();
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));

        for (uint32_t i = 0; i < NUM_TASKS; i++) {
            
            if (absoluteDeadline[i] >= 0) {

                TickType_t deadline = absoluteDeadline[i] + pdMS_TO_TICKS(taskConfigs[i].period_ms);
                TickType_t currentTime = xTaskGetTickCount();

                if (currentTime >= deadline) {
                    deadlineMisses[i]++;
                    totalDeadlineMisses++;
                    absoluteDeadline[i] = deadline;
                }
            }
        }
        
        if ((xTaskGetTickCount() - lastPrintTime) >= pdMS_TO_TICKS(1000)) {
            lastPrintTime = xTaskGetTickCount();
            printf("-----Iter i: %d, Deadline Misses: %u\r\n", iter++, (unsigned long)totalDeadlineMisses);
        }
    }
}

void main_blinky(void) {
    static TaskContext_t contexts[NUM_TASKS];
    
    for (uint32_t i = 0; i < NUM_TASKS; i++) {
        contexts[i].taskIndex = i;
        contexts[i].startTime = 0;
        contexts[i].deadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        xTaskCreate(prvWorkTask,
                    taskConfigs[i].name,
                    configMINIMAL_STACK_SIZE * 2,
                    &contexts[i],
                    mainTASK_PRIORITY,
                    &taskHandles[i]);

        double power = (double)(compute_energy_j(&taskConfigs[i]));
        TickType_t request = pdMS_TO_TICKS(taskConfigs[i].C_ms);
        TickType_t initialDeadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        vTaskSetLazyDeadline(taskHandles[i], initialDeadline);
        vTaskSetLazyPowerConsumption(taskHandles[i], power);
        vTaskSetLazyRequest(taskHandles[i], request);

    }

    vTaskSetEnergyParameters(configSYSTEM_POWERON_WATERMARK, POWER_IN_SYSTEM);    
    xTaskCreate(prvMonitorTask,
                "Monitor",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainTASK_PRIORITY + 1,
                NULL);
    
    vTaskStartScheduler();
    
    for (;;) {}
}

#endif















#if ( configUSE_ZYGARDE_SCHEDULER == 1 )

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define mainTASK_PRIORITY (tskIDLE_PRIORITY + 1)

// typedef struct {
//     char name[10];
//     uint32_t release_ms;
//     uint32_t C_ms;
//     uint32_t period_ms;
//     uint32_t deadline_ms;
//     uint32_t M_kB;
//     double alpha;
//     double beta;
//     double gamma;
// } TaskConfig_t;

int iter = 0;

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 20, 20, 25, 0.5, 0.02, 0.3},
//     {"crc", 0, 5, 25, 25, 30, 0.6, 0.02, 0.4},
//     {"ludcmp", 2, 10, 40, 40, 60, 0.7, 0.03, 0.5},
//     {"qsort", 1, 8, 35, 35, 50, 0.8, 0.02, 0.6},
//     {"matmult", 3, 12, 45, 45, 70, 0.9, 0.03, 0.7}
// };
// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control",0,10,50,50,40,0.6,0.02,0.5},
//     {"sensor_fusion",0,6,30,30,35,0.7,0.03,0.4},
//     {"fft_task",5,12,60,60,60,0.5,0.02,0.6},
//     {"filter",0,5,25,25,20,0.4,0.01,0.3},
//     {"path_planning",10,15,70,70,70,0.8,0.02,0.7}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1",0,2,8,8,30,0.6,0.02,0.5},
//     {"T2",1,3,10,10,10,0.5,0.03,0.4},
//     {"T3",2,4,12,12,80,0.7,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1",0,7,25,25,40,0.6,0.02,0.4},
//     {"jfdctint",0,5,20,20,30,0.7,0.03,0.5},
//     {"adpcm",2,6,30,30,25,0.5,0.02,0.3},
//     {"cnt",1,3,15,15,10,0.4,0.01,0.2},
//     {"compress",3,8,35,35,50,0.8,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors",0,3,15,15,20,0.4,0.01,0.3},
//     {"process_data",2,5,25,25,40,0.6,0.02,0.5},
//     {"update_control",6,4,30,30,30,0.5,0.02,0.4},
//     {"log_results",8,6,40,40,25,0.4,0.01,0.2},
//     {"send_actuators",12,7,35,35,50,0.7,0.03,0.6},
// };



// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
//     {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
//     {"update_control", 6, 4, 20, 20, 30, 0.5, 0.02, 0.4, true},
//     {"log_results", 8, 6, 25, 25, 25, 0.4, 0.01, 0.2, true},
//     {"send_actuators", 12, 7, 25, 25, 50, 0.7, 0.03, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1", 0, 7, 15, 15, 40, 0.6, 0.02, 0.4, true},
//     {"jfdctint", 0, 5, 10, 10, 30, 0.7, 0.03, 0.5, true},
//     {"adpcm", 2, 6, 20, 20, 25, 0.5, 0.02, 0.3, true},
//     {"cnt", 1, 3, 10, 10, 10, 0.4, 0.01, 0.2, true},
//     {"compress", 3, 8, 25, 25, 50, 0.8, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1", 0, 2, 5, 5, 30, 0.6, 0.02, 0.5, true},
//     {"T2", 1, 3, 8, 8, 10, 0.5, 0.03, 0.4, true},
//     {"T3", 2, 4, 10, 10, 80, 0.7, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 15, 15, 25, 0.5, 0.02, 0.3, true},
//     {"crc", 0, 5, 20, 20, 30, 0.6, 0.02, 0.4, true},
//     {"ludcmp", 2, 10, 30, 30, 60, 0.7, 0.03, 0.5, true},
//     {"qsort", 1, 8, 20, 20, 50, 0.8, 0.02, 0.6, true},
//     {"matmult", 3, 12, 25, 25, 70, 0.9, 0.03, 0.7, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
//     {"sensor_fusion", 0, 6, 20, 20, 35, 0.7, 0.03, 0.4, true},
//     {"fft_task", 5, 12, 50, 50, 60, 0.5, 0.02, 0.6, true},
//     {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
//     {"path_planning", 10, 15, 50, 50, 70, 0.8, 0.02, 0.7, true}
// };

static inline double compute_energy_j(const TaskConfig_t *d)
{
    if (!d)
        return 0.0;
    double t_s = ((double)d->C_ms) / 1000.0;
    double e = P_ACTIVE_W * t_s + MEM_ENERGY_J_PER_KB * (double)d->M_kB;

    double model = d->alpha * (double)d->C_ms + d->beta * (double)d->M_kB + d->gamma;
    e *= (1.0 + model * 0.001);

    printf("Task Name: %s, Power: %d uj\r\n", d->name, (unsigned int)(e*1000*1000));

    return e;
}

#define NUM_TASKS (sizeof(taskConfigs) / sizeof(TaskConfig_t))

static TaskHandle_t taskHandles[NUM_TASKS];
static volatile uint32_t deadlineMisses[NUM_TASKS] = {0};
static volatile uint32_t totalDeadlineMisses = 0;
static volatile TickType_t absoluteDeadline[NUM_TASKS] = {0};
static volatile BaseType_t taskActive[NUM_TASKS] = {0};

typedef struct {
    uint32_t taskIndex;
    TickType_t startTime;
    TickType_t deadline;
} TaskContext_t;

static void prvWorkTask(void *pvParameters) {
    TaskContext_t *ctx = (TaskContext_t *)pvParameters;
    uint32_t taskIndex = ctx->taskIndex;
    const TaskConfig_t *config = &taskConfigs[taskIndex];
    TickType_t nextWakeTime;
    TickType_t instanceStartTime;
    TickType_t instanceCompleteTime;
    volatile uint32_t workCounter;
    
    vTaskDelay(pdMS_TO_TICKS(config->release_ms));
    
    nextWakeTime = xTaskGetTickCount();
    
    for (;;) {
        // printf("Task %s executing at tick %u\r\n", config->name, (unsigned int)instanceStartTime);

        instanceStartTime = xTaskGetTickCount();
        
        if (taskActive[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        
        taskActive[taskIndex] = pdTRUE;
        absoluteDeadline[taskIndex] = instanceStartTime + pdMS_TO_TICKS(config->deadline_ms);
        
        workCounter = 0;
        vTaskResetZygardeBudget(taskHandles[taskIndex]);

        while (vTaskGetZygardeBudget(taskHandles[taskIndex]) < config->C_ms) {
            workCounter++;
            __asm volatile ("nop");
            
        }
        
        instanceCompleteTime = xTaskGetTickCount();
        taskActive[taskIndex] = pdFALSE;
        
        if (instanceCompleteTime > absoluteDeadline[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        vTaskDelayUntil(&nextWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}

static void prvMonitorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t lastPrintTime = xTaskGetTickCount();
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));

        for (uint32_t i = 0; i < NUM_TASKS; i++) {
            
            if (absoluteDeadline[i] >= 0) {

                TickType_t deadline = absoluteDeadline[i] + pdMS_TO_TICKS(taskConfigs[i].period_ms);
                TickType_t currentTime = xTaskGetTickCount();
                // printf("absDeadLine: %u, deadline: %u, currentTime: %u\r\n", absoluteDeadline[i], deadline, currentTime);

                if (currentTime >= deadline) {
                    // printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n");
                    deadlineMisses[i]++;
                    totalDeadlineMisses++;
                    absoluteDeadline[i] = deadline;
                }
            }
        }
        
        if ((xTaskGetTickCount() - lastPrintTime) >= pdMS_TO_TICKS(1000)) {
            lastPrintTime = xTaskGetTickCount();
            printf("-----Iter i: %d, Deadline Misses: %u\r\n", iter++, (unsigned long)totalDeadlineMisses);
        }
    }
}

void main_blinky(void) {
    static TaskContext_t contexts[NUM_TASKS];
    
    for (uint32_t i = 0; i < NUM_TASKS; i++) {
        contexts[i].taskIndex = i;
        contexts[i].startTime = 0;
        contexts[i].deadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        xTaskCreate(prvWorkTask,
                    taskConfigs[i].name,
                    configMINIMAL_STACK_SIZE * 2,
                    &contexts[i],
                    mainTASK_PRIORITY,
                    &taskHandles[i]);
        
        UBaseType_t weight = (UBaseType_t)(5);
        TickType_t request = pdMS_TO_TICKS(taskConfigs[i].C_ms);
        double power = (double)(compute_energy_j(&taskConfigs[i]));
        
        vTaskSetZygardeWeight(taskHandles[i], 0.01/power);
        vTaskSetZygardeRequest(taskHandles[i], request);
        vTaskSetZygardePowerConsumption(taskHandles[i], power);
        vTaskSetZygardeUtility(taskHandles[i], 0.001/power);
    }

    vTaskSetEnergyParameters(configSYSTEM_POWERON_WATERMARK, POWER_IN_SYSTEM);    
    
    xTaskCreate(prvMonitorTask,
                "Monitor",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainTASK_PRIORITY + 1,
                NULL);
    
    vTaskStartScheduler();
    
    for (;;) {}
}

#endif











#if ( configUSE_OURSCHED_EEVDF_SCHEDULER == 1 )

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define mainTASK_PRIORITY (tskIDLE_PRIORITY + 1)

// typedef struct {
//     char name[10];
//     uint32_t release_ms;
//     uint32_t C_ms;
//     uint32_t period_ms;
//     uint32_t deadline_ms;
//     uint32_t M_kB;
//     double alpha;
//     double beta;
//     double gamma;
// } TaskConfig_t;

int iter = 0;

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 20, 20, 25, 0.5, 0.02, 0.3},
//     {"crc", 0, 5, 25, 25, 30, 0.6, 0.02, 0.4},
//     {"ludcmp", 2, 10, 40, 40, 60, 0.7, 0.03, 0.5},
//     {"qsort", 1, 8, 35, 35, 50, 0.8, 0.02, 0.6},
//     {"matmult", 3, 12, 45, 45, 70, 0.9, 0.03, 0.7}
// };
// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control",0,10,50,50,40,0.6,0.02,0.5},
//     {"sensor_fusion",0,6,30,30,35,0.7,0.03,0.4},
//     {"fft_task",5,12,60,60,60,0.5,0.02,0.6},
//     {"filter",0,5,25,25,20,0.4,0.01,0.3},
//     {"path_planning",10,15,70,70,70,0.8,0.02,0.7}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1",0,2,8,8,30,0.6,0.02,0.5},
//     {"T2",1,3,10,10,10,0.5,0.03,0.4},
//     {"T3",2,4,12,12,80,0.7,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1",0,7,25,25,40,0.6,0.02,0.4},
//     {"jfdctint",0,5,20,20,30,0.7,0.03,0.5},
//     {"adpcm",2,6,30,30,25,0.5,0.02,0.3},
//     {"cnt",1,3,15,15,10,0.4,0.01,0.2},
//     {"compress",3,8,35,35,50,0.8,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors",0,3,15,15,20,0.4,0.01,0.3},
//     {"process_data",2,5,25,25,40,0.6,0.02,0.5},
//     {"update_control",6,4,30,30,30,0.5,0.02,0.4},
//     {"log_results",8,6,40,40,25,0.4,0.01,0.2},
//     {"send_actuators",12,7,35,35,50,0.7,0.03,0.6},
// };




// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
//     {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
//     {"update_control", 6, 4, 20, 20, 30, 0.5, 0.02, 0.4, true},
//     {"log_results", 8, 6, 25, 25, 25, 0.4, 0.01, 0.2, true},
//     {"send_actuators", 12, 7, 25, 25, 50, 0.7, 0.03, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1", 0, 7, 15, 15, 40, 0.6, 0.02, 0.4, true},
//     {"jfdctint", 0, 5, 10, 10, 30, 0.7, 0.03, 0.5, true},
//     {"adpcm", 2, 6, 20, 20, 25, 0.5, 0.02, 0.3, true},
//     {"cnt", 1, 3, 10, 10, 10, 0.4, 0.01, 0.2, true},
//     {"compress", 3, 8, 25, 25, 50, 0.8, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1", 0, 2, 5, 5, 30, 0.6, 0.02, 0.5, true},
//     {"T2", 1, 3, 8, 8, 10, 0.5, 0.03, 0.4, true},
//     {"T3", 2, 4, 10, 10, 80, 0.7, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 15, 15, 25, 0.5, 0.02, 0.3, true},
//     {"crc", 0, 5, 20, 20, 30, 0.6, 0.02, 0.4, true},
//     {"ludcmp", 2, 10, 30, 30, 60, 0.7, 0.03, 0.5, true},
//     {"qsort", 1, 8, 20, 20, 50, 0.8, 0.02, 0.6, true},
//     {"matmult", 3, 12, 25, 25, 70, 0.9, 0.03, 0.7, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
//     {"sensor_fusion", 0, 6, 20, 20, 35, 0.7, 0.03, 0.4, true},
//     {"fft_task", 5, 12, 50, 50, 60, 0.5, 0.02, 0.6, true},
//     {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
//     {"path_planning", 10, 15, 50, 50, 70, 0.8, 0.02, 0.7, true}
// };

static inline double compute_energy_j(const TaskConfig_t *d)
{
    if (!d)
        return 0.0;
    double t_s = ((double)d->C_ms) / 1000.0;
    double e = P_ACTIVE_W * t_s + MEM_ENERGY_J_PER_KB * (double)d->M_kB;

    double model = d->alpha * (double)d->C_ms + d->beta * (double)d->M_kB + d->gamma;
    e *= (1.0 + model * 0.001);

    printf("Task Name: %s, Power: %d uj\r\n", d->name, (unsigned int)(e*1000*1000));

    return e;
}

#define NUM_TASKS (sizeof(taskConfigs) / sizeof(TaskConfig_t))

static TaskHandle_t taskHandles[NUM_TASKS];
static volatile uint32_t deadlineMisses[NUM_TASKS] = {0};
static volatile uint32_t totalDeadlineMisses = 0;
static volatile TickType_t absoluteDeadline[NUM_TASKS] = {0};
static volatile BaseType_t taskActive[NUM_TASKS] = {0};

typedef struct {
    uint32_t taskIndex;
    TickType_t startTime;
    TickType_t deadline;
} TaskContext_t;

static void prvWorkTask(void *pvParameters) {
    TaskContext_t *ctx = (TaskContext_t *)pvParameters;
    uint32_t taskIndex = ctx->taskIndex;
    const TaskConfig_t *config = &taskConfigs[taskIndex];
    TickType_t nextWakeTime;
    TickType_t instanceStartTime;
    TickType_t instanceCompleteTime;
    volatile uint32_t workCounter;
    
    vTaskDelay(pdMS_TO_TICKS(config->release_ms));
    
    nextWakeTime = xTaskGetTickCount();
    
    for (;;) {
        // printf("Task %s executing at tick %u\r\n", config->name, (unsigned int)instanceStartTime);

        instanceStartTime = xTaskGetTickCount();
        
        if (taskActive[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        
        taskActive[taskIndex] = pdTRUE;
        absoluteDeadline[taskIndex] = instanceStartTime + pdMS_TO_TICKS(config->deadline_ms);
        
        workCounter = 0;
        vTaskResetOurSchedEEVDFBudget(taskHandles[taskIndex]);

        while (vTaskGetOurSchedEEVDFBudget(taskHandles[taskIndex]) < config->C_ms) {
            workCounter++;
            __asm volatile ("nop");
            
        }
        
        instanceCompleteTime = xTaskGetTickCount();
        taskActive[taskIndex] = pdFALSE;
        
        if (instanceCompleteTime > absoluteDeadline[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        vTaskDelayUntil(&nextWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}

static void prvMonitorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t lastPrintTime = xTaskGetTickCount();
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));

        for (uint32_t i = 0; i < NUM_TASKS; i++) {
            
            if (absoluteDeadline[i] >= 0) {

                TickType_t deadline = absoluteDeadline[i] + pdMS_TO_TICKS(taskConfigs[i].period_ms);
                TickType_t currentTime = xTaskGetTickCount();
                // printf("absDeadLine: %u, deadline: %u, currentTime: %u\r\n", absoluteDeadline[i], deadline, currentTime);

                if (currentTime >= deadline) {
                    // printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n");
                    deadlineMisses[i]++;
                    totalDeadlineMisses++;
                    absoluteDeadline[i] = deadline;
                }
            }
        }

        
        if ((xTaskGetTickCount() - lastPrintTime) >= pdMS_TO_TICKS(1000)) {
            lastPrintTime = xTaskGetTickCount();
            printf("-----Iter i: %d, Deadline Misses: %u\r\n", iter++, (unsigned long)totalDeadlineMisses);
        }
    }
}

void main_blinky(void) {
    static TaskContext_t contexts[NUM_TASKS];
    
    for (uint32_t i = 0; i < NUM_TASKS; i++) {
        contexts[i].taskIndex = i;
        contexts[i].startTime = 0;
        contexts[i].deadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        xTaskCreate(prvWorkTask,
                    taskConfigs[i].name,
                    configMINIMAL_STACK_SIZE * 2,
                    &contexts[i],
                    mainTASK_PRIORITY,
                    &taskHandles[i]);
        
        UBaseType_t weight = (UBaseType_t)(5);
        TickType_t request = pdMS_TO_TICKS(taskConfigs[i].C_ms);
        double power = (double)(compute_energy_j(&taskConfigs[i]));
        
        vTaskSetOurSchedEEVDFWeight(taskHandles[i], 0.01/power);
        vTaskSetOurSchedEEVDFRequest(taskHandles[i], request);
        vTaskSetOurSchedEEVDFPowerConsumption(taskHandles[i], power);
        vTaskSetOurSchedEEVDFUtility(taskHandles[i], 0.001/power);
    }

    vTaskSetEnergyParameters(configSYSTEM_POWERON_WATERMARK, POWER_IN_SYSTEM);    
    
    xTaskCreate(prvMonitorTask,
                "Monitor",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainTASK_PRIORITY + 1,
                NULL);
    
    vTaskStartScheduler();
    
    for (;;) {}
}

#endif























#if ( configUSE_OURSCHED_EDF_SCHEDULER == 1 )

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define mainTASK_PRIORITY (tskIDLE_PRIORITY + 1)

// typedef struct {
//     char name[10];
//     uint32_t release_ms;
//     uint32_t C_ms;
//     uint32_t period_ms;
//     uint32_t deadline_ms;
//     uint32_t M_kB;
//     double alpha;
//     double beta;
//     double gamma;
// } TaskConfig_t;

int iter = 0;

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 20, 20, 25, 0.5, 0.02, 0.3},
//     {"crc", 0, 5, 25, 25, 30, 0.6, 0.02, 0.4},
//     {"ludcmp", 2, 10, 40, 40, 60, 0.7, 0.03, 0.5},
//     {"qsort", 1, 8, 35, 35, 50, 0.8, 0.02, 0.6},
//     {"matmult", 3, 12, 45, 45, 70, 0.9, 0.03, 0.7}
// };
// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control",0,10,50,50,40,0.6,0.02,0.5},
//     {"sensor_fusion",0,6,30,30,35,0.7,0.03,0.4},
//     {"fft_task",5,12,60,60,60,0.5,0.02,0.6},
//     {"filter",0,5,25,25,20,0.4,0.01,0.3},
//     {"path_planning",10,15,70,70,70,0.8,0.02,0.7}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1",0,2,8,8,30,0.6,0.02,0.5},
//     {"T2",1,3,10,10,10,0.5,0.03,0.4},
//     {"T3",2,4,12,12,80,0.7,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1",0,7,25,25,40,0.6,0.02,0.4},
//     {"jfdctint",0,5,20,20,30,0.7,0.03,0.5},
//     {"adpcm",2,6,30,30,25,0.5,0.02,0.3},
//     {"cnt",1,3,15,15,10,0.4,0.01,0.2},
//     {"compress",3,8,35,35,50,0.8,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors",0,3,15,15,20,0.4,0.01,0.3},
//     {"process_data",2,5,25,25,40,0.6,0.02,0.5},
//     {"update_control",6,4,30,30,30,0.5,0.02,0.4},
//     {"log_results",8,6,40,40,25,0.4,0.01,0.2},
//     {"send_actuators",12,7,35,35,50,0.7,0.03,0.6},
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
//     {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
//     {"update_control", 6, 4, 20, 20, 30, 0.5, 0.02, 0.4, true},
//     {"log_results", 8, 6, 25, 25, 25, 0.4, 0.01, 0.2, true},
//     {"send_actuators", 12, 7, 25, 25, 50, 0.7, 0.03, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1", 0, 7, 15, 15, 40, 0.6, 0.02, 0.4, true},
//     {"jfdctint", 0, 5, 10, 10, 30, 0.7, 0.03, 0.5, true},
//     {"adpcm", 2, 6, 20, 20, 25, 0.5, 0.02, 0.3, true},
//     {"cnt", 1, 3, 10, 10, 10, 0.4, 0.01, 0.2, true},
//     {"compress", 3, 8, 25, 25, 50, 0.8, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1", 0, 2, 5, 5, 30, 0.6, 0.02, 0.5, true},
//     {"T2", 1, 3, 8, 8, 10, 0.5, 0.03, 0.4, true},
//     {"T3", 2, 4, 10, 10, 80, 0.7, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 15, 15, 25, 0.5, 0.02, 0.3, true},
//     {"crc", 0, 5, 20, 20, 30, 0.6, 0.02, 0.4, true},
//     {"ludcmp", 2, 10, 30, 30, 60, 0.7, 0.03, 0.5, true},
//     {"qsort", 1, 8, 20, 20, 50, 0.8, 0.02, 0.6, true},
//     {"matmult", 3, 12, 25, 25, 70, 0.9, 0.03, 0.7, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
//     {"sensor_fusion", 0, 6, 20, 20, 35, 0.7, 0.03, 0.4, true},
//     {"fft_task", 5, 12, 50, 50, 60, 0.5, 0.02, 0.6, true},
//     {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
//     {"path_planning", 10, 15, 50, 50, 70, 0.8, 0.02, 0.7, true}
// };




static inline double compute_energy_j(const TaskConfig_t *d)
{
    if (!d)
        return 0.0;
    double t_s = ((double)d->C_ms) / 1000.0;
    double e = P_ACTIVE_W * t_s + MEM_ENERGY_J_PER_KB * (double)d->M_kB;

    double model = d->alpha * (double)d->C_ms + d->beta * (double)d->M_kB + d->gamma;
    e *= (1.0 + model * 0.001);

    printf("Task Name: %s, Power: %d uj\r\n", d->name, (unsigned int)(e*1000*1000));

    return e;
}

#define NUM_TASKS (sizeof(taskConfigs) / sizeof(TaskConfig_t))

static TaskHandle_t taskHandles[NUM_TASKS];
static volatile uint32_t deadlineMisses[NUM_TASKS] = {0};
static volatile uint32_t totalDeadlineMisses = 0;
static volatile TickType_t absoluteDeadline[NUM_TASKS] = {0};
static volatile BaseType_t taskActive[NUM_TASKS] = {0};

typedef struct {
    uint32_t taskIndex;
    TickType_t startTime;
    TickType_t deadline;
} TaskContext_t;

static void prvWorkTask(void *pvParameters) {
    TaskContext_t *ctx = (TaskContext_t *)pvParameters;
    uint32_t taskIndex = ctx->taskIndex;
    const TaskConfig_t *config = &taskConfigs[taskIndex];
    TickType_t nextWakeTime;
    TickType_t instanceStartTime;
    TickType_t instanceCompleteTime;
    volatile uint32_t workCounter;
    
    vTaskDelay(pdMS_TO_TICKS(config->release_ms));
    
    nextWakeTime = xTaskGetTickCount();
    
    for (;;) {
        instanceStartTime = xTaskGetTickCount();
        
        if (taskActive[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        
        taskActive[taskIndex] = pdTRUE;
        absoluteDeadline[taskIndex] = instanceStartTime + pdMS_TO_TICKS(config->deadline_ms);
        
        vTaskSetOurSchedEDFDeadline(taskHandles[taskIndex], absoluteDeadline[taskIndex]);
        
        workCounter = 0;
        vTaskResetOurSchedEDFBudget(taskHandles[taskIndex]);

        // printf("Task %s executing at tick %u\r\n", config->name, (unsigned int)instanceStartTime);


        while (vTaskGetOurSchedEDFBudget(taskHandles[taskIndex]) < config->C_ms) {
            workCounter++;
            __asm volatile ("nop");
        }
        
        instanceCompleteTime = xTaskGetTickCount();
        taskActive[taskIndex] = pdFALSE;
        
        if (instanceCompleteTime > absoluteDeadline[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        vTaskDelayUntil(&nextWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}

static void prvMonitorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t lastPrintTime = xTaskGetTickCount();
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));

        for (uint32_t i = 0; i < NUM_TASKS; i++) {
            
            if (absoluteDeadline[i] >= 0) {

                TickType_t deadline = absoluteDeadline[i] + pdMS_TO_TICKS(taskConfigs[i].period_ms);
                TickType_t currentTime = xTaskGetTickCount();

                if (currentTime >= deadline) {
                    deadlineMisses[i]++;
                    totalDeadlineMisses++;
                    absoluteDeadline[i] = deadline;
                }
            }
        }
        
        if ((xTaskGetTickCount() - lastPrintTime) >= pdMS_TO_TICKS(1000)) {
            lastPrintTime = xTaskGetTickCount();
            printf("-----Iter i: %d, Deadline Misses: %u\r\n", iter++, (unsigned long)totalDeadlineMisses);
        }
    }
}

void main_blinky(void) {
    static TaskContext_t contexts[NUM_TASKS];
    
    for (uint32_t i = 0; i < NUM_TASKS; i++) {
        contexts[i].taskIndex = i;
        contexts[i].startTime = 0;
        contexts[i].deadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        xTaskCreate(prvWorkTask,
                    taskConfigs[i].name,
                    configMINIMAL_STACK_SIZE * 2,
                    &contexts[i],
                    mainTASK_PRIORITY,
                    &taskHandles[i]);

        double power = (double)(compute_energy_j(&taskConfigs[i]));
        TickType_t request = pdMS_TO_TICKS(taskConfigs[i].C_ms);
        TickType_t initialDeadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        vTaskSetOurSchedEDFDeadline(taskHandles[i], initialDeadline);
        vTaskSetOurSchedEDFPowerConsumption(taskHandles[i], power);
        vTaskSetOurSchedEDFRequest(taskHandles[i], request);

        vTaskSetOurSchedEDFWeight(taskHandles[i], 5);
        vTaskSetOurSchedEDFUtility(taskHandles[i], 0.001/power);

    }

    vTaskSetEnergyParameters(configSYSTEM_POWERON_WATERMARK, POWER_IN_SYSTEM);    
    xTaskCreate(prvMonitorTask,
                "Monitor",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainTASK_PRIORITY + 1,
                NULL);
    
    vTaskStartScheduler();
    
    for (;;) {}
}

#endif






































#if ( configUSE_EEDH_SCHEDULER == 1 )

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define mainTASK_PRIORITY (tskIDLE_PRIORITY + 1)

// typedef struct {
//     char name[10];
//     uint32_t release_ms;
//     uint32_t C_ms;
//     uint32_t period_ms;
//     uint32_t deadline_ms;
//     uint32_t M_kB;
//     double alpha;
//     double beta;
//     double gamma;
// } TaskConfig_t;

int iter = 0;

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 20, 20, 25, 0.5, 0.02, 0.3},
//     {"crc", 0, 5, 25, 25, 30, 0.6, 0.02, 0.4},
//     {"ludcmp", 2, 10, 40, 40, 60, 0.7, 0.03, 0.5},
//     {"qsort", 1, 8, 35, 35, 50, 0.8, 0.02, 0.6},
//     {"matmult", 3, 12, 45, 45, 70, 0.9, 0.03, 0.7}
// };
// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control",0,10,50,50,40,0.6,0.02,0.5},
//     {"sensor_fusion",0,6,30,30,35,0.7,0.03,0.4},
//     {"fft_task",5,12,60,60,60,0.5,0.02,0.6},
//     {"filter",0,5,25,25,20,0.4,0.01,0.3},
//     {"path_planning",10,15,70,70,70,0.8,0.02,0.7}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1",0,2,8,8,30,0.6,0.02,0.5},
//     {"T2",1,3,10,10,10,0.5,0.03,0.4},
//     {"T3",2,4,12,12,80,0.7,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1",0,7,25,25,40,0.6,0.02,0.4},
//     {"jfdctint",0,5,20,20,30,0.7,0.03,0.5},
//     {"adpcm",2,6,30,30,25,0.5,0.02,0.3},
//     {"cnt",1,3,15,15,10,0.4,0.01,0.2},
//     {"compress",3,8,35,35,50,0.8,0.02,0.6}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors",0,3,15,15,20,0.4,0.01,0.3},
//     {"process_data",2,5,25,25,40,0.6,0.02,0.5},
//     {"update_control",6,4,30,30,30,0.5,0.02,0.4},
//     {"log_results",8,6,40,40,25,0.4,0.01,0.2},
//     {"send_actuators",12,7,35,35,50,0.7,0.03,0.6},
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
//     {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
//     {"update_control", 6, 4, 20, 20, 30, 0.5, 0.02, 0.4, true},
//     {"log_results", 8, 6, 25, 25, 25, 0.4, 0.01, 0.2, true},
//     {"send_actuators", 12, 7, 25, 25, 50, 0.7, 0.03, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"fft1", 0, 7, 15, 15, 40, 0.6, 0.02, 0.4, true},
//     {"jfdctint", 0, 5, 10, 10, 30, 0.7, 0.03, 0.5, true},
//     {"adpcm", 2, 6, 20, 20, 25, 0.5, 0.02, 0.3, true},
//     {"cnt", 1, 3, 10, 10, 10, 0.4, 0.01, 0.2, true},
//     {"compress", 3, 8, 25, 25, 50, 0.8, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"T1", 0, 2, 5, 5, 30, 0.6, 0.02, 0.5, true},
//     {"T2", 1, 3, 8, 8, 10, 0.5, 0.03, 0.4, true},
//     {"T3", 2, 4, 10, 10, 80, 0.7, 0.02, 0.6, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"bs", 0, 4, 15, 15, 25, 0.5, 0.02, 0.3, true},
//     {"crc", 0, 5, 20, 20, 30, 0.6, 0.02, 0.4, true},
//     {"ludcmp", 2, 10, 30, 30, 60, 0.7, 0.03, 0.5, true},
//     {"qsort", 1, 8, 20, 20, 50, 0.8, 0.02, 0.6, true},
//     {"matmult", 3, 12, 25, 25, 70, 0.9, 0.03, 0.7, true}
// };

// static const TaskConfig_t taskConfigs[] = {
//     {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
//     {"sensor_fusion", 0, 6, 20, 20, 35, 0.7, 0.03, 0.4, true},
//     {"fft_task", 5, 12, 50, 50, 60, 0.5, 0.02, 0.6, true},
//     {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
//     {"path_planning", 10, 15, 50, 50, 70, 0.8, 0.02, 0.7, true}
// };




static inline double compute_energy_j(const TaskConfig_t *d)
{
    if (!d)
        return 0.0;
    double t_s = ((double)d->C_ms) / 1000.0;
    double e = P_ACTIVE_W * t_s + MEM_ENERGY_J_PER_KB * (double)d->M_kB;

    double model = d->alpha * (double)d->C_ms + d->beta * (double)d->M_kB + d->gamma;
    e *= (1.0 + model * 0.001);

    printf("Task Name: %s, Power: %d uj\r\n", d->name, (unsigned int)(e*1000*1000));

    return e;
}

#define NUM_TASKS (sizeof(taskConfigs) / sizeof(TaskConfig_t))

TaskHandle_t taskHandles[NUM_TASKS];
volatile uint32_t deadlineMisses[NUM_TASKS] = {0};
volatile uint32_t totalDeadlineMisses = 0;
volatile TickType_t absoluteDeadline[NUM_TASKS] = {0};
volatile BaseType_t taskActive[NUM_TASKS] = {0};

typedef struct {
    uint32_t taskIndex;
    TickType_t startTime;
    TickType_t deadline;
} TaskContext_t;

static void prvWorkTask(void *pvParameters) {
    TaskContext_t *ctx = (TaskContext_t *)pvParameters;
    uint32_t taskIndex = ctx->taskIndex;
    const TaskConfig_t *config = &taskConfigs[taskIndex];
    TickType_t nextWakeTime;
    TickType_t instanceStartTime;
    TickType_t instanceCompleteTime;
    volatile uint32_t workCounter;
    
    vTaskDelay(pdMS_TO_TICKS(config->release_ms));
    
    nextWakeTime = xTaskGetTickCount();
    
    for (;;) {
        instanceStartTime = xTaskGetTickCount();
        
        if (taskActive[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        
        taskActive[taskIndex] = pdTRUE;
        absoluteDeadline[taskIndex] = instanceStartTime + pdMS_TO_TICKS(config->deadline_ms);
        
        vTaskSetEEDHDeadline(taskHandles[taskIndex], absoluteDeadline[taskIndex]);
        vTaskSetEEDHPeriod(taskHandles[taskIndex], config->period_ms);
        vTaskSetEEDHReleaseTime(taskHandles[taskIndex], instanceStartTime);
        workCounter = 0;

        // printf("Task %s executing at tick %u\r\n", config->name, (unsigned int)instanceStartTime);


        vTaskResetEEDHBudget(taskHandles[taskIndex]);
        while (vTaskGetEEDHBudget(taskHandles[taskIndex]) < config->C_ms) {
            workCounter++;
            __asm volatile ("nop");
        }
        
        instanceCompleteTime = xTaskGetTickCount();
        taskActive[taskIndex] = pdFALSE;
        
        if (instanceCompleteTime > absoluteDeadline[taskIndex]) {
            deadlineMisses[taskIndex]++;
            totalDeadlineMisses++;
        }
        vTaskDelayUntil(&nextWakeTime, pdMS_TO_TICKS(config->period_ms));
    }
}

static void prvMonitorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t lastPrintTime = xTaskGetTickCount();
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));

        for (uint32_t i = 0; i < NUM_TASKS; i++) {
            
            if (absoluteDeadline[i] >= 0) {
///
                TickType_t deadline = absoluteDeadline[i] + pdMS_TO_TICKS(taskConfigs[i].period_ms);
                TickType_t currentTime = xTaskGetTickCount();

                if (currentTime >= deadline) {
                    deadlineMisses[i]++;
                    totalDeadlineMisses++;
                    absoluteDeadline[i] = deadline;
                }
            }
        }
        
        if ((xTaskGetTickCount() - lastPrintTime) >= pdMS_TO_TICKS(1000)) {
            lastPrintTime = xTaskGetTickCount();
            printf("-----Iter i: %d, Deadline Misses: %u\r\n", iter++, (unsigned long)totalDeadlineMisses);
            // printAllTaskInfo();
        }
    }
}

void main_blinky(void) {
    static TaskContext_t contexts[NUM_TASKS];
    printf("sizeof(double) = %d\n", (int)sizeof(double));

    for (uint32_t i = 0; i < NUM_TASKS; i++) {
        contexts[i].taskIndex = i;
        contexts[i].startTime = 0;
        contexts[i].deadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        
        xTaskCreate(prvWorkTask,
                    taskConfigs[i].name,
                    configMINIMAL_STACK_SIZE * 2,
                    &contexts[i],
                    mainTASK_PRIORITY,
                    &taskHandles[i]);

        double power = (double)(compute_energy_j(&taskConfigs[i]));
        TickType_t request = pdMS_TO_TICKS(taskConfigs[i].C_ms);
        TickType_t initialDeadline = pdMS_TO_TICKS(taskConfigs[i].deadline_ms);
        TickType_t period = pdMS_TO_TICKS(taskConfigs[i].period_ms);
        
        vTaskSetEEDHDeadline(taskHandles[i], initialDeadline);
        vTaskSetEEDHPowerConsumption(taskHandles[i], power);
        vTaskSetEEDHRequest(taskHandles[i], request);
        vTaskSetEEDHPeriod(taskHandles[i], period);
    }
    vTaskSetEEDHTaskHandles(taskHandles, NUM_TASKS);


    vTaskSetEnergyParameters(configSYSTEM_POWERON_WATERMARK, POWER_IN_SYSTEM);    
    
    xTaskCreate(prvMonitorTask,
                "Monitor",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainTASK_PRIORITY + 1,
                NULL);
    
    vTaskStartScheduler();
    
    for (;;) {}
}

#endif

