/*
  This sample is a test for the depth penetration computation
*/

#include <stdio.h>
#include <fstream>
#include <SOLID/solid.h>

#include <assert.h>

#include <3D/Point.h>
#include <3D/Quaternion.h>
#include <vector>

#include "SDL.h"
#include "SDL_opengl.h"

void DrawSphere(float radius);
void DrawBox(float width, float height, float depth);

int main(int argc, char *argv[]);



//----------------------
// Globals
//----------------------
int sphere_id;
int box_id;
DtShapeRef sphere;
DtShapeRef box;
Quaternion camquat,qtemp;
float camrot[3]={0.0f,0.0f,0.0f};
float spherepos[3]={0.0f,0.0f,10.0f};
GLUquadricObj *sphereqobj;
float matrix[16];
Vector depth_penetration;

void CollideCallback(void * client_data, DtObjectRef obj1, DtObjectRef obj2,
		     const DtCollData *coll_data)
{
  if(coll_data!=NULL)
    {
//       fprintf(stdout,"Collided Point 1 %f,%f,%f\nPoint 2 %f,%f,%f\nNormal %f,%f,%f\n",
// 	      coll_data->point1[X],coll_data->point1[Y],coll_data->point1[Z],
// 	      coll_data->point1[X],coll_data->point1[Y],coll_data->point1[Z],
// 	      coll_data->normal[X],coll_data->normal[Y],coll_data->normal[Z]);
      depth_penetration[X]=coll_data->normal[X];
      depth_penetration[Y]=coll_data->normal[Y];
      depth_penetration[Z]=coll_data->normal[Z];
//       spherepos[0]+=depth_penetration[X];
//       spherepos[1]+=depth_penetration[Y];
//       spherepos[2]+=depth_penetration[Z];
  }
}

int main(int argc, char *argv[])
{
  sphere_id=1;
  box_id=2;

  //box = dtSphere(10.0f);
  box = dtBox(10.0f,10.0f,10.0f);
  sphere = dtSphere(10.0f);
  dtCreateObject(&box_id, box);
  dtCreateObject(&sphere_id, sphere);

  //dtDisableCaching();

  dtSetDefaultResponse(CollideCallback, DT_SMART_RESPONSE, stdout);



  bool bRunning = true;
  SDL_Event event;

  if(SDL_Init(SDL_INIT_VIDEO)<0) // try to initialize Video System
    {
      fprintf( stderr, "Video initialization failed: %s\n",SDL_GetError()); // if it failed, find out why
      return -1;
    }

  SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,1);

  if(SDL_SetVideoMode(800,600,0,SDL_OPENGL) == 0 )
    {
      fprintf(stderr, "Video mode set failed: %s\n",SDL_GetError());
      SDL_Quit();
      return -1;
    }

  glViewport(0,0,800,600); // set the view port to take the whole screen
  glMatrixMode(GL_PROJECTION); // switch to the Projection Matrix
  glLoadIdentity(); // reset Projection Matrix
  gluPerspective(45.0f,800.0f/600.0f, 0.1f ,1000.0f); // set our view to perspective
  glMatrixMode(GL_MODELVIEW); // switch to the Model View Matrix
  glLoadIdentity(); // reset Model View Matrix
  glClearColor(0.0f,0.0f,0.0f,0.0f); // Set our Clear Color to Black
  glShadeModel(GL_SMOOTH); // Enable Smooth Shading
  glClearDepth(1.0f); // Set the depth buffer
  glEnable(GL_DEPTH_TEST); // Enables Depth Testing
  glDepthFunc(GL_LEQUAL); // Type of depth test
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Use really nice perspective calculations
  glPolygonMode(GL_FRONT, GL_LINE);
  sphereqobj = gluNewQuadric();

  dtDisableCaching();

  while(bRunning)
    {
      if(SDL_PollEvent(&event))
	{
	  switch(event.type)
	    {
	    case SDL_MOUSEMOTION:
	      if(event.motion.state & SDL_BUTTON(1))
		{
		  camrot[1] += event.motion.xrel;
		  camrot[0] += event.motion.yrel;
		  //camrot[2] += event.motion.v[X]rel;
		}
	      break;
            case SDL_QUIT: // test to see if the user closed the app
	      bRunning = false;
	      break;
            case SDL_KEYDOWN: /* Handle a KEYDOWN event */
	      switch(event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
		  bRunning = false; // quit if ESCAPE is pressed
		  break;
		case SDLK_w:
		  spherepos[Y]+=0.5;
		  break;
		case SDLK_a:
		  spherepos[X]+=0.5;
		  break;
		case SDLK_s:
		  spherepos[Y]-=0.5;
		  break;
		case SDLK_d:
		  spherepos[X]-=0.5;
		  break;
		}
	      break;
	    }
	}
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glLoadIdentity();
      gluLookAt(0.0f,-100.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f);

      glRotatef(camrot[0],1.0f,0.0f,0.0f);
      glRotatef(camrot[1],0.0f,1.0f,0.0f);
      glRotatef(camrot[2],0.0f,0.0f,1.0f);


      glPushMatrix();
      dtSelectObject(&sphere_id);
      dtLoadIdentity();
      dtTranslate(spherepos[0],spherepos[1],spherepos[2]);
      glTranslatef(spherepos[0],spherepos[1],spherepos[2]);
      DrawSphere(10.0f);
      glPopMatrix();
      glPushMatrix();
      DrawBox(10.0f,10.0f,10.0f);
      //DrawSphere(10.0f);
      glPopMatrix();

      glDisable(GL_DEPTH_TEST);
      glColor3ub(0,0,255);

      dtTest();

      glBegin(GL_LINES);
      {
 	glVertex3f(spherepos[0],spherepos[1],spherepos[2]);
 	glVertex3f(spherepos[0]+depth_penetration[X],
		   spherepos[1]+depth_penetration[Y],
		   spherepos[2]+depth_penetration[Z]);
 	glVertex3f(0.0f,0.0f,0.0f);
 	glVertex3f(depth_penetration[X],
		   depth_penetration[Y],
		   depth_penetration[Z]);
      }
      glEnd();
      glEnable(GL_DEPTH_TEST);

      SDL_GL_SwapBuffers();
    }
  gluDeleteQuadric(sphereqobj);
  dtDeleteObject(&box_id);
  dtDeleteObject(&sphere_id);
  return 0;
}


void DrawBox(float width, float height, float depth)
{
  glColor3ub(255,0,0);
  float wdiv2=width/2;
  float hdiv2=height/2;
  float ddiv2=depth/2;

  //glDisable(GL_TEXTURE_2D);
  glPushMatrix();
  glBegin(GL_LINES);
  {
    glVertex3f(wdiv2,hdiv2,ddiv2);
    glVertex3f(-wdiv2,hdiv2,ddiv2);

    glVertex3f(-wdiv2,-hdiv2,ddiv2);
    glVertex3f(wdiv2,-hdiv2,ddiv2);

    glVertex3f(wdiv2,hdiv2,ddiv2);
    glVertex3f(wdiv2,-hdiv2,ddiv2);

    glVertex3f(-wdiv2,-hdiv2,ddiv2);
    glVertex3f(-wdiv2,hdiv2,ddiv2);

    glVertex3f(wdiv2,hdiv2,-ddiv2);
    glVertex3f(-wdiv2,hdiv2,-ddiv2);

    glVertex3f(-wdiv2,-hdiv2,-ddiv2);
    glVertex3f(wdiv2,-hdiv2,-ddiv2);

    glVertex3f(wdiv2,hdiv2,-ddiv2);
    glVertex3f(wdiv2,-hdiv2,-ddiv2);

    glVertex3f(-wdiv2,-hdiv2,-ddiv2);
    glVertex3f(-wdiv2,hdiv2,-ddiv2);

    glVertex3f(-wdiv2,-hdiv2,ddiv2);
    glVertex3f(-wdiv2,-hdiv2,-ddiv2);

    glVertex3f(wdiv2,-hdiv2,ddiv2);
    glVertex3f(wdiv2,-hdiv2,-ddiv2);

    glVertex3f(wdiv2,hdiv2,-ddiv2);
    glVertex3f(wdiv2,hdiv2,ddiv2);

    glVertex3f(-wdiv2,hdiv2,ddiv2);
    glVertex3f(-wdiv2,hdiv2,-ddiv2);
  }
  glEnd();
  glPopMatrix();
  //glEnable(GL_TEXTURE_2D);
}

void DrawSphere(float radius)
{
  glPolygonMode(GL_FRONT, GL_LINE);
  glPolygonMode(GL_BACK, GL_LINE);
  glColor3ub(255,128,255);
  gluSphere(sphereqobj, radius, 8, 4);
}
