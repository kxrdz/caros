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

### 3. engine.cpp – fuel ≤ 0 early-exit blocks EV code ⚠️

In the real `SimEngineUpdateTq()` the very first guard is:

```c
if ((car->fuel <= 0.0f) || (car->carElt->_state & ...))
{
    engine->rads = 0;
    engine->Tq   = 0;
    return;   // ← EXITS BEFORE EV CODE RUNS
}
```

Because the EV XML sets `tank = 0`, `car->fuel` is `0.0` from lap 1 and the function returns immediately — the discharge logic never executes.

**Fix:** make the fuel check EV-aware:

```c
if ((car->fuel <= 0.0f && !car->battery.isEV) || (car->carElt->_state & ...))
{
    engine->rads = 0;
    engine->Tq   = 0;
    return;
}
```

This guard must be applied before any other EV logic is added to the function.

---

### 4. engine.cpp – same fuel ≤ 0 guard in SimEngineUpdateRpm() ⚠️

`SimEngineUpdateRpm()` has an identical early-exit on `car->fuel <= 0.0f`. It controls clutch torque transfer and RPM integration. Without the same fix, the EV's RPM stays at zero even after the Tq guard is patched.

**Fix:** apply the same EV-aware guard:

```c
if ((car->fuel <= 0.0f && !car->battery.isEV) || (car->carElt->_state & ...))
{
    engine->rads = 0;
    return;
}
```

---

### 5. engine.cpp – soft torque limit at low SOC (variable names corrected)

Instead of hard cutoff, reduce power gradually. Early drafts used `torque`, `rpm_rads`, and `dt` — the real `simuv5` variable names are `engine->Tq_cur`, `engine->rads`, and `SimDeltaTime`:

```c
if (car->battery.isEV) {
    tBattery *bat = &car->battery;
    
    /* Soft power taper below 5% SOC */
    if (bat->soc < 0.05f) {
        engine->Tq *= (bat->soc / 0.05f);
    }
    if (bat->soc <= 0.0f) {
        engine->Tq = 0.0f;
        car->carElt->_state |= RM_CAR_STATE_OUTOFGAS;
    }

    /* Discharge battery based on torque output */
    tdble power_kW = engine->Tq * engine->rads / 1000.0f;
    tdble tempFactor = 1.0f - 0.005f * MAX(0.0f, bat->temperature - 25.0f);
    tdble effective_capacity = bat->capacity * MAX(tempFactor, 0.5f);
    bat->soc -= (power_kW * SimDeltaTime) / (effective_capacity * 3600.0f);
    bat->soc = MAX(bat->soc, 0.0f);
}
```

### 6. brake.cpp – regen: add fabs() safety net and document capping behaviour

`wheel->spinVel` is signed (positive = forward, negative = reverse). The `spinVel > 0.0f` guard already prevents regen in reverse and prevents `theoretical_kW` from going negative. However, `fabs()` is a cheap defensive guard worth adding:

```c
tdble theoretical_kW = (fabs(wheel->spinVel) * brake->Tq) / 1000.0f;
```

The capping behaviour to understand: when `theoretical_kW` exceeds `bat->maxRegen / 4.0f`, `actualRatio` drops below 1.0. This means mechanical braking absorbs the uncaptured fraction — which is physically correct. The torque reduction formula `brake->Tq *= (1.0f - actualRatio)` correctly represents the fraction of total braking power that is transferred to regen. The `regenFactor` separately controls battery efficiency (energy recovered vs. heat dissipated in the motor), not torque reduction.

### 7. car.cpp – recalculate Minv after adding battery mass ⚠️

`car->Minv` (inverse mass, used in dynamics integration) must be updated after the battery mass addition, otherwise the car handles as if it weighs less than it does:

```c
if (car->battery.isEV) {
    car->mass += car->battery.capacity * 5.0f;  /* ~5 kg/kWh, Tesla-scale */
    car->Minv  = 1.0f / car->mass;              /* ← must follow mass change */
}
```
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

Both `SimEngineUpdateTq()` and `SimEngineUpdateRpm()` have identical `fuel <= 0` early-exits that must be patched (see issues #3 and #4). Inside `SimEngineUpdateTq()`, after the patched guard, the EV discharge block runs:

```c
/* Patched guard — must come before EV logic */
if ((car->fuel <= 0.0f && !car->battery.isEV) || (car->carElt->_state & ...))
{
    engine->rads = 0;
    engine->Tq   = 0;
    return;
}

if (car->battery.isEV) {
    tBattery *bat = &car->battery;

    /* Soft power taper below 5% SOC */
    if (bat->soc < 0.05f) {
        engine->Tq *= (bat->soc / 0.05f);
    }
    if (bat->soc <= 0.0f) {
        engine->Tq = 0.0f;
        car->carElt->_state |= RM_CAR_STATE_OUTOFGAS;
    }

    /* Discharge battery based on actual torque output */
    tdble power_kW = engine->Tq * engine->rads / 1000.0f;
    tdble tempFactor = 1.0f - 0.005f * MAX(0.0f, bat->temperature - 25.0f);
    tdble effective_capacity = bat->capacity * MAX(tempFactor, 0.5f);
    bat->soc -= (power_kW * SimDeltaTime) / (effective_capacity * 3600.0f);
    bat->soc = MAX(bat->soc, 0.0f);

    /* Suppress fuel deduction */
    /* (fuel deduction skipped via if-else guard at function start) */
}
else {
    /* Existing combustion fuel deduction */
    /* tdble cons = ... existing calc ... */
}
```

The existing combustion fuel deduction runs in the `else` branch — no change for non-EV cars.

---

## Regenerative Braking (brake.cpp)

Inside `SimBrakeUpdate()`, after brake torque is computed, before it is applied to the wheel:

```c
if (car->battery.isEV && brake->pressure > 0.0f && wheel->spinVel > 0.0f) {
    tBattery *bat = &car->battery;

    /* Power available from regen: P = ω × T — fabs() guards against negative spinVel slip-through */
    tdble theoretical_kW = (fabs(wheel->spinVel) * brake->Tq) / 1000.0f;

    /* Cap at battery's max regen power (split across 4 wheels) */
    tdble regenPower_kW = MIN(theoretical_kW, bat->maxRegen / 4.0f);

    /* Recover energy into battery */
    tdble tempFactor = 1.0f - 0.005f * MAX(0.0f, bat->temperature - 25.0f);
    tdble effective_capacity = bat->capacity * MAX(tempFactor, 0.5f);
    bat->soc += (regenPower_kW * bat->regenFactor * SimDeltaTime)
                / (effective_capacity * 3600.0f);
    bat->soc = MIN(bat->soc, 1.0f);

    /* Reduce mechanical brake torque by the fraction actually captured by regen.
       Energy partitions as: regen captured → battery × regenFactor,
       + motor regen heat losses × (1 - regenFactor), + remaining mechanical braking. */
    tdble actualRatio = (theoretical_kW > 0.0f)
                        ? (regenPower_kW / theoretical_kW)
                        : 0.0f;
    brake->Tq *= (1.0f - actualRatio);
}
```

Regen only runs when `spinVel > 0` (wheel moving) and brake is pressed. At zero speed, full mechanical braking applies normally.

When `theoretical_kW` exceeds `maxRegen / 4.0f`, `actualRatio < 1.0` and the uncaptured fraction remains as mechanical braking — this is correct physics. The torque reduction is determined solely by `actualRatio`, which represents the fraction of total braking captured by regen. The `regenFactor` affects only battery energy storage efficiency, not torque reduction.

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

Battery mass is added to car mass. `Minv` (inverse mass used by the dynamics integrator) must be recalculated immediately after — forgetting it leaves the car with wrong inertia:
```c
if (car->battery.isEV) {
    /* Tesla Model S ~500 kg pack for 100 kWh; scale linearly */
    car->mass += car->battery.capacity * 5.0f;
    car->Minv  = 1.0f / car->mass;  /* must follow mass change */
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
