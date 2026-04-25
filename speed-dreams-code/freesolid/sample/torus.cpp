#include "SDL.h"
#include "SDL_opengl.h"

#include <SOLID/solid.h>
#include <3D/Point.h>
#include <3D/Quaternion.h>

#define USE_QUADS

typedef struct MyObject
{
  int id;
} MyObject;

GLfloat LightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightPosition[]= { 0.0f, 0.0f, 200.0f, 100.0f };

/* ARGSUSED */
void collide1(void * client_data, DtObjectRef obj1, DtObjectRef obj2,
	      const DtCollData *coll_data)
{
  glColor3f(0.125f,1.0f,0.125f);
}

/* ARGSUSED */
void collide2(void * client_data, DtObjectRef obj1, DtObjectRef obj2,
	      const DtCollData *coll_data) {
  FILE *stream = (FILE *)client_data;
  fprintf(stream, "Object %d interferes with object %d\n",
	  (*(MyObject *)obj1).id, (*(MyObject *)obj2).id);
}

MyObject object1, object2;
DtShapeRef shape;

int main(int argc, char *argv[]);
void DrawTorus();
void CreateObjects();

int main(int argc, char *argv[])
{
  float fTorus1Rotation = 0.0f;
  float fTorus2Rotation = 0.0f;
  float offset=0;
  float speed=0.5f;
  Quaternion q1,q2;
  if(SDL_Init(SDL_INIT_VIDEO)<0) // try to initialize Video System
    {
      fprintf( stderr, "Video initialization failed: %s\n",SDL_GetError()); // if it failed, find out why
      return -1;
    }

  SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 ); // Set red frame buffer size
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 ); // Set green frame buffer size
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 ); // Set blue frame buffer size
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 ); // Set depth frame buffer size
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,1); // Tell SDL we want Double buffering

  if(SDL_SetVideoMode(800,600,0,SDL_OPENGL) == 0 )
    {
      fprintf(stderr, "Video mode set failed: %s\n",SDL_GetError()); // why did it failed?
      SDL_Quit(); // Release whatever SDL grabbed during SDL_Init
      return -1;
    }

  glViewport(0,0,800,600); // set the view port to take the whole screen
  glMatrixMode(GL_PROJECTION); // switch to the Projection Matrix
  glLoadIdentity(); // reset Projection Matrix
  gluPerspective(45.0f,800.0f/600.0f, 0.1f ,100.0f); // set our view to perspective
  glMatrixMode(GL_MODELVIEW); // switch to the Model View Matrix
  glLoadIdentity(); // reset Model View Matrix
  glClearColor(0.0f,0.0f,0.0f,0.0f); // Set our Clear Color to Black
  glShadeModel(GL_SMOOTH); // Enable Smooth Shading
  glClearDepth(1.0f); // Set the depth buffer
  glEnable(GL_DEPTH_TEST); // Enables Depth Testing
  glDepthFunc(GL_LEQUAL); // Type of depth test
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Use really nice perspective calculations

  //glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
  //glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
  //glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);
  //glEnable(GL_LIGHT1);
  //glEnable(GL_LIGHTING);

  bool bRunning = true;
  SDL_Event event;
  CreateObjects();
  while(bRunning)
    {
      if(SDL_PollEvent(&event))
	{
	  switch(event.type)
	    {
            case SDL_QUIT: // test to see if the user closed the app
	      bRunning = false;
	      break;
            case SDL_KEYDOWN: /* Handle a KEYDOWN event */
	      switch(event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
		case SDLK_q:
		  bRunning = false; // quit if ESCAPE is pressed
		  break;
		}
	      break;
	    }
	}
      if(offset>30.0f) speed = -0.5f;
      else if(offset<-30.0f) speed = 0.5f;

      offset+=speed;
      dtSelectObject(&object1);
      dtLoadIdentity();
      dtTranslate(offset,0.0f,-80.0f);
      q1.setRotation(Vector(1.0f,0.0f,0.0f),90);
      dtRotate(q1[X], q1[Y], q1[Z], q1[W]);

      dtSelectObject(&object2);
      dtLoadIdentity();
      dtTranslate(0.0f,0.0f,-80.0f);
      //q2.setRotation(Vector(0.0f,1.0f,-80.0f),fTorus2Rotation);
      //dtRotate(q2[X], q2[Y], q2[Z], q2[W]);

      dtTest();

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glLoadIdentity();
      glTranslatef(offset,0.0f,-80.0f);
      glRotatef(90,1.0f,0.0f,0.0f);
      DrawTorus();

      glLoadIdentity();
      glTranslatef(0.0f,0.0f,-80.0f);
      //glRotatef(fTorus2Rotation,0.0f,1.0f,0.0f);
      DrawTorus();
      glColor3f(1.0f,1.0f,1.0f);

      SDL_GL_SwapBuffers();
    }
  dtDeleteObject(&object1);
  dtDeleteObject(&object2);
  dtDeleteShape(shape);
  return 0;
}

void DrawTorus()
{
  // Taken from sample.cpp
  double a = 10;
  double b = 2;

  const int n1 = 50;
  const int n2 = 50;

  for (int uc = 0; uc < n1; uc++)
    for (int vc = 0; vc < n2; vc++) {
      double u1 = (TWO_PI*uc) / n1;
      double u2 = (TWO_PI*(uc+1)) / n1;
      double v1 = (TWO_PI*vc) / n2;
      double v2 = (TWO_PI*(vc+1)) / n2;

      double p1[3], p2[3], p3[3], p4[3];

      p1[0] = (a - b * cos(v1)) * cos(u1);
      p2[0] = (a - b * cos(v1)) * cos(u2);
      p3[0] = (a - b * cos(v2)) * cos(u1);
      p4[0] = (a - b * cos(v2)) * cos(u2);
      p1[1] = (a - b * cos(v1)) * sin(u1);
      p2[1] = (a - b * cos(v1)) * sin(u2);
      p3[1] = (a - b * cos(v2)) * sin(u1);
      p4[1] = (a - b * cos(v2)) * sin(u2);
      p1[2] = b * sin(v1);
      p2[2] = b * sin(v1);
      p3[2] = b * sin(v2);
      p4[2] = b * sin(v2);

#ifdef USE_QUADS
      glBegin(GL_QUADS);
      {
	glVertex3f(p1[0], p1[1], p1[2]);
	glVertex3f(p2[0], p2[1], p2[2]);
	glVertex3f(p4[0], p4[1], p4[2]);
	glVertex3f(p3[0], p3[1], p3[2]);
      }
      glEnd();
#else
      glBegin(GL_TRIANGLES);
      {
	glVertex3f(p4[0], p4[1], p4[2]);
	glVertex3f(p1[0], p1[1], p1[2]);
	glVertex3f(p2[0], p2[1], p2[2]);
      }
      glEnd();
#endif
    }
}

void CreateObjects()
{
  object1.id = 1;
  object2.id = 2;

  shape = dtNewComplexShape();

  double a = 10;
  double b = 2;

  fprintf(stdout, "Loading a torus with a major radius of %d and a minor radius of %d%,\n", (int)a, (int)b);

  const int n1 = 50;
  const int n2 = 50;

#ifdef USE_QUADS
  fprintf(stdout, "composed of %d quads...",n1 * n2); fflush(stdout);
#else
  fprintf(stdout, "composed of %d triangles...", 2 * n1 * n2); fflush(stdout);
#endif
  for (int uc = 0; uc < n1; uc++)
    for (int vc = 0; vc < n2; vc++) {
      double u1 = (TWO_PI*uc) / n1;
      double u2 = (TWO_PI*(uc+1)) / n1;
      double v1 = (TWO_PI*vc) / n2;
      double v2 = (TWO_PI*(vc+1)) / n2;

      double p1[3], p2[3], p3[3], p4[3];

      p1[0] = (a - b * cos(v1)) * cos(u1);
      p2[0] = (a - b * cos(v1)) * cos(u2);
      p3[0] = (a - b * cos(v2)) * cos(u1);
      p4[0] = (a - b * cos(v2)) * cos(u2);
      p1[1] = (a - b * cos(v1)) * sin(u1);
      p2[1] = (a - b * cos(v1)) * sin(u2);
      p3[1] = (a - b * cos(v2)) * sin(u1);
      p4[1] = (a - b * cos(v2)) * sin(u2);
      p1[2] = b * sin(v1);
      p2[2] = b * sin(v1);
      p3[2] = b * sin(v2);
      p4[2] = b * sin(v2);

#ifdef USE_QUADS
      dtBegin(DT_POLYGON);
      dtVertex(p1[0], p1[1], p1[2]);
      dtVertex(p2[0], p2[1], p2[2]);
      dtVertex(p4[0], p4[1], p4[2]);
      dtVertex(p3[0], p3[1], p3[2]);
      dtEnd();
#else
      dtBegin(DT_SIMPLEX);
      dtVertex(p1[0], p1[1], p1[2]);
      dtVertex(p2[0], p2[1], p2[2]);
      dtVertex(p3[0], p3[1], p3[2]);
      dtEnd();
      dtBegin(DT_SIMPLEX);
      dtVertex(p4[0], p4[1], p4[2]);
      dtVertex(p1[0], p1[1], p1[2]);
      dtVertex(p2[0], p2[1], p2[2]);
      dtEnd();
#endif
    }
  fprintf(stdout, "done.\n");
  fprintf(stdout, "Building hierarchy..."); fflush(stderr);
  dtEndComplexShape();
  fprintf(stdout, "done.\n");

  dtCreateObject(&object1, shape);
  dtCreateObject(&object2, shape);

  dtDisableCaching();

  dtSetDefaultResponse(collide1, DT_SIMPLE_RESPONSE, stdout);
}
