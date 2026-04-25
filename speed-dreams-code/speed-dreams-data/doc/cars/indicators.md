Here are the list of features related to measuring instruments for SSGGraph in car config:

## Basics

`tachometer texture`, `speedometer texture` - picture used as background, both width and height of the picture have to be power of 2 value, e.g. 128, 256 and so on.

```
<attstr name="tachometer texture" val="rpm20000.png"/>
```

If the texture's width and height are not equal, it is needed to set dimensions to avoid stretching.

```
<attnum name="tachometer width" val="256"/>
```

## Positioning

Position is the coordinate of the bottom left angle of the background, where x=0 at the middle of the screen, x>0 to the right, y=0 at the bottom of the screen, y>0 to the top of the screen.

Positioning of the scales is done by "x pos" and "y pos" properties and can be set as follows:

```
<attnum name="tachometer x pos" val="-128"/>
<attnum name="tachometer y pos" val="-128"/>
```

`tachometer min value` - minimum value represented by the scale, doesn't have to match tickover value.

```
<attnum name="tachometer min value" unit="rpm" val="0"/>
```

`tachometer max value` - maximum value represented by scale, set bigger or equal to max rpm.

```
<attnum name="tachometer max value" unit="rpm" val="7000"/>
```

`speedometer min value`, `speedometer max value`:

```
<attnum name="speedometer min value" unit="km/h" val="0"/>
<attnum name="speedometer max value" unit="km/h" val="240"/>
```

## Angles

Minimum and maximum angle can be set for both scales. Minimum angle is where the needle would be located when representing minimum value of the scale. Same goes for maximum angle.

Needle directions:

- 0 degrees - pointing to the right

- 90 degrees (or -270) - to bottom

- 180 degrees - to left

- 270 degrees (or -90) - to top

Angles can be positive or negative. If the minimum angle is positive, the needle rotates in clockwise direction. If negative - counter-clockwise.

```
<attnum name="tachometer min angle" unit="deg" val="180"/>
<attnum name="tachometer max angle" unit="deg" val="0"/>
```

## Needles

`needle width` - length of the needle

`needle height` - thickness of the needle

`needle x center`, `needle y center` - set position of a rotation axis for needle, by default it is in the middle of the indicator. Position is set related to the meter.

```
<attnum name="tachometer needle width" val="120"/>
<attnum name="tachometer needle height" val="10"/>
<attnum name="tachometer needle y center" val="0"/>
```

## Digits
Speedometer and tachometer both have additional values. `speedometer digit` displays speed, `tachometer digit` shows selected gear. Digits can be positioned too, position is set related to screen.

```
<attnum name="tachometer digit y center" val="-200"/>
<attnum name="tachometer needle y center" val="0"/>
```

## Needle color
Needle color is set as separate RGBA values. The color applies to digits as well.

Example of white color:

```
<attnum name="needle red" val="1"/>
<attnum name="needle green" val="1"/>
<attnum name="needle blue" val="1"/>
<attnum name="needle alpha" val="1"/>
```

## How to hide a meter from UI
- Set transparent texture for the meter
- Move meter's text outside of screen
- Set meter's needle width and height to 0.

### Example for tachometer:

```
<attstr name="tachometer texture" val="transparent-picture.png"/>
<attnum name="tachometer digit y center" val="-200"/>
<attnum name="tachometer needle width" val="0"/>
<attnum name="tachometer needle height" val="0"/>
```