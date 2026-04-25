# EV Car in Speed Dreams — Design Spec
**Date:** 2026-04-23
**Status:** inprogress

## Overview

Add an electric vehicle (EV) car to the Speed Dreams simulator with:
- Battery state-of-charge (SOC) replacing fuel
- Motor discharge proportional to torque and RPM
- Regenerative braking that recovers energy during deceleration
- Backward compatibility — all existing combustion cars unchanged

The simulation output will later be used as a data source for a CAN bus pipeline:
Raspberry Pi (Speed Dreams) → Arduino (CAN module) → Gateway → Speedhut instruments.
## What is missing or should be improved ⚠️

### 1. tBattery needs more fields

```c
typedef struct {
    tdble capacity;      /* kWh — total capacity */
    tdble soc;           /* 0.0–1.0 state of charge */
    tdble maxPower;      /* kW — peak motor output */
    tdble regenFactor;   /* 0.0–1.0 regen efficiency */

    /* NEW: */
    tdble temperature;   /* °C — affects capacity at extremes */
    tdble maxRegen;      /* kW — peak regen power */
    int   isEV;          /* flag: 1=EV, 0=combustion — keeps existing cars working */
} tBattery;
```

### 2. isEV flag in tCar

Critical so existing combustion cars are not broken:

```c
// carstruct.h
typedef struct CarElt {
    // ... existing fields ...
    tBattery battery;   // only active when battery.isEV == 1
} tCar;
```

### 3. engine.cpp – soft torque limit at low SOC

Instead of hard cutoff, reduce power gradually:

```c
if (battery.isEV) {
    if (battery.soc < 0.05) {
        // Below 5% SOC: linearly reduce torque
        torque *= (battery.soc / 0.05);
    }
    if (battery.soc <= 0.0) {
        torque = 0.0;
        car->pub.state |= RM_CAR_STATE_OUTOFGAS;
    }

    // Discharge battery
    tdble power_kW = torque * rpm_rads / 1000.0;
    battery.soc -= (power_kW * dt) / (battery.capacity * 3600.0);
    battery.soc = MAX(battery.soc, 0.0);
}
```

### 4. More precise regen formula

```c
// brake.cpp
if (battery.isEV && brakePressure > 0 && wheelSpeed > 0) {

    tdble regenPower_kW = wheelSpeed * brakeTorque / 1000.0;

    // Cap at maxRegen:
    regenPower_kW = MIN(regenPower_kW, battery.maxRegen);

    // Add energy back to battery:
    battery.soc += (regenPower_kW * battery.regenFactor * dt)
                   / (battery.capacity * 3600.0);

    // Clamp SOC to 1.0:
    battery.soc = MIN(battery.soc, 1.0);

    // Reduce mechanical brake torque proportionally:
    brakeTorque *= (1.0 - battery.regenFactor);
}
```
when i am wrong with that tell me. why iam wrong 
---

## Architecture

### Approach
Patch the existing `simuv5` physics module. No new module is created. An `isEV` flag gates all EV logic so combustion cars are unaffected.

### Files Changed
| File | Change |
|------|--------|
| `src/modules/simu/simuv5/carstruct.h` | Add `tBattery` struct and embed in `tCar` |
| `src/interfaces/car.h` | Expose `batterySOC`, `batteryTemp` in public `tCarElt` |
| `src/modules/simu/simuv5/engine.cpp` | Replace fuel deduction with battery discharge when `isEV` |
| `src/modules/simu/simuv5/brake.cpp` | Add regen braking logic when `isEV` |
| `src/modules/simu/simuv5/car.cpp` | Include battery in mass calculation; init from XML |
| `speed-dreams-data/data/cars/models/ev-tesla/ev-tesla.xml` | New EV car definition |

---

## Data Structures

### tBattery (new, in carstruct.h)
```c
typedef struct {
    tdble capacity;     /* kWh — total usable capacity */
    tdble soc;          /* 0.0–1.0 state of charge */
    tdble maxPower;     /* kW — peak motor output */
    tdble maxRegen;     /* kW — peak regenerative braking power */
    tdble regenFactor;  /* 0.0–1.0 — regen efficiency (e.g. 0.7) */
    tdble temperature;  /* °C — battery temperature, affects capacity */
    int   isEV;         /* 1 = EV mode active, 0 = combustion (default) */
} tBattery;
```

Embedded in `tCar`:
```c
tBattery battery;   /* only active when battery.isEV == 1 */
```

### Public Interface (car.h)
Two new fields added to `tCarElt` public struct:
```c
tdble batterySOC;   /* 0.0–1.0, mirrors battery.soc */
tdble batteryTemp;  /* °C, mirrors battery.temperature */
```
These are written each physics tick alongside the existing `fuel` fields.

---

## Motor Discharge (engine.cpp)

Inside `SimEngineUpdateTq()`, after torque is computed:

```c
if (car->battery.isEV) {
    tBattery *bat = &car->battery;

    /* Soft power taper below 5% SOC */
    if (bat->soc < 0.05f) {
        engine->Tq_cur *= (bat->soc / 0.05f);
    }
    if (bat->soc <= 0.0f) {
        engine->Tq_cur = 0.0f;
        car->pub.state |= RM_CAR_STATE_OUTOFGAS;
    }

    /* Discharge */
    tdble power_kW = engine->Tq_cur * engine->rads / 1000.0f;
    tdble tempFactor = 1.0f - 0.005f * MAX(0.0f, bat->temperature - 25.0f);
    tdble effective_capacity = bat->capacity * MAX(tempFactor, 0.5f);
    bat->soc -= (power_kW * SimDeltaTime) / (effective_capacity * 3600.0f);
    bat->soc = MAX(bat->soc, 0.0f);

    /* Suppress fuel deduction */
    /* (existing fuel line is skipped via else branch) */
}
```

The existing combustion fuel deduction runs in the `else` branch — no change for non-EV cars.

---

## Regenerative Braking (brake.cpp)

Inside `SimBrakeUpdate()`, after brake torque is computed, before it is applied to the wheel:

```c
if (car->battery.isEV && brake->pressure > 0.0f && wheel->spinVel > 0.0f) {
    tBattery *bat = &car->battery;

    /* Power available from regen: P = ω × T */
    tdble theoretical_kW = (wheel->spinVel * brake->Tq) / 1000.0f;

    /* Cap at battery's max regen power (split across 4 wheels) */
    tdble regenPower_kW = MIN(theoretical_kW, bat->maxRegen / 4.0f);

    /* Recover energy into battery */
    tdble tempFactor = 1.0f - 0.005f * MAX(0.0f, bat->temperature - 25.0f);
    tdble effective_capacity = bat->capacity * MAX(tempFactor, 0.5f);
    bat->soc += (regenPower_kW * bat->regenFactor * SimDeltaTime)
                / (effective_capacity * 3600.0f);
    bat->soc = MIN(bat->soc, 1.0f);

    /* Reduce mechanical brake torque by the fraction actually captured */
    tdble actualRatio = (theoretical_kW > 0.0f)
                        ? (regenPower_kW / theoretical_kW)
                        : 0.0f;
    brake->Tq *= (1.0f - actualRatio * bat->regenFactor);
}
```

Regen only runs when `spinVel > 0` (wheel moving) and brake is pressed. At zero speed, full mechanical braking applies normally.

---

## Initialisation (car.cpp)

In `SimCarConfig()`, read EV parameters from XML when the `[Battery]` section is present:

```c
car->battery.isEV = 0;  /* default: combustion */
if (GfParmSectionExists(hdle, "Battery")) {
    car->battery.isEV       = 1;
    car->battery.capacity   = GfParmGetNum(hdle, "Battery", "capacity",   "kWh", 60.0f);
    car->battery.soc        = GfParmGetNum(hdle, "Battery", "initial soc","",    1.0f);
    car->battery.maxPower   = GfParmGetNum(hdle, "Battery", "max power",  "kW",  350.0f);
    car->battery.maxRegen   = GfParmGetNum(hdle, "Battery", "max regen",  "kW",  150.0f);
    car->battery.regenFactor= GfParmGetNum(hdle, "Battery", "regen factor","",   0.7f);
    car->battery.temperature= 25.0f;  /* ambient start temp */
}
```

Battery mass is added to car mass:
```c
if (car->battery.isEV) {
    /* Tesla Model S ~500 kg pack for 100 kWh; scale linearly */
    car->mass += car->battery.capacity * 5.0f;
}
```

---

## EV Car XML (ev-tesla.xml)

New car at `speed-dreams-data/data/cars/models/ev-tesla/ev-tesla.xml`.

Key sections (abbreviated):

```xml
<section name="Car">
  <attnum name="mass" unit="kg" val="1800"/>
  <attnum name="tank" unit="kWh" val="0"/>   <!-- no fuel tank -->
</section>

<section name="Battery">
  <attnum name="capacity"    unit="kWh" val="100"/>
  <attnum name="initial soc" val="1.0"/>
  <attnum name="max power"   unit="kW"  val="350"/>
  <attnum name="max regen"   unit="kW"  val="150"/>
  <attnum name="regen factor" val="0.7"/>
</section>

<section name="Engine">
  <!-- Flat torque curve, 0–500 Nm across 0–18000 RPM (electric motor) -->
  <section name="Tq">
    <attnum name="rpm 1" val="0"/>    <attnum name="tq 1" val="500"/>
    <attnum name="rpm 2" val="18000"/> <attnum name="tq 2" val="300"/>
  </section>
  <attnum name="fuel cons factor" val="0"/>  <!-- disabled, battery handles this -->
</section>
```

---

## Error Handling & Edge Cases

| Case | Behaviour |
|------|-----------|
| SOC = 0 | Motor torque → 0, `RM_CAR_STATE_OUTOFGAS` set |
| SOC < 5% | Torque tapered linearly to zero |
| Battery full during regen | SOC clamped to 1.0, excess energy lost as heat |
| `spinVel = 0` during braking | Regen skipped, full mechanical braking applied |
| High temperature | Effective capacity derated (min 50% at extreme temp) |
| Combustion car | `isEV = 0`, all EV code paths skipped entirely |

---

## Out of Scope (Phase 1)

- CAN bus output from Speed Dreams to Arduino/Raspberry Pi
- Speedhut instrument integration
- Battery thermal heating model (temperature stays at 25°C for now)
- Motor temperature / overheating
- Multiple motor configurations (single vs dual motor)
