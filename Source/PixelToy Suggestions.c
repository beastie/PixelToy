/*                      Updates/Suggestions Digest PT 2.5

Loose ends, ideas, planned features, etc.

¥ crashes hard during sound input on G4 Cube

¥ more gracefully handle window resize out-of-memory situations

¥ define default text/image/mask in preferences

¥ Plug-in architecture for actions and filters

¥ MP3 reaction (SoundJam plug-in? MP3 playing capability? Something else?)

¥ live video in image editor

¥ quicktime in image editor

¥ Altivec-optimized filters

¥ More (3D) actions

¥ When in 8bit, image reloads cause gray flash.

¥ Live QT output (incl. sound)
  HARD?: Dialog after selecting Create Movie, asks
  ( ) Real-Time or (¥) Fixed + [X] Include Sound

¥ Bitmap action to be continuously applied
  HARD: New Action, "Image".  Will open a QuickTime movie or QT-openable
  image.  Editor with preview, low slider and high slider to define visible
  area, checkbox to allow resize of window to image/movie size, and if
  movie, specify FPS sample rate.   Store FSSpec, low, high, and sample
  rate in set information.  Change preferences entry to "Complain about
  missing fonts/images".  This action occurs before any other actions.
  Real color/PixelToy color option? Optionally (¥) Loop Movie ( ) Last
  Frame ( ) Nothing

m¥ Spin filters
  HARD?: but could make new filters inaccessible until registering.
  Accessible via option-letter keystrokes allows another 26 filters.

¥ It would be nice if loading a set did not affect parameters of unused
  actions.
  HARD: If I do it, should it be optional in Preferences?

¥ AppleScripting support
  HARD: tried this nastiness already.  Try again later.

This program is using following calculation. Consult it if You need.

3D point (dot point)    = (x,y,z)
view point              = (ox,oy,oz)
screen point            = (ex,ey,ez)

a    =sqrt((ex-ox)^2+(ey-oy)^2);
b    =sqrt((ex-ox)^2+(ey-oy)^2+(ez-oz)^2);
sin_s    =(ey-oy)/a;
cos_s    =(ex-ox)/a;
sin_f    =(ez-oz)/b;
cos_f    =a/b;

d  =cos_f*cos_s*(x-ex)+cos_f*sin_s*(y-ey)    +sin_f*(z-ez)
h  =-sin_s*(x-ex) +cos_s*(y-ey)
v  =-sin_f*cos_s*(x-ex) -sin_f*sin_s*(y-ey)+cos_f*(z-ez);

sx  =-h/d
sy  =-v/d

2D point (screen point)  =(sx,sy)

Filter speed check:

B: 15 FPS Blur More     now 30 FPS
H: 20 FPS Vert Smear	now 35 FPS
J: 18 FPS Zoom Fast		now 23 FPS
M: 18 FPS Horiz Spread	now 23 FPS

A: 50 FPS
C: 50 FPS
D: 35 FPS
E: 35 FPS
F: 35 FPS
G: 30 FPS
I: 33 FPS
K: 30 FPS
L: 30 FPS
N: 35 FPS
O: 35 FPS
P: 35 FPS
Q: 80 FPS
R: 30 FPS
S: 80 FPS
T: 50 FPS
U: 35 FPS
V: 40 FPS
W: 60 FPS
X: 80 FPS
*/
