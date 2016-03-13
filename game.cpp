#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;

    double origin[3];
    int isCube;
    double length;
    double width;
    double height;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

VAO *cube, *player,*border;
VAO* Objects[1000];
int no_objects=0;
int iskeypressed=0;
int isplateformrotating = 0;
int isview = 0;
int isplatformpresent[50][50];
int instant_player_x;
int instant_player_y;
int instant_player_z;
int tile_x=0;
int tile_z=0;
double player_velocity_x=0;
double player_velocity_y=0;
double player_velocity_z=0;
int isjumping = 0;
double time_elapsed;
double xpos,ypos;
int  width,height;
int ismousepressed=0;
GLFWwindow* window;
VAO* ismovingupdownblock[401];
VAO* ismovingupdownborder[401];
int no_moving_objects=0;
int movingblockvelocity=0;
double t=0;
int moving_block_tile_x,moving_block_tile_z;



/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;
    vao->isCube = 0;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_C:
                rectangle_rot_status = !rectangle_rot_status;
                break;
            case GLFW_KEY_P:
                triangle_rot_status = !triangle_rot_status;
                break;
            case GLFW_KEY_X:
                // do something ..
                break;
            case GLFW_KEY_RIGHT:
                iskeypressed = 0;
                break;
            case GLFW_KEY_LEFT:
                iskeypressed = 0;
                break;
            case GLFW_KEY_UP:
                iskeypressed = 0;
                break;
            case GLFW_KEY_DOWN:
                iskeypressed = 0;
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            case GLFW_KEY_RIGHT:
                //iskeypressed = 1;
            	if(isjumping)
            		player_velocity_x=5;
            	else	
            		tile_x++;
                break;
            case GLFW_KEY_LEFT:
                //iskeypressed = 2;
            	if(isjumping)
            		player_velocity_x=-5;
            	else
            		tile_x--;
                break;
            case GLFW_KEY_UP:
                //iskeypressed = 3;
            	if(isjumping)
            		player_velocity_z=-5;
            	else
            		tile_z--;
                break;
            case GLFW_KEY_DOWN:
                //iskeypressed = 4;
            	if(isjumping)
            		player_velocity_z=5;
            	else
            		tile_z++;
                break;
            case GLFW_KEY_SPACE:

               // if(isplateformrotating == 0)
                 //   isplateformrotating = 1;
                //else
                  //  isplateformrotating = 0;
            	player_velocity_y=50;
            	isjumping=1;
                break;
            case GLFW_KEY_V:
                    isview++;
                    isview%=5;
                  break;
            case GLFW_KEY_R:
            	  ismousepressed = 0;
            	  break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
        	if(action == GLFW_PRESS)
        		ismousepressed = 1;
        	if(action == GLFW_RELEASE)
        		ismousepressed = 0;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                rectangle_rot_dir *= -1;
            }
            break;
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-160.0f, 160.0f, -160.0f, 160.0f, -160.0f, 160.0f);
}


// Creates the triangle object used in this sample code
void createCube (double length,double width,double height,double x, double y, double z)
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  GLfloat vertex_buffer_data [] = {
    0, 0,0, // vertex 0
    length,width,0, // vertex 1
    0,width,0, // vertex 2

    0,0,0,
    length,0,0,
    length,width,0,

    0,0,0,
    0,width,0,
    0,width,height,


    0,0,0,
    0,width,height,
    0,0,height,

    0,width,height,
    0,0,height,
    length,0,height,

    0,width,height,
    length,0,height,
    length,width,height,

    length,0,0,
    length,width,0,
    length,0,height,

    length,width,0,
    length,0,height,
    length,width,height,

    0,0,0,
    0,0,height,
    length,0,0,

    0,0,height,
    length,0,0,
    length,0,height,

    0,width,0,
    0,width,height,
    length,width,0,

    0,width,height,
    length,width,0,
    length,width,height

  };

  GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    1,0,0, // color 1
    1,0,0, // color 2

    

    1,0,0, // color 0
    1,0,0, // color 1
    1,0,0, // color 2


    0,1,0, // color 3
    0,1,0, // color 4
    0,1,0,  // color 1

    0,1,0, // color 3
    0,1,0, // color 4
    0,1,0,  // color 1

    0,0,1, // color 0
    0,0,1, // color 1
    0,0,1, // color 2

    

    0,0,1, // color 0
    0,0,1, // color 1
    0,0,1, // color 2

    1,0,0, // color 0
    1,0,0, // color 1
    1,0,0, // color 2

    

    1,0,0, // color 0
    1,0,0, // color 1
    1,0,0, // color 2


    0,1,0, // color 3
    0,1,0, // color 4
    0,1,0,  // color 1

    0,1,0, // color 3
    0,1,0, // color 4
    0,1,0,  // color 1

    1,1,1, // color 0
    1,1,1, // color 1
    1,1,1, // color 2

    

    1,1,1, // color 0
    1,1,1, // color 1
    1,1,1, // color 2

    
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  cube = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
  cube->isCube = 1;
  cube->length = length;
  cube->width = width;
  cube->height = height;
  cube->origin[0]=x;
  cube->origin[1]=y;
  cube->origin[2]=z;
  Objects[no_objects]=cube;
  no_objects++;

}

void createPlayer(double length,double width,double height,double x,double y,double z)
{

    GLfloat vertex_buffer_data [] = {
    0, 0,0, // vertex 0
    length,width,0, // vertex 1
    0,width,0, // vertex 2

    0,0,0,
    length,0,0,
    length,width,0,

    0,0,0,
    0,width,0,
    0,width,height,


    0,0,0,
    0,width,height,
    0,0,height,

    0,width,height,
    0,0,height,
    length,0,height,

    0,width,height,
    length,0,height,
    length,width,height,

    length,0,0,
    length,width,0,
    length,0,height,

    length,width,0,
    length,0,height,
    length,width,height,

    0,0,0,
    0,0,height,
    length,0,0,

    0,0,height,
    length,0,0,
    10,0,height,

    0,width,0,
    0,width,height,
    length,width,0,

    0,width,height,
    length,width,0,
    length,width,height

  };

  GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0,  // color 1

    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0,  // color 1

    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0,  // color 1


    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0,  // color 1

    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0,  // color 1

    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0,  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  player = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
  player->isCube = 1;
  player->length = length;
  player->width = width;
  player->height = height;
  player->origin[0]=x;
  player->origin[1]=y;
  player->origin[2]=z;


}
void createBorder(double length,double width,double height,double x,double y,double z)
{

  GLfloat vertex_buffer_data [] = {

    0,height,0,
    length,height,width,
    length,height,0,

    0,height,0,
    0,height,width,
    length,height,width



  };

  GLfloat color_buffer_data [] = {
    0,0,0,
    0,0,0,
    0,0,0,

    0,0,0,
    0,0,0,
    0,0,0,
  };
  border = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_LINE);
  border->origin[0]=x;
  border->origin[1]=y;
  border->origin[2]=z;
  Objects[no_objects]=border;
  no_objects++;
}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  //470
  glm::vec3 eye;
  glm::vec3 target;
  glm::vec3 up;
  if(isview == 1)
  {  camera_rotation_angle = 460;
 // camera_rotation_angle = 88;
  	//0.8
  	if(ismousepressed==1){
  	glfwGetCursorPos(window, &xpos, &ypos);
  glfwGetFramebufferSize(window, &width, &height);
  xpos=-160+(float)320.0/width*xpos;
  ypos=-160+(float)320.0/height*ypos;
  ypos*=-1;
  eye = glm::vec3 (xpos,40,ypos);}
  if(ismousepressed == 0){
	 eye = glm::vec3 ( 1*cos(camera_rotation_angle*M_PI/180.0f), 2, 1*sin(camera_rotation_angle*M_PI/180.0f) );
     }

     target = glm::vec3 (0, 0, 0);
     up = glm::vec3 (0, 1, 0);
    Matrices.projection = glm::ortho(-160.0f, 160.0f, -160.0f, 160.0f, -160.0f, 160.0f);

 // cout << camera_rotation_angle << endl;
}
  else if(isview == 0){
   eye = glm::vec3 (0,1, 1);
    target = glm::vec3(0, 0, 0);
    up = glm::vec3  (0, 1, 0);
    Matrices.projection = glm::ortho(-160.0f, 160.0f, -160.0f, 160.0f, -160.0f, 160.0f);
    
}
  else if(isview==2){
     eye = glm::vec3 ( 5*cos(camera_rotation_angle*M_PI/180.0f), 3, 5*sin(camera_rotation_angle*M_PI/180.0f) );
   target = glm::vec3 (0, 0, 0);
     up =  glm::vec3 (0, 1, 0);
    Matrices.projection = glm::ortho(-160.0f, 160.0f, -160.0f, 160.0f, -160.0f, 160.0f);
     
  }
  else if(isview==3)
  {
  	eye = glm::vec3 ( player->origin[0]-5, 70, player->origin[2]+2);
  	target =  glm::vec3 (player->origin[0]+5, 0, player->origin[2]-2);
     up =  glm::vec3 (0, 1, 0);
  	Matrices.projection = glm::ortho(0.0f, 50.0f, -70.0f, 70.0f, -70.0f, 70.0f);
  }
  else
  {
  	eye = glm::vec3 ( player->origin[0]+14, 55, player->origin[2]);
  	target =  glm::vec3 (player->origin[0]+20, 0, player->origin[2]-3);
     up =  glm::vec3 (0, 1, 0);
  	Matrices.projection = glm::ortho(0.0f, 50.0f, -70.0f, 70.0f, -70.0f, 70.0f);
  }
  glfwGetCursorPos(window, &xpos, &ypos);
  glfwGetFramebufferSize(window, &width, &height);
  xpos=-160+(float)320.0/width*xpos;
  ypos=-160+(float)320.0/height*ypos;
  ypos*=-1;
  time_elapsed = 0.004;

  // Target - Where is the camera looking at.  Don't change unless you are sure!!

  //Compute Camera matrix (view)
   Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  //Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);


  /* Render your scene */
  for(int k=0;k<no_moving_objects;k++)
  {
  	// if(ismovingupdownblock[k]->origin[0]<=0)
  	// {
  	// 	movingblockvelocity=10;
  	// 	ismovingupdownblock[k]->origin[1]=0;
  	// 	ismovingupdownborder[k]->origin[1]=0;
  	// }
  	// if(ismovingupdownblock[k]->origin[0]>=15)
  	// 	movingblockvelocity=-10;
  	// ismovingupdownblock[k]->origin[1]+=movingblockvelocity*time_elapsed;
  	// ismovingupdownborder[k]->origin[1]+=movingblockvelocity*time_elapsed;
  //	movingblockvelocity-=1;
  	
  	ismovingupdownborder[k]->origin[1]=10*sin(2*(t+time_elapsed)*0.02);
  	ismovingupdownblock[k]->origin[1]=10*sin(2*(t+time_elapsed)*0.02);
  	moving_block_tile_x = int((ismovingupdownblock[k]->origin[0]+100)/11);
  	moving_block_tile_z = int((ismovingupdownblock[k]->origin[2]+100)/11);
  	isplatformpresent[moving_block_tile_x][moving_block_tile_z]=ismovingupdownblock[k]->origin[1]+ismovingupdownblock[k]->width+1;
  	t+=time_elapsed;
  }
  for(int i=0;i<no_objects;i++)
  {

  glm::mat4 translateTriangle = glm::translate (glm::vec3(Objects[i]->origin[0],Objects[i]->origin[1],Objects[i]->origin[2])); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 triangleTransform = translateTriangle;
  Matrices.model = triangleTransform; 
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(Objects[i]);
  }

  if(iskeypressed == 1)
  		tile_x++;
  if(iskeypressed == 2)
        tile_x--;
  if(iskeypressed == 4)
  		tile_z++;
  if(iskeypressed == 3)
  		tile_z--;
  if(isjumping)
  {
  time_elapsed = 0.004;
  player_velocity_y-=10*(time_elapsed*60);
  player->origin[0]+=player_velocity_x*(time_elapsed*60);
  player->origin[1]+=player_velocity_y*(time_elapsed*60)-5*time_elapsed*time_elapsed*3600;
  player->origin[2]+=player_velocity_z*(time_elapsed*60);
  if(player->origin[1]<10)
  {		isjumping=0;
  		tile_x=int((player->origin[0]+100)/11);
  		tile_z=int((player->origin[2]+100)/11);
  		player_velocity_x=0;
  		player_velocity_z=0;
  		player_velocity_y=0;
  }
}
  if(!isjumping){
  player->origin[0]=-100+tile_x*11+2;
  player->origin[2]=-100+tile_z*11+2;
  instant_player_x = int((player->origin[0]+100)/10);
  instant_player_z = int((player->origin[2]+100)/10);
  player->origin[1]=isplatformpresent[tile_x][tile_z];	
  }	
  
  glm::mat4 translatePlayer = glm::translate (glm::vec3(player->origin[0],player->origin[1],player->origin[2])); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 PlayerTransform = translatePlayer;
  Matrices.model = PlayerTransform; 
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(player);
  if(isplatformpresent[tile_x][tile_z]==1000)
  {
  	cout << "you lost" << endl;
  	quit(window);
  }
  if(tile_x==19&&tile_z==19)
  {
  	cout << "you won " << endl;
  	quit(window);
  }
  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
  

  // Increment angles
  float increments = 1;
  if(isplateformrotating)
    camera_rotation_angle++; // Simulating camera rotation
  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    } 

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
  no_objects=0;
  int x_addition=0,y_addition=0;
  for(int i=0;i<20;i++)
  {
    x_addition=0;
    for(int j=0;j<20;j++)
    {
         if(rand()%2 || i==0 && j==0)
         {
          	    int width = rand()%31 + 5;
              createCube(10,width,10,-100+x_addition,0,-100+y_addition);
              createBorder(10,10,width,-100+x_addition,0,-100+y_addition);
              if(rand()%2)
              {
              		ismovingupdownblock[no_moving_objects]=Objects[no_objects-2];
              		ismovingupdownborder[no_moving_objects]=Objects[no_objects-1];
              		no_moving_objects++;
              }
              isplatformpresent[j][i]=width;
         }
         else
         	isplatformpresent[j][i]=1000;
         x_addition+=11;
    }
    y_addition+=11;
  }
  createPlayer(5,10,5,-100,35,-100);
	
	
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 1400;
	int height = 700;

     window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        draw();

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
