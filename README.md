# SA-EDF: Shortage-Adaptive Earliest Deadline First Scheduler

---

## 📌 Table of Contents
- [Overview](#overview)
- [Key Features](#key-features)
- [Architecture & Algorithm](#architecture--algorithm)
- [Results (from paper)](#results-from-paper)
  - [Deadline Miss Rate](#deadline-miss-rate)
  - [Energy Efficiency](#energy-efficiency)
  - [Slack Time Distribution](#slack-time-distribution)
  - [FreeRTOS Evaluation](#freertos-evaluation)
  - [CPU Utilization](#cpu-utilization)
- [Repository Structure](#repository-structure)
- [Quick Start](#quick-start)
- [Citation](#citation)
- [License](#license)

---

## Overview

**SA-EDF** (Shortage-Adaptive Earliest Deadline First) is a real-time scheduling algorithm designed for **intermittently powered cyber-physical systems** such as energy-harvesting IoT devices, batteryless sensors, and medical implants.

Conventional schedulers (EDF, RM, EEVDF) ignore short-term energy feasibility, causing wasted execution and deadline misses.  
SA-EDF solves this by dynamically adapting scheduling decisions based on:

- Task deadlines
- Remaining energy
- Execution feasibility
- Energy scarcity level

---

## Key Features

| Feature | Description |
|---------|-------------|
| ⚡ Energy-aware | Uses remaining energy and per-task energy consumption |
| 📊 Scarcity-adaptive | Adjusts α(t) and β(t) based on energy scarcity |
| 🔄 Linear O(N) complexity | ~28 arithmetic ops / task |
| 🎯 Real-time guarantees | Extends EDF semantics under energy constraints |
| 🔌 Batteryless-ready | Designed for supercapacitor + harvester |
| 🧪 Fully evaluated | 12 workloads + FreeRTOS |

---

## Architecture & Algorithm


