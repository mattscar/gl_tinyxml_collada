#define VERTEX_SHADER "draw_sphere.vert"
#define FRAGMENT_SHADER "draw_sphere.frag"

#define GL3_PROTOTYPES
#include <GL3/gl3.h>
#define __gl_h_

#include <cstdlib>
#include <string>
#include <fstream>
#include <iostream>
#include <glm/glm.hpp>
#include <GL/freeglut.h>
#include "colladainterface.h"

struct LightParameters {
  glm::vec4 diffuse_intensity;
  glm::vec4 ambient_intensity;
  glm::vec4 light_direction;
};

std::vector<ColGeom> geom_vec;    // Vector containing COLLADA meshes
int num_objects;                  // Number of meshes in the vector
GLuint *vaos, *vbos;              // OpenGL vertex objects
GLuint ubo;                       // OpenGL uniform buffer object

// Read a character buffer from a file
std::string read_file(const char* filename) {

  // Open the file
  std::ifstream ifs(filename, std::ifstream::in);
  if(!ifs.good()) {
    std::cerr << "Couldn't find the shader file " << filename << std::endl;
    exit(1);
  }
  
  // Read file text into string and close stream
  std::string str((std::istreambuf_iterator<char>(ifs)), 
                   std::istreambuf_iterator<char>());
  ifs.close();
  return str;
}

// Compile the shader
void compile_shader(GLint shader) {

  GLint success;
  GLsizei log_size;
  char *log;

  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
    log = new char[log_size+1];
    log[log_size] = '\0';
    glGetShaderInfoLog(shader, log_size+1, NULL, log);
    std::cout << log;
    delete(log);
    exit(1);
  }
}

// Create, compile, and deploy shaders
GLuint init_shaders(void) {

  GLuint vs, fs, prog;
  std::string vs_source, fs_source;
  const char *vs_chars, *fs_chars;
  GLint vs_length, fs_length;

  // Create shader descriptors
  vs = glCreateShader(GL_VERTEX_SHADER);
  fs = glCreateShader(GL_FRAGMENT_SHADER);   

  // Read shader text from files
  vs_source = read_file(VERTEX_SHADER);
  fs_source = read_file(FRAGMENT_SHADER);

  // Set shader source code
  vs_chars = vs_source.c_str();
  fs_chars = fs_source.c_str();
  vs_length = (GLint)vs_source.length();
  fs_length = (GLint)fs_source.length();
  glShaderSource(vs, 1, &vs_chars, &vs_length);
  glShaderSource(fs, 1, &fs_chars, &fs_length);

  // Compile shaders and chreate program
  compile_shader(vs);
  compile_shader(fs);
  prog = glCreateProgram();

  // Bind attributes
  glBindAttribLocation(prog, 0, "in_coords");
  glBindAttribLocation(prog, 1, "in_normals");

  // Attach shaders
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);

  glLinkProgram(prog);
  glUseProgram(prog);

  return prog;
}

// Create and initialize vertex array objects (VAOs)
// and vertex buffer objects (VBOs)
void init_buffers(GLuint program) {
  
  int loc;

  // Create a VAO for each geometry
  vaos = new GLuint[num_objects];
  glGenVertexArrays(num_objects, vaos);

  // Create a VBO for each geometry
  vbos = new GLuint[2 * num_objects];
  glGenBuffers(2 * num_objects, vbos);

  // Configure VBOs to hold positions and normals for each geometry
  for(int i=0; i<num_objects; i++) {

    glBindVertexArray(vaos[i]);

    // Set vertex coordinate data
    glBindBuffer(GL_ARRAY_BUFFER, vbos[2*i]);
    glBufferData(GL_ARRAY_BUFFER, geom_vec[i].map["POSITION"].size, 
                 geom_vec[i].map["POSITION"].data, GL_STATIC_DRAW);
    loc = glGetAttribLocation(program, "in_coords");
    glVertexAttribPointer(loc, geom_vec[i].map["POSITION"].stride, 
                          geom_vec[i].map["POSITION"].type, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Set normal vector data
    glBindBuffer(GL_ARRAY_BUFFER, vbos[2*i+1]);
    glBufferData(GL_ARRAY_BUFFER, geom_vec[i].map["NORMAL"].size, 
                 geom_vec[i].map["NORMAL"].data, GL_STATIC_DRAW);
    loc = glGetAttribLocation(program, "in_normals");
    glVertexAttribPointer(loc, geom_vec[i].map["NORMAL"].stride, 
                          geom_vec[i].map["NORMAL"].type, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

// Initialize uniform data
void init_uniforms(GLuint program) {

  GLuint program_index, ubo_index;
  struct LightParameters params;

  // Specify the rotation matrix
  glm::vec4 diff_color = glm::vec4(0.3f, 0.3f, 1.0f, 1.0f);
  GLint location = glGetUniformLocation(program, "diffuse_color");
  glUniform4fv(location, 1, &(diff_color[0])); 

  // Initialize UBO data
  params.diffuse_intensity = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
  params.ambient_intensity = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
  params.light_direction = glm::vec4(-1.0f, -1.0f, 0.25f, 1.0f);

  // Set the uniform buffer object
  glUseProgram(program);
  glGenBuffers(1, &ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, ubo);
  glBufferData(GL_UNIFORM_BUFFER, 3*sizeof(glm::vec4), &params, GL_STREAM_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  glUseProgram(program);

  // Match the UBO to the uniform block
  glUseProgram(program);
  ubo_index = 0;
  program_index = glGetUniformBlockIndex(program, "LightParameters");
  glUniformBlockBinding(program, program_index, ubo_index);
  glBindBufferRange(GL_UNIFORM_BUFFER, ubo_index, ubo, 0, 3*sizeof(glm::vec4));
  glUseProgram(program);
}

// Initialize the rendering objects
void init(int argc, char* argv[]) {

  // Initialize the main window
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(300, 300);
  glutCreateWindow("Draw Sphere");
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

  // Configure culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CW);

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glDepthRange(0.0f, 1.0f);

  // Initialize shaders and buffers
  GLuint program = init_shaders();
  init_buffers(program);
  init_uniforms(program);
}

// Respond to paint events
void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Draw elements of each mesh in the vector
  for(int i=0; i<num_objects; i++) {
    glBindVertexArray(vaos[i]);
    glDrawElements(geom_vec[i].primitive, geom_vec[i].index_count, 
                   GL_UNSIGNED_SHORT, geom_vec[i].indices);
  }

  glBindVertexArray(0);
  glutSwapBuffers();
}

// Respond to reshape events
void reshape(int w, int h) {
   glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}

// Deallocate memory
void deallocate() {

  // Deallocate mesh data
  ColladaInterface::freeGeometries(&geom_vec);

  // Deallocate OpenGL objects
  glDeleteBuffers(num_objects, vbos);
  glDeleteBuffers(2 * num_objects, vaos);
  glDeleteBuffers(1, &ubo);
  delete(vbos);
  delete(vaos);
}

int main(int argc, char* argv[]) {

  // Initialize COLLADA geometries
  ColladaInterface::readGeometries(&geom_vec, "sphere.dae");
  num_objects = (int) geom_vec.size();

  // Start GL processing
  init(argc, argv);

  // Set callback functions
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);   

  // Configure deallocation callback
  atexit(deallocate);

  // Start processing loop
  glutMainLoop();

  return 0;
}
