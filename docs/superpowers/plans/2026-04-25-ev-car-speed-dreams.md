# EV Car in Speed Dreams — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a battery-powered electric vehicle to the simuv5 physics module, replacing fuel logic with SOC-based discharge and adding regenerative braking, while leaving all combustion cars unchanged.

**Architecture:** Patch the existing `simuv5` module. A new `tBattery` struct is embedded in the internal `tCar` (not the public `tCarElt`). An `isEV` flag gates every EV code path. Two public fields (`batterySOC`, `batteryTemp`) are added to `tPrivCar` in `car.h` and synced each tick in `simu.cpp`. No new module or file is created.

**Tech Stack:** C (C99-compatible), Speed Dreams GfParm XML API, CMake/Ninja build inside Docker.

---

## File Map

| File | Change |
|------|--------|
| `src/modules/simu/simuv5/carstruct.h` | Add `tBattery` struct; embed as `battery` field in `tCar` |
| `src/interfaces/car.h` | Add `batterySOC` + `batteryTemp` fields to `tPrivCar`; add `_batterySOC` / `_batteryTemp` macros |
| `src/modules/simu/simuv5/car.cpp` | Init battery from XML in `SimCarConfig()`; add battery mass + recalc `Minv` |
| `src/modules/simu/simuv5/engine.cpp` | Patch two `fuel <= 0` guards; add EV discharge block in `SimEngineUpdateTq()` |
| `src/modules/simu/simuv5/brake.cpp` | Add regen block in `SimBrakeUpdate()` |
| `src/modules/simu/simuv5/simu.cpp` | Prevent EV car elimination and sync `_batterySOC` / `_batteryTemp` each tick |
| `speed-dreams-data/data/cars/models/ev-tesla/ev-tesla.xml` | New EV car definition with `[Battery]` section |

---

## Task 1: Add `tBattery` struct to `carstruct.h`

**Files:**
- Modify: `src/modules/simu/simuv5/carstruct.h:56-101`

This is a pure header change with no logic. Do it first so all later tasks can compile against it.

- [ ] **Step 1: Add the `tBattery` typedef before the `tCar` struct**

In [carstruct.h](speed-dreams-code/src/modules/simu/simuv5/carstruct.h), insert the following block immediately before the `typedef struct {` that opens `tCar` (currently line 35):

```c
typedef struct {
    tdble capacity;     /* kWh — total usable capacity */
    tdble soc;          /* 0.0–1.0 state of charge */
    tdble maxPower;     /* kW — peak motor output */
    tdble maxRegen;     /* kW — peak regenerative braking power */
    tdble regenFactor;  /* 0.0–1.0 — regen efficiency (e.g. 0.7) */
    tdble temperature;  /* °C — battery temperature */
    int   isEV;         /* 1 = EV mode, 0 = combustion (default) */
} tBattery;
```

- [ ] **Step 2: Embed `tBattery` in `tCar`**

Inside the `tCar` struct (after the `int dammage;` field on line 88), add:

```c
    tBattery battery;   /* only active when battery.isEV == 1 */
```

- [ ] **Step 3: Verify the build still compiles**

Run inside the Docker container:
```bash
docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) simuv5 2>&1 | tail -20"
```
Expected: no errors (warnings about unused fields are acceptable).

- [ ] **Step 4: Commit**

```bash
git add speed-dreams-code/src/modules/simu/simuv5/carstruct.h
git commit -m "feat(simuv5): add tBattery struct and embed in tCar"
```

---

## Task 2: Expose battery SOC and temperature on the public `tCarElt`

**Files:**
- Modify: `src/interfaces/car.h:456-503` (the `tPrivCar` struct)

`tPrivCar` is the `priv` sub-struct of `tCarElt`. Adding fields here makes them accessible to AI drivers, the HUD, and the future CAN bus pipeline.

- [ ] **Step 1: Add fields to `tPrivCar`**

In [car.h](speed-dreams-code/src/interfaces/car.h), inside `tPrivCar` (after the `tdble fuel_consumption_instant;` field on line 469), add:

```c
    tdble   batterySOC;         /**< 0.0–1.0 state of charge (EV only) */
    tdble   batteryTemp;        /**< °C battery temperature (EV only) */
```

- [ ] **Step 2: Add convenience macros**

After the `#define _fuel priv.fuel` line (line 524), add:

```c
#define _batterySOC     priv.batterySOC
#define _batteryTemp    priv.batteryTemp
```

- [ ] **Step 3: Verify the build compiles**

```bash
docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) 2>&1 | grep -E 'error:|warning:' | head -20"
```
Expected: no new errors.

- [ ] **Step 4: Commit**

```bash
git add speed-dreams-code/src/interfaces/car.h
git commit -m "feat(car.h): expose batterySOC and batteryTemp in tPrivCar"
```

---

## Task 3: Initialise battery from XML in `SimCarConfig()`

**Files:**
- Modify: `src/modules/simu/simuv5/car.cpp:189-211`

`SimCarConfig()` reads the XML and populates the internal `tCar`. Battery init goes immediately after the existing mass + `Minv` lines (lines 209-210).

- [ ] **Step 1: Zero-initialise the battery struct unconditionally**

In [car.cpp](speed-dreams-code/src/modules/simu/simuv5/car.cpp), after line 210 (`car->Minv = (tdble)(1.0 / car->mass);`), insert:

```c
    /* Battery — zero-initialise so combustion cars are safe */
    memset(&car->battery, 0, sizeof(tBattery));

    if (GfParmExistsSection(hdle, "Battery")) {
        car->battery.isEV        = 1;
        car->battery.capacity    = GfParmGetNum(hdle, "Battery", "capacity",     "kWh", 60.0f);
        car->battery.soc         = GfParmGetNum(hdle, "Battery", "initial soc",  (char*)NULL, 1.0f);
        car->battery.maxPower    = GfParmGetNum(hdle, "Battery", "max power",    "kW",  350.0f);
        car->battery.maxRegen    = GfParmGetNum(hdle, "Battery", "max regen",    "kW",  150.0f);
        car->battery.regenFactor = GfParmGetNum(hdle, "Battery", "regen factor", (char*)NULL, 0.7f);
        car->battery.temperature = 25.0f;  /* ambient start */

        car->mass += car->battery.capacity * 5.0f;   /* ~5 kg/kWh */
        car->Minv  = 1.0f / car->mass;               /* recalc after mass change */
    }
```

- [ ] **Step 2: Verify compile**

```bash
docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) simuv5 2>&1 | tail -10"
```
Expected: no errors.

- [ ] **Step 3: Commit**

```bash
git add speed-dreams-code/src/modules/simu/simuv5/car.cpp
git commit -m "feat(simuv5/car): init tBattery from XML, add battery mass to tCar"
```

---

## Task 4: Patch the `fuel <= 0` early-exit in `SimEngineUpdateTq()`

**Files:**
- Modify: `src/modules/simu/simuv5/engine.cpp:196-201`

The current guard at line 196 fires immediately for any EV because `tank=0` makes `car->fuel` start at `0.0`. Until this is patched, none of the EV discharge code in Task 5 will ever execute.

- [ ] **Step 1: Replace the guard**

In [engine.cpp](speed-dreams-code/src/modules/simu/simuv5/engine.cpp), replace lines 196–201:

Old:
```c
    if ((car->fuel <= 0.0f) || (car->carElt->_state & (RM_CAR_STATE_BROKEN | RM_CAR_STATE_ELIMINATED)))
    {
        engine->rads = 0;
        engine->Tq = 0;
        return;
    }
```

New:
```c
    if ((car->fuel <= 0.0f && !car->battery.isEV) || (car->carElt->_state & (RM_CAR_STATE_BROKEN | RM_CAR_STATE_ELIMINATED)))
    {
        engine->rads = 0;
        engine->Tq = 0;
        return;
    }
```

- [ ] **Step 2: Patch the identical guard in `SimEngineUpdateRpm()`**

In the same file, replace lines 306–312:

Old:
```c
    if (car->fuel <= 0.0)
    {
        engine->rads = 0;
        clutch->state = CLUTCH_APPLIED;
        clutch->transferValue = 0.0;
        return 0.0;
    }
```

New:
```c
    if (car->fuel <= 0.0 && !car->battery.isEV)
    {
        engine->rads = 0;
        clutch->state = CLUTCH_APPLIED;
        clutch->transferValue = 0.0;
        return 0.0;
    }
```

- [ ] **Step 3: Verify compile**

```bash
docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) simuv5 2>&1 | tail -10"
```
Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add speed-dreams-code/src/modules/simu/simuv5/engine.cpp
git commit -m "fix(simuv5/engine): skip fuel<=0 early-exit for EV cars"
```

---

## Task 5: Add battery discharge logic in `SimEngineUpdateTq()`

**Files:**
- Modify: `src/modules/simu/simuv5/engine.cpp:261-277`

The discharge block replaces the fuel deduction (`tdble cons = Tq_cur * 0.75f; ...`) for EV cars. For combustion cars the existing deduction stays in the `else` branch.

- [ ] **Step 1: Replace the fuel deduction with an if/else**

In [engine.cpp](speed-dreams-code/src/modules/simu/simuv5/engine.cpp), replace lines 270–276:

Old:
```c
        tdble cons = Tq_cur * 0.75f;
        if (cons > 0)
        {
            car->fuel -= (tdble) (cons * engine->rads * engine->fuelcons * 0.0000001 * SimDeltaTime);
        }

        car->fuel = (tdble) MAX(car->fuel, 0.0);
```

New:
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

            /* Discharge: P = ω × T */
            tdble power_kW = engine->Tq * engine->rads / 1000.0f;
            tdble tempFactor = 1.0f - 0.005f * MAX(0.0f, bat->temperature - 25.0f);
            tdble effective_capacity = bat->capacity * MAX(tempFactor, 0.5f);
            bat->soc -= (power_kW * SimDeltaTime) / (effective_capacity * 3600.0f);
            bat->soc = MAX(bat->soc, 0.0f);
        } else {
            tdble cons = Tq_cur * 0.75f;
            if (cons > 0)
            {
                car->fuel -= (tdble) (cons * engine->rads * engine->fuelcons * 0.0000001 * SimDeltaTime);
            }
            car->fuel = (tdble) MAX(car->fuel, 0.0);
        }
```

- [ ] **Step 2: Verify compile**

```bash
docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) simuv5 2>&1 | tail -10"
```
Expected: no errors.

- [ ] **Step 3: Commit**

```bash
git add speed-dreams-code/src/modules/simu/simuv5/engine.cpp
git commit -m "feat(simuv5/engine): EV battery discharge in SimEngineUpdateTq"
```

---

## Task 6: Add regenerative braking in `SimBrakeUpdate()`

**Files:**
- Modify: `src/modules/simu/simuv5/brake.cpp:57-84`

Insert the regen block after `brake->Tq` is computed (after the TCL block, before the temperature lines). For combustion cars `isEV == 0` and the block is skipped entirely.

- [ ] **Step 1: Insert the regen block**

In [brake.cpp](speed-dreams-code/src/modules/simu/simuv5/brake.cpp), after line 77 (the closing `// ... Option TCL` comment) and before line 80 (`brake->temp -= ...`), insert:

```c
    /* Regenerative braking (EV only) */
    if (car->battery.isEV && brake->pressure > 0.0f && wheel->spinVel > 0.0f) {
        tBattery *bat = &car->battery;

        tdble theoretical_kW = (fabs(wheel->spinVel) * brake->Tq) / 1000.0f;

        /* Cap at battery's max regen power (split across 4 wheels) */
        tdble regenPower_kW = MIN(theoretical_kW, bat->maxRegen / 4.0f);

        tdble tempFactor = 1.0f - 0.005f * MAX(0.0f, bat->temperature - 25.0f);
        tdble effective_capacity = bat->capacity * MAX(tempFactor, 0.5f);
        bat->soc += (regenPower_kW * bat->regenFactor * SimDeltaTime)
                    / (effective_capacity * 3600.0f);
        bat->soc = MIN(bat->soc, 1.0f);

        /* Reduce mechanical braking by the fraction captured by regen */
        tdble actualRatio = (theoretical_kW > 0.0f)
                            ? (regenPower_kW / theoretical_kW)
                            : 0.0f;
        brake->Tq *= (1.0f - actualRatio);
    }
```

- [ ] **Step 2: Verify compile**

```bash
docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) simuv5 2>&1 | tail -10"
```
Expected: no errors.

- [ ] **Step 3: Commit**

```bash
git add speed-dreams-code/src/modules/simu/simuv5/brake.cpp
git commit -m "feat(simuv5/brake): regenerative braking for EV cars"
```

---

## Task 7: Update `simu.cpp` (prevent EV elimination and sync battery fields)

**Files:**
- Modify: `src/modules/simu/simuv5/simu.cpp:588` (Elimination guard)
- Modify: `src/modules/simu/simuv5/simu.cpp:711-724` (Tick sync)

The public `_fuel` sync happens at line 714. Add the battery sync immediately after it so dashboards, AI drivers, and future CAN pipeline code can read `_batterySOC` and `_batteryTemp`. Also, patch the fuel elimination guard to prevent EVs from being instantly removed.

- [ ] **Step 1: Patch the out-of-fuel elimination guard**

In [simu.cpp](speed-dreams-code/src/modules/simu/simuv5/simu.cpp), around line 588, there is a guard that eliminates out-of-fuel cars:

Old:
```c
    else if (((s->_maxDammage) && (car->dammage > s->_maxDammage)) ||
             (car->fuel == 0) ||
             (car->carElt->_state & RM_CAR_STATE_ELIMINATED))
```

New:
```c
    else if (((s->_maxDammage) && (car->dammage > s->_maxDammage)) ||
             (car->fuel == 0 && !car->battery.isEV) ||
             (car->carElt->_state & RM_CAR_STATE_ELIMINATED))
```

- [ ] **Step 2: Add sync lines in `SimUpdate()`**

In [simu.cpp](speed-dreams-code/src/modules/simu/simuv5/simu.cpp), after line 714 (`carElt->_fuel = car->fuel;`), add:

```c
        carElt->_batterySOC  = car->battery.soc;
        carElt->_batteryTemp = car->battery.temperature;
```

There is also a second `carElt->_fuel = car->fuel;` at line 847 (in the collision/reset path). Add the same two lines immediately after it:

```c
        carElt->_batterySOC  = car->battery.soc;
        carElt->_batteryTemp = car->battery.temperature;
```

- [ ] **Step 3: Verify compile**

```bash
docker compose run --rm speed-dreams-build bash -c "cd /workspace/build && ninja -j$(nproc) 2>&1 | tail -10"
```
Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add speed-dreams-code/src/modules/simu/simuv5/simu.cpp
git commit -m "feat(simuv5/simu): prevent EV elimination and sync battery fields"
```

---

## Task 8: Create the ev-tesla XML car definition

**Files:**
- Create: `speed-dreams-data/data/cars/models/ev-tesla/ev-tesla.xml`

This is the minimal XML needed for the car to load. It uses the existing `sc-cavallo-360` body mesh as a placeholder visual (the car will look like a 360 for now — cosmetics are out of scope). The `Battery` section is what makes `SimCarConfig()` activate EV mode.

- [ ] **Step 1: Create the directory**

```bash
mkdir -p speed-dreams-code/speed-dreams-data/data/cars/models/ev-tesla
```

- [ ] **Step 2: Write `ev-tesla.xml`**

Create [ev-tesla.xml](speed-dreams-code/speed-dreams-data/data/cars/models/ev-tesla/ev-tesla.xml) with this content:

```xml
<?xml version="1.0"?>
<!DOCTYPE params SYSTEM "../../../../src/libs/tgf/params.dtd">
<params name="EV Tesla" type="template">

  <section name="Features">
    <attstr name="shifting aero coordinate" in="yes" val="yes"/>
    <attstr name="tire temperature and degradation" in="yes" val="yes"/>
    <attstr name="enable abs" in="yes,no" val="yes"/>
  </section>

  <section name="Bonnet">
    <attnum name="xpos" val="0.85" unit="m"/>
    <attnum name="ypos" val="0.00" unit="m"/>
    <attnum name="zpos" val="0.95" unit="m"/>
  </section>

  <section name="Driver">
    <attnum name="xpos" val="0.10" unit="m"/>
    <attnum name="ypos" val="0.36" unit="m"/>
    <attnum name="zpos" val="0.85" unit="m"/>
  </section>

  <section name="Sound">
    <!-- Reuse existing sample; replace with a motor hum when available -->
    <attstr name="engine sample" val="360.wav"/>
    <attnum name="rpm scale" val="0.2"/>
  </section>

  <section name="Graphic Objects">
    <!-- Placeholder: reuse sc-cavallo-360 body mesh -->
    <attstr name="env" val="../sc-cavallo-360/sc-cavallo-360.acc"/>
    <attstr name="shadow texture" val="../sc-cavallo-360/shadow.png"/>
    <attstr name="tachometer texture" val="../sc-cavallo-360/sc-cavallo-360-rpm.png"/>
    <attnum name="tachometer min value" val="0" unit="rpm"/>
    <attnum name="tachometer max value" val="18000" unit="rpm"/>
    <attnum name="speedometer min value" val="0" unit="km/h"/>
    <attnum name="speedometer max value" val="300" unit="km/h"/>
  </section>

  <section name="Car">
    <attnum name="body length" unit="m" val="4.97"/>
    <attnum name="body width"  unit="m" val="1.96"/>
    <attnum name="body height" unit="m" val="1.44"/>
    <attnum name="overall width" unit="m" val="2.19"/>
    <attnum name="mass"  unit="kg"  val="1800"/>
    <!-- tank=0: no fuel — battery section handles energy -->
    <attnum name="fuel tank" unit="l" val="0"/>
    <attnum name="initial fuel" unit="l" min="0" max="0" val="0"/>
    <attnum name="fuel mass mult" val="0.72"/>
    <attnum name="GC height" unit="m" val="0.45"/>
    <attnum name="front-rear weight repartition" val="0.45" min="0.30" max="0.70"/>
    <attnum name="front right-left weight repartition" val="0.5" min="0.5" max="0.5"/>
    <attnum name="rear right-left weight repartition" val="0.5" min="0.5" max="0.5"/>
    <attnum name="mass repartition coefficient" val="0.7"/>
    <attnum name="yaw inertia" unit="kg.m2" val="2500"/>
    <attnum name="pitch inertia" unit="kg.m2" val="1200"/>
    <attnum name="roll inertia" unit="kg.m2" val="600"/>
  </section>

  <section name="Battery">
    <attnum name="capacity"     unit="kWh" val="100"/>
    <attnum name="initial soc"             val="1.0"/>
    <attnum name="max power"    unit="kW"  val="350"/>
    <attnum name="max regen"    unit="kW"  val="150"/>
    <attnum name="regen factor"            val="0.7"/>
  </section>

  <section name="Engine">
    <attnum name="revs limiter"  unit="rpm" val="18000"/>
    <attnum name="revs maximum"  unit="rpm" val="18000"/>
    <attnum name="tickover"      unit="rpm" val="100"/>
    <attnum name="inertia" unit="kg.m2" val="0.05"/>
    <!-- fuel cons factor=0: combustion deduction disabled; battery handles energy -->
    <attnum name="fuel cons factor" val="0"/>
    <attnum name="engine brake coefficient" val="0.005"/>
    <attnum name="engine brake linear coefficient" val="0.005"/>
    <section name="data points">
      <section name="1">
        <attnum name="rpm" unit="rpm" val="0"/>
        <attnum name="Tq"  unit="N.m" val="500"/>
      </section>
      <section name="2">
        <attnum name="rpm" unit="rpm" val="18000"/>
        <attnum name="Tq"  unit="N.m" val="300"/>
      </section>
    </section>
  </section>

  <section name="Clutch">
    <attnum name="inertia" unit="kg.m2" val="0.1"/>
  </section>

  <section name="Gearbox">
    <attnum name="shift time" unit="s" val="0.05"/>
    <section name="gears">
      <section name="r">
        <attnum name="ratio" val="-3.5"/>
      </section>
      <section name="n">
        <attnum name="ratio" val="0"/>
      </section>
      <section name="1">
        <!-- Single fixed ratio — electric motor needs no gear changes -->
        <attnum name="ratio" val="8.0"/>
        <attnum name="efficiency" val="0.95"/>
      </section>
    </section>
  </section>

  <section name="Drivetrain">
    <attstr name="type" val="RWD"/>
  </section>

  <section name="Differential">
    <attstr name="type" val="VISCOUS COUPLER"/>
    <attnum name="inertia" unit="kg.m2" val="0.1"/>
  </section>

  <section name="Front Axle">
    <attnum name="inertia" unit="kg.m2" val="0.05"/>
    <attnum name="ride height" unit="m" val="0.15"/>
    <attnum name="toe" unit="deg" val="0.0"/>
    <attnum name="camber" unit="deg" val="-1.0"/>
  </section>

  <section name="Rear Axle">
    <attnum name="inertia" unit="kg.m2" val="0.05"/>
    <attnum name="ride height" unit="m" val="0.15"/>
    <attnum name="toe" unit="deg" val="0.0"/>
    <attnum name="camber" unit="deg" val="-1.5"/>
  </section>

  <section name="Front Right Suspension">
    <attnum name="spring" unit="N/m" val="30000" min="10000" max="100000"/>
    <attnum name="suspension course" unit="m" val="0.2"/>
    <attnum name="bellcrank" val="1.6"/>
    <attnum name="packers" unit="m" val="0.05"/>
    <attnum name="slow bump" unit="N.s/m" val="3500" min="100" max="10000"/>
    <attnum name="slow rebound" unit="N.s/m" val="3500" min="100" max="10000"/>
    <attnum name="fast bump" unit="N.s/m" val="1200"/>
    <attnum name="fast rebound" unit="N.s/m" val="1200"/>
  </section>

  <section name="Front Left Suspension">
    <attnum name="spring" unit="N/m" val="30000" min="10000" max="100000"/>
    <attnum name="suspension course" unit="m" val="0.2"/>
    <attnum name="bellcrank" val="1.6"/>
    <attnum name="packers" unit="m" val="0.05"/>
    <attnum name="slow bump" unit="N.s/m" val="3500" min="100" max="10000"/>
    <attnum name="slow rebound" unit="N.s/m" val="3500" min="100" max="10000"/>
    <attnum name="fast bump" unit="N.s/m" val="1200"/>
    <attnum name="fast rebound" unit="N.s/m" val="1200"/>
  </section>

  <section name="Rear Right Suspension">
    <attnum name="spring" unit="N/m" val="35000" min="10000" max="100000"/>
    <attnum name="suspension course" unit="m" val="0.2"/>
    <attnum name="bellcrank" val="1.6"/>
    <attnum name="packers" unit="m" val="0.05"/>
    <attnum name="slow bump" unit="N.s/m" val="3500" min="100" max="10000"/>
    <attnum name="slow rebound" unit="N.s/m" val="3500" min="100" max="10000"/>
    <attnum name="fast bump" unit="N.s/m" val="1200"/>
    <attnum name="fast rebound" unit="N.s/m" val="1200"/>
  </section>

  <section name="Rear Left Suspension">
    <attnum name="spring" unit="N/m" val="35000" min="10000" max="100000"/>
    <attnum name="suspension course" unit="m" val="0.2"/>
    <attnum name="bellcrank" val="1.6"/>
    <attnum name="packers" unit="m" val="0.05"/>
    <attnum name="slow bump" unit="N.s/m" val="3500" min="100" max="10000"/>
    <attnum name="slow rebound" unit="N.s/m" val="3500" min="100" max="10000"/>
    <attnum name="fast bump" unit="N.s/m" val="1200"/>
    <attnum name="fast rebound" unit="N.s/m" val="1200"/>
  </section>

  <section name="Front Heave Spring">
    <attnum name="spring" unit="N/m" val="30000" min="0" max="1000000"/>
  </section>

  <section name="Rear Heave Spring">
    <attnum name="spring" unit="N/m" val="35000" min="0" max="1000000"/>
  </section>

  <section name="Front Anti-Roll Bar">
    <attnum name="spring" unit="N/m" val="8000" min="0" max="100000"/>
  </section>

  <section name="Rear Anti-Roll Bar">
    <attnum name="spring" unit="N/m" val="6000" min="0" max="100000"/>
  </section>

  <!-- Wheels: tyre spec cloned from sc-cavallo-360 -->
  <section name="Front Right Wheel">
    <attnum name="ypos" unit="m" val="-0.735"/>
    <attnum name="rim radius" unit="m" val="0.2286"/>
    <attnum name="tire height" unit="m" val="0.0813"/>
    <attnum name="tire width" unit="m" val="0.235"/>
    <attnum name="rim width" unit="m" val="0.200"/>
    <attnum name="mass" unit="kg" val="17"/>
    <attnum name="I" unit="kg.m2" val="1.2"/>
    <attnum name="stiffness" val="1.8"/>
    <attnum name="nominal pressure" unit="kPa" val="270"/>
    <attnum name="pressure sensitivity" val="0.05"/>
    <attnum name="rolling resistance" val="0.02"/>
    <attnum name="traction pass through tire" val="1"/>
    <attstr name="compound" val="soft"/>
  </section>

  <section name="Front Left Wheel">
    <attnum name="ypos" unit="m" val="0.735"/>
    <attnum name="rim radius" unit="m" val="0.2286"/>
    <attnum name="tire height" unit="m" val="0.0813"/>
    <attnum name="tire width" unit="m" val="0.235"/>
    <attnum name="rim width" unit="m" val="0.200"/>
    <attnum name="mass" unit="kg" val="17"/>
    <attnum name="I" unit="kg.m2" val="1.2"/>
    <attnum name="stiffness" val="1.8"/>
    <attnum name="nominal pressure" unit="kPa" val="270"/>
    <attnum name="pressure sensitivity" val="0.05"/>
    <attnum name="rolling resistance" val="0.02"/>
    <attnum name="traction pass through tire" val="1"/>
    <attstr name="compound" val="soft"/>
  </section>

  <section name="Rear Right Wheel">
    <attnum name="ypos" unit="m" val="-0.735"/>
    <attnum name="rim radius" unit="m" val="0.2413"/>
    <attnum name="tire height" unit="m" val="0.0813"/>
    <attnum name="tire width" unit="m" val="0.265"/>
    <attnum name="rim width" unit="m" val="0.230"/>
    <attnum name="mass" unit="kg" val="18"/>
    <attnum name="I" unit="kg.m2" val="1.5"/>
    <attnum name="stiffness" val="1.8"/>
    <attnum name="nominal pressure" unit="kPa" val="280"/>
    <attnum name="pressure sensitivity" val="0.05"/>
    <attnum name="rolling resistance" val="0.02"/>
    <attnum name="traction pass through tire" val="1"/>
    <attstr name="compound" val="soft"/>
  </section>

  <section name="Rear Left Wheel">
    <attnum name="ypos" unit="m" val="0.735"/>
    <attnum name="rim radius" unit="m" val="0.2413"/>
    <attnum name="tire height" unit="m" val="0.0813"/>
    <attnum name="tire width" unit="m" val="0.265"/>
    <attnum name="rim width" unit="m" val="0.230"/>
    <attnum name="mass" unit="kg" val="18"/>
    <attnum name="I" unit="kg.m2" val="1.5"/>
    <attnum name="stiffness" val="1.8"/>
    <attnum name="nominal pressure" unit="kPa" val="280"/>
    <attnum name="pressure sensitivity" val="0.05"/>
    <attnum name="rolling resistance" val="0.02"/>
    <attnum name="traction pass through tire" val="1"/>
    <attstr name="compound" val="soft"/>
  </section>

  <section name="Front Right Brake">
    <attnum name="disk diameter" unit="m" val="0.33"/>
    <attnum name="piston area" unit="m2" val="0.002"/>
    <attnum name="mu" val="0.30"/>
    <attnum name="inertia" unit="kg.m2" val="0.13"/>
  </section>

  <section name="Front Left Brake">
    <attnum name="disk diameter" unit="m" val="0.33"/>
    <attnum name="piston area" unit="m2" val="0.002"/>
    <attnum name="mu" val="0.30"/>
    <attnum name="inertia" unit="kg.m2" val="0.13"/>
  </section>

  <section name="Rear Right Brake">
    <attnum name="disk diameter" unit="m" val="0.30"/>
    <attnum name="piston area" unit="m2" val="0.002"/>
    <attnum name="mu" val="0.30"/>
    <attnum name="inertia" unit="kg.m2" val="0.13"/>
  </section>

  <section name="Rear Left Brake">
    <attnum name="disk diameter" unit="m" val="0.30"/>
    <attnum name="piston area" unit="m2" val="0.002"/>
    <attnum name="mu" val="0.30"/>
    <attnum name="inertia" unit="kg.m2" val="0.13"/>
  </section>

  <section name="Brake System">
    <attnum name="front-rear brake repartition" val="0.6" min="0.3" max="0.7"/>
    <attnum name="max pressure" unit="kPa" val="13000" min="100" max="150000"/>
  </section>

  <section name="Aerodynamics">
    <attnum name="Cx"              val="0.33"/>
    <attnum name="front area"      unit="m2" val="2.3"/>
    <attnum name="front Clift"     val="0.0"/>
    <attnum name="rear Clift"      val="0.0"/>
  </section>

  <section name="Steer">
    <attnum name="steer lock" unit="deg" val="25"/>
    <attnum name="max steer speed" unit="deg/s" val="360"/>
    <attnum name="steering help corr" val="1.0"/>
    <attnum name="steering help limit" val="0.2"/>
  </section>

</params>
```

- [ ] **Step 3: Register the car in the categories / track-cars XML**

Speed Dreams requires that a car appear in at least one category XML under `speed-dreams-data/data/cars/categories/`. Check which categories exist:

```bash
ls speed-dreams-code/speed-dreams-data/data/cars/categories/
```

Open an appropriate category file (e.g. `road-cars.xml`) and add an entry for `ev-tesla` following the same pattern as existing entries. The exact format varies per category — copy an existing `<car>` entry and change the `name` and directory.

- [ ] **Step 4: Full build**

```bash
docker compose run --rm speed-dreams-build bash /build.sh 2>&1 | tail -30
```
Expected: build completes with no errors. The install outputs the ev-tesla data files.

- [ ] **Step 5: Commit**

```bash
git add speed-dreams-code/speed-dreams-data/data/cars/models/ev-tesla/
git add speed-dreams-code/speed-dreams-data/data/cars/categories/
git commit -m "feat(data): add ev-tesla XML car definition"
```

---

## Task 9: Smoke-test — verify EV car loads and SOC drains

**Files:** (none — runtime test only)

There is no automated test harness for simuv5 physics. This task runs the game to confirm the EV path executes end-to-end.

- [ ] **Step 1: Launch the game**

```bash
./bin/run.sh
```

- [ ] **Step 2: Select the ev-tesla car in a race**

In the game menu: Practice → select any track → set car to "ev-tesla" → start.

- [ ] **Step 3: Confirm observable behaviour**

| What to observe | Expected |
|-----------------|----------|
| Car accelerates from standing start | Yes — EV motor delivers torque |
| Engine RPM rises with speed | Yes — electric motor spins |
| Braking causes less deceleration at high speed than without regen | Yes — regen absorbs some brake force |
| After depleting (very long run or set `soc=0.001` in XML), car stops | Yes — `RM_CAR_STATE_OUTOFGAS` fires |
| A combustion car (sc-cavallo-360) still works normally | Yes — `isEV=0` skips all EV paths |

- [ ] **Step 4: Commit (no code changes)**

If behaviour matches, commit the note:

```bash
git commit --allow-empty -m "test(ev-tesla): smoke-test confirms SOC drain and regen functional"
```

---

## Self-Review

### Spec coverage check

| Spec requirement | Task |
|-----------------|------|
| `tBattery` with all 7 fields | Task 1 |
| `isEV` flag in `tCar` | Task 1 |
| `engine.cpp` `fuel<=0` guard in `UpdateTq` | Task 4 |
| `engine.cpp` `fuel<=0` guard in `UpdateRpm` | Task 4 |
| Soft torque taper at low SOC | Task 5 |
| `bat->soc` drain with temp derating | Task 5 |
| `RM_CAR_STATE_OUTOFGAS` at SOC=0 | Task 5 |
| `fabs()` in regen, capping behaviour | Task 6 |
| `car->Minv` recalc after battery mass | Task 3 |
| Battery XML init via `GfParmExistsSection` | Task 3 |
| Public `batterySOC` / `batteryTemp` fields | Tasks 2 + 7 |
| `ev-tesla.xml` with `[Battery]` section | Task 8 |
| Combustion cars unaffected | `isEV=0` default in Tasks 1, 3 |

All spec requirements are covered.

### Placeholder scan

No TBD, TODO, or "handle edge cases" phrases remain. All code blocks are complete.

### Type consistency

- `tBattery` defined in Task 1, used in Tasks 3, 4, 5, 6, 7 — consistent.
- `bat->soc`, `bat->capacity`, `bat->regenFactor`, `bat->temperature`, `bat->maxRegen` — same field names throughout.
- `_batterySOC` / `_batteryTemp` macros defined in Task 2, used in Task 7 — consistent.
- `GfParmExistsSection` (the correct API name from `tgf.h:506`) used in Task 3.
