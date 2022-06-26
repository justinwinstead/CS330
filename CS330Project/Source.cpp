/*
 * Name: Justin Winstead
 * Date: June 19, 2022
 * Course ID: CS-330/T5625
 * Description: Creates a 3D scene in OpenGL with 4 textured objects and 2 lights
 */



#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "headers/stb_image.h"      // Image loading Utility functions
#include "headers/Camera.h"
#include "headers/Sphere.h"
#include "headers/Cylinder.h"

 /*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif


// constant values to derive our screen information from
const char* const SCR_TITLE = "Project";

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const float PI = 3.1415926f;

// mesh struct to contain the vertex array object and buffer objects
struct GLMesh
{
    GLuint vao;         // variable for the vertex array object
    GLuint vbo;         // variable for the vertex buffer object
    GLuint ebo;         // variable for the element buffer object
    GLuint nIndices;    // number of indices for the mesh
};

// camera
Camera gCamera(glm::vec3(0.0f, 0.0f, 5.0f));
float gLastX = SCR_WIDTH / 2.0f;
float gLastY = SCR_HEIGHT / 2.0f;
bool gFirstMouse = true;

float gDeltaTime = 0.0f; // time between current frame and last frame
float gLastFrame = 0.0f;

GLFWwindow* window = nullptr;

// mesh objects
GLMesh meshBottleSphere;
GLMesh meshBottleTopCylinder;
GLMesh meshBottleBottomCylinder;
GLMesh meshPenCone;
GLMesh meshPenCylinder;
GLMesh meshPenSphere;
GLMesh meshPerfume;
GLMesh meshBox;
GLMesh meshPlane;
GLMesh meshLight;

// sphere objects
Sphere bottleSphere;
Sphere penSphere;

// cylinder objects
Cylinder bottleBottomCylinder;
Cylinder bottleTopCylinder;
Cylinder perfumeCylinder;
Cylinder penCylinder;
Cylinder penCone;

// shader programs
GLuint objectProgramId;
GLuint planeProgramId;
GLuint lightProgramId;

glm::vec3 gObjectColor(1.0f, 0.2f, 0.0f);

// light position, scale, and color
glm::vec3 lightColor1(1.0f, 1.0f, 1.0f); // white light
glm::vec3 lightPosition1(0.0f, 4.0f, 2.0f);// place light up on y-axis and forward on z-axis
glm::vec3 lightColor2(1.0f, 1.0f, 0.8784f); // soft yellow light
glm::vec3 lightPosition2(0.0f, -1.0f, -4.0f); // place light down on y-axis and back on z-axis
glm::vec3 lightScale(0.2f);

GLuint textureId;
glm::vec2 textureScale(1.0f, 1.0f);

// variables for textures
GLuint glassTextureId;
GLuint labelTextureId;
GLuint planeTextureId;
GLuint penTextureId;
GLuint boxTextureId;
GLuint perfumeTextureId;

// user defined functions
void resizeWindow(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void processMousePosition(GLFWwindow* window, double xpos, double ypos);
void processMouseScroll(GLFWwindow* window, double xoffset, double yoffset);
void createPlaneMesh(GLMesh& mesh);
void createLightMesh(GLMesh& mesh);
void createBoxMesh(GLMesh& mesh);
void createSphereMesh(GLMesh& mesh, Sphere sphere);
void createCylinderMesh(GLMesh& mesh, Cylinder cylinder);
void deleteMesh(GLMesh& mesh);
void render();
bool createTexture(const char* filename, GLuint& textureId);
void flipImageVertically(unsigned char* image, int width, int height, int channels);
bool createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource, GLuint& programId);
void deleteShaderProgram(GLuint programId);

// shader source code
/* Textured Object Vertex Shader Source Code*/
const GLchar* objectVertexShaderSource = GLSL(440,

layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);

/* Object Fragment Shader Source Code
 * Lower specular intensity and option for secondary texture
 */
const GLchar* objectFragmentShaderSource = GLSL(440,
in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor1;
uniform vec3 lightPos1;
uniform vec3 lightColor2;
uniform vec3 lightPos2;
uniform vec3 viewPosition;
uniform bool multipleTextures;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform sampler2D uTexture2; // Useful when working with multiple textures
uniform vec2 textureScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    // first light calculations
    //Calculate Ambient lighting*/
    float ambientStrength = 0.1f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor1; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos1 - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor1; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.1f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor1;

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular);

    // second light calculations
    //Calculate Ambient lighting*/
    ambientStrength = 0.1f; // Set ambient or global lighting strength
    ambient = ambientStrength * lightColor2; // Generate ambient light color

    //Calculate Diffuse lighting*/
    norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    lightDirection = normalize(lightPos2 - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    diffuse = impact * lightColor2; // Generate diffuse light color

    //Calculate Specular lighting*/
    specularIntensity = 0.1f; // Set specular light strength
    highlightSize = 16.0f; // Set specular highlight size
    viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    specular = specularIntensity * specularComponent * lightColor2;

    vec3 phong2 = (ambient + diffuse + specular) * 0.5f; // attempt to reduce impact of second light to 10% intensity

    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * textureScale);

    // Texture holds the color to be used for all three components
    if (multipleTextures)
    {
        if (texture(uTexture2, vertexTextureCoordinate).a != 0)
        {
            textureColor = texture(uTexture2, vertexTextureCoordinate * textureScale);
        }
    }

    // Calculate phong result
    phong = (phong + phong2) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);

/* Plane Fragment Shader Source Code
 * Higher specular intensity for reflection
 */
const GLchar* planeFragmentShaderSource = GLSL(440,

in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor1;
uniform vec3 lightPos1;
uniform vec3 lightColor2;
uniform vec3 lightPos2;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 textureScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    // first light calculations
    //Calculate Ambient lighting*/
    float ambientStrength = 0.1f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor1; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos1 - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor1; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.1f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor1;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * textureScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular);

    // second light calculations
    //Calculate Ambient lighting*/
    ambientStrength = 0.1f; // Set ambient or global lighting strength
    ambient = ambientStrength * lightColor2; // Generate ambient light color

    //Calculate Diffuse lighting*/
    norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    lightDirection = normalize(lightPos2 - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    diffuse = impact * lightColor2; // Generate diffuse light color

    //Calculate Specular lighting*/
    specularIntensity = 0.1f; // Set specular light strength
    highlightSize = 16.0f; // Set specular highlight size
    viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    specular = specularIntensity * specularComponent * lightColor2;

    vec3 phong2 = (ambient + diffuse + specular) * 0.5f; // attempt to reduce impact of second light to 10% intensity

    // Calculate phong result
    phong = (phong + phong2) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);


/* Light Shader Source Code*/
const GLchar* lightVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

    //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lightFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, SCR_TITLE, NULL, NULL); // create window with specified size and name

    // if window fails to initialize
    if (window == NULL)
    {
        std::cout << "Failed to create window" << std::endl; // output failure to console
        glfwTerminate(); // terminate GLFW and the program
        return -1;
    }

    glfwMakeContextCurrent(window); // set current context to the window previously created
    glfwSetFramebufferSizeCallback(window, resizeWindow); // set Framebuffer Size Callback to function created to handle window resizing
    glfwSetCursorPosCallback(window, processMousePosition);
    glfwSetScrollCallback(window, processMouseScroll);

    // GLEW: initialize
     // ----------------
     // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return -1;
    }
    
    // CREATE BOTTLE MESHES
    //_________________________
    bottleTopCylinder.set(0.2f, 0.2f, 1.0f, 24, 12, true);
    createCylinderMesh(meshBottleTopCylinder, bottleTopCylinder); // call the createCylinderMesh() function to initialize our data and buffer it to GPU

    bottleBottomCylinder.set(0.5f, 0.5f, 2.0f, 24, 12, true);
    createCylinderMesh(meshBottleBottomCylinder, bottleBottomCylinder); // call the createCylinderMesh() function to initialize our data and buffer it to GPU

    bottleSphere.set(.5f, 24, 12, true);
    createSphereMesh(meshBottleSphere, bottleSphere); // call the createSphereMesh() function to initialize our data and buffer it to GPU

    // CREATE PEN MESHES
    //_________________________
    penCone.set(0.04f, 0.0f, 0.1f, 24, 12, true);
    createCylinderMesh(meshPenCone, penCone); // call the createCylinderMesh() function to initialize our data and buffer it to GPU
    
    penCylinder.set(0.04f, 0.04f, .75f, 24, 12, true);
    createCylinderMesh(meshPenCylinder, penCylinder); // call the createCylinderMesh() function to initialize our data and buffer it to GPU
    
    penSphere.set(.04f, 24, 12, true);
    createSphereMesh(meshPenSphere, penSphere); // call the createSphereMesh() function to initialize our data and buffer it to GPU

    // CREATE OTHER MESHES
    //_________________________
    perfumeCylinder.set(0.25f, 0.25f, 2.0f, 24, 12, true);
    createCylinderMesh(meshPerfume, perfumeCylinder);

    createPlaneMesh(meshPlane); // call the createPlaneMesh() function to initialize our data and buffer it to GPU
    createLightMesh(meshLight); // call the createLightMesh() function to initialize our data and buffer it to GPU
    createBoxMesh(meshBox); // call the createBoxMesh() function to initialize our data and buffer it to GPU


    // initialize shader program and ensure that it was done properly using createShaderProgram() function
    if (!createShaderProgram(objectVertexShaderSource, objectFragmentShaderSource, objectProgramId))
    {
        return -1;
    }

    // initialize shader program and ensure that it was done properly using createShaderProgram() function
    if (!createShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, lightProgramId))
    {
        return -1;
    }

    // initialize shader program and ensure that it was done properly using createShaderProgram() function
    if (!createShaderProgram(objectVertexShaderSource, planeFragmentShaderSource, planeProgramId))
    {
        return -1;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // enables wireframe view to verify that all triangles are shown
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    const char* texFilename = "textures/glass.jpg"; // variable to load image

    // create glass texture
    if (!createTexture(texFilename, glassTextureId))
    {
        std::cout << "Failed to load texture " << texFilename << std::endl;
        return -1;
    }
    texFilename = "textures/Label.png"; // variable to load image

    // create label texture
    if (!createTexture(texFilename, labelTextureId))
    {
        std::cout << "Failed to load texture " << texFilename << std::endl;
        return -1;
    }
    texFilename = "textures/plane.jpg"; // variable to load image

    // create plane texture
    if (!createTexture(texFilename, planeTextureId))
    {
        std::cout << "Failed to load texture " << texFilename << std::endl;
        return -1;
    }
    texFilename = "textures/pen.jpg"; // variable to load image

    // create pen texture
    if (!createTexture(texFilename, penTextureId))
    {
        std::cout << "Failed to load texture " << texFilename << std::endl;
        return -1;
    }
    texFilename = "textures/box.jpg"; // variable to load image

    // create box texture
    if (!createTexture(texFilename, boxTextureId))
    {
        std::cout << "Failed to load texture " << texFilename << std::endl;
        return -1;
    }
    texFilename = "textures/perfume.jpg"; // variable to load image

    // create perfume texture
    if (!createTexture(texFilename, perfumeTextureId))
    {
        std::cout << "Failed to load texture " << texFilename << std::endl;
        return -1;
    }
    // Tell OpenGL for each sampler which texture unit it belongs to (only has to be done once).
    glUseProgram(objectProgramId);
    // We set the glass texture as texture unit 0.
    glUniform1i(glGetUniformLocation(objectProgramId, "uTexture"), 0);
    // We set the label texture as texture unit 1.
    glUniform1i(glGetUniformLocation(objectProgramId, "uTexture2"), 1);
    // We set the plane texture as texture unit 0 for its program.
    glUniform1i(glGetUniformLocation(planeProgramId, "uTexture"), 0);

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input processing
        processInput(window);

        // rendering command
        render();

        glfwPollEvents();
    }

    // use delete functions to destroy the mesh and shader program before ending the program
    deleteMesh(meshBottleTopCylinder);
    deleteMesh(meshBottleBottomCylinder);
    deleteMesh(meshBottleSphere);
    deleteMesh(meshPenSphere);
    deleteMesh(meshPenCylinder);
    deleteMesh(meshPenCone);
    deleteMesh(meshBox);
    deleteMesh(meshPlane);
    deleteMesh(meshPerfume);
    deleteMesh(meshLight);

    deleteShaderProgram(planeProgramId);
    deleteShaderProgram(objectProgramId);
    deleteShaderProgram(lightProgramId);

    glfwTerminate(); // terminate GLFW when done rendering
    return 0;
}

// function to process user input.
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // if statements to handle all movement keys
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);

    // attempt to perform perspective shift
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, SCR_WIDTH, 0, SCR_HEIGHT, 0.1, 100);
    }
}

// function to process cursor movement
void processMousePosition(GLFWwindow* window, double xpos, double ypos)
{
    // sets initial mouse position
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    // generate offsets to move mouse
    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // Reversed since y-coordinates go from bottom to top

    // set last location to the current location
    gLastX = xpos;
    gLastY = ypos;

    // process mouse movement using generated offsets
    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// function to process all mouse scrolling
void processMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// function to handle window resizing
void resizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// function that contains all rendering functions
void render()
{
    // enable Z-depth.
    glEnable(GL_DEPTH_TEST);

    // clear the background color and Z buffers.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0); // set active texture
    glBindTexture(GL_TEXTURE_2D, planeTextureId); // bind texture that was created

    // Model matrix: Transformations are applied right-to-left.
    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 view = gCamera.GetViewMatrix();

    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.1f, 100.0f);

    /*
     * INITIALIZE planeProgramId UNIFORMS
     */

     // use the shader program created in createShaderProgram()
    glUseProgram(planeProgramId);

    // retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(planeProgramId, "model");
    GLint viewLoc = glGetUniformLocation(planeProgramId, "view");
    GLint projLoc = glGetUniformLocation(planeProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // reference matrix uniforms from the plane shader program for the object color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(planeProgramId, "objectColor");
    GLint lightColor1Loc = glGetUniformLocation(planeProgramId, "lightColor1");
    GLint lightPosition1Loc = glGetUniformLocation(planeProgramId, "lightPos1");
    GLint lightColor2Loc = glGetUniformLocation(planeProgramId, "lightColor2");
    GLint lightPosition2Loc = glGetUniformLocation(planeProgramId, "lightPos2");
    GLint viewPositionLoc = glGetUniformLocation(planeProgramId, "viewPosition");

    // pass color, light, and camera data to the plane shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColor1Loc, lightColor1.r, lightColor1.g, lightColor1.b);
    glUniform3f(lightPosition1Loc, lightPosition1.x, lightPosition1.y, lightPosition1.z);
    glUniform3f(lightColor2Loc, lightColor2.r, lightColor2.g, lightColor2.b);
    glUniform3f(lightPosition2Loc, lightPosition2.x, lightPosition2.y, lightPosition2.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint TextureScaleLoc = glGetUniformLocation(planeProgramId, "textureScale");
    glUniform2fv(TextureScaleLoc, 1, glm::value_ptr(textureScale));

    glBindVertexArray(meshPlane.vao);

    glDrawElements(GL_TRIANGLES, meshPlane.nIndices, GL_UNSIGNED_INT, (void*)0); // draw triangles

    /*
     * INITIALIZE objectProgramId UNIFORMS
     */

     // use the shader program created in createShaderProgram()
    glUseProgram(objectProgramId);

    // retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(objectProgramId, "model");
    viewLoc = glGetUniformLocation(objectProgramId, "view");
    projLoc = glGetUniformLocation(objectProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // reference matrix uniforms from the object shader program for the object color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(objectProgramId, "objectColor");
    lightColor1Loc = glGetUniformLocation(objectProgramId, "lightColor1");
    lightPosition1Loc = glGetUniformLocation(objectProgramId, "lightPos1");
    lightColor2Loc = glGetUniformLocation(objectProgramId, "lightColor2");
    lightPosition2Loc = glGetUniformLocation(objectProgramId, "lightPos2");
    viewPositionLoc = glGetUniformLocation(objectProgramId, "viewPosition");

    // pass color, light, and camera data to the object shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColor1Loc, lightColor1.r, lightColor1.g, lightColor1.b);
    glUniform3f(lightPosition1Loc, lightPosition1.x, lightPosition1.y, lightPosition1.z);
    glUniform3f(lightColor2Loc, lightColor2.r, lightColor2.g, lightColor2.b);
    glUniform3f(lightPosition2Loc, lightPosition2.x, lightPosition2.y, lightPosition2.z);
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    TextureScaleLoc = glGetUniformLocation(objectProgramId, "textureScale");
    glUniform2fv(TextureScaleLoc, 1, glm::value_ptr(textureScale));

    GLuint multipleTexturesLoc = glGetUniformLocation(objectProgramId, "multipleTextures"); // initialize boolean for extra textures
    glUniform1i(multipleTexturesLoc, true); // set multipleTextures to true

    // BOTTLE: draw bottle
    //----------------
    glBindTexture(GL_TEXTURE_2D, glassTextureId); // bind texture that was created

    glActiveTexture(GL_TEXTURE1); // set active texture for extra texture
    glBindTexture(GL_TEXTURE_2D, labelTextureId); // bind texture that was created

    // scales the object by 1.25
    glm::mat4 scale = glm::scale(glm::vec3(1.25f, 1.25f, 1.25f));
    // rotates the shape by 90 degrees on the x axis
    glm::mat4 rotation = glm::rotate(PI / 2, glm::vec3(1.0f, 0.0f, 0.0f));
    // places object lower than the origin on the y-axis and forward on z-axis
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, -0.75f, -1.5f));
    // Model matrix: Transformations are applied right-to-left.
    model = translation * rotation * scale;

    // retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(objectProgramId, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(meshBottleBottomCylinder.vao);

    glDrawElements(GL_TRIANGLES, bottleBottomCylinder.getIndexCount(), GL_UNSIGNED_INT, (void*)0); // draw triangles

    glUniform1i(multipleTexturesLoc, false);// turn off the extra texture

    // scales the object by 1.25
    scale = glm::scale(glm::vec3(1.25f, 1.25f, 1.25f));
    // rotates the shape by 90 degrees on the x axis
    rotation = glm::rotate(PI / 2, glm::vec3(1.0f, 0.0f, 0.0f));
    // places object higher than the origin on the y-axis and forward on z-axis
    translation = glm::translate(glm::vec3(0.0f, 1.5f, -1.5f));
    // Model matrix: Transformations are applied right-to-left.
    model = translation * rotation * scale;

    // retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(objectProgramId, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(meshBottleTopCylinder.vao);

    glDrawElements(GL_TRIANGLES, bottleTopCylinder.getIndexCount(), GL_UNSIGNED_INT, (void*)0); // draw triangles

    // scales the object by 1.25
    scale = glm::scale(glm::vec3(1.25f, 1.25f, 1.25f));
    // rotates the shape by 90 degrees on the x axis
    rotation = glm::rotate(PI / 2, glm::vec3(1.0f, 0.0f, 0.0f));
    // places object slightly higher than the origin on the y-axis and back on z-axis
    translation = glm::translate(glm::vec3(0.0f, 0.5f, -1.5f));
    // Model matrix: Transformations are applied right-to-left.
    model = translation * rotation * scale;

    // retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(objectProgramId, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(meshBottleSphere.vao);

    glDrawElements(GL_TRIANGLES, bottleSphere.getIndexCount(), GL_UNSIGNED_INT, (void*)0); // draw triangles

    // PEN: draw pen
    //----------------

    glActiveTexture(GL_TEXTURE0); // set active texture
    glBindTexture(GL_TEXTURE_2D, penTextureId); // bind texture that was created

    // scales the object by 1.25
    scale = glm::scale(glm::vec3(1.25f, 1.25f, 1.25f));
    // rotates the shape by 90 degrees on the x axis
    rotation = glm::rotate(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    // places object lower than the origin on the y-axis and to the left on x-axis
    translation = glm::translate(glm::vec3(-0.470f, -1.95f, 0.0f));
    // Model matrix: Transformations are applied right-to-left.
    model = translation * rotation * scale;

    // retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(objectProgramId, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(meshPenSphere.vao);

    glDrawElements(GL_TRIANGLES, penSphere.getIndexCount(), GL_UNSIGNED_INT, (void*)0); // draw triangles

    // scales the object by 1.25
    scale = glm::scale(glm::vec3(1.25f, 1.25f, 1.25f));
    // rotates the shape by 90 degrees on the x axis
    rotation = glm::rotate(PI / 2, glm::vec3(0.0f, 1.0f, 0.0f));
    // places object lower than the origin on the y-axis
    translation = glm::translate(glm::vec3(0.0f, -1.95f, 0.0f));
    // Model matrix: Transformations are applied right-to-left.
    model = translation * rotation * scale;

    // retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(objectProgramId, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(meshPenCylinder.vao);

    glDrawElements(GL_TRIANGLES, penCylinder.getIndexCount(), GL_UNSIGNED_INT, (void*)0); // draw triangles

    // scales the object by 1.25
    scale = glm::scale(glm::vec3(1.25f, 1.25f, 1.25f));
    // rotates the shape by 90 degrees on the x axis
    rotation = glm::rotate(PI / 2, glm::vec3(0.0f, 1.0f, 0.0f));
    // places object lower than the origin on the y-axis and to the right on the x-axis
    translation = glm::translate(glm::vec3(0.530f, -1.95f, 0.0f));
    // Model matrix: Transformations are applied right-to-left.
    model = translation * rotation * scale;

    // retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(objectProgramId, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(meshPenCone.vao);

    glDrawElements(GL_TRIANGLES, penCone.getIndexCount(), GL_UNSIGNED_INT, (void*)0); // draw triangles
    
    // BOX: draw box
    //----------------

    glBindTexture(GL_TEXTURE_2D, boxTextureId); // bind texture that was created

    // scales the object by 1.25
    scale = glm::scale(glm::vec3(1.25f, 1.25f, 1.25f));
    // rotates the shape by 90 degrees on the x axis
    rotation = glm::rotate(PI / 4, glm::vec3(0.0f, 1.0f, 0.0f));
    // places object lower than the origin on the y-axis, left on the x-axis, and back on the z-axis
    translation = glm::translate(glm::vec3(-2.0f, -1.05f, -1.0f));
    // Model matrix: Transformations are applied right-to-left.
    model = translation * rotation * scale;

    // retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(objectProgramId, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(meshBox.vao);

    glDrawArrays(GL_TRIANGLES, 0, meshBox.nIndices);

    // PERFUME: draw perfume
    //----------------

    glBindTexture(GL_TEXTURE_2D, perfumeTextureId); // bind texture that was created

    // scales the object by 1.25
    scale = glm::scale(glm::vec3(1.25f, 1.25f, 1.25f));
    // rotates the shape by 90 degrees on the x axis
    rotation = glm::rotate(PI / 2, glm::vec3(1.0f, 0.0f, 0.0f));
    // places object lower than the origin on the y-axis, right on the x-axis, and back on the z-axis
    translation = glm::translate(glm::vec3(1.5f, -0.75f, -1.0f));
    // Model matrix: Transformations are applied right-to-left.
    model = translation * rotation * scale;

    // retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(objectProgramId, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(meshPerfume.vao);

    glDrawElements(GL_TRIANGLES, perfumeCylinder.getIndexCount(), GL_UNSIGNED_INT, (void*)0); // draw triangles

    // LIGHTS: draw lights
    //----------------
    glUseProgram(lightProgramId);

    // transform the cube to be used as a visual representation of the second light
    model = glm::translate(lightPosition1) * glm::scale(lightScale);

    // reference matrix uniforms from the light shader program
    modelLoc = glGetUniformLocation(lightProgramId, "model");
    viewLoc = glGetUniformLocation(lightProgramId, "view");
    projLoc = glGetUniformLocation(lightProgramId, "projection");

    // pass matrix data to the light shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(meshLight.vao);

    glDrawArrays(GL_TRIANGLES, 0, meshLight.nIndices);

    // transform the cube to be used as a visual representation of the second light
    model = glm::translate(lightPosition2) * glm::scale(lightScale);

    // reference matrix uniforms from the light shader program
    modelLoc = glGetUniformLocation(lightProgramId, "model");
    // pass matrix data to the light shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLES, 0, meshLight.nIndices);

    glfwSwapBuffers(window);
}

// function to create mesh to buffer vertex and index data to GPU
void createPlaneMesh(GLMesh& mesh)
{
    float vertices[] =
    {
        // vertex 0
        -5.0f, -3.0f, -5.0f, // back left
        0.0f, 1.0f, 0.0f, // positive y normal
        0.0f, 0.0f,

        // vertex 1
        5.0f,-3.0f, -5.0f, // back right
        0.0f, 1.0f, 0.0f, // positive y normal
        1.0f, 0.0f,

        // vertex 2
        -5.0f, -3.0f, 5.0f, // front left
        0.0f, 1.0f, 0.0f, // positive y normal
        0.0f, 1.0f,

        // vertex 3
        5.0f, -3.0f, 5.0f, // front right
        0.0f, 1.0f, 0.0f, // positive y normal
        1.0f, 1.0f
    };

    GLuint indices[] =
    {
        0, 1, 2, // first triangle
        1, 2, 3
    };

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

    glGenVertexArrays(1, &mesh.vao); // generate vertex array
    glGenBuffers(1, &mesh.vbo); // generate vertex buffer object
    glGenBuffers(1, &mesh.ebo); // generate element buffer object

    glBindVertexArray(mesh.vao); // bind the created vertex array

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // bind vertex buffer object
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // buffer vertex data to GPU

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo); // bind element buffer object
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // buffer index data to GPU

    // variables that indicate the amount of floats for each vertex and color attribute
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerTex = 2;

    // variable to indicate the stride for vertex and color attributes 
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerTex);

    // 
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    // 
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerTex, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// create a mesh using the vertices of a Sphere object
void createSphereMesh(GLMesh& mesh, Sphere sphere) {
    glGenVertexArrays(1, &mesh.vao); // generate vertex array
    glGenBuffers(1, &mesh.vbo); // generate vertex buffer object
    glGenBuffers(1, &mesh.ebo); // generate element buffer object

    glBindVertexArray(mesh.vao); // bind the created vertex array

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // bind vertex buffer object
    glBufferData(GL_ARRAY_BUFFER, sphere.getInterleavedVertexSize(), sphere.getInterleavedVertices(), GL_STATIC_DRAW); // buffer vertex data to GPU

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo); // bind element buffer object
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere.getIndexSize(), sphere.getIndices(), GL_STATIC_DRAW);

    // variables that indicate the amount of floats for each vertex and color attribute
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerTex = 2;

    int stride = sphere.getInterleavedStride();   // should be 32 bytes

    // initialize vertex attributes
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerTex, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// create a mesh using the vertices of a Cylinder object
void createCylinderMesh(GLMesh& mesh, Cylinder cylinder) {
    glGenVertexArrays(1, &mesh.vao); // generate vertex array
    glGenBuffers(1, &mesh.vbo); // generate vertex buffer object
    glGenBuffers(1, &mesh.ebo); // generate element buffer object

    glBindVertexArray(mesh.vao); // bind the created vertex array

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // bind vertex buffer object
    glBufferData(GL_ARRAY_BUFFER, cylinder.getInterleavedVertexSize(), cylinder.getInterleavedVertices(), GL_STATIC_DRAW); // buffer vertex data to GPU

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo); // bind element buffer object
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylinder.getIndexSize(), cylinder.getIndices(), GL_STATIC_DRAW);

    // variables that indicate the amount of floats for each vertex and color attribute
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerTex = 2;

    int stride = cylinder.getInterleavedStride();   // should be 32 bytes

    // initialize vertex attributes
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerTex, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// function to create mesh to buffer vertex and index data to GPU
void createLightMesh(GLMesh& mesh)
{
    float vertices[] =
    {
    //Positions          //Normals
    // ------------------------------------------------------
    //Back Face          //Negative Z Normal  Texture Coords.
   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
    0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    //Front Face         //Positive Z Normal
   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
   -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

    //Left Face          //Negative X Normal
   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
   -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
   -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    //Right Face         //Positive X Normal
    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    //Bottom Face        //Negative Y Normal
   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
   -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    //Top Face           //Positive Y Normal
   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
   -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    // variables that indicate the amount of floats for each vertex and color attribute
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerTex = 2;


    mesh.nIndices = sizeof(vertices) / (sizeof(vertices[0]) * (floatsPerVertex + floatsPerNormal + floatsPerTex));

    glGenVertexArrays(1, &mesh.vao); // generate vertex array
    glGenBuffers(1, &mesh.vbo); // generate vertex buffer object

    glBindVertexArray(mesh.vao); // bind the created vertex array

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // bind vertex buffer object
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // buffer vertex data to GPU


    // variable to indicate the stride for vertex and color attributes 
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerTex);

    // create vertex attribute pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerTex, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

void createBoxMesh(GLMesh& mesh)
{
    float vertices[] =
    {
    //Positions          //Normals
    // ------------------------------------------------------
    //Back Face          //Negative Z Normal  Texture Coords.
   -0.5f, -0.75f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
    0.5f, -0.75f, -0.25f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
    0.5f,  0.75f, -0.25f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
   -0.5f,  0.75f, -0.25f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    0.5f,  0.75f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
   -0.5f, -0.75f, -0.25f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    //Front Face         //Positive Z Normal
   -0.5f, -0.75f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
    0.5f, -0.75f,  0.25f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
    0.5f,  0.75f,  0.25f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    0.5f,  0.75f,  0.25f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
   -0.5f,  0.75f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
   -0.5f, -0.75f,  0.25f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

    //Left Face          //Negative X Normal
   -0.5f,  0.75f,  0.25f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
   -0.5f,  0.75f, -0.25f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
   -0.5f, -0.75f, -0.25f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
   -0.5f, -0.75f, -0.25f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
   -0.5f, -0.75f,  0.25f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
   -0.5f,  0.75f,  0.25f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
   
    //Right Face         //Positive X Normal
    0.5f,  0.75f,  0.25f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    0.5f,  0.75f, -0.25f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    0.5f, -0.75f, -0.25f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    0.5f, -0.75f, -0.25f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    0.5f, -0.75f,  0.25f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    0.5f,  0.75f,  0.25f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    //Bottom Face        //Negative Y Normal
   -0.5f, -0.75f, -0.25f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
    0.5f, -0.75f, -0.25f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
    0.5f, -0.75f,  0.25f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    0.5f, -0.75f,  0.25f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
   -0.5f, -0.75f,  0.25f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
   -0.5f, -0.75f, -0.25f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    //Top Face           //Positive Y Normal
   -0.5f,  0.75f, -0.25f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    0.5f,  0.75f, -0.25f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
    0.5f,  0.75f,  0.25f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    0.5f,  0.75f,  0.25f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
   -0.5f,  0.75f,  0.25f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
   -0.5f,  0.75f, -0.25f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };


    // variables that indicate the amount of floats for each vertex and color attribute
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerTex = 2;


    mesh.nIndices = sizeof(vertices) / (sizeof(vertices[0]) * (floatsPerVertex + floatsPerNormal + floatsPerTex));

    glGenVertexArrays(1, &mesh.vao); // generate vertex array
    glGenBuffers(1, &mesh.vbo); // generate vertex buffer object

    glBindVertexArray(mesh.vao); // bind the created vertex array

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // bind vertex buffer object
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // buffer vertex data to GPU


    // variable to indicate the stride for vertex and color attributes 
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerTex);

    // create vertex attribute pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerTex, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}

// function to get rid of the mesh prior to ending the software
void deleteMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ebo);
}

// function to flip the image to match the correct axis
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

// function to create the textures to be put on object
bool createTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0); // load image
    if (image)
    {
        flipImageVertically(image, width, height, channels); // flip image

        // generate and bind textures
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            std::cout << "Not implemented to handle image with " << channels << " channels" << std::endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D); // generate mipmap

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // unbind the texture.

        return true;
    }

    // error loading the image
    return false;
}

// function to create shader program. returns a boolean to show whether the process was successful or not
bool createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource, GLuint& programId)
{
    int successful;
    char errorLog[512];

    unsigned int vertexShader; // declare variable for vertex shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER); // create vertex shader and assign it to vertexShader
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); // specify the source for the vertex shader
    glCompileShader(vertexShader); // compile the vertex shader

    // ensure that the vertex shader was compiled correctly
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &successful);
    if (!successful)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cout << "Failed to compile vertex shader\n" << errorLog << std::endl;

        return false;
    }

    unsigned int fragmentShader; // declare variable for fragment shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); // create fragment shader and assign it to fragmentShader
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); // specify the source for the fragment shader
    glCompileShader(fragmentShader); // compile the fragment shader

    // ensure that the fragment shader was compiled correctly
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &successful);
    if (!successful)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
        std::cout << "Failed to compile fragment shader\n" << errorLog << std::endl;

        return false;
    }

    programId = glCreateProgram(); // create shader program and assign it to programId
    glAttachShader(programId, vertexShader); // attach vertex shader to shader program
    glAttachShader(programId, fragmentShader); // attach fragment shader to shader program
    glLinkProgram(programId); // link shader program

    // ensure that the shader program was linked correctly
    glGetProgramiv(programId, GL_LINK_STATUS, &successful);
    if (!successful)
    {
        glGetProgramInfoLog(programId, 512, NULL, errorLog);
        std::cout << "Failed to link shader program\n" << errorLog << std::endl;

        return false;
    }

    glUseProgram(programId);

    return true;
}


// function to delete shader program prior to ending the software
void deleteShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}