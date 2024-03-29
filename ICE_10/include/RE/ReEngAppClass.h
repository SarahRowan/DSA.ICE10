/*----------------------------------------------
Programmer: Alberto Bobadilla (labigm@gmail.com)
Date: 2015/08
----------------------------------------------*/
#ifndef __REENGAPPCLASS_H_
#define __REENGAPPCLASS_H_
#pragma warning(disable:4251)

#include "RE\ReEng.h"
#include <locale>
#include <codecvt>
#include <string>

/* Winappi callback for the window */
ReEngDLL LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ReEng
{

	class ReEngDLL ReEngAppClass
	{
	protected:
		bool m_bFPC = false;// First Person Camera flag
		bool m_bArcBall = false;// Arcball flag

		int m_nCmdShow;	// Number of starting commands on startup
		HINSTANCE m_hInstance; // Windows application Instance
		LPWSTR m_lpCmdLine; // Command line arguments

		//Standard variables
		SystemSingleton* m_pSystem = nullptr;// Singleton of the system

		WindowClass* m_pWindow = nullptr;// Window class
		GLSystemSingleton* m_pGLSystem = nullptr;// Singleton of the OpenGL rendering context

		LightManagerSingleton* m_pLightMngr = nullptr;// Singleton for the Light Manager
		MeshManagerSingleton* m_pMeshMngr = nullptr;//Mesh Manager

		GridClass* m_pGrid = nullptr; // Grid that represents the Coordinate System
		CameraSingleton* m_pCamera = nullptr; // Singleton for the camera that represents our scene
		vector4 m_v4ClearColor;//Color of the scene
		matrix4 m_m4ArcBall;//ArcBall matrix

	public:
		/* Constructor - When inheriting do not add into the constructor */
		ReEngAppClass(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow) : m_hInstance(hInstance), m_lpCmdLine(lpCmdLine), m_nCmdShow(nCmdShow) {}

		/* Destructor */
		~ReEngAppClass()	{ Release(); };

		/*
		Run
		Runs the main loop of this class
		DO NOT OVERRIDE
		*/
		virtual void Run(void) final
		{
			Init(m_hInstance, m_lpCmdLine, m_nCmdShow);

			//Run the main loop until the exit message is sent
			MSG msg = { 0 };
			while (WM_QUIT != msg.message)
			{
				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					ProcessKeyboard();
					ProcessMouse();
					ProcessJoystick();
					Update();
					Display();
				}
				Idle();
			}
		}

	private:
		/* Copy Constructor - Cant be defined */
		ReEngAppClass(ReEngAppClass const& input);

		/* Copy Assignment Operator - Cant be defined*/
		ReEngAppClass& operator=(ReEngAppClass const& input);

		/*
		Reshape
		Resizes the window
		DO NOT OVERRIDE
		*/
		virtual void Reshape(int a_nWidth, int a_nHeight) final { /*WORK IN PROGRES*/ }

		/*
		Init
		Initializes the ReEng window and rendering context
		DO NOT OVERRIDE
		*/
		virtual void Init(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow) final
		{
			// Is this running out of Visual Studio?
			if (IsDebuggerPresent())
			{
				system("cmd.exe /C xcopy \"../include/RE/Shaders\" \"Shaders\" /y /q");
				m_pWindow->CreateConsoleWindow();
				printf("Shaders: ");
			}

			// Get the system singleton
			m_pSystem = SystemSingleton::GetInstance();

			// Init the App System
			InitApplication("Rendering Engine");
			m_pLightMngr = LightManagerSingleton::GetInstance();

			// Read the configuration file
			ReadConfig();

#pragma region Window Construction and Context setup
			// Create a new window and set its properties
			m_pWindow = new WindowClass(hInstance, nCmdShow, WndProc);
			m_pWindow->SetFullscreen(m_pSystem->IsWindowFullscreen());
			m_pWindow->SetBorderless(m_pSystem->IsWindowBorderless());

			// Make the Window name a wide string
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			std::wstring wide = converter.from_bytes(m_pSystem->WindowName);

			// Create a window in the window class
			m_pWindow->CreateMEWindow(wide.c_str(), m_pSystem->WindowWidth, m_pSystem->WindowHeight);

			// Get the instance of the class
			m_pGLSystem = GLSystemSingleton::GetInstance();

			// Set OPENGL 3.x Context
			m_pSystem->m_RenderingContext = OPENGL3X;

			// Create context
			m_pGLSystem->InitGLDevice(m_pWindow->GetHandler());

			// Verify what is the OpenGL rendering context and save it to system (3.x might fail, in which case exit)
			if (m_pGLSystem->IsNewOpenGLRunning() == false)
				exit(0);
#pragma endregion

			// Get the singletons
			m_pCamera = CameraSingleton::GetInstance();

			// Initialize the App Variables
			InitApplicationVariables();
			InitUserVariables();

			//Color of the window
			if (m_pSystem->m_RenderingContext == OPENGL3X)
				glClearColor(m_v4ClearColor.r, m_v4ClearColor.g, m_v4ClearColor.b, m_v4ClearColor.a);

			//Start the clock
			m_pSystem->StartClock();

			printf("\n");
		}

	protected:
		/*
		ArcBall
		Process the arcball of the scene, rotating an object in the center of it
		a_fSensitivity is a factor of change
		DO NOT OVERRIDE
		*/
		virtual matrix4 ArcBall(float a_fSensitivity = 0.1f) final
		{
			static matrix4 arcball;
			UINT	MouseX, MouseY;		// Coordinates for the mouse
			UINT	CenterX, CenterY;	// Coordinates for the center of the screen.

			CenterX = m_pSystem->WindowX + m_pSystem->WindowWidth / 2;
			CenterY = m_pSystem->WindowY + m_pSystem->WindowHeight / 2;

			float DeltaMouse;
			POINT pt;

			GetCursorPos(&pt);

			MouseX = pt.x;
			MouseY = pt.y;

			SetCursorPos(CenterX, CenterY);

			static float fVerticalAngle = 0.0f;
			static float fHorizontalAngle = 0.0f;

			float fSpeed = 0.001f;

			if (MouseX < CenterX)
			{
				DeltaMouse = static_cast<float>(CenterX - MouseX);
				arcball = glm::rotate(arcball, a_fSensitivity * DeltaMouse, vector3(0.0f, 1.0f, 0.0f));
			}
			else if (MouseX > CenterX)
			{
				DeltaMouse = static_cast<float>(MouseX - CenterX);
				arcball = glm::rotate(arcball, -a_fSensitivity * DeltaMouse, vector3(0.0f, 1.0f, 0.0f));
			}

			if (MouseY < CenterY)
			{
				DeltaMouse = static_cast<float>(CenterY - MouseY);
				arcball = glm::rotate(arcball, a_fSensitivity * DeltaMouse, vector3(1.0f, 0.0f, 0.0f));
			}
			else if (MouseY > CenterY)
			{
				DeltaMouse = static_cast<float>(MouseY - CenterY);
				arcball = glm::rotate(arcball, -a_fSensitivity * DeltaMouse, vector3(1.0f, 0.0f, 0.0f));
			}

			return arcball;
		}

		/*
		CameraRotation
		Manages the rotation of the camera
		a_fSpeed is a factor of change
		DO NOT OVERRIDE
		*/
		virtual void CameraRotation(float a_fSpeed = 0.005f) final
		{
			UINT	MouseX, MouseY;		// Coordinates for the mouse
			UINT	CenterX, CenterY;	// Coordinates for the center of the screen.

			CenterX = m_pSystem->WindowX + m_pSystem->WindowWidth / 2;
			CenterY = m_pSystem->WindowY + m_pSystem->WindowHeight / 2;

			float DeltaMouse;
			POINT pt;

			GetCursorPos(&pt);

			MouseX = pt.x;
			MouseY = pt.y;

			SetCursorPos(CenterX, CenterY);

			float fAngleX = 0.0f;
			float fAngleY = 0.0f;

			if (MouseX < CenterX)
			{
				DeltaMouse = static_cast<float>(CenterX - MouseX);
				fAngleY += DeltaMouse * a_fSpeed;
			}
			else if (MouseX > CenterX)
			{
				DeltaMouse = static_cast<float>(MouseX - CenterX);
				fAngleY -= DeltaMouse * a_fSpeed;
			}

			if (MouseY < CenterY)
			{
				DeltaMouse = static_cast<float>(CenterY - MouseY);
				fAngleX -= DeltaMouse * a_fSpeed;
			}
			else if (MouseY > CenterY)
			{
				DeltaMouse = static_cast<float>(MouseY - CenterY);
				fAngleX += DeltaMouse * a_fSpeed;
			}
			m_pCamera->ChangeHeading(fAngleY * 3.0f);//fAngleY and fAngleX are no longer static, the value is saved inside of the camera object
			m_pCamera->ChangePitch(-fAngleX * 3.0f);
		}

		/*
		InitApplication
		Initialize ReEng variables necesary to create the window
		*/
		virtual void InitApplication(String a_sWindowName = "ReEng")
		{
			//These are the default values for the windows construction but they will
			//not have any effect if the .cfg file is present next to the binary folder
			//(the .cfg will be created if not existant using these values, but once
			//created its values will have priority over these)

			// Indicate window properties
			m_pSystem->WindowName = a_sWindowName;
			m_pSystem->WindowWidth = 1080;
			m_pSystem->WindowHeight = 720;
			m_pSystem->WindowFullscreen = false;
			m_pSystem->WindowBorderless = false;

			// Set the clear color based on Microsoft's CornflowerBlue (default in XNA)
			//if this line is in Init Application it will depend on the .cfg file, if it
			//is on the InitUserVariables it will always force it regardless of the .cfg
			m_v4ClearColor = vector4(0.4f, 0.6f, 0.9f, 0.0f);
		}

		/*
		ReadConfig
		Reads the configuration of the application to a file
		*/
		virtual void ReadConfig(void)
		{
			//If we are reading the changes the changes, know what file to open
			String sRoot = m_pSystem->m_pFolder->GetFolderRoot();
			String App = m_pSystem->ApplicationName;
			App = sRoot + App + ".cfg";

			FileReaderClass reader;
			//If the file doesnt exist, create it and exit this method
			if (reader.ReadFile(App.c_str()) == REERRORS::ERROR_FILE_MISSING)
			{
				WriteConfig();
				return;
			}

			//read the file
			reader.Rewind();
			while (reader.ReadNextLine() == RUNNING)
			{
				String sWord = reader.GetFirstWord();

				int nLenght = reader.m_sLine.length();
				char* zsTemp = new char[nLenght];

				if (sWord == "Fullscreen:")
				{
					int nValue;
					sscanf_s(reader.m_sLine.c_str(), "Fullscreen: %d", &nValue);
					if (nValue == 0)
						m_pSystem->SetWindowFullscreen(false);
					else
						m_pSystem->SetWindowFullscreen(true);
				}
				else if (sWord == "Borderless:")
				{
					int nValue;
					sscanf_s(reader.m_sLine.c_str(), "Borderless: %d", &nValue);
					if (nValue == 0)
						m_pSystem->SetWindowBorderless(false);
					else
						m_pSystem->SetWindowBorderless(true);
				}
				else if (sWord == "Resolution:")
				{
					int nValue1;
					int nValue2;
					sscanf_s(reader.m_sLine.c_str(), "Resolution: [ %d x %d ]", &nValue1, &nValue2);
					m_pSystem->WindowWidth = nValue1;
					m_pSystem->WindowHeight = nValue2;
				}
				else if (sWord == "Ambient:")
				{
					float fValueX;
					float fValueY;
					float fValueZ;
					sscanf_s(reader.m_sLine.c_str(), "Ambient: [%f,%f,%f]", &fValueX, &fValueY, &fValueZ);
					m_pLightMngr->SetColor(vector3(fValueX, fValueY, fValueZ), 0);
				}
				else if (sWord == "Background:")
				{
					float fValueX;
					float fValueY;
					float fValueZ;
					float fValueW;
					sscanf_s(reader.m_sLine.c_str(), "Background: [%f,%f,%f,%f]", &fValueX, &fValueY, &fValueZ, &fValueW);
					m_v4ClearColor = vector4(fValueX, fValueY, fValueZ, fValueW);
				}
				else if (sWord == "AmbientPower:")
				{
					float fValue;
					sscanf_s(reader.m_sLine.c_str(), "AmbientPower: %f", &fValue);
					m_pLightMngr->SetIntensity(fValue, 0);
				}
				else if (sWord == "Data:")
				{
					sscanf_s(reader.m_sLine.c_str(), "Data: %s", zsTemp, nLenght);
					m_pSystem->m_pFolder->SetFolderData(zsTemp);
				}
				else if (sWord == "3DS:")
				{
					sscanf_s(reader.m_sLine.c_str(), "3DS: %s", zsTemp, nLenght);
					m_pSystem->m_pFolder->SetFolderM3DS(zsTemp);
				}
				else if (sWord == "BTO:")
				{
					sscanf_s(reader.m_sLine.c_str(), "BTO: %s", zsTemp, nLenght);
					m_pSystem->m_pFolder->SetFolderMBTO(zsTemp);
				}
				else if (sWord == "FBX:")
				{
					sscanf_s(reader.m_sLine.c_str(), "FBX: %s", zsTemp, nLenght);
					m_pSystem->m_pFolder->SetFolderMFBX(zsTemp);
				}
				else if (sWord == "OBJ:")
				{
					sscanf_s(reader.m_sLine.c_str(), "OBJ: %s", zsTemp, nLenght);
					m_pSystem->m_pFolder->SetFolderMOBJ(zsTemp);
				}
				else if (sWord == "POM:")
				{
					sscanf_s(reader.m_sLine.c_str(), "POM: %s", zsTemp, nLenght);
					m_pSystem->m_pFolder->SetFolderMPOM(zsTemp);
				}
				else if (sWord == "Level:")
				{
					sscanf_s(reader.m_sLine.c_str(), "Level: %s", zsTemp, nLenght);
					m_pSystem->m_pFolder->SetFolderLVL(zsTemp);
				}
				else if (sWord == "Textures:")
				{
					sscanf_s(reader.m_sLine.c_str(), "Textures: %s", zsTemp, nLenght);
					m_pSystem->m_pFolder->SetFolderTextures(zsTemp);
				}

				delete[] zsTemp;
				zsTemp = nullptr;
			}
			reader.CloseFile();
		}

		/*
		WriteConfig
		Writes the configuration of the application to a file
		*/
		virtual void WriteConfig(void)
		{
			// Write the configuration for this application
			String sRoot = m_pSystem->m_pFolder->GetFolderRoot();
			String App = m_pSystem->ApplicationName;
			App = sRoot + App + ".cfg";

			FILE *pFile;
			fopen_s(&pFile, App.c_str(), "w");
			if (!pFile)	//If we couldn't create the file we exit without changes.
				return;

			rewind(pFile);
			fprintf(pFile, "# Configuration file for the program: %s", m_pSystem->GetAppName().c_str());

			fprintf(pFile, "\n\nFullscreen: ");
			if (m_pSystem->IsWindowFullscreen())
				fprintf(pFile, "1");
			else
				fprintf(pFile, "0");

			fprintf(pFile, "\nBorderless: ");
			if (m_pSystem->IsWindowBorderless())
				fprintf(pFile, "1");
			else
				fprintf(pFile, "0");

			fprintf(pFile, "\nContext: OPENGL3X"); //Only openGL3X context is supported ATM

			fprintf(pFile, "\n\n# Resolution: [ 640 x 480 ]");
			fprintf(pFile, "\n# Resolution: [ 1280 x 720 ]");
			fprintf(pFile, "\n# Resolution: [ 1680 x 1050 ]");
			fprintf(pFile, "\n# Resolution: [ 1920 x 1080 ]");
			fprintf(pFile, "\n# Resolution: [ 2650 x 1440 ]");
			fprintf(pFile, "\nResolution: [ %d x %d ]", m_pSystem->WindowWidth, m_pSystem->WindowHeight);

			fprintf(pFile, "\n\nAmbient: [%.2f,%.2f,%.2f]",
				m_pLightMngr->GetColor(0).r, m_pLightMngr->GetColor(0).g, m_pLightMngr->GetColor(0).b);
			fprintf(pFile, "\nAmbientPower: %.2f", m_pLightMngr->GetIntensity(0));

			fprintf(pFile, "\n\nBackground: [%.3f,%.3f,%.3f,%.3f]",
				m_v4ClearColor.r, m_v4ClearColor.g, m_v4ClearColor.b, m_v4ClearColor.a);

			fprintf(pFile, "\n\n# Folders:");

			fprintf(pFile, "\nData:		%s", m_pSystem->m_pFolder->GetFolderData().c_str());
			fprintf(pFile, "\n3DS:		%s", m_pSystem->m_pFolder->GetFolderM3DS().c_str());
			fprintf(pFile, "\nBTO:		%s", m_pSystem->m_pFolder->GetFolderMBTO().c_str());
			fprintf(pFile, "\nFBX:		%s", m_pSystem->m_pFolder->GetFolderMFBX().c_str());
			fprintf(pFile, "\nOBJ:		%s", m_pSystem->m_pFolder->GetFolderMOBJ().c_str());
			fprintf(pFile, "\nLevel:		%s", m_pSystem->m_pFolder->GetFolderLVL().c_str());
			fprintf(pFile, "\nTextures:	%s", m_pSystem->m_pFolder->GetFolderTextures().c_str());

			fclose(pFile);
		}

		/*
		InitApplicationVariables
		It initializes this application member variables, usually those that
		are not the focus of the lesson.
		*/
		virtual void InitApplicationVariables(void)
		{
			//Ambient light (Ambient Light is always the first light, or light[0])
			m_pLightMngr->SetPosition(glm::vec3(0, 0, 0), 0);
			m_pLightMngr->SetIntensity(0.75f, 0);
			m_pLightMngr->SetColor(glm::vec3(1, 1, 1), 0);

			//Point Light (light[1])
			m_pLightMngr->SetPosition(glm::vec3(0, 0, 10), 1);
			m_pLightMngr->SetIntensity(5.00f, 1);
			m_pLightMngr->SetColor(glm::vec3(1, 1, 1), 1);

			// Create a new grid initializing its properties and compiling it
			m_pGrid = new GridClass();

			m_pMeshMngr = MeshManagerSingleton::GetInstance();

			m_pCamera->SetPosition(vector3(0.0f, 0.5f, 10.0f));
		}

		/*
		InitUserVariables
		Initializes user specific variables, this is executed right after InitApplicationVariables,
		the purpose of this member function is to initialize member variables specific for this lesson
		*/
		virtual void InitUserVariables(void){}

		/*
		Release
		Releases the application
		IF INHERITED AND OVERRIDEN MAKE SURE TO RELEASE BASE POINTERS (OR CALL BASED CLASS RELEASE)
		*/
		virtual void Release(void)
		{
			SafeDelete(m_pGrid);
			SafeDelete(m_pWindow);

			// Release all the singletons used in the dll
			ReleaseAllSingletons();
		}

		/*
		Update
		Updates the scene
		*/
		virtual void Update(void)
		{
			//Update the system so it knows how much time has passed since the last call
			m_pSystem->UpdateTime();
			//Update the information on the Mesh manager I will not check for collision detection so the argument is false
			m_pMeshMngr->Update(false);
			//Add the sphere to the render queue
			m_pMeshMngr->AddTorusToQueue(glm::rotate(REIDENTITY, 90.0f, vector3(90.0f, 0.0f, 0.0f)) * m_m4ArcBall, RERED, SOLID | WIRE);

			//Is the arcball active?
			if (m_bArcBall == true)
				m_m4ArcBall = ArcBall();

			//Is the first person camera active?
			if (m_bFPC == true)
				CameraRotation();

			m_pCamera->CalculateView();

			//print info into the console
			printf("FPS: %d            \r", m_pSystem->GetFPS());//print the Frames per Second
		}

		/*
		Display
		Displays the scene
		*/
		virtual void Display(void)
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the window

			m_pGrid->Render(); //renders the grid

			m_pMeshMngr->Render(); //Renders everything set up in the render queue

			m_pGLSystem->GLSwapBuffers(); //Swaps the OpenGL buffers
		}

		/*
		Idle
		Runs faster than the update
		*/
		virtual void Idle(void) {}

		/*
		ProcessKeyboard
		Manage the response of key presses
		*/
		virtual void ProcessKeyboard(void){}

		/*
		ProcessMouse
		Manage the response of key presses and mouse position
		*/
		virtual void ProcessMouse(void){}

		/*
		ProcessJoystick
		Manage the response of gamepad controllers
		*/
		virtual void ProcessJoystick(void){}
	};

}
#endif //__REENGAPPCLASS_H_