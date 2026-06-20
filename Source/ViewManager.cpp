///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;

	// Scroll callback declared at file scope so it can be passed to
	// glfwSetScrollCallback without requiring a change to ViewManager.h.
	// Bound in CreateDisplayWindow(); modifies camera MovementSpeed
	// inside a clamped range so scroll-up speeds up motion and
	// scroll-down slows it down.
	void Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
	{
		if (g_pCamera != nullptr)
		{
			g_pCamera->MovementSpeed += static_cast<float>(yOffset) * 0.5f;
			if (g_pCamera->MovementSpeed < 0.1f)
			{
				g_pCamera->MovementSpeed = 0.1f;
			}
			if (g_pCamera->MovementSpeed > 25.0f)
			{
				g_pCamera->MovementSpeed = 25.0f;
			}
		}
	}
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events — required for FPS-style
	// look around with the mouse cursor.
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// scroll wheel adjusts camera movement speed (not zoom)
	glfwSetScrollCallback(window, Mouse_Scroll_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 *  Mouse movement maps to camera yaw (horizontal) and pitch
 *  (vertical) so the user can look around the scene.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// First frame: seed last-position with current to avoid a jump
	// the first time the cursor moves.
	if (gFirstMouse)
	{
		gLastX = static_cast<float>(xMousePos);
		gLastY = static_cast<float>(yMousePos);
		gFirstMouse = false;
	}

	// Offsets are deltas since the previous frame. Y is inverted because
	// screen-space Y increases downward but camera pitch increases upward.
	float xOffset = static_cast<float>(xMousePos) - gLastX;
	float yOffset = gLastY - static_cast<float>(yMousePos);

	gLastX = static_cast<float>(xMousePos);
	gLastY = static_cast<float>(yMousePos);

	if (g_pCamera != nullptr)
	{
		g_pCamera->ProcessMouseMovement(xOffset, yOffset);
	}
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 *
 *  Bindings:
 *    ESC        — close the window
 *    W / S      — move forward / backward along camera Front
 *    A / D      — strafe left / right along camera Right
 *    Q / E      — move up / down along world Y
 *    P          — perspective projection
 *    O          — orthographic projection
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	if (g_pCamera == nullptr)
	{
		return;
	}

	// Frame-rate-independent step. MovementSpeed lives on the Camera
	// object and is adjusted by the scroll wheel callback.
	const float velocity = g_pCamera->MovementSpeed * gDeltaTime;
	const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// W / S — move along the camera's Front vector
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->Position += g_pCamera->Front * velocity;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->Position -= g_pCamera->Front * velocity;
	}

	// A / D — strafe along the right vector (Front cross Up).
	// Computed locally so this works whether or not the Camera class
	// exposes a Right member.
	glm::vec3 right = glm::normalize(glm::cross(g_pCamera->Front, g_pCamera->Up));
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->Position -= right * velocity;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->Position += right * velocity;
	}

	// Q / E — move along world up so vertical motion stays vertical
	// even when the camera is tilted (pitched up or down).
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->Position += worldUp * velocity;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->Position -= worldUp * velocity;
	}

	// P / O — switch projection mode.
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		bOrthographicProjection = false;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		bOrthographicProjection = true;
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// build either a perspective or orthographic projection matrix
	// depending on the current mode (toggled with P / O).
	if (bOrthographicProjection == false)
	{
		// perspective: standard 3D, FOV from camera zoom
		projection = glm::perspective(
			glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f,
			100.0f);
	}
	else
	{
		// orthographic: parallel projection, no foreshortening.
		// The visible volume is a box of half-size orthoSize on Y,
		// scaled in X by the aspect ratio so the scene is not stretched.
		const float orthoSize = 5.0f;
		const float aspect = (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT;
		projection = glm::ortho(
			-orthoSize * aspect, orthoSize * aspect,
			-orthoSize, orthoSize,
			0.1f, 100.0f);
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}