#include "RenderProject.h"

/* Initialize the Project */
void RenderProject::init()
{
    bRenderer::log("start init");
	bRenderer::loadConfigFile("config.json");	// load custom configurations replacing the default values in Configuration.cpp
    bRenderer::log("init 5");
	// let the renderer create an OpenGL context and the main window
	if(Input::isTouchDevice())
		bRenderer().initRenderer(true);										// full screen on iOS
	else
		bRenderer().initRenderer(1200, 700, false, "Underwater Game");		// windowed mode on desktop
		//bRenderer().initRenderer(View::getScreenWidth(), View::getScreenHeight(), true);		// full screen using full width and height of the screen
    bRenderer::log("init 10");
	// start main loop 
	bRenderer().runRenderer();
    bRenderer::log("init 20");
}

/* This function is executed when initializing the renderer */
void RenderProject::initFunction()
{
	// get OpenGL and shading language version
	bRenderer::log("OpenGL Version: ", glGetString(GL_VERSION));
	bRenderer::log("Shading Language Version: ", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// initialize variables
	_offset = 0.0f;
	_randomOffset = 0.0f;
	_cameraSpeed = 40.0f;
	_running = false; _lastStateSpaceKey = bRenderer::INPUT_UNDEFINED;
	_viewMatrixHUD = Camera::lookAt(vmml::Vector3f(0.0f, 0.0f, 0.25f), vmml::Vector3f::ZERO, vmml::Vector3f::UP);

	// set shader versions (optional)
	bRenderer().getObjects()->setShaderVersionDesktop("#version 120");
	bRenderer().getObjects()->setShaderVersionES("#version 100");



	// create camera
	bRenderer().getObjects()->createCamera("camera", vmml::Vector3f(-33.0, 0.f, -13.0), vmml::Vector3f(0.f, -M_PI_F / 2, 0.f));

	// load models
	bRenderer().getObjects()->loadObjModel("ground.obj", true, true, false, 0, true, false);


	// sprites
	bRenderer().getObjects()->createSprite("groundSprite", "sand.jpg");


	/* Begin postprocessing stuff */
	bRenderer().getObjects()->createFramebuffer("fbo");					// create framebuffer object
	bRenderer().getObjects()->createTexture("fbo_texture1", 0.f, 0.f);	// create texture to bind to the fbo

	bRenderer().getObjects()->createFramebuffer("fogFbo");
	bRenderer().getObjects()->createTexture("fogFbo_texture", 0.f, 0.f);
	DepthMapPtr dmapp = bRenderer().getObjects()->createDepthMap("dmapp", bRenderer().getView()->getWidth(), bRenderer().getView()->getHeight());

	ShaderPtr fogShader = bRenderer().getObjects()->loadShaderFile("fogShader", 0, false, false, false, false, false);
	MaterialPtr fogMaterial = bRenderer().getObjects()->createMaterial("fogMaterial", fogShader);
	bRenderer().getObjects()->createSprite("fogSprite", fogMaterial);
	/* END postprocessing stuff */



	// Update render queue
	updateRenderQueue("camera", 0.0f);
}

/* Draw your scene here */
void RenderProject::loopFunction(const double &deltaTime, const double &elapsedTime)
{
//	bRenderer::log("FPS: " + std::to_string(1 / deltaTime));	// write number of frames per second to the console every frame

	//// Draw Scene and do post processing ////


	/* Begin postprocessing */

	// bind depth map and render the scene
	GLint defaultFBO;
	defaultFBO = Framebuffer::getCurrentFramebuffer();

	bRenderer().getObjects()->getFramebuffer("fogFbo")->bindDepthMap(bRenderer().getObjects()->getDepthMap("dmapp"), true);

	bRenderer().getModelRenderer()->drawQueue(/*GL_LINES*/);

	// bind the texture and render the scene again
	bRenderer().getObjects()->getFramebuffer("fbo")->bindTexture(bRenderer().getObjects()->getTexture("fogFbo_texture"), true);
	bRenderer().getModelRenderer()->drawQueue(/*GL_LINES*/);
	bRenderer().getModelRenderer()->clearQueue();

	bRenderer().getObjects()->getFramebuffer("fogFbo")->unbind(defaultFBO);

	bRenderer().getObjects()->getShader("fogShader")->setUniform("depthmap_texture", bRenderer().getObjects()->getDepthMap("dmapp"));
	bRenderer().getObjects()->getShader("fogShader")->setUniform("fogFbo_texture", bRenderer().getObjects()->getTexture("fogFbo_texture"));
	float passedTime = float(elapsedTime);
	//passedTime =  modf(passedTime, M_PI);
	bRenderer().getObjects()->getShader("fogShader")->setUniform("passedTime", passedTime);	

	vmml::Matrix4f modelMatrix = vmml::create_translation(vmml::Vector3f(0.0f, 0.0f, -0.5));


	bRenderer().getModelRenderer()->drawModel(bRenderer().getObjects()->getModel("fogSprite"), modelMatrix, _viewMatrixHUD, vmml::Matrix4f::IDENTITY, std::vector<std::string>({}), false);

	/* End postprocessing */
	




	//// Camera Movement ////
	updateCamera("camera", deltaTime);
	

	/// Update render queue ///
	updateRenderQueue("camera", deltaTime);

	// Quit renderer when escape is pressed
	if (bRenderer().getInput()->getKeyState(bRenderer::KEY_ESCAPE) == bRenderer::INPUT_PRESS)
		bRenderer().terminateRenderer();
}

/* This function is executed when terminating the renderer */
void RenderProject::terminateFunction()
{
	bRenderer::log("I totally terminated this Renderer :-)");
}

/* Update render queue */
void RenderProject::updateRenderQueue(const std::string &camera, const double &deltaTime)
{
    // draw Ground
    vmml::Matrix4f modelMatrix6 = vmml::create_translation(vmml::Vector3f(0.0f, -100.0f, -10.0f)) * vmml::create_scaling(vmml::Vector3f(10.0f));
    //bRenderer().getModelRenderer()->drawModel("plane", camera, modelMatrix6, std::vector<std::string>({ "testLight"}), true, false);
    modelMatrix6 *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(0.0f, 1.f, 0.f));
    //bRenderer().getModelRenderer()->drawModel("groundModel", camera, modelMatrix6, std::vector<std::string>({ "testLight"}), true, false);
    bRenderer().getModelRenderer()->queueModelInstance("ground", "ground_instance", camera, modelMatrix6, std::vector<std::string>({}));
    
    // ground Sprite
    vmml::Matrix4f modelMatrix5 = vmml::create_translation(vmml::Vector3f(0.0f, -100.0f, -10.0f)) * vmml::create_scaling(vmml::Vector3f(50.0f));
    modelMatrix5 *= vmml::create_rotation(M_PI_2_F, vmml::Vector3f(1.0f, 0.f, 0.f));
    bRenderer().getModelRenderer()->drawModel("groundSprite", camera, modelMatrix5, std::vector<std::string>({}), true, false);
    
}

/* Camera movement */
void RenderProject::updateCamera(const std::string &camera, const double &deltaTime)
{
	//// Adjust aspect ratio ////
	bRenderer().getObjects()->getCamera(camera)->setAspectRatio(bRenderer().getView()->getAspectRatio());

	double deltaCameraY = 0.0;
	double deltaCameraX = 0.0;
	double cameraForward = 0.0;
	double cameraSideward = 0.0;

	/* iOS: control movement using touch screen */
	if (Input::isTouchDevice()){

		// pause using double tap
		if (bRenderer().getInput()->doubleTapRecognized()){
			_running = !_running;
		}

		if (_running){
			// control using touch
			TouchMap touchMap = bRenderer().getInput()->getTouches();
			int i = 0;
			for (auto t = touchMap.begin(); t != touchMap.end(); ++t)
			{
				Touch touch = t->second;
				// If touch is in left half of the view: move around
				if (touch.startPositionX < bRenderer().getView()->getWidth() / 2){
					cameraForward = -(touch.currentPositionY - touch.startPositionY) / 100;
					cameraSideward = (touch.currentPositionX - touch.startPositionX) / 100;

				}
				// if touch is in right half of the view: look around
				else
				{
					deltaCameraY = (touch.currentPositionX - touch.startPositionX) / 2000;
					deltaCameraX = (touch.currentPositionY - touch.startPositionY) / 2000;
				}
				if (++i > 2)
					break;
			}
		}

	}
	/* Windows: control movement using mouse and keyboard */
	else{
		// use space to pause and unpause
		GLint currentStateSpaceKey = bRenderer().getInput()->getKeyState(bRenderer::KEY_SPACE);
		if (currentStateSpaceKey != _lastStateSpaceKey)
		{
			_lastStateSpaceKey = currentStateSpaceKey;
			if (currentStateSpaceKey == bRenderer::INPUT_PRESS){
				_running = !_running;
				bRenderer().getInput()->setCursorEnabled(!_running);
			}
		}

		// mouse look
		double xpos, ypos; bool hasCursor = false;
		bRenderer().getInput()->getCursorPosition(&xpos, &ypos, &hasCursor);

		deltaCameraY = (xpos - _mouseX) / 1000;
		_mouseX = xpos;
		deltaCameraX = (ypos - _mouseY) / 1000;
		_mouseY = ypos;

		if (_running){
			// movement using wasd keys
			if (bRenderer().getInput()->getKeyState(bRenderer::KEY_W) == bRenderer::INPUT_PRESS)
				if (bRenderer().getInput()->getKeyState(bRenderer::KEY_LEFT_SHIFT) == bRenderer::INPUT_PRESS) 			cameraForward = 2.0;
				else			cameraForward = 1.0;
			else if (bRenderer().getInput()->getKeyState(bRenderer::KEY_S) == bRenderer::INPUT_PRESS)
				if (bRenderer().getInput()->getKeyState(bRenderer::KEY_LEFT_SHIFT) == bRenderer::INPUT_PRESS) 			cameraForward = -2.0;
				else			cameraForward = -1.0;
			else
				cameraForward = 0.0;

			if (bRenderer().getInput()->getKeyState(bRenderer::KEY_A) == bRenderer::INPUT_PRESS)
				cameraSideward = -1.0;
			else if (bRenderer().getInput()->getKeyState(bRenderer::KEY_D) == bRenderer::INPUT_PRESS)
				cameraSideward = 1.0;
			if (bRenderer().getInput()->getKeyState(bRenderer::KEY_UP) == bRenderer::INPUT_PRESS)
				bRenderer().getObjects()->getCamera(camera)->moveCameraUpward(_cameraSpeed*deltaTime);
			else if (bRenderer().getInput()->getKeyState(bRenderer::KEY_DOWN) == bRenderer::INPUT_PRESS)
				bRenderer().getObjects()->getCamera(camera)->moveCameraUpward(-_cameraSpeed*deltaTime);
			if (bRenderer().getInput()->getKeyState(bRenderer::KEY_LEFT) == bRenderer::INPUT_PRESS)
				bRenderer().getObjects()->getCamera(camera)->rotateCamera(0.0f, 0.0f, 0.03f*_cameraSpeed*deltaTime);
			else if (bRenderer().getInput()->getKeyState(bRenderer::KEY_RIGHT) == bRenderer::INPUT_PRESS)
				bRenderer().getObjects()->getCamera(camera)->rotateCamera(0.0f, 0.0f, -0.03f*_cameraSpeed*deltaTime);
		}
	}

	//// Update camera ////
	if (_running){
		bRenderer().getObjects()->getCamera(camera)->moveCameraForward(cameraForward*_cameraSpeed*deltaTime);
		bRenderer().getObjects()->getCamera(camera)->rotateCamera(deltaCameraX, deltaCameraY, 0.0f);
		bRenderer().getObjects()->getCamera(camera)->moveCameraSideward(cameraSideward*_cameraSpeed*deltaTime);
	}	
}

/* For iOS only: Handle device rotation */
void RenderProject::deviceRotated()
{
	if (bRenderer().isInitialized()){
		// set view to full screen after device rotation
		bRenderer().getView()->setFullscreen(true);
		bRenderer::log("Device rotated");
	}
}

/* For iOS only: Handle app going into background */
void RenderProject::appWillResignActive()
{
	if (bRenderer().isInitialized()){
		// stop the renderer when the app isn't active
		bRenderer().stopRenderer();
	}
}

/* For iOS only: Handle app coming back from background */
void RenderProject::appDidBecomeActive()
{
	if (bRenderer().isInitialized()){
		// run the renderer as soon as the app is active
		bRenderer().runRenderer();
	}
}

/* For iOS only: Handle app being terminated */
void RenderProject::appWillTerminate()
{
	if (bRenderer().isInitialized()){
		// terminate renderer before the app is closed
		bRenderer().terminateRenderer();
	}
}

/* Helper functions */
GLfloat RenderProject::randomNumber(GLfloat min, GLfloat max){
	return min + static_cast <GLfloat> (rand()) / (static_cast <GLfloat> (RAND_MAX / (max - min)));
}