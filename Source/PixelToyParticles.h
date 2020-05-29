//  PixelToy Particles header

void ParticleEditor(void);
void ResetParticles(void);
char CreateNewGenerator(void);
void SetLiveParticleSliderValue(short item, short value);
void ValidateParticleGeneratorBrightnesses(void *aSet);
void DoParticles(void);
long LongAbsolute(long n);

/* Particle brainstorm

individual particles:
x, y (0,0 = unused)
dx, dy
generator id #

generator parms:
behavior: 0=empty generator 1=rain 2=fountain
solid thresh: 0-255
rate 1-10
rate variance +- 0-10
gravity 0-100
x location 0-255
x loc variance +- 0-255
y location 0-255
y loc variance +- 0-255
x speed -10 to 10
x speed variance +- 0-10
y speed -10 to 10
y speed variance +- 0-10
	Str255		pg_name[MAXGENERATORS];
	short		pg_pbehavior[MAXGENERATORS];
	short		pg_solid[MAXGENERATORS];
	short		pg_rate[MAXGENERATORS];
	short		pg_ratev[MAXGENERATORS];
	short		pg_gravity[MAXGENERATORS];
	short		pg_xloc[MAXGENERATORS];
	short		pg_xlocv[MAXGENERATORS];
	short		pg_yloc[MAXGENERATORS];
	short		pg_ylocv[MAXGENERATORS];
	short		pg_dx[MAXGENERATORS];
	short		pg_dxv[MAXGENERATORS];
	short		pg_dy[MAXGENERATORS];
	short		pg_dyv[MAXGENERATORS];
*/