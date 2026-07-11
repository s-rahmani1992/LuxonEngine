#include "SceneEditorWindow.h"
#include <QResizeEvent>
#include <QSize>
#include <StringUtilities.h>

LuxonEditor::GUI::QT::SceneEditorWindow::SceneEditorWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.contextWidget->setAttribute(Qt::WA_NativeWindow);
}

LuxonEditor::GUI::QT::SceneEditorWindow::~SceneEditorWindow()
{
	
}

void LuxonEditor::GUI::QT::SceneEditorWindow::resizeEvent(QResizeEvent * event)
{
	auto& size = event->size();

	if (m_context == nullptr) {
		HWND h = (HWND)ui.contextWidget->winId();
		LuxonEngine::Platform::WindowProperties props{
			.width = (UInt16)size.width(),
			.height = (UInt16)size.height(),
		};

		m_window = std::make_shared<LuxonEngine::Platform::GraphicWindow>(props, h);
		m_context = GetGPUApplication()->CreateHybridContextForWindows(m_window);

		auto camtransform = std::make_shared<Transform>(Vector3(-5.2f, 1.9f, -1.1f), Vector3(1.0f), Vector3(-0.17f, -0.95f, 0.17f), 84);
		ref<PerspectiveCamera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)props.width / props.height, 45);

		std::wstring root = CharToString((GetProjectPath() + "/Data/InternalShaders/").c_str());
		std::string errorStr;
		auto shaderRegistery = GetGPUApplication()->CreateShaderRegistery();
		std::wstring simpleLightRasterPath = root + L"simple_color.hlsl";
		auto lightRasterProgram = shaderRegistery->CompileProgram(simpleLightRasterPath, errorStr);

		if (lightRasterProgram == nullptr) {
			errorStr = "Error in Compiling Shader At: \n" + WStringToString(simpleLightRasterPath) + "Error: \n" + errorStr;
			return;
		}

		auto materialFactory = GetGPUApplication()->CreateMaterialFactory();

		auto sphereMaterial = materialFactory->CreateMaterial(lightRasterProgram);
		sphereMaterial->SetValue("color", Color(1.0f, 0.0f, 0.0f, 1.0f));
		sphereMaterial->SetValue("ambient", 0.1f);
		sphereMaterial->SetValue("diffuse", 0.5f);
		sphereMaterial->SetValue("specular", 1.1f);

		ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

		auto sphereTransform = std::make_shared<Transform>(Vector3(5.2f, 0.8f, -3.0f), Vector3(2.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
		auto sphereGBufferRenderer = std::make_shared<MeshRenderer>(sphereMesh, sphereMaterial);
		auto sphereEntity = std::make_shared<LuxonEngine::GameEntity>(sphereTransform, sphereGBufferRenderer, nullptr);

		SceneLightData lightData;

		lightData.directionalLights.push_back(DirectionalLight{
			.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			.direction = Vector3(2.0f, -6.0f, 2.0f),
			.intensity = 0.5f,
			});

		lightData.pointLights.push_back(PointLight{
			.color = Color(1.0f, 1.0f, 1.0f, 1.0f),
			.position = Vector3(0.2f, 4.4f, 0.0f),
			.intensity = 4.0f,
			.attenuation = Attenuation{
				.c0 = 0.0f,
				.c1 = 0.5f,
				.c2 = 0.5f,
			},
			.radius = 9.0f,
			});

		m_scene = std::make_shared<Scene>();
		m_scene->mainCamera = mainCamera;
		m_scene->lightData = lightData;
		m_scene->entities = { sphereEntity };
		m_scene->behaviours = { };
		m_scene->rtGlobalMaterial = nullptr;
		m_context->PrepareScene(m_scene);
	}

	std::dynamic_pointer_cast<PerspectiveCamera>(m_scene->mainCamera)->ChangeAspect((float)size.width() / size.height());
	m_context->Render();
}

