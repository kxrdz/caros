# EV Car in Speed Dreams — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a driveable electric car to Speed Dreams with battery SOC replacing fuel and regenerative braking recovering energy during deceleration.

**Architecture:** Patch `simuv5` physics module — add a `tBattery` struct to `carstruct.h`, gate all EV logic behind `battery.isEV`, replace fuel deduction with battery discharge in `engine.cpp`, inject regen into `brake.cpp`. All existing combustion cars are unaffected. A new `ev-tesla` car XML activates EV mode.

**Tech Stack:** C++ (simuv5 physics), Speed Dreams XML car config format (`GfParmGetNum`), CMake/Ninja build inside Docker.

---

## File Map

| File | What changes |
|------|-------------|
| `src/modules/simu/simuv5/carstruct.h` | Add `tBattery` typedef; embed `battery` field in `tCar` |
| `src/interfaces/car.h` | Add `batterySOC` + `batteryTemp` to `tCarPriv` (line ~469) |
| `src/modules/simu/simuv5/car.cpp` | Init battery in `SimCarConfig()` (line ~189); sync public fields in `SimCarUpdateSpeed()` (line ~768); include battery mass in `SimCarUpdateForces()` (line ~639) |
| `src/modules/simu/simuv5/engine.cpp` | After line 274 (fuel deduction): wrap in `if (!isEV)`, add EV discharge block |
| `src/modules/simu/simuv5/brake.cpp` | After line 83 in `SimBrakeUpdate()`: add regen block before function returns |
| `speed-dreams-data/data/cars/models/ev-tesla/ev-tesla.xml` | New car XML with `[Battery]` section |
| `speed-dreams-data/data/cars/categories/EV.xml` | New category file for EV cars |

---

## Task 1: Add tBattery struct to carstruct.h

**Files:**
- Modify: `speed-dreams-code/src/modules/simu/simuv5/carstruct.h:52`

- [ ] **Step 1: Open carstruct.h and verify the insertion point**

  Read `carstruct.h`. The `tCar` struct ends around line 101. The `engine` field is on line 52. We will add `tBattery` typedef before the `tCar` struct and add `battery` after the `engine` field.

- [ ] **Step 2: Add tBattery typedef before the tCar struct**

  In `speed-dreams-code/src/modules/simu/simuv5/carstruct.h`, insert after the `#include "simulationOptions.h"` line (line 33), before `typedef struct {`:

  ```c
  typedef struct {
      tdble capacity;     /* kWh — total usable capacity */
      tdble soc;          /* 0.0–1.0 state of charge */
      tdble maxPower;     /* kW — peak motor output */
      tdble maxRegen;     /* kW — peak regenerative braking power */
      tdble regenFactor;  /* 0.0–1.0 — regen efficiency */
      tdble temperature;  /* °C — battery temperature, affects capacity */
      int   isEV;         /* 1 = EV mode active, 0 = combustion */
  } tBattery;
  ```

- [ ] **Step 3: Embed battery field in tCar**

  In the `tCar` struct, after line 52 (`tEngine engine;`), add:

  ```c
      tBattery    battery;
  ```

- [ ] **Step 4: Verify the file compiles**

  ```bash
  docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) 2>&1 | head -40"
  ```

  Expected: compile error only if other tasks haven't been done yet — but `carstruct.h` alone adds only a struct definition, so no errors from this file specifically. If errors mention `tBattery`, fix the typedef syntax.

- [ ] **Step 5: Commit**

  ```bash
  git add speed-dreams-code/src/modules/simu/simuv5/carstruct.h
  git commit -m "feat(simu): add tBattery struct to tCar for EV support"
  ```

---

## Task 2: Expose battery fields in public car interface (car.h)

**Files:**
- Modify: `speed-dreams-code/src/interfaces/car.h:469`

- [ ] **Step 1: Add batterySOC and batteryTemp to tCarPriv**

  In `speed-dreams-code/src/interfaces/car.h`, after line 469 (`fuel_consumption_instant`):

  ```c
      tdble   batterySOC;             /**< 0.0–1.0 state of charge (EV only) */
      tdble   batteryTemp;            /**< °C battery temperature (EV only) */
  ```

- [ ] **Step 2: Add shortcut macros**

  Near line 505 (where `_fuelInstant` macro is defined), add:

  ```c
  #define _batterySOC     priv.batterySOC
  #define _batteryTemp    priv.batteryTemp
  ```

- [ ] **Step 3: Commit**

  ```bash
  git add speed-dreams-code/src/interfaces/car.h
  git commit -m "feat(interface): expose batterySOC and batteryTemp in tCarPriv"
  ```

---

## Task 3: Initialise battery in SimCarConfig (car.cpp)

**Files:**
- Modify: `speed-dreams-code/src/modules/simu/simuv5/car.cpp:189`

- [ ] **Step 1: Find the initialisation block**

  In `car.cpp`, line 189 reads:
  ```c
  car->tank = GfParmGetNum(hdle, SECT_CAR, PRM_TANK, (char*)NULL, 80);
  ```
  We insert our battery init block immediately after line 196 (after `car->fuelMass = ...`).

- [ ] **Step 2: Add battery initialisation**

  After line 196 (`car->fuelMass = GfParmGetNum(...)`), insert:

  ```c
      /* EV battery initialisation — only active when [Battery] section present */
      car->battery.isEV = 0;
      if (GfParmSectionExists(hdle, "Battery")) {
          car->battery.isEV        = 1;
          car->battery.capacity    = GfParmGetNum(hdle, "Battery", "capacity",    "kWh", 60.0f);
          car->battery.soc         = GfParmGetNum(hdle, "Battery", "initial soc", (char*)NULL, 1.0f);
          car->battery.maxPower    = GfParmGetNum(hdle, "Battery", "max power",   "kW",  350.0f);
          car->battery.maxRegen    = GfParmGetNum(hdle, "Battery", "max regen",   "kW",  150.0f);
          car->battery.regenFactor = GfParmGetNum(hdle, "Battery", "regen factor",(char*)NULL, 0.7f);
          car->battery.temperature = 25.0f;
          /* Zero out fuel for EV — no combustion tank */
          car->fuel  = 0.0f;
          car->tank  = 0.0f;
      }
  ```

- [ ] **Step 3: Add battery mass to car mass**

  In `car.cpp`, line 209 reads `car->mass = GfParmGetNum(...)`. After line 210 (`car->Minv = ...`), add:

  ```c
      if (car->battery.isEV) {
          /* ~5 kg per kWh (Tesla Model S pack approximation) */
          car->mass += car->battery.capacity * 5.0f;
          car->Minv  = (tdble)(1.0 / car->mass);
      }
  ```

- [ ] **Step 4: Sync public battery fields each tick in SimCarUpdateSpeed**

  In `car.cpp` around line 782 (after `_fuelInstant` is written), add:

  ```c
      /* Sync EV battery state to public interface */
      if (car->battery.isEV) {
          car->carElt->_batterySOC  = car->battery.soc;
          car->carElt->_batteryTemp = car->battery.temperature;
      }
  ```

- [ ] **Step 5: Build to check for compile errors**

  ```bash
  docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) 2>&1 | grep -E 'error:|warning:' | head -30"
  ```

  Expected: no errors from `car.cpp`. Warnings about unused variables are acceptable at this stage.

- [ ] **Step 6: Commit**

  ```bash
  git add speed-dreams-code/src/modules/simu/simuv5/car.cpp
  git commit -m "feat(simu): init and sync EV battery in SimCarConfig and SimCarUpdateSpeed"
  ```

---

## Task 4: Replace fuel deduction with EV battery discharge (engine.cpp)

**Files:**
- Modify: `speed-dreams-code/src/modules/simu/simuv5/engine.cpp:270`

- [ ] **Step 1: Locate the fuel deduction block**

  Current code at lines 270–276:
  ```c
  tdble cons = Tq_cur * 0.75f;
  if (cons > 0)
  {
      car->fuel -= (tdble) (cons * engine->rads * engine->fuelcons * 0.0000001 * SimDeltaTime);
  }
  car->fuel = (tdble) MAX(car->fuel, 0.0);
  ```

- [ ] **Step 2: Wrap combustion block and add EV discharge block**

  Replace those 6 lines with:

  ```c
  if (car->battery.isEV) {
      tBattery *bat = &car->battery;

      /* Soft taper below 5% SOC */
      if (bat->soc < 0.05f) {
          engine->Tq *= (bat->soc / 0.05f);
          Tq_cur = engine->Tq + EngBrkK;
      }
      if (bat->soc <= 0.0f) {
          engine->Tq  = -EngBrkK;
          Tq_cur      = 0.0f;
          car->pub.state |= RM_CAR_STATE_OUTOFGAS;
      }

      /* Discharge: P = T × ω → kW */
      tdble power_kW = (Tq_cur > 0.0f) ? (Tq_cur * engine->rads / 1000.0f) : 0.0f;
      tdble tempFactor       = 1.0f - 0.005f * MAX(0.0f, bat->temperature - 25.0f);
      tdble effective_cap    = bat->capacity * MAX(tempFactor, 0.5f);
      bat->soc -= (power_kW * SimDeltaTime) / (effective_cap * 3600.0f);
      bat->soc  = MAX(bat->soc, 0.0f);
  } else {
      /* Original combustion fuel deduction */
      tdble cons = Tq_cur * 0.75f;
      if (cons > 0) {
          car->fuel -= (tdble)(cons * engine->rads * engine->fuelcons * 0.0000001 * SimDeltaTime);
      }
      car->fuel = (tdble)MAX(car->fuel, 0.0);
  }
  ```

- [ ] **Step 3: Build**

  ```bash
  docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) 2>&1 | grep -E 'error:|warning:' | head -30"
  ```

  Expected: no errors. If `car->pub` is not accessible, use `car->carElt->pub` — check the `tCar` struct to confirm the correct member name for the public state.

- [ ] **Step 4: Commit**

  ```bash
  git add speed-dreams-code/src/modules/simu/simuv5/engine.cpp
  git commit -m "feat(simu): EV battery discharge in engine, combustion unchanged"
  ```

---

## Task 5: Add regenerative braking (brake.cpp)

**Files:**
- Modify: `speed-dreams-code/src/modules/simu/simuv5/brake.cpp:83`

- [ ] **Step 1: Locate insertion point in SimBrakeUpdate**

  Current last lines of `SimBrakeUpdate()` (lines 80–84):
  ```c
  brake->temp -= (tdble)(fabs(car->DynGC.vel.x) * 0.0001 + 0.0002);
  if (brake->temp < 0) brake->temp = 0;
  brake->temp += (tdble)(brake->pressure * brake->radius * fabs(wheel->spinVel) * 0.00000000005);
  if (brake->temp > 1.0) brake->temp = 1.0;
  ```
  We insert regen **after** the temperature update, before the closing `}`.

- [ ] **Step 2: Add regen block**

  After line 83 (`if (brake->temp > 1.0) brake->temp = 1.0;`), add:

  ```c
      /* Regenerative braking — EV only */
      if (car->battery.isEV && brake->pressure > 0.0f && wheel->spinVel > 0.0f) {
          tBattery *bat = &car->battery;

          /* Power available from braking: P = ω × T (in kW) */
          tdble theoretical_kW = (wheel->spinVel * brake->Tq) / 1000.0f;

          /* Cap at one quarter of battery's max regen (split across 4 wheels) */
          tdble regenPower_kW = MIN(theoretical_kW, bat->maxRegen / 4.0f);

          /* Recover energy into battery with temperature derating */
          tdble tempFactor    = 1.0f - 0.005f * MAX(0.0f, bat->temperature - 25.0f);
          tdble effective_cap = bat->capacity * MAX(tempFactor, 0.5f);
          bat->soc += (regenPower_kW * bat->regenFactor * SimDeltaTime)
                      / (effective_cap * 3600.0f);
          bat->soc  = MIN(bat->soc, 1.0f);

          /* Reduce mechanical brake torque by the fraction actually captured */
          tdble actualRatio = (theoretical_kW > 0.0f)
                              ? (regenPower_kW / theoretical_kW)
                              : 0.0f;
          brake->Tq *= (1.0f - actualRatio * bat->regenFactor);
      }
  ```

- [ ] **Step 3: Add SimDeltaTime include check**

  `SimDeltaTime` is declared in `sim.h` which is already included at the top of `brake.cpp` (line 19: `#include "sim.h"`). No additional include needed.

- [ ] **Step 4: Build**

  ```bash
  docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) 2>&1 | grep -E 'error:|warning:' | head -30"
  ```

  Expected: no errors.

- [ ] **Step 5: Commit**

  ```bash
  git add speed-dreams-code/src/modules/simu/simuv5/brake.cpp
  git commit -m "feat(simu): regenerative braking with corrected torque reduction"
  ```

---

## Task 6: Create EV car XML and category

**Files:**
- Create: `speed-dreams-code/speed-dreams-data/data/cars/models/ev-tesla/ev-tesla.xml`
- Create: `speed-dreams-code/speed-dreams-data/data/cars/categories/EV.xml`

- [ ] **Step 1: Copy an existing car XML as baseline**

  ```bash
  cp -r speed-dreams-code/speed-dreams-data/data/cars/models/sc-cavallo-360 \
        speed-dreams-code/speed-dreams-data/data/cars/models/ev-tesla
  mv speed-dreams-code/speed-dreams-data/data/cars/models/ev-tesla/sc-cavallo-360.xml \
     speed-dreams-code/speed-dreams-data/data/cars/models/ev-tesla/ev-tesla.xml
  ```

- [ ] **Step 2: Edit ev-tesla.xml — update Car section**

  Find the `[Car]` section and set:
  ```xml
  <section name="Car">
    <attnum name="mass" unit="kg" val="1800"/>
    <attnum name="fuel tank" unit="l" val="0"/>
    <attnum name="initial fuel" unit="l" val="0"/>
  </section>
  ```

- [ ] **Step 3: Add Battery section to ev-tesla.xml**

  After the `[Car]` closing tag, insert:
  ```xml
  <section name="Battery">
    <attnum name="capacity"    unit="kWh" val="100"/>
    <attnum name="initial soc" val="1.0"/>
    <attnum name="max power"   unit="kW"  val="350"/>
    <attnum name="max regen"   unit="kW"  val="150"/>
    <attnum name="regen factor" val="0.7"/>
  </section>
  ```

- [ ] **Step 4: Update Engine section for flat EV torque curve**

  Replace the existing torque table in `[Engine]`:
  ```xml
  <section name="Engine">
    <attnum name="fuel cons factor" val="0"/>
    <section name="Tq">
      <attnum name="rpm 1" val="0"/>     <attnum name="tq 1" val="500"/>
      <attnum name="rpm 2" val="9000"/>  <attnum name="tq 2" val="500"/>
      <attnum name="rpm 3" val="18000"/> <attnum name="tq 3" val="300"/>
    </section>
    <attnum name="revs limiter" unit="rpm" val="18000"/>
    <attnum name="inertia" val="0.1"/>
  </section>
  ```

- [ ] **Step 5: Create EV category file**

  Create `speed-dreams-code/speed-dreams-data/data/cars/categories/EV.xml`:
  ```xml
  <?xml version="1.0" encoding="UTF-8"?>
  <!DOCTYPE params SYSTEM "../../libs/tgf/params.dtd">
  <params name="EV" type="category" mode="mw">
    <section name="Category">
      <attstr name="name" val="EV"/>
    </section>
  </params>
  ```

- [ ] **Step 6: Commit**

  ```bash
  git add speed-dreams-code/speed-dreams-data/data/cars/models/ev-tesla/
  git add speed-dreams-code/speed-dreams-data/data/cars/categories/EV.xml
  git commit -m "feat(cars): add ev-tesla car with battery config and EV category"
  ```

---

## Task 7: Full build and smoke test

- [ ] **Step 1: Full rebuild inside Docker**

  ```bash
  docker compose run --rm speed-dreams-build bash /build.sh
  ```

  Expected: build completes with no errors. Warnings are acceptable.

- [ ] **Step 2: Launch the game**

  ```bash
  ./bin/run.sh
  ```

  Expected: Speed Dreams opens.

- [ ] **Step 3: Verify EV car appears**

  In the game UI: go to **Race → Practice → Select Car**. The `ev-tesla` car should appear in the EV category (or the car selection list). Select it and start a race.

- [ ] **Step 4: Drive and verify battery drains**

  Accelerate for 30 seconds. The fuel gauge (repurposed as battery) should show a lower value. If the HUD shows `_fuel` (which will be 0 for EV), check that `_batterySOC` is being read — the default HUD shows fuel, so you may see 0 until the HUD is updated.

- [ ] **Step 5: Verify regen braking**

  Accelerate to 100 km/h, then brake hard. The battery SOC should increase slightly (check via debug print or `_batterySOC` in a robot/logger).

- [ ] **Step 6: Add a quick debug print to verify values (temporary)**

  In `car.cpp` around line 782, temporarily add:
  ```c
  if (car->battery.isEV && ((int)(SimTime * 10) % 50 == 0)) {
      fprintf(stderr, "EV SOC: %.3f  Temp: %.1f°C\n",
              car->battery.soc, car->battery.temperature);
  }
  ```
  Run the game and watch terminal output. Remove this line after confirming values look correct.

- [ ] **Step 7: Remove debug print and final commit**

  ```bash
  git add speed-dreams-code/src/modules/simu/simuv5/car.cpp
  git commit -m "feat(ev): verified EV car runs — battery drains and regen works"
  ```

---

## Self-Review Checklist

- [x] `tBattery` struct defined before `tCar` → Task 1
- [x] `isEV` flag gates all EV logic → Tasks 3, 4, 5
- [x] Combustion cars unchanged (`else` branch in engine.cpp) → Task 4
- [x] Battery discharge formula: `P_kW × dt / (cap_kWh × 3600)` → Task 4
- [x] Regen corrected formula — actualRatio accounts for maxRegen cap → Task 5
- [x] Temperature derating in both discharge and regen → Tasks 4, 5
- [x] SOC clamped [0, 1] → Tasks 4, 5
- [x] Public fields `_batterySOC` / `_batteryTemp` synced every tick → Task 3
- [x] Battery mass added to `car->mass` and `Minv` recalculated → Task 3
- [x] EV car XML with `[Battery]` section → Task 6
- [x] Smoke test with debug print → Task 7
