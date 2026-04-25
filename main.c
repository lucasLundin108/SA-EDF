/*
 * main.c
 *
 * Runner for scheduler simulation.
 *
 * Compile:
 *   gcc -O2 -std=c11 -Wall -Wextra main.c -o scheduler -lm
 *
 * Run:
 *   ./scheduler
 */

#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Hardcoded benches */

///////////////////////////////////////////////////////////////////////
//////////////////////////WorkLoads///////////////////////////////

// static TaskDef bench_sensors[] = {
//     {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
//     {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
//     {"update_control", 6, 4, 20, 20, 30, 0.5, 0.02, 0.4, true},
//     {"log_results", 8, 6, 25, 25, 25, 0.4, 0.01, 0.2, true},
//     {"send_actuators", 12, 7, 25, 25, 50, 0.7, 0.03, 0.6, true}};

// static TaskDef bench_media[] = {
//     {"fft1", 0, 7, 15, 15, 40, 0.6, 0.02, 0.4, true},
//     {"jfdctint", 0, 5, 10, 10, 30, 0.7, 0.03, 0.5, true},
//     {"adpcm", 2, 6, 20, 20, 25, 0.5, 0.02, 0.3, true},
//     {"cnt", 1, 3, 10, 10, 10, 0.4, 0.01, 0.2, true},
//     {"compress", 3, 8, 25, 25, 50, 0.8, 0.02, 0.6, true}};

// static TaskDef bench_small[] = {
//     {"T1", 0, 2, 5, 5, 30, 0.6, 0.02, 0.5, true},
//     {"T2", 1, 3, 8, 8, 10, 0.5, 0.03, 0.4, true},
//     {"T3", 2, 4, 10, 10, 80, 0.7, 0.02, 0.6, true}};

// static TaskDef bench_compute[] = {
//     {"bs", 0, 4, 15, 15, 25, 0.5, 0.02, 0.3, true},
//     {"crc", 0, 5, 20, 20, 30, 0.6, 0.02, 0.4, true},
//     {"ludcmp", 2, 10, 30, 30, 60, 0.7, 0.03, 0.5, true},
//     {"qsort", 1, 8, 20, 20, 50, 0.8, 0.02, 0.6, true},
//     {"matmult", 3, 12, 25, 25, 70, 0.9, 0.03, 0.7, true}};

// static TaskDef bench_robotics[] = {
//     {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
//     {"sensor_fusion", 0, 6, 20, 20, 35, 0.7, 0.03, 0.4, true},
//     {"fft_task", 5, 12, 50, 50, 60, 0.5, 0.02, 0.6, true},
//     {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
//     {"path_planning", 10, 15, 50, 50, 70, 0.8, 0.02, 0.7, true}};

// EO main

static TaskDef bench_singlecore_overload[] = {
    {"read_sensors", 0, 3, 10, 10, 20, 0.4, 0.01, 0.3, true},
    {"process_data", 2, 5, 15, 15, 40, 0.6, 0.02, 0.5, true},
    {"update_control", 6, 4, 20, 6, 30, 0.5, 0.02, 0.4, true},
    {"log_results", 8, 5, 25, 25, 25, 0.4, 0.01, 0.2, true},
    {"send_actuators", 12, 3, 25, 8, 50, 0.7, 0.03, 0.6, true}};

static TaskDef bench_sof_realtime[] = {
    {"fft1", 0, 4, 15, 15, 40, 0.6, 0.02, 0.4, true},
    {"jfdctint", 0, 3, 10, 10, 30, 0.7, 0.03, 0.5, true},
    {"adpcm", 2, 4, 20, 20, 25, 0.5, 0.02, 0.3, true},
    {"cnt", 1, 2, 10, 10, 10, 0.4, 0.01, 0.2, true},
    {"compress", 3, 4, 25, 7, 50, 0.8, 0.02, 0.6, true}};

static TaskDef bench_bounded_utilization[] = {
    {"T1", 0, 4, 7, 7, 30, 0.6, 0.02, 0.5, true},
    {"T2", 1, 3, 9, 9, 10, 0.5, 0.03, 0.4, true},
    {"T3", 2, 4, 30, 6, 80, 0.7, 0.02, 0.6, true}};

static TaskDef bench_cpuIntensive_Starvation[] = {
    {"bs", 0, 6, 15, 15, 25, 0.5, 0.02, 0.3, true},
    {"crc", 0, 4, 20, 20, 30, 0.6, 0.02, 0.4, true},
    {"ludcmp", 2, 6, 30, 30, 60, 0.7, 0.03, 0.5, true},
    {"qsort", 1, 4, 20, 20, 50, 0.8, 0.02, 0.6, true},
    {"matmult", 3, 6, 50, 10, 70, 0.9, 0.03, 0.7, true}};

static TaskDef bench_realtime_multirate[] = {
    {"motor_control", 0, 10, 40, 40, 40, 0.6, 0.02, 0.5, true},
    {"sensor_fusion", 0, 7, 20, 20, 35, 0.7, 0.03, 0.4, true},
    {"fft_task", 5, 8, 50, 50, 60, 0.5, 0.02, 0.6, true},
    {"filter", 0, 5, 15, 15, 20, 0.4, 0.01, 0.3, true},
    {"path_planning", 10, 6, 60, 12, 70, 0.8, 0.02, 0.7, true}};

///////////////////////////////////////////////////////////////////////
//////////////////////////WorkLoads///////////////////////////////

///////////////////////////////////////////////////////////////////////
///////////////////////////// workloads ///////////////////////////

TaskDef bench_MixedCriticalityControl[] = {
    {"IMU_Read", 0, 1, 5, 3, 8, 0.8, 0.01, 0.7, true},
    {"Safety_Chk", 0, 1, 10, 5, 16, 0.8, 0.01, 0.7, true},

    {"Motor_A", 0, 2, 12, 6, 16, 0.7, 0.02, 0.5, true},
    {"Motor_B", 0, 2, 12, 6, 16, 0.7, 0.02, 0.5, true},
    {"Motor_C", 0, 2, 12, 6, 16, 0.7, 0.02, 0.5, true},
    {"Motor_D", 0, 2, 12, 6, 16, 0.7, 0.02, 0.5, true},

    {"IK_Solver", 1, 6, 25, 12, 64, 0.9, 0.04, 0.7, true},

    {"Path_Plan", 5, 12, 120, 60, 512, 0.3, 0.08, 0.3, true},

    {"Obj_Detect", 0, 20, 200, 100, 1024, 0.4, 0.10, 0.4, true},

    {"Telemetry", 10, 4, 60, 60, 64, 0.2, 0.06, 0.2, true},
    {"UI_Refresh", 15, 3, 50, 50, 256, 0.2, 0.06, 0.2, true},

    {"Bat_Mon", 0, 1, 100, 100, 8, 0.2, 0.01, 0.15, true},
};

TaskDef bench_harmonic_periods[] = {
    {"T10", 0, 3, 10, 5, 32, 0.6, 0.02, 0.4, true},
    {"T20", 0, 5, 20, 10, 32, 0.6, 0.02, 0.4, true},
    {"T40", 0, 7, 40, 20, 32, 0.6, 0.02, 0.4, true},
    {"T80", 0, 10, 80, 40, 32, 0.6, 0.02, 0.4, true},
    {"T160", 0, 16, 160, 80, 32, 0.6, 0.02, 0.4, true},
    {"T320", 0, 24, 320, 160, 32, 0.6, 0.02, 0.4, true},
};

TaskDef bench_non_harmonic_jitter_sporadic[] = {
    {"T_7", 0, 2, 7, 3, 16, 0.7, 0.02, 0.5, true},
    {"T_11", 0, 3, 11, 5, 16, 0.7, 0.02, 0.5, true},
    {"T_13", 0, 4, 13, 6, 16, 0.7, 0.02, 0.5, true},
    {"T_17", 0, 5, 17, 8, 16, 0.7, 0.02, 0.5, true},
    {"T_19", 0, 6, 19, 9, 16, 0.7, 0.02, 0.5, true},

    {"Sporadic", 5, 10, 1000, 40, 64, 0.2, 0.08, 0.2, true},
    {"Bg", 0, 1, 60, 60, 8, 0.2, 0.01, 0.1, true},
};
TaskDef bench_MixedCriticality_HardSoftTaskInteraction[] = {
    {"Crit_1", 0, 2, 6, 3, 16, 0.9, 0.02, 0.7, true},
    {"Crit_2", 0, 3, 12, 6, 16, 0.9, 0.02, 0.7, true},
    {"Crit_3", 1, 2, 10, 5, 16, 0.9, 0.02, 0.7, true},

    {"Loose_1", 0, 5, 60, 50, 64, 0.3, 0.03, 0.2, true},
    {"Loose_2", 0, 5, 60, 50, 64, 0.3, 0.03, 0.2, true},

    {"Sporadic", 10, 10, 1000, 40, 128, 0.2, 0.08, 0.2, true},

    {"Bg_1", 0, 1, 30, 30, 8, 0.2, 0.01, 0.1, true},
    {"Bg_2", 0, 1, 30, 30, 8, 0.2, 0.01, 0.1, true},
    {"Bg_3", 0, 1, 30, 30, 8, 0.2, 0.01, 0.1, true},
};

TaskDef bench_high_leakage[] = {
    {"Leak_A", 0, 5, 25, 12, 32, 0.2, 0.50, 0.2, true},
    {"Leak_B", 0, 5, 25, 12, 32, 0.2, 0.50, 0.2, true},

    {"Active_A", 0, 2, 10, 5, 16, 0.9, 0.01, 0.8, true},
    {"Active_B", 0, 2, 10, 5, 16, 0.9, 0.01, 0.8, true},

    {"Sporadic", 5, 8, 500, 30, 64, 0.3, 0.10, 0.3, true},
    {"Bg", 0, 1, 60, 60, 8, 0.2, 0.01, 0.1, true},
};

TaskDef bench_sporadic_overload[] = {
    {"P1", 0, 5, 20, 20, 32, 0.9, 0.02, 0.5, true},
    {"P2", 0, 5, 20, 20, 32, 0.9, 0.02, 0.5, true},
    {"P3", 0, 5, 20, 20, 32, 0.7, 0.02, 0.5, true},
    {"P4", 0, 5, 20, 20, 32, 0.7, 0.02, 0.5, true},
    {"P5", 0, 5, 20, 20, 32, 0.7, 0.02, 0.5, true},
    {"P6", 0, 5, 20, 20, 32, 0.7, 0.02, 0.5, true},

    {"S_Over_H1", 10, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},
    {"S_Over_H2", 10, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},
    {"S_Over_H3", 10, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},
    {"S_Over_H4", 10, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},
    {"S_Over_H5", 30, 8, 80, 20, 64, 1.0, 0.08, 0.8, true},

    {"S_Over_L1", 20, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
    {"S_Over_L2", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
    {"S_Over_L3", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
    {"S_Over_L4", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
    {"S_Over_L5", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
    {"S_Over_L6", 40, 10, 200, 200, 16, 0.2, 0.08, 0.2, true},
    //

    {"Bg", 0, 2, 100, 100, 8, 0.1, 0.01, 0.1, true},
};

TaskDef bench_ultimate_stress[] = {
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

///////////////////////////////////////////////////////////////////////
///////////////////////////// workloads ///////////////////////////

typedef struct
{
    const char *name;
    TaskDef *defs;
    int ndefs;
} BenchInfo;

int main(void)
{

    BenchInfo benches[] = {
        {"bench_singlecore_overload", bench_singlecore_overload,
         sizeof(bench_singlecore_overload) / sizeof(TaskDef)},
        {"bench_sof_realtime", bench_sof_realtime,
         sizeof(bench_sof_realtime) / sizeof(TaskDef)},
        {"bench_bounded_utilization", bench_bounded_utilization,
         sizeof(bench_bounded_utilization) / sizeof(TaskDef)},
        {"bench_cpuIntensive_Starvation", bench_cpuIntensive_Starvation,
         sizeof(bench_cpuIntensive_Starvation) / sizeof(TaskDef)},
        {"bench_realtime_multirate", bench_realtime_multirate,
         sizeof(bench_realtime_multirate) / sizeof(TaskDef)},
        {"bench_MixedCriticalityControl", bench_MixedCriticalityControl,
         sizeof(bench_MixedCriticalityControl) / sizeof(TaskDef)},
        {"bench_harmonic_periods", bench_harmonic_periods,
         sizeof(bench_harmonic_periods) / sizeof(TaskDef)},
        {"bench_non_harmonic_jitter_sporadic", bench_non_harmonic_jitter_sporadic,
         sizeof(bench_non_harmonic_jitter_sporadic) / sizeof(TaskDef)},
        {"bench_MixedCriticality_HardSoftTaskInteraction", bench_MixedCriticality_HardSoftTaskInteraction,
         sizeof(bench_MixedCriticality_HardSoftTaskInteraction) / sizeof(TaskDef)},
        {"bench_high_leakage", bench_high_leakage,
         sizeof(bench_high_leakage) / sizeof(TaskDef)},

        {"bench_sporadic_overload", bench_sporadic_overload,
         sizeof(bench_sporadic_overload) / sizeof(TaskDef)},

        {"bench_ultimate_stress", bench_ultimate_stress,
         sizeof(bench_ultimate_stress) / sizeof(TaskDef)}

    };

    int bench_count = sizeof(benches) / sizeof(benches[0]);

    // EnergyMode energy_mode = ENERGY_MODE_BATTERY;
    EnergyMode energy_mode = ENERGY_MODE_HARVEST;

    int loaded_count = load_energy_profile("input_power.txt");
    if (loaded_count < 0)
    {
        fprintf(stderr, "Failed to load energy profile!\n");
        return 1;
    }
    printf("Loaded %d energy values from file\n", loaded_count);
    size_t max_results = 4096;
    RunResult *results = calloc(max_results, sizeof(RunResult));
    if (!results)
    {
        perror("calloc results");
        return 1;
    }
    int results_count = 0;

    const char *outdir = "logs";

    printf("Energy mode: %s (ENERGY_HARVEST_PER_MS_J=%.3e J/ms)\n",
           (energy_mode == ENERGY_MODE_HARVEST ? "HARVEST" : "BATTERY"),
           (double)ENERGY_HARVEST_PER_MS_J);

    for (int b = 0; b < bench_count; ++b)
    {
        const char *benchname = benches[b].name;
        TaskDef *defs = benches[b].defs;
        int ndefs = benches[b].ndefs;
        if (ndefs <= 0)
            continue;

        /* auto energy budget */
        double est_total_energy = 0.0;
        double sum_one_instance = 0.0;
        for (int i = 0; i < ndefs; ++i)
        {
            double e1 = compute_energy_j(&defs[i]);
            sum_one_instance += e1;
            long releases = estimate_releases_for(&defs[i]);
            est_total_energy += e1 * (double)releases;
        }

        // double auto_budget = est_total_energy * 1.05;
        // if (auto_budget < sum_one_instance * 1.1)
        // auto_budget = sum_one_instance * 1.1;
        // double energy_budget = auto_budget;
        double auto_budget = est_total_energy * 0.5;
        if (auto_budget < sum_one_instance * 1.1)
            auto_budget = sum_one_instance * 0.1;
        double energy_budget = auto_budget;

        printf("\nRunning bench=%s (tasks=%d)  auto_budget=%.9e J  chosen_budget=%.9e J\n",
               benchname, ndefs, auto_budget, energy_budget);

        long expected_instances = estimate_total_instances(defs, ndefs);

        /* iterate algorithms */
        for (SchedAlgo alg = ALG_SAEDF; alg <= ALG_CKP; alg = (SchedAlgo)(alg + 1))
        {
            printf("  -> algo=%s ...\n", sched_algo_name(alg));
            RunResult r;
            memset(&r, 0, sizeof(r));
            int rc = run_one(benchname, defs, ndefs,
                             alg, energy_budget, energy_mode,
                             outdir, &r);
            if (rc == 0)
            {
                if (results_count < (int)max_results)
                    results[results_count++] = r;
                else
                    fprintf(stderr, "warning: results buffer full\n");
            }
            else
            {
                fprintf(stderr, "run failed for algo %s on bench %s (rc=%d)\n",
                        sched_algo_name(alg), benchname, rc);
            }
        }

        /* per-bench summary (only rows for this bench) */
        printf("\nSummary for bench %s (expected instances=%ld):\n",
               benchname, expected_instances);
        printf("algo   | misses | energy_used(J)   | completed | total_exec_ms | cpu_util(%%) | avg_slack(ms) | min_slack(ms) | completion_rate(%%)\n");
        printf("-------------------------------------------------------------------------------------------------\n");
        for (int i = 0; i < results_count; ++i)
        {
            if (strcmp(results[i].bench, benchname) != 0)
                continue;
            double completion_rate = expected_instances > 0
                                         ? (100.0 * (double)results[i].completed_count /
                                            (double)expected_instances)
                                         : 0.0;
            printf("%-5s  | %6d | %15.6e | %9d | %13d | %10.2f | %12.3f | %12.3f | %7.2f\n",
                   results[i].algo,
                   results[i].deadline_miss_count,
                   results[i].used_energy,
                   results[i].completed_count,
                   results[i].total_exec_ms,
                   results[i].cpu_utilization,
                   results[i].avg_slack_ms,
                   results[i].min_slack_ms,
                   completion_rate);
        }
        printf("\n");
    }

    /* write global summary.csv */
    FILE *sf = fopen("summary.csv", "w");
    if (!sf)
    {
        perror("summary.csv");
        free(results);
        return 1;
    }
    fprintf(sf, "bench,algo,deadline_miss_count,energy_used_J,completed_count,"
                "total_exec_ms,cpu_utilization_percent,notes\n");
    for (int i = 0; i < results_count; ++i)
    {
        fprintf(sf, "%s,%s,%d,%.9e,%d,%d,%.2f,%s\n",
                results[i].bench,
                results[i].algo,
                results[i].deadline_miss_count,
                results[i].used_energy,
                results[i].completed_count,
                results[i].total_exec_ms,
                results[i].cpu_utilization,
                results[i].notes);
    }
    fclose(sf);

    free(results);
    printf("All runs complete. summary.csv created with %d rows.\n", results_count);
    return 0;
}
