#ifndef SCHEDULER_H
#define SCHEDULER_H
/*
 * scheduler.h
 * Modular scheduler simulation (EDF, RM, LLF, EZ, EEVDF, LS, CKP, and SA-EDF).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>
#include <limits.h>

/* ---------------- Configuration ---------------- */
#ifndef SCHED_MAX_TASKS
#define SCHED_MAX_TASKS 128
#endif
#ifndef SCHED_NAME_MAX
#define SCHED_NAME_MAX 64
#endif
#ifndef SCHED_SIM_TIME_MS
#define SCHED_SIM_TIME_MS 600000 /* 10 min */
#endif
#ifndef SCHED_TIME_UNIT_MS
#define SCHED_TIME_UNIT_MS 1
#endif
#ifndef ENABLE_STEP_LOG
#define ENABLE_STEP_LOG 1
#endif
#ifndef SCHED_EPS
#define SCHED_EPS 1e-12
#endif

/* Platform energy params */
#ifndef P_ACTIVE_W
#define P_ACTIVE_W 0.066 /* W */
#endif
#ifndef MEM_ENERGY_J_PER_KB
#define MEM_ENERGY_J_PER_KB 1e-5 /* J per kB */
#endif

/* Per-operation energy (approximate, can be tuned) */
#ifndef ENERGY_PER_ADD_J
#define ENERGY_PER_ADD_J 1e-9 /* J per addition */
#endif
#ifndef ENERGY_PER_MUL_J
#define ENERGY_PER_MUL_J 2e-9 /* J per multiplication */
#endif
#ifndef ENERGY_PER_MEM_ACCESS_J
#define ENERGY_PER_MEM_ACCESS_J 5e-10 /* J per memory access */
#endif

/* Energy-harvesting / CKP */
#ifndef ENERGY_THRESHOLD_J
#define ENERGY_THRESHOLD_J 0.001
#endif
#ifndef CHECKPOINT_SLICE_MS
#define CHECKPOINT_SLICE_MS 2
#endif

/* Energy harvesting: small recharge per time unit (J per ms) */
#ifndef ENERGY_HARVEST_PER_MS_J
#define ENERGY_HARVEST_PER_MS_J 0 /* set >0 and use ENERGY_MODE_HARVEST to enable harvesting */
#endif

/* EZ (Energy-Zeta) tunables */
#ifndef ENERGY_ZETA_ALPHA
#define ENERGY_ZETA_ALPHA 0.2
#endif
#ifndef ENERGY_ZETA_BETA
#define ENERGY_ZETA_BETA 0.8
#endif
#ifndef ENERGY_ZETA_PLANNING_HORIZON_MS
#define ENERGY_ZETA_PLANNING_HORIZON_MS 10
#endif

// Energy Profile
#define MAX_ENERGY_PROFILE 600000
static double g_energy_profile[MAX_ENERGY_PROFILE];
static bool g_energy_profile_loaded = false;

/* ---------- SAEDF++ (adaptive EDF-Z style) tunables ---------- */
#ifndef SAEDF_ALPHA_BASE
#define SAEDF_ALPHA_BASE 0.5
#endif
#ifndef SAEDF_BETA_BASE
#define SAEDF_BETA_BASE 0.5
#endif
#ifndef SAEDF_GAMMA_ENERGY
#define SAEDF_GAMMA_ENERGY 1.0
#endif
#ifndef SAEDF_HORIZON_MS
#define SAEDF_HORIZON_MS 10
#endif
#ifndef SAEDF_SLACK_PENALTY
#define SAEDF_SLACK_PENALTY 1000.0
#endif

/* ---------- EEVDF weights (fairness) ---------- */
#ifndef DEFAULT_TASK_WEIGHT
#define DEFAULT_TASK_WEIGHT 1.0
#endif
double slack_now;

/* ---------------- Public types ---------------- */
typedef enum
{
    ALG_SAEDF = 0, /* SAEDF++ adaptive EDF-Z style */
    ALG_EZ = 1,    /* Energy-Zeta */
    ALG_EEVDF = 2, /* Earliest Eligible Virtual Deadline First */
    ALG_EDF = 3,   /* Earliest Deadline First */
    ALG_RM = 4,    /* Rate Monotonic */
    ALG_LLF = 5,   /* Least Laxity First */
    ALG_LS = 6,    /* Lazy Scheduling (battery-less) */
    ALG_CKP = 7    /* Checkpoint-based */
} SchedAlgo;

static inline const char *sched_algo_name(SchedAlgo a)
{
    switch (a)
    {
    case ALG_SAEDF:
        return "SAEDF";
    // case ALG_EZ:
    //     return "EZ";
    case ALG_EEVDF:
        return "EEVDF";
    case ALG_EDF:
        return "EDF";
    case ALG_RM:
        return "RM";
    // case ALG_LLF:
    //     return "LLF";
    // case ALG_LS:
    //     return "LS";
    // case ALG_CKP:
    //     return "CKP";
    default:
        return "UNK";
    }
}

/* Energy mode: battery-only vs harvesting */
typedef enum
{
    ENERGY_MODE_BATTERY = 0,
    ENERGY_MODE_HARVEST = 1
} EnergyMode;

typedef struct
{
    char name[SCHED_NAME_MAX];
    int release_ms;
    int C_ms;
    int period_ms;
    int deadline_ms;
    int M_kB;
    double alpha, beta, gamma;
    bool has_energy_params;
} TaskDef;

/* Runtime state per task */
typedef struct
{
    char name[SCHED_NAME_MAX];
    int period_ms;
    int C_ms;
    int deadline_ms;
    int M_kB;
    double energy_j;

    int remaining_ms;
    int next_release_ms;
    int abs_deadline_ms;

    /* EEVDF-related fields */
    double v_start; /* virtual eligible time (ve) */
    double v_dead;  /* virtual deadline (informational) */
    double weight;  /* fairness weight (independent of energy) */

    bool active;
    bool completed;
    int finish_time_ms;

    int executed_ms;
    int slice_executed_ms;
} TaskState;

/* Summary per run */
typedef struct
{
    char bench[SCHED_NAME_MAX];
    char algo[SCHED_NAME_MAX];
    int deadline_miss_count;
    double used_energy;
    int completed_count;
    int total_exec_ms;
    double cpu_utilization;
    char notes[512];
    double avg_slack_ms;
    double min_slack_ms;

} RunResult;

/* ---------------- API declarations ---------------- */
static inline double compute_energy_j(const TaskDef *d);
static inline int init_task_states(const TaskDef defs[], int ndefs, TaskState ts[]);
static inline int pick_next(const TaskState ts[], int n, SchedAlgo algo, double V,
                            double *sum_w_out, double energy_left, int now_ms);
static inline int run_one(const char *benchname, const TaskDef defs[], int ndefs,
                          SchedAlgo algo, double energy_budget_J, EnergyMode emode,
                          const char *outdir, RunResult *res);
static inline long estimate_releases_for(const TaskDef *d);
static inline long estimate_total_instances(const TaskDef defs[], int ndefs);
static inline int load_energy_profile(const char *filename);
/* ---------------- Implementations ---------------- */
static inline int load_energy_profile(const char *filename)
{
    if (!filename)
        return -1;

    FILE *f = fopen(filename, "r");
    if (!f)
    {
        fprintf(stderr, "Error: cannot open energy profile file: %s\n", filename);
        return -1;
    }

    int count = 0;
    double val;
    while (fscanf(f, "%lf", &val) == 1 && count < MAX_ENERGY_PROFILE)
    {
        g_energy_profile[count++] = val;
    }
    fclose(f);

    for (int i = count; i < MAX_ENERGY_PROFILE; i++)
        g_energy_profile[i] = 0.0;

    g_energy_profile_loaded = true;
    return count;
}

static inline double compute_energy_j(const TaskDef *d)
{
    if (!d)
        return 0.0;

    /* Base dynamic energy from active power */
    double t_s = ((double)d->C_ms) / 1000.0;
    double e_dyn = P_ACTIVE_W * t_s;

    /* Base static memory cost */
    double e_mem = MEM_ENERGY_J_PER_KB * (double)d->M_kB;

    /* Per-operation energy model (add/mul/mem) */
    double e_ops = 0.0;
    if (d->has_energy_params)
    {
        double adds = d->alpha * (double)d->C_ms;
        double muls = d->beta * (double)d->C_ms;
        double memops = 2 * d->gamma * (double)d->C_ms;

        if (adds < 0.0)
            adds = 0.0;
        if (muls < 0.0)
            muls = 0.0;
        if (memops < 0.0)
            memops = 0.0;

        e_ops = adds * ENERGY_PER_ADD_J + muls * ENERGY_PER_MUL_J + memops * ENERGY_PER_MEM_ACCESS_J;
    }

    return e_dyn + e_mem + e_ops;
}

static inline int init_task_states(const TaskDef defs[], int ndefs, TaskState ts[])
{
    if (!defs || !ts || ndefs <= 0 || ndefs > SCHED_MAX_TASKS)
        return -1;
    for (int i = 0; i < ndefs; ++i)
    {
        memset(&ts[i], 0, sizeof(TaskState));
        snprintf(ts[i].name, sizeof(ts[i].name), "%s", defs[i].name);
        ts[i].period_ms = defs[i].period_ms;
        ts[i].C_ms = defs[i].C_ms;
        ts[i].deadline_ms = defs[i].deadline_ms;
        ts[i].M_kB = defs[i].M_kB;
        ts[i].energy_j = compute_energy_j(&defs[i]);

        ts[i].remaining_ms = 0;
        ts[i].next_release_ms = defs[i].release_ms;
        ts[i].abs_deadline_ms = -1;

        ts[i].v_start = 0.0;
        ts[i].v_dead = 0.0;

        ts[i].weight = DEFAULT_TASK_WEIGHT;

        ts[i].active = false;
        ts[i].completed = false;
        ts[i].finish_time_ms = -1;
        ts[i].executed_ms = 0;
        ts[i].slice_executed_ms = 0;
    }
    return 0;
}

static inline long estimate_releases_for(const TaskDef *d)
{
    if (!d)
        return 0;
    if (d->release_ms > SCHED_SIM_TIME_MS)
        return 0;
    if (d->period_ms <= 0)
        return 1;
    long count = 1 + (SCHED_SIM_TIME_MS - d->release_ms) / d->period_ms;
    if (count < 0)
        count = 0;
    return count;
}

static inline long estimate_total_instances(const TaskDef defs[], int ndefs)
{
    long sum = 0;
    for (int i = 0; i < ndefs; ++i)
        sum += estimate_releases_for(&defs[i]);
    return sum;
}

/* pick_next: chooses an index or -1 for IDLE */
static inline int pick_next(const TaskState ts[], int n, SchedAlgo algo, double V,
                            double *sum_w_out, double energy_left, int now_ms)
{
    if (!ts || n <= 0)
    {
        if (sum_w_out)
            *sum_w_out = 0.0;
        return -1;
    }

    int chosen = -1;
    double best_metric = 1e300;
    double sum_w = 0.0;

    /* --- EEVDF / virtual-time weights (independent of energy) --- */
    for (int i = 0; i < n; ++i)
    {
        if (ts[i].active && !ts[i].completed && ts[i].remaining_ms > 0)
        {
            double w = ts[i].weight;
            if (w <= 0.0)
                w = 1.0;
            sum_w += w;
        }
    }
    if (sum_w_out)
        *sum_w_out = sum_w;

    /* LS: energy threshold */

    if (algo == ALG_LS)
    {
        if (energy_left < ENERGY_THRESHOLD_J)
            return -1;
    }

    /* Pre-compute Tmin/Tmax for normalization (used by EZ and SAEDF++) */
    double Tmin = 1e300, Tmax = -1e300;
    if (algo == ALG_EZ || algo == ALG_SAEDF)
    {
        for (int i = 0; i < n; ++i)
        {
            if (ts[i].active && !ts[i].completed && ts[i].remaining_ms > 0 && ts[i].abs_deadline_ms >= now_ms)
            {
                // change abs_deadline to Vd
                double T = (double)(ts[i].abs_deadline_ms - now_ms);
                if (T < SCHED_EPS)
                    T = SCHED_EPS;
                if (T < Tmin)
                    Tmin = T;
                if (T > Tmax)
                    Tmax = T;
            }
        }
        if (!(Tmax > Tmin))
        {
            Tmin = 0.0;
            Tmax = 1.0;
        }
    }

    for (int i = 0; i < n; ++i)
    {
        if (!(ts[i].active && !ts[i].completed && ts[i].remaining_ms > 0 && ts[i].abs_deadline_ms >= now_ms))
            continue;

        double metric = 0.0;
        switch (algo)
        {
        case ALG_EDF:
            metric = (double)ts[i].abs_deadline_ms;
            break;

        case ALG_RM:
            metric = (double)(ts[i].period_ms > 0 ? ts[i].period_ms : INT_MAX);
            break;

        case ALG_LLF:
            metric = (double)(ts[i].abs_deadline_ms - ts[i].remaining_ms);
            break;

        case ALG_SAEDF: /* --- SAEDF++ adaptive EDF-Z style --- */
        {
            // abs_deadline --> Vd
            double T = (double)(ts[i].abs_deadline_ms - now_ms);
            if (T < SCHED_EPS)
                T = SCHED_EPS;

            double deadline_term = (T - Tmin) / ((Tmax - Tmin) + SCHED_EPS);

            double energy_per_ms = ts[i].energy_j / fmax(1.0, (double)ts[i].C_ms);
            double E_rem = energy_per_ms * (double)ts[i].remaining_ms; /* J */
            double P_req = E_rem / T;                                  /* J/ms */
            // to do
            double horizon_ms = (double)SAEDF_HORIZON_MS;
            if (horizon_ms < 1.0)
                horizon_ms = 1.0;
            double P_cap = energy_left / horizon_ms; /* J/ms */

            double E_need_h = 0.0;
            for (int j = 0; j < n; ++j)
            {
                if (ts[j].active && !ts[j].completed && ts[j].remaining_ms > 0 && ts[j].abs_deadline_ms >= now_ms)
                {
                    double ej_per_ms = ts[j].energy_j / fmax(1.0, (double)ts[j].C_ms);
                    int slice = ts[j].remaining_ms < (int)horizon_ms ? ts[j].remaining_ms : (int)horizon_ms;
                    E_need_h += ej_per_ms * (double)slice;
                }
            }
            double scarcity = E_need_h / (energy_left + SCHED_EPS);

            double alpha = SAEDF_ALPHA_BASE / (1.0 + scarcity);
            double beta = SAEDF_BETA_BASE * (1.0 + scarcity);

            double energy_term_raw = P_req / (P_cap + SCHED_EPS);
            if (energy_term_raw < 0.0)
                energy_term_raw = 0.0;
            double energy_term = pow(energy_term_raw, SAEDF_GAMMA_ENERGY);

            double slack_penalty = 0.0;
            if ((double)ts[i].remaining_ms > T + 1e-9)
                slack_penalty = SAEDF_SLACK_PENALTY;

            metric = alpha * deadline_term + beta * energy_term + slack_penalty + 1e-6 * ts[i].v_start;
            break;
        }

        case ALG_EZ:
        {
            /* Energy-Zeta */
            // abs --> Vd
            double T = (double)(ts[i].v_dead - V);
            if (T < SCHED_EPS)
                T = SCHED_EPS;

            double deadline_term = (T - Tmin) / ((Tmax - Tmin) + SCHED_EPS);

            double energy_per_ms = ts[i].energy_j / fmax(1.0, (double)ts[i].C_ms);
            double E_rem = energy_per_ms * (double)ts[i].remaining_ms;
            double P_req = E_rem / T;

            double horizon_ms = (double)ENERGY_ZETA_PLANNING_HORIZON_MS;
            if (horizon_ms < 1.0)
                horizon_ms = 1.0;
            double P_cap = energy_left / horizon_ms;

            double energy_term = P_req / (P_cap + SCHED_EPS);

            metric = ENERGY_ZETA_ALPHA * deadline_term + ENERGY_ZETA_BETA * energy_term;
            break;
        }

        case ALG_EEVDF:
        {
            /* Classic EEVDF-style virtual deadline: Vd = v_start + C_rem / w */
            double w = ts[i].weight;
            if (w <= 0.0)
                w = 1.0;
            // v_start --> V eligible (ED-EEVDF)
            double vd = ts[i].v_start +
                        ((double)ts[i].remaining_ms) / (w + SCHED_EPS);
            metric = vd;
            break;
        }

        case ALG_LS:
        case ALG_CKP:
            metric = (double)ts[i].abs_deadline_ms; /* EDF-like */
            break;

        default:
            metric = (double)ts[i].abs_deadline_ms;
            break;
        }

        if (metric < best_metric - 1e-12)
        {
            best_metric = metric;
            chosen = i;
        }
        else if (fabs(metric - best_metric) < 1e-12)
        {
            /* tie-break: 1) earlier real deadline  2) smaller remaining  3) name */
            if (chosen == -1 ||
                ts[i].abs_deadline_ms < ts[chosen].abs_deadline_ms ||
                (ts[i].abs_deadline_ms == ts[chosen].abs_deadline_ms && ts[i].remaining_ms < ts[chosen].remaining_ms) ||
                (ts[i].abs_deadline_ms == ts[chosen].abs_deadline_ms && ts[i].remaining_ms == ts[chosen].remaining_ms &&
                 strcmp(ts[i].name, ts[chosen].name) < 0))
            {
                chosen = i;
            }
        }
    }

    return chosen;
}

/* run_one: main simulation for one bench+algorithm */
static inline int run_one(const char *benchname, const TaskDef defs[], int ndefs,
                          SchedAlgo algo, double energy_budget_J, EnergyMode emode,
                          const char *outdir, RunResult *res)
{
    if (!benchname || !defs || ndefs <= 0 || !res)
        return -1;
    if (ndefs > SCHED_MAX_TASKS)
        return -1;

    TaskState ts[SCHED_MAX_TASKS];
    if (init_task_states(defs, ndefs, ts) != 0)
        return -1;

    const char *od = outdir ? outdir : ".";

    char txt_path[512];
    snprintf(txt_path, sizeof(txt_path), "%s/%s-%s_log.txt", od, benchname, sched_algo_name(algo));
    FILE *lf = fopen(txt_path, "w");
    if (!lf)
    {
        fprintf(stderr, "warning: could not open %s\n", txt_path);
        lf = fopen("logtmp.txt", "w");
        if (!lf)
            return -1;
    }

#if ENABLE_STEP_LOG
    char csv_path[512];
    snprintf(csv_path, sizeof(csv_path), "%s/%s-%s-steps.csv", od, benchname, sched_algo_name(algo));
    FILE *sf = fopen(csv_path, "w");
    if (!sf)
    {
        fprintf(lf, "warning: could not open %s\n", csv_path);
        sf = NULL;
    }
    else
    {
        fprintf(sf, "time_ms,task,event,rem_before_ms,rem_after_ms,energy_used_J,energy_left_J,V,sum_w,slack_time\n");
    }
#else
    FILE *sf = NULL;
#endif

    fprintf(lf, "Benchmark: %s  Algorithm: %s  EnergyBudget(J)=%.9e  EnergyMode=%s\n",
            benchname, sched_algo_name(algo), energy_budget_J,
            (emode == ENERGY_MODE_HARVEST ? "HARVEST" : "BATTERY"));
    fprintf(lf, "Platform: P_ACTIVE_W=%.6f J/s, MEM_ENERGY_J_PER_KB=%.9e J/kB\n\n", P_ACTIVE_W, MEM_ENERGY_J_PER_KB);
    fprintf(lf, "Time(ms)\tEvent\n");

    double V = 0.0;
    bool energy_exhausted = false;
    double energy_left = (energy_budget_J < 0.0 ? 1e100 : energy_budget_J);
    int deadline_miss_count = 0;
    int completed_count = 0;
    int total_executed_ms = 0;

    /* slack accounting */
    double slack_sum_ms = 0.0;
    long slack_samples = 0;
    double min_slack_ms = 1e300;

    for (int t = 0; t < SCHED_SIM_TIME_MS; t += SCHED_TIME_UNIT_MS)
    {
        /* Releases */
        for (int i = 0; i < ndefs; ++i)
        {
            if (t == ts[i].next_release_ms)
            {
                if (ts[i].remaining_ms > 0)
                {
                    deadline_miss_count++;
                    fprintf(lf, "[t=%d] TASK %s: previous instance unfinished -> deadline miss\n", t, ts[i].name);
#if ENABLE_STEP_LOG
                    if (sf)
                        fprintf(sf, "%d,%s,%s,%d,%d,%.9e,%.9e,%.9e,%.9e,%f\n",
                                t, ts[i].name, "DEADLINE_MISS", ts[i].remaining_ms, ts[i].remaining_ms, 0.0, energy_left, V, 0.0, slack_now);
#endif
                }
                ts[i].remaining_ms = ts[i].C_ms;
                ts[i].abs_deadline_ms = t + ts[i].deadline_ms;
                ts[i].active = true;
                ts[i].completed = false;
                ts[i].finish_time_ms = -1;

                /* EEVDF: job becomes eligible at current virtual time */
                ts[i].v_start = V;
                // ts[i].v_start = 0;
                ts[i].v_dead = 0.0;

                ts[i].slice_executed_ms = 0;
                // to do
                if (ts[i].period_ms > 0)
                    ts[i].next_release_ms += ts[i].period_ms;
                else
                    ts[i].next_release_ms = SCHED_SIM_TIME_MS + 1;

                fprintf(lf, "[t=%d] Release %s (C=%d, dl=%d)\n", t, ts[i].name, ts[i].C_ms, ts[i].abs_deadline_ms);
#if ENABLE_STEP_LOG
                if (sf)
                    fprintf(sf, "%d,%s,%s,%d,%d,%.9e,%.9e,%.9e,%.9e\n",
                            t, ts[i].name, "RELEASE", 0, ts[i].remaining_ms, 0.0, energy_left, V, 0.0);
#endif
            }
        }

        double sum_w = 0.0;
        int chosen = pick_next(ts, ndefs, algo, V, &sum_w, energy_left, t);

        if (chosen >= 0)
        {
            int rem_before = ts[chosen].remaining_ms;
            double energy_per_ms = ts[chosen].energy_j / fmax(1.0, (double)ts[chosen].C_ms);

            if (energy_left - energy_per_ms < -1e-15)
            {
                fprintf(lf, "[t=%d] Cannot run %s: NOT ENOUGH ENERGY (need %.9e, left %.9e)\n",
                        t, ts[chosen].name, energy_per_ms, energy_left);
                energy_exhausted = true;
#if ENABLE_STEP_LOG
                if (sf)
                    fprintf(sf, "%d,%s,%s,%d,%d,%.9e,%.9e,%.9e,%.9e\n",
                            t, "NOT_ENOUGH_ENERGY", "SYSTEM", 0, 0, 0.0, energy_left, V, sum_w);
#endif
                break;
            }
            slack_now = (double)(ts[chosen].abs_deadline_ms - t) - (double)(ts[chosen].remaining_ms);
            slack_sum_ms += slack_now;
            slack_samples += 1;
            if (slack_now < min_slack_ms)
                min_slack_ms = slack_now;
            ts[chosen].remaining_ms -= 1;
            ts[chosen].executed_ms += 1;
            ts[chosen].slice_executed_ms += 1;
            if (energy_left < 1e90)
                energy_left -= energy_per_ms;
            total_executed_ms += 1;

            fprintf(lf, "[t=%d] Run %s (rem=%d ms) energy_used=%.9e left=%.9e current_slack=%f\n",
                    t, ts[chosen].name, ts[chosen].remaining_ms, energy_per_ms, energy_left, slack_now);
#if ENABLE_STEP_LOG
            if (sf)
                fprintf(sf, "%d,%s,%s,%d,%d,%.9e,%.9e,%.9e,%.9e,%f\n",
                        t, ts[chosen].name, "RUN", rem_before, ts[chosen].remaining_ms, energy_per_ms, energy_left, V, sum_w, slack_now);
#endif

            if (algo == ALG_CKP)
            {
                if (ts[chosen].slice_executed_ms >= CHECKPOINT_SLICE_MS && ts[chosen].remaining_ms > 0)
                {
                    ts[chosen].v_start = V + (double)CHECKPOINT_SLICE_MS + 0.5;
                    ts[chosen].slice_executed_ms = 0;
                }
            }

            if (algo == ALG_EEVDF)
            {
                double w = ts[chosen].weight;
                if (w <= 0.0)
                    w = 1.0;
                // to do
                ts[chosen].v_start += ((double)SCHED_TIME_UNIT_MS) / (w + SCHED_EPS);
                ts[chosen].v_dead = ts[chosen].v_start +
                                    ((double)ts[chosen].remaining_ms) / (w + SCHED_EPS);
            }

            if (ts[chosen].remaining_ms <= 0)
            {
                ts[chosen].completed = true;
                ts[chosen].active = false;
                ts[chosen].finish_time_ms = t;
                fprintf(lf, "   --> %s finished at t=%d (dl=%d)\n",
                        ts[chosen].name, ts[chosen].finish_time_ms, ts[chosen].abs_deadline_ms);
                if (ts[chosen].finish_time_ms > ts[chosen].abs_deadline_ms)
                {
                    deadline_miss_count++;
                    fprintf(lf, "   >>> MISS DEADLINE for %s (finish %d > dl %d)\n",
                            ts[chosen].name, ts[chosen].finish_time_ms, ts[chosen].abs_deadline_ms);
                }
                else
                {
                    completed_count++;
                }
            }
        }
        else
        {
            fprintf(lf, "[t=%d] IDLE\n", t);
#if ENABLE_STEP_LOG
            if (sf)
                fprintf(sf, "%d,%s,%s,%d,%d,%.9e,%.9e,%.9e,%.9e\n",
                        t, "IDLE", "SYSTEM", 0, 0, 0.0, energy_left, V, sum_w);
#endif
        }

        /* System virtual time: dV/dt = 1 / sum_w  (if there are active jobs) */
        if (sum_w > 0.0)
            V += ((double)SCHED_TIME_UNIT_MS) / sum_w;

        /* --- Energy harvesting: small recharge every time unit if enabled --- */
        if (emode == ENERGY_MODE_HARVEST &&
            energy_budget_J >= 0.0 &&
            ENERGY_HARVEST_PER_MS_J > 0.0 &&
            energy_left < 1e90)
        {
            double harvest_this_step = 0.0;

            if (g_energy_profile_loaded && t < MAX_ENERGY_PROFILE)
            {
                harvest_this_step = g_energy_profile[t] * (double)SCHED_TIME_UNIT_MS;
            }
            else if (ENERGY_HARVEST_PER_MS_J > 0.0)
            {
                harvest_this_step = ENERGY_HARVEST_PER_MS_J * (double)SCHED_TIME_UNIT_MS;
            }

            energy_left += harvest_this_step;

            // energy_left += ENERGY_HARVEST_PER_MS_J * (double)SCHED_TIME_UNIT_MS;
        }

        bool any_future = false;
        for (int i = 0; i < ndefs; ++i)
        {
            if (ts[i].remaining_ms > 0 || ts[i].next_release_ms <= SCHED_SIM_TIME_MS)
            {
                any_future = true;
                break;
            }
        }
        if (!any_future)
            break;
    }

    char notes[512] = "";
    if (energy_exhausted)
        strncat(notes, "energy_exhausted;", sizeof(notes) - strlen(notes) - 1);
    for (int i = 0; i < ndefs; ++i)
    {
        if (!ts[i].completed && ts[i].remaining_ms > 0)
        {
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "task_%s_unfinished;", ts[i].name);
            strncat(notes, tmp, sizeof(notes) - strlen(notes) - 1);
        }
    }

    double used_energy = 0.0;
    if (energy_budget_J >= 0.0)
    {
        if (energy_left <= energy_budget_J)
            used_energy = -(energy_left - energy_budget_J);
        else
            used_energy = 0.0;
    }

    double cpu_util = 100.0 * ((double)total_executed_ms) / (double)SCHED_SIM_TIME_MS;

    /* utilization CSV */
    double theoretical_U = 0.0;
    for (int i = 0; i < ndefs; ++i)
    {
        if (defs[i].period_ms > 0)
            theoretical_U += ((double)defs[i].C_ms) / (double)defs[i].period_ms;
        else
            theoretical_U += ((double)defs[i].C_ms) / (double)SCHED_SIM_TIME_MS;
    }
    double observed_cpu_util_percent = 100.0 * ((double)total_executed_ms) / (double)SCHED_SIM_TIME_MS;

    char util_csv_path[512];
    snprintf(util_csv_path, sizeof(util_csv_path), "%s/%s-%s-utilization.csv", od, benchname, sched_algo_name(algo));
    FILE *uf = fopen(util_csv_path, "w");
    if (uf)
    {
        fprintf(uf, "task,C_ms,period_ms,theoretical_Ui,executed_ms,observed_share_percent,energy_j\n");
        for (int i = 0; i < ndefs; ++i)
        {
            double Ui = 0.0;
            if (defs[i].period_ms > 0)
                Ui = ((double)defs[i].C_ms) / (double)defs[i].period_ms;
            else
                Ui = ((double)defs[i].C_ms) / (double)SCHED_SIM_TIME_MS;
            double observed_share = 100.0 * ((double)ts[i].executed_ms) / (double)SCHED_SIM_TIME_MS;
            fprintf(uf, "%s,%d,%d,%.9f,%d,%.6f,%.9e\n",
                    ts[i].name, ts[i].C_ms, ts[i].period_ms, Ui, ts[i].executed_ms, observed_share, ts[i].energy_j);
        }
        fprintf(uf, "\n#summary,theoretical_U,observed_cpu_util_percent,total_executed_ms\n");
        fprintf(uf, ",%.9f,%.6f,%d\n", theoretical_U, observed_cpu_util_percent, total_executed_ms);
        fclose(uf);
    }
    else
    {
        fprintf(lf, "warning: could not open %s for utilization output\n", util_csv_path);
    }

    snprintf(res->bench, sizeof(res->bench), "%s", benchname);
    snprintf(res->algo, sizeof(res->algo), "%s", sched_algo_name(algo));
    res->deadline_miss_count = deadline_miss_count;
    res->used_energy = used_energy;
    res->completed_count = completed_count;
    res->total_exec_ms = total_executed_ms;
    double avg_slack_ms = 0.0;
    if (slack_samples > 0)
        avg_slack_ms = slack_sum_ms / (double)slack_samples;
    if (min_slack_ms > 1e200)
        min_slack_ms = 0.0;
    res->avg_slack_ms = avg_slack_ms;
    res->min_slack_ms = min_slack_ms;

    res->cpu_utilization = cpu_util;
    snprintf(res->notes, sizeof(res->notes), "%s", notes);

    fprintf(lf, "\n--- Utilization summary ---\n");
    fprintf(lf, "theoretical_U = %.9f\n", theoretical_U);
    fprintf(lf, "observed_cpu_util_percent = %.6f%% (total_executed_ms=%d, SIM_TIME_MS=%d)\n",
            observed_cpu_util_percent, total_executed_ms, SCHED_SIM_TIME_MS);
    fprintf(lf, "per-task executed_ms and observed share written to %s\n", util_csv_path);

#if ENABLE_STEP_LOG
    if (sf)
        fclose(sf);
#endif
    if (lf)
        fclose(lf);
    return 0;
}
#endif /* SCHEDULER_H */
