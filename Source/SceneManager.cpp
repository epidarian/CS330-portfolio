///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// Start with an empty texture table. Without this the first
	// CreateGLTexture() call writes to a random array index because
	// m_loadedTextures would otherwise be uninitialized.
	m_loadedTextures = 0;
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "";
		m_textureIDs[i].ID = 0;
	}
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	// report whether a match was found; returning true unconditionally would
	// hand back an uninitialized material for any tag that was not defined.
	return(bFound);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  DefineObjectMaterials()
 *
 *  Defines the surface materials the lit shader uses. The
 *  fragment shader derives diffuse and specular entirely from
 *  these values (material.diffuseColor, material.specularColor,
 *  material.shininess), so every drawn shape applies one before
 *  drawing or it would reflect no diffuse or specular light.
 *  shininess scales specular strength in this shader rather than
 *  acting as the exponent, so higher means a brighter highlight.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Wood: the butcher-block tabletop. Matte, warm, with a faint
	// highlight so the plane still catches and reflects the light.
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.25f, 0.20f, 0.15f);
	woodMaterial.ambientStrength = 0.3f;
	woodMaterial.diffuseColor = glm::vec3(0.55f, 0.45f, 0.35f);
	woodMaterial.specularColor = glm::vec3(0.25f, 0.22f, 0.18f);
	woodMaterial.shininess = 4.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// Metal: canister bands, valve, cap, nozzle, can rim. Brightest,
	// tightest highlight so the brushed-steel parts read as metal.
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.20f, 0.20f, 0.22f);
	metalMaterial.ambientStrength = 0.2f;
	metalMaterial.diffuseColor = glm::vec3(0.55f, 0.55f, 0.58f);
	metalMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	metalMaterial.shininess = 22.0f;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	// Plastic: bodies, housings, printed labels. Soft, moderate
	// highlight — duller than metal, brighter than wood.
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.20f, 0.20f, 0.20f);
	plasticMaterial.ambientStrength = 0.3f;
	plasticMaterial.diffuseColor = glm::vec3(0.65f, 0.65f, 0.65f);
	plasticMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	plasticMaterial.shininess = 8.0f;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	// Glass: the balsamic bottle neck. Low diffuse, sharp specular so
	// it glints without looking solid.
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.10f, 0.12f, 0.10f);
	glassMaterial.ambientStrength = 0.2f;
	glassMaterial.diffuseColor = glm::vec3(0.30f, 0.35f, 0.30f);
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	glassMaterial.shininess = 30.0f;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  Adds the light sources for the scene. Two point lights model
 *  a warm indoor kitchen counter: a warm key light from the
 *  upper front-right does the main shaping, and a cooler, dimmer
 *  fill from the left lifts the opposite side so no surface falls
 *  into complete shadow. The shader exposes lightSources[4]; the
 *  remaining two stay unset (and therefore dark).
 *
 *  Each LightSource field matches the shader's struct exactly:
 *  position, ambientColor, diffuseColor, specularColor,
 *  focalStrength (the specular exponent) and specularIntensity.
 *  The shader adds ambientColor directly, so it is kept small.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// route every fragment through the shader's lit path; without
	// this the scene draws flat and unlit through objectColor/texture
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// --- lightSources[0]: warm key light, upper front-right ---------------
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(4.0f, 8.0f, 6.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.12f, 0.11f, 0.10f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 0.95f, 0.85f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 1.0f, 0.95f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.6f);

	// --- lightSources[1]: cool fill light, front-left, dimmer -------------
	// Fills the shadow side so nothing reads as black, and its cool tone
	// balances the warm key without overpowering it.
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-5.0f, 4.0f, 4.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.08f, 0.09f, 0.12f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.35f, 0.40f, 0.50f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.3f, 0.3f, 0.35f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.3f);
}

/***********************************************************
 *  LoadSceneTextures()
 *
 *  Loads every image file the scene uses into a texture slot,
 *  then binds the slots for rendering. Tags are resolved later
 *  by SetShaderTexture(), so load order does not matter. All
 *  files live in the shared Utilities/textures folder — the
 *  same relative path the shaders load from in MainCode.cpp.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// butcher-block tabletop; tiled in RenderScene so the grain repeats
	CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "wood");
	// brushed steel: canister band, valve neck, head cap, and nozzle
	CreateGLTexture("../../Utilities/textures/brushed_metal.jpg", "metal");
	// printed red wrapper of the butane canister
	CreateGLTexture("../../Utilities/textures/butane_canister.jpg", "canister");
	// white plastic of the trigger-head housing
	CreateGLTexture("../../Utilities/textures/white_plastic.jpg", "whiteplastic");
	// dark plastic of the base and grip collar
	CreateGLTexture("../../Utilities/textures/black_plastic.jpg", "blackplastic");
	// printed label wrapping the balsamic vinegar bottle
	CreateGLTexture("../../Utilities/textures/balsamic_label.jpg", "balsamic");
	// printed label wrapping the lemonade canister body
	CreateGLTexture("../../Utilities/textures/lemonade_label.jpg", "lemonade");
	// printed label wrapping the Diet Coke can body
	CreateGLTexture("../../Utilities/textures/coke_label.jpg", "coke");

	// bind every loaded texture to its OpenGL slot (16 available)
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures used by the scene before any rendering
	LoadSceneTextures();

	// define the surface materials and add the light sources so the
	// shader's lit path has both inputs before the first frame draws
	DefineObjectMaterials();
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes.
 *
 *  Object: Iwatani butane kitchen torch
 *  Composition (bottom to top):
 *    1. flat oval base         (cylinder, black plastic)
 *    2. red butane canister    (cylinder, red wrapper)
 *    3. silver upper can       (cylinder, exposed metal)
 *    4. valve neck             (cylinder, smaller silver step)
 *    5. white head body        (box, white plastic housing)
 *    6. metallic head top      (box, silver top cap)
 *    7. black grip collar      (cylinder, horizontal, at nozzle base)
 *    8. chrome nozzle barrel   (cylinder, horizontal, extends -X)
 *
 *  Horizontal parts (7 & 8) are rotated 90 degrees around the Z axis,
 *  which maps the cylinder's +Y axis onto -X, so they extend out the
 *  left side of the head.
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** 0. Tabletop — flat plane, butcher-block wood tone ***/
	// The torch base bottom sits at y = 0, so the plane lives at y = 0
	// underneath it. The plane is drawn first so depth testing layers
	// every torch part on top of it cleanly.
	scaleXYZ = glm::vec3(5.0f, 1.0f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(2.0f, 2.0f); // tile so the grain repeats, not stretches
	SetShaderMaterial("wood"); // material for the lit path
	m_basicMeshes->DrawPlaneMesh();

	/*** 1. Base — flat oval cylinder, dark plastic ***/
	scaleXYZ = glm::vec3(1.0f, 0.1f, 0.8f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.8f, 0.0f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("blackplastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	/*** 2. Butane canister — red wrapper section ***/
	scaleXYZ = glm::vec3(0.45f, 1.50f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.8f, 0.10f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("canister");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	/*** 3. Canister upper silver band (above the wrapper) ***/
	scaleXYZ = glm::vec3(0.43f, 0.20f, 0.43f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.8f, 1.60f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	/*** 4. Valve neck — smaller cylinder step before the head ***/
	scaleXYZ = glm::vec3(0.16f, 0.13f, 0.16f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.8f, 1.80f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	/*** 5. Head body — white plastic housing (box centered) ***/
	// Box is positioned at its center; bottom face needs to sit at y = 1.93
	// (top of valve neck), so center.y = 1.93 + scaleY/2 = 1.93 + 0.16 = 2.09.
	scaleXYZ = glm::vec3(0.55f, 0.32f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.8f, 2.09f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("whiteplastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic"); // material for the lit path
	m_basicMeshes->DrawBoxMesh();

	/*** 6. Head metallic top — silver cap (smaller box, stacked) ***/
	// Top of white body is at y = 2.09 + 0.16 = 2.25; this box bottom
	// sits there, so center.y = 2.25 + 0.075 = 2.325.
	scaleXYZ = glm::vec3(0.50f, 0.15f, 0.40f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.8f, 2.325f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal"); // material for the lit path
	m_basicMeshes->DrawBoxMesh();

	/*** 7. Black grip collar — short cylinder rotated to horizontal ***/
	// Rotated 90 deg around Z: +Y axis -> -X axis. The cylinder extends from
	// its position outward in -X. Position is the LEFT face of the head body
	// (x = -0.275), at the vertical midline of the head (y ≈ 2.18).
	scaleXYZ = glm::vec3(0.13f, 0.20f, 0.13f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(-2.075f, 2.18f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("blackplastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	/*** 8. Chrome nozzle barrel — long cylinder rotated to horizontal ***/
	// Starts where the grip collar ends (x = -0.275 - 0.20 = -0.475)
	// and extends another 0.85 units in -X to x = -1.325.
	scaleXYZ = glm::vec3(0.085f, 0.85f, 0.085f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(-2.275f, 2.18f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 4.0f); // tile the grain down the long barrel
	SetShaderMaterial("metal"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Additional countertop items, left-to-right in the scene: ***/
	/*** torch, balsamic, lemonade, Diet Coke. The plane stays    ***/
	/*** centered as the counter. Torch and lemonade hold the     ***/
	/*** back row (z = 0); the balsamic bottle (front-left) and   ***/
	/*** Diet Coke can (front-right) step forward to z = 1.1 and  ***/
	/*** cant evenly about Y. Each label body uses SetShaderColor ***/
	/*** as a stand-in; swap it for SetShaderTexture once the     ***/
	/*** label image exists.                                      ***/
	/****************************************************************/

	// Reset rotation: the nozzle above left ZrotationDegrees at 90. The
	// front pair is canted evenly about Y (+/-18 deg) so the two turn
	// slightly outward; the flanking items stay unrotated. Cylinders are
	// bottom-origin (base at position.y); boxes are center-origin.
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	/*** Country Time Lemonade canister — back row, midway between ***/
	/*** the bottle and the can so the line reads evenly.          ***/
	// Body: cylinder, bottom-origin, base on the plane at y = 0.
	scaleXYZ = glm::vec3(0.70f, 2.20f, 0.70f);
	positionXYZ = glm::vec3(0.7f, 0.0f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("lemonade");
	SetTextureUVScale(1.0f, 1.0f); // wraps once around the body
	SetShaderMaterial("plastic"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	// Lid: short, slightly wider cylinder overhanging the body top (y = 2.20).
	scaleXYZ = glm::vec3(0.74f, 0.12f, 0.74f);
	positionXYZ = glm::vec3(0.7f, 2.20f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("whiteplastic");
	SetTextureUVScale(1.0f, 1.0f); // reuse the head-housing plastic
	SetShaderMaterial("plastic"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	/*** Balsamic vinegar bottle — front-left of center, canted +18 deg ***/
	YrotationDegrees = 18.0f;
	// Body: box, center-origin, so center.y = height/2 = 0.70 rests on plane.
	scaleXYZ = glm::vec3(0.30f, 1.40f, 0.30f);
	positionXYZ = glm::vec3(-0.40f, 0.70f, 1.10f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("balsamic");
	SetTextureUVScale(1.0f, 1.0f); // box maps the label to every face
	SetShaderMaterial("plastic"); // material for the lit path
	m_basicMeshes->DrawBoxMesh();

	// Neck: thin cylinder seated on the body top (y = 1.40).
	scaleXYZ = glm::vec3(0.08f, 0.25f, 0.08f);
	positionXYZ = glm::vec3(-0.40f, 1.40f, 1.10f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0.18f, 0.14f, 0.09f, 1.0f); // glass neck
	SetShaderMaterial("glass"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	// Cap: short cylinder on the neck top (y = 1.65).
	scaleXYZ = glm::vec3(0.10f, 0.15f, 0.10f);
	positionXYZ = glm::vec3(-0.40f, 1.65f, 1.10f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f); // black cap
	SetShaderMaterial("plastic"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	/*** Diet Coke can — front-right of center, canted -18 deg ***/
	YrotationDegrees = -18.0f;
	// Body: cylinder, bottom-origin, base on the plane at y = 0.
	scaleXYZ = glm::vec3(0.33f, 1.30f, 0.33f);
	positionXYZ = glm::vec3(1.8f, 0.0f, 1.10f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("coke");
	SetTextureUVScale(1.0f, 1.0f); // wraps once around the body
	SetShaderMaterial("plastic"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();

	// Top rim: narrower short cylinder on the body top (y = 1.30).
	scaleXYZ = glm::vec3(0.27f, 0.08f, 0.27f);
	positionXYZ = glm::vec3(1.8f, 1.30f, 1.10f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f); // reuse the brushed-metal texture
	SetShaderMaterial("metal"); // material for the lit path
	m_basicMeshes->DrawCylinderMesh();
}