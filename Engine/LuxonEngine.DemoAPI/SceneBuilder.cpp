#include "SceneBuilder.h"
#include "StringUtilities.h"

#include <Platform/Application.h>
#include <Platform/GraphicWindow.h>

#include "DX12GPUDeviceManager.h"
#include <Rendering/ShaderRegistery.h>
#include <Rendering/GPUAssetManager.h>
#include <Rendering/GraphicContext.h>
#include <Rendering/MaterialFactory.h>

#include <Rendering/MeshRenderer.h>
#include <Rendering/GBufferRTReflectionRenderer.h>
#include <Rendering/SplineRenderer.h>
#include <Rendering/RayTracingComponent.h>

#include <Core/Light/Lights.h>
#include <Core/WICTexture2DImporter.h>
#include <Core/AssimpModel3DImporter.h>
#include <Core/Camera/PerspectiveCamera.h>
#include <Core/Model3DAsset.h>
#include <Core/GameEntity.h>
#include <Core/ShapeBuilder.h>
#include <Core/Mesh.h>
#include <Core/Transform.h>
#include <Core/Scene.h>

#include "Behaviours/CameraController.h"
#include "Behaviours/FrameRateLogger.h"
#include "Behaviours/EntityMover.h"
#include "Behaviours/EntityRotator.h"
#include "Behaviours/EntityPositionController.h"
#include "Behaviours/CurveModifier.h"
#include "Behaviours/TextureSwitcher.h"
#include "Behaviours/MaterialValueModifier.h"

using namespace QuantumEngine;

#define IMPORT_RETRO_CAR_MESH(MESH_VAR, ERROR_VAR)   \
    auto retroCarModelPath = root + L"\\Assets\\Models\\RetroCar.fbx";  \
    auto retroCarModel = AssimpModel3DImporter::Import(WCharToString(retroCarModelPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.05f) }, ERROR_VAR);    \
    if (retroCarModel == nullptr) { \
        error = "Error in Importing Model At: \n" + WStringToString(retroCarModelPath) + "Error: \n" + ERROR_VAR;   \
        return nullptr; \
    }   \
    auto MESH_VAR = retroCarModel->GetMesh("Cube.002");

#define IMPORT_PICKUP_TRUCK_MESH(MESH_VAR, ERROR_VAR)   \
    auto pickupTruckModelPath = root + L"\\Assets\\Models\\PickupTruck.fbx";  \
    auto pickupTruckModel = AssimpModel3DImporter::Import(WCharToString(pickupTruckModelPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(0.01f) }, ERROR_VAR);    \
    if (pickupTruckModel == nullptr) { \
        error = "Error in Importing Model At: \n" + WStringToString(pickupTruckModelPath) + "Error: \n" + ERROR_VAR;   \
        return nullptr; \
    }   \
    auto MESH_VAR = pickupTruckModel->GetMesh("Mesh.006");

#define IMPORT_PEDESTAL_MESH(MESH_VAR, ERROR_VAR)   \
    auto pedestalModelPath = root + L"\\Assets\\Models\\tech_pedestal.fbx";  \
    auto pedestalModel = AssimpModel3DImporter::Import(WCharToString(pedestalModelPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.1f) }, ERROR_VAR);    \
    if (pedestalModel == nullptr) { \
        error = "Error in Importing Model At: \n" + WStringToString(pedestalModelPath) + "Error: \n" + ERROR_VAR;   \
        return nullptr; \
    }   \
    auto MESH_VAR = pedestalModel->GetMesh("Circle.001");

#define IMPORT_CONTAINER_MESH(MESH_VAR, ERROR_VAR)   \
    auto containerModelPath = root + L"\\Assets\\Models\\Scifi_Container.fbx";  \
    auto containerModel = AssimpModel3DImporter::Import(WCharToString(containerModelPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(1.0f) }, ERROR_VAR);    \
    if (containerModel == nullptr) { \
        error = "Error in Importing Model At: \n" + WStringToString(containerModelPath) + "Error: \n" + ERROR_VAR;   \
        return nullptr; \
    }   \
    auto MESH_VAR = containerModel->GetMesh("Container Free");

#define IMPORT_LION_STATUE_MESH(MESH_VAR, ERROR_VAR)   \
    auto lionStatueModelPath = root + L"\\Assets\\Models\\lion-lp.fbx";  \
    auto lionStatueModel = AssimpModel3DImporter::Import(WCharToString(lionStatueModelPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(1.0f) }, ERROR_VAR);    \
    if (lionStatueModel == nullptr) { \
        error = "Error in Importing Model At: \n" + WStringToString(lionStatueModelPath) + "Error: \n" + ERROR_VAR;   \
        return nullptr; \
    }   \
    auto MESH_VAR = lionStatueModel->GetMesh("Model.004");

#define IMPORT_DRONE_MESH(MESH_VAR, ERROR_VAR)   \
    auto droneModelPath = root + L"\\Assets\\Models\\304_Drone.fbx";  \
    auto droneModel = AssimpModel3DImporter::Import(WCharToString(droneModelPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(0.2f) }, ERROR_VAR);    \
    if (droneModel == nullptr) { \
        error = "Error in Importing Model At: \n" + WStringToString(droneModelPath) + "Error: \n" + ERROR_VAR;   \
        return nullptr; \
    }   \
    auto MESH_VAR = droneModel->GetMesh("Sheet_196");

ref<Scene> SceneBuilder::BuildSimpleLightScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& error)
{
	std::string errorStr;

	std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring simpleLightRasterPath = root + L"\\Assets\\Shaders\\simple_light_raster_program.hlsl";
    auto lightRasterProgram = shaderRegistery->CompileProgram(simpleLightRasterPath, errorStr);

    if(lightRasterProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(simpleLightRasterPath) + "Error: \n" + errorStr;
        return nullptr;
	}

	std::wstring curveRasterPath = root + L"\\Assets\\Shaders\\curve_raster_program.hlsl";
    auto curveRasterProgram = shaderRegistery->CompileProgram(curveRasterPath, errorStr);

    if (curveRasterProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(curveRasterPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
	auto globalRTProgram = shaderRegistery->CompileProgram(rtGlobalShaderPath, errorStr);

    if (globalRTProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
	auto simpleRTLightProgram = shaderRegistery->CompileProgram(rtSimpleLightShaderPath, errorStr);

    if(simpleRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleLightShaderPath) + "Error: \n" + errorStr;
        return nullptr;
	}

    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(-5.2f, 1.9f, -1.1f), Vector3(1.0f), Vector3(-0.17f, -0.95f, 0.17f), 84);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

	////// Importing Textures

    auto retroCarTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    auto pickupTruckGreenTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\truck_color-green.jpg", errorStr);
    auto pickupTruckRedTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\truck_color-red.jpg", errorStr);
    auto pickupTruckSilverTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\truck_color-silver.jpg", errorStr);
    auto pedestalTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\tech_pedestal_COL.png", errorStr);
    auto containerTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Sci-fi_Container_BaseColor.png", errorStr);
    auto groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", errorStr);


    auto rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Rabbit_basecolor.jpg", errorStr);
    auto lionStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", errorStr);
    
    assetManager->UploadTextureToGPU(retroCarTex);
    assetManager->UploadTextureToGPU(pickupTruckGreenTex);
    assetManager->UploadTextureToGPU(pickupTruckRedTex);
    assetManager->UploadTextureToGPU(pickupTruckSilverTex);
    assetManager->UploadTextureToGPU(pedestalTex);
    assetManager->UploadTextureToGPU(containerTex);
    assetManager->UploadTextureToGPU(groundBrickTex1);


	assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(lionStatueTex1);

	////// Importing meshes

    IMPORT_RETRO_CAR_MESH(retroCarMesh, errorStr)

    IMPORT_PICKUP_TRUCK_MESH(pickupTruckMesh, errorStr)

    IMPORT_PEDESTAL_MESH(pedestalMesh, errorStr)

    IMPORT_CONTAINER_MESH(containerMesh, errorStr)

    auto rabbitStatuePath = root + L"\\Assets\\Models\\Scifi_Container.fbx";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto lionStatuePath = root + L"\\Assets\\Models\\lion-lp.fbx";
    auto lionStatueModel1 = AssimpModel3DImporter::Import(WCharToString(lionStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(1.0f) }, errorStr);
    
    if (lionStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(lionStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }    
    
    auto lionStatueMesh1 = lionStatueModel1->GetMesh("Model.004");

    auto chairPath = root + L"\\Assets\\Models\\leather_chair.fbx";
    auto chairModel1 = AssimpModel3DImporter::Import(WCharToString(chairPath.c_str()), ModelImportProperties{.axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(1.0f)}, errorStr);

    if (chairModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(chairPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto chairMesh1 = chairModel1->GetMesh("leather_chair.001");

    std::vector<Vertex> planeVertices = {
        Vertex(Vector3(-1.0f, 0, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, -1.0f), Vector2(10.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, 1.0f), Vector2(10.0f, 10.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(-1.0f, 0, 1.0f), Vector2(0.0f, 10.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> planeIndices = {
        0, 1, 2, 0, 2, 3,
    };

    std::shared_ptr<Mesh> planeMesh = std::make_shared<Mesh>(planeVertices, planeIndices);

    ////// Creating the materials

	auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
	rtGlobalMaterial->SetValue("missColor", Color(0.2f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto retroCarMaterial = materialFactory->CreateMaterial(lightRasterProgram);
    retroCarMaterial->SetValue("ambient", 0.1f);
    retroCarMaterial->SetValue("diffuse", 0.5f);
    retroCarMaterial->SetValue("specular", 2.1f);
    retroCarMaterial->SetTexture2D("mainTexture", retroCarTex);

    auto pedestalMaterial = materialFactory->CreateMaterial(lightRasterProgram);
    pedestalMaterial->SetValue("ambient", 0.1f);
    pedestalMaterial->SetValue("diffuse", 0.5f);
    pedestalMaterial->SetValue("specular", 2.1f);
    pedestalMaterial->SetTexture2D("mainTexture", pedestalTex);

    auto pedestalRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    pedestalRTMaterial->SetTexture2D("mainTexture", pedestalTex);
    pedestalRTMaterial->SetValue("ambient", 0.1f);
    pedestalRTMaterial->SetValue("diffuse", 0.5f);
    pedestalRTMaterial->SetValue("specular", 2.1f);

	auto retroCarRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
	retroCarRTMaterial->SetTexture2D("mainTexture", retroCarTex);
	retroCarRTMaterial->SetValue("ambient", 0.1f);
	retroCarRTMaterial->SetValue("diffuse", 0.5f);
	retroCarRTMaterial->SetValue("specular", 2.1f);

    auto pickupTruckMaterial = materialFactory->CreateMaterial(lightRasterProgram);
    pickupTruckMaterial->SetValue("ambient", 0.1f);
    pickupTruckMaterial->SetValue("diffuse", 0.5f);
    pickupTruckMaterial->SetValue("specular", 2.1f);
    pickupTruckMaterial->SetTexture2D("mainTexture", pickupTruckGreenTex);

    auto pickupTruckRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    pickupTruckRTMaterial->SetTexture2D("mainTexture", pickupTruckGreenTex);
    pickupTruckRTMaterial->SetValue("ambient", 0.1f);
    pickupTruckRTMaterial->SetValue("diffuse", 0.5f);
    pickupTruckRTMaterial->SetValue("specular", 2.1f);

    auto rabbitStatueMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    rabbitStatueMaterial1->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueMaterial1->SetValue("ambient", 0.1f);
    rabbitStatueMaterial1->SetValue("diffuse", 0.8f);
    rabbitStatueMaterial1->SetValue("specular", 0.1f);

	auto rabbitStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
	rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
	rabbitStatueRTMaterial->SetValue("ambient", 0.1f);
	rabbitStatueRTMaterial->SetValue("diffuse", 0.8f);
	rabbitStatueRTMaterial->SetValue("specular", 0.1f);

    auto lionStatueMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    lionStatueMaterial1->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueMaterial1->SetValue("ambient", 0.1f);
    lionStatueMaterial1->SetValue("diffuse", 0.8f);
    lionStatueMaterial1->SetValue("specular", 0.1f);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueRTMaterial->SetValue("ambient", 0.1f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);

    auto containerMaterial = materialFactory->CreateMaterial(lightRasterProgram);
    containerMaterial->SetTexture2D("mainTexture", containerTex);
    containerMaterial->SetValue("ambient", 0.1f);
    containerMaterial->SetValue("diffuse", 0.8f);
    containerMaterial->SetValue("specular", 0.1f);

    auto containerRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    containerRTMaterial->SetTexture2D("mainTexture", containerTex);
    containerRTMaterial->SetValue("ambient", 0.1f);
    containerRTMaterial->SetValue("diffuse", 0.8f);
    containerRTMaterial->SetValue("specular", 0.3f);

    auto groundMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    groundMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundMaterial1->SetValue("ambient", 0.1f);
    groundMaterial1->SetValue("diffuse", 0.8f);
    groundMaterial1->SetValue("specular", 0.4f);

    auto groundRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    groundRTMaterial->SetTexture2D("mainTexture", groundBrickTex1);
    groundRTMaterial->SetValue("ambient", 0.1f);
    groundRTMaterial->SetValue("diffuse", 0.8f);
    groundRTMaterial->SetValue("specular", 0.4f);

	auto curveMaterial = materialFactory->CreateMaterial(curveRasterProgram);
    curveMaterial->SetValue("ambient", 0.1f);
    curveMaterial->SetValue("diffuse", 0.8f);
    curveMaterial->SetValue("specular", 0.4f);
	curveMaterial->SetValue("color", Color(0.5f, 0.2f, 0.6f, 1.0f));

	////// Creating the entities

    auto retroCarTransform = std::make_shared<Transform>(Vector3(-2.0f, 0.3f, -2.0f), Vector3(0.3f), Vector3(0.0f, 1.0f, 0.0f), -30);
    auto retroCarRenderer = std::make_shared<Render::MeshRenderer>(retroCarMesh, retroCarMaterial);
    auto retroCarRTComponent = std::make_shared<Render::RayTracingComponent>(retroCarMesh, retroCarRTMaterial);
    auto retroCarEntity = std::make_shared<QuantumEngine::GameEntity>(retroCarTransform, retroCarRenderer, retroCarRTComponent);

    auto retroCarTransform1 = std::make_shared<Transform>(Vector3(2.0f, 0.3f, 2.0f), Vector3(0.3f), Vector3(0.0f, 1.0f, 0.0f), 100);
    auto retroCarRenderer1 = std::make_shared<Render::MeshRenderer>(retroCarMesh, retroCarMaterial);
    auto retroCarRTComponent1 = std::make_shared<Render::RayTracingComponent>(retroCarMesh, retroCarRTMaterial);
    auto retroCarEntity1 = std::make_shared<QuantumEngine::GameEntity>(retroCarTransform1, retroCarRenderer1, retroCarRTComponent1);

    auto pedestalTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.5f, 1.0f, 2.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto pedestalRenderer = std::make_shared<Render::MeshRenderer>(pedestalMesh, pedestalMaterial);
    auto pedestalRTComponent = std::make_shared<Render::RayTracingComponent>(pedestalMesh, pedestalRTMaterial);
    auto pedestalEntity = std::make_shared<QuantumEngine::GameEntity>(pedestalTransform, pedestalRenderer, pedestalRTComponent);

    auto pickupTruckTransform = std::make_shared<Transform>(Vector3(0.0f, 0.14f, 0.0f), Vector3(0.23f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto pickupTruckRenderer = std::make_shared<Render::MeshRenderer>(pickupTruckMesh, pickupTruckMaterial);
    auto pickupTruckRTComponent = std::make_shared<Render::RayTracingComponent>(pickupTruckMesh, pickupTruckRTMaterial);
    auto pickupTruckEntity = std::make_shared<QuantumEngine::GameEntity>(pickupTruckTransform, pickupTruckRenderer, pickupTruckRTComponent);

    auto pickupTruckRotator = std::make_shared<EntityRotator>(pickupTruckTransform, 15);
    auto pickupTruckTextureSwitcher = std::make_shared<TextureSwitcher>(pickupTruckMaterial, "mainTexture", std::vector<ref<Texture2D>>({pickupTruckGreenTex, pickupTruckRedTex, pickupTruckSilverTex}));
    auto pickupTruckTextureRTSwitcher = std::make_shared<TextureSwitcher>(pickupTruckRTMaterial, "mainTexture", std::vector<ref<Texture2D>>({ pickupTruckGreenTex, pickupTruckRedTex, pickupTruckSilverTex }));

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(2.0f, 0.0f, -2.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), -90);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, rabbitStatueMaterial1);
	auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

    auto lionStatueTransform1 = std::make_shared<Transform>(Vector3(0.0f, 1.5f, 2.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer3 = std::make_shared<Render::MeshRenderer>(lionStatueMesh1, lionStatueMaterial1);
    auto rtComponent3 = std::make_shared<Render::RayTracingComponent>(lionStatueMesh1, lionStatueRTMaterial);
    auto lionStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform1, meshRenderer3, rtComponent3);

    auto groundTransform1 = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer4 = std::make_shared<Render::MeshRenderer>(planeMesh, groundMaterial1);
    auto rtComponent4 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundRTMaterial);
    auto grountEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform1, meshRenderer4, rtComponent4);

    auto containerTransform = std::make_shared<Transform>(Vector3(-1.0f, 0.0f, 2.0f), Vector3(0.5f), Vector3(0.0f, 1.0f, 0.0f), 30);
    auto containerMeshRenderer = std::make_shared<Render::MeshRenderer>(containerMesh, containerMaterial);
    auto containerRTComponent = std::make_shared<Render::RayTracingComponent>(containerMesh, containerRTMaterial);
    auto containerEntity1 = std::make_shared<QuantumEngine::GameEntity>(containerTransform, containerMeshRenderer, containerRTComponent);

    auto containerTransform2 = std::make_shared<Transform>(Vector3(1.5f, 0.25f, -1.5f), Vector3(0.3f), Vector3(0.0f, 0.0f, 1.0f), 90);
    auto containerMeshRenderer2 = std::make_shared<Render::MeshRenderer>(containerMesh, containerMaterial);
    auto containerRTComponent2 = std::make_shared<Render::RayTracingComponent>(containerMesh, containerRTMaterial);
    auto containerEntity2 = std::make_shared<QuantumEngine::GameEntity>(containerTransform2, containerMeshRenderer2, containerRTComponent2);

	auto curveTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
	auto curveRenderer = std::make_shared<Render::SplineRenderer>(curveMaterial, std::vector<Vector3>{ Vector3(-4.0f, 0.0f, -4.0f), Vector3(0.0f, 4.0f, 0.0f), Vector3(4.0f, 0.0f, 4.0f) }, 2.2f, 10);
	auto curveEntity = std::make_shared<QuantumEngine::GameEntity>(curveTransform, curveRenderer, nullptr);
    
    auto curveTransform1 = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), 0);
    auto curveRenderer1 = std::make_shared<Render::SplineRenderer>(curveMaterial, std::vector<Vector3>{ Vector3(-4.0f, 0.0f, 0.0f), Vector3(0.0f, 12.0f, 0.0f), Vector3(4.0f, 0.0f, 0.0f) }, 0.8f, 20);
    auto curveEntity1 = std::make_shared<QuantumEngine::GameEntity>(curveTransform1, curveRenderer1, nullptr);

    ////// Creating the lights

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
    
    auto frameLogger = std::make_shared<FrameRateLogger>();
	auto curveModifier = std::make_shared<CurveModifier>(curveRenderer, 2.0f);
    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    //scene->entities = { retroCarEntity, retroCarEntity1, rabbitStatueEntity1, lionStatueEntity1, grountEntity1, chairEntity1, chairEntity2, curveEntity, curveEntity1};
    scene->entities = { retroCarEntity, retroCarEntity1, pedestalEntity, pickupTruckEntity, containerEntity1, containerEntity2, grountEntity1 };

    scene->behaviours = { cameraController, pickupTruckTextureSwitcher, pickupTruckTextureRTSwitcher, pickupTruckRotator };
	scene->rtGlobalMaterial = rtGlobalMaterial;
    
    return scene;
}

ref<Scene> SceneBuilder::BuildReflectionScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& error)
{
    std::string errorStr;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring simpleLightRasterPath = root + L"\\Assets\\Shaders\\simple_light_raster_program.hlsl";
    auto lightRasterProgram = shaderRegistery->CompileProgram(simpleLightRasterPath, errorStr);

    if (lightRasterProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(simpleLightRasterPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    auto globalRTProgram = shaderRegistery->CompileProgram(rtGlobalShaderPath, errorStr);

    if (globalRTProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    auto simpleRTLightProgram = shaderRegistery->CompileProgram(rtSimpleLightShaderPath, errorStr);

    if (simpleRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleLightShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring rtReflectionShaderPath = root + L"\\Assets\\Shaders\\reflection_rt.lib.hlsl";
    auto reflectionRTLightProgram = shaderRegistery->CompileProgram(rtReflectionShaderPath, errorStr);

    if (reflectionRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtReflectionShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring rtStandardReflectionShaderPath = root + L"\\Assets\\Shaders\\reflection_standard_rt.hlsl";
    auto reflectionStandardRTLightProgram = shaderRegistery->CompileProgram(rtStandardReflectionShaderPath, errorStr);

    if (reflectionStandardRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtStandardReflectionShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring gBufferReflectionVertPath = root + L"\\Assets\\Shaders\\reflection_g_buffer.hlsl";
    auto gBufferReflectionProgram = shaderRegistery->CompileProgram(gBufferReflectionVertPath, errorStr);

    if (gBufferReflectionProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(gBufferReflectionVertPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring gBufferStandardReflectionPath = root + L"\\Assets\\Shaders\\reflection_standard_g_buffer.hlsl";
    auto gBufferStandardReflectionProgram = shaderRegistery->CompileProgram(gBufferStandardReflectionPath, errorStr);

    if (gBufferStandardReflectionProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(gBufferStandardReflectionPath) + "Error: \n" + errorStr;
        return nullptr;
    }
    
    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(-8.0f, 6.4f, -4.1f), Vector3(1.0f), Vector3(-0.27f, -0.69f, 0.08f), 60);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    auto retroCarTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    auto rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Rabbit_basecolor.jpg", errorStr);
    auto lionStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", errorStr);
    auto chairTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\leather_chair_BaseColor.png", errorStr);
    auto groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", errorStr);
    auto swampTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\swampTex.jpg", errorStr);
    auto wallColorTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\CorrugatedGlassFrame_basecolor.png", errorStr);
    auto wallReflectionMaskTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\CorrugatedGlassFrame_metallic.png", errorStr);
    assetManager->UploadTextureToGPU(retroCarTex);
    assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(lionStatueTex1);
    assetManager->UploadTextureToGPU(groundBrickTex1);
    assetManager->UploadTextureToGPU(chairTex1);
    assetManager->UploadTextureToGPU(swampTex1);
    assetManager->UploadTextureToGPU(wallColorTex);
    assetManager->UploadTextureToGPU(wallReflectionMaskTex);

    ////// Importing meshes

    IMPORT_RETRO_CAR_MESH(retroCarMesh, errorStr)

    IMPORT_LION_STATUE_MESH(lionStatueMesh, errorStr)

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.fbx";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto chairPath = root + L"\\Assets\\Models\\leather_chair.fbx";
    auto chairModel1 = AssimpModel3DImporter::Import(WCharToString(chairPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(1.0f) }, errorStr);

    if (chairModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(chairPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto chairMesh1 = chairModel1->GetMesh("leather_chair.001");

    std::vector<Vertex> planeVertices = {
        Vertex(Vector3(-1.0f, 0, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, -1.0f), Vector2(2.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, 1.0f), Vector2(2.0f, 2.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(-1.0f, 0, 1.0f), Vector2(0.0f, 2.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> planeIndices = {
        0, 1, 2, 0, 2, 3,
    };

    ref<Mesh> planeMesh = std::make_shared<Mesh>(planeVertices, planeIndices);

    ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

    ////// Creating the materials

    auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
    rtGlobalMaterial->SetValue("missColor", Color(0.2f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto retroCarMaterial = materialFactory->CreateMaterial(lightRasterProgram);
    retroCarMaterial->SetTexture2D("mainTexture", retroCarTex);
    retroCarMaterial->SetValue("ambient", 0.1f);
    retroCarMaterial->SetValue("diffuse", 0.5f);
    retroCarMaterial->SetValue("specular", 1.1f);

    auto retroCarRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    retroCarRTMaterial->SetTexture2D("mainTexture", retroCarTex);
    retroCarRTMaterial->SetValue("ambient", 0.1f);
    retroCarRTMaterial->SetValue("diffuse", 0.5f);
    retroCarRTMaterial->SetValue("specular", 1.1f);

    auto rabbitStatueMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    rabbitStatueMaterial1->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueMaterial1->SetValue("ambient", 0.1f);
    rabbitStatueMaterial1->SetValue("diffuse", 0.8f);
    rabbitStatueMaterial1->SetValue("specular", 0.1f);

    auto rabbitStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueRTMaterial->SetValue("ambient", 0.1f);
    rabbitStatueRTMaterial->SetValue("diffuse", 0.8f);
    rabbitStatueRTMaterial->SetValue("specular", 0.1f);

    auto lionStatueMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    lionStatueMaterial1->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueMaterial1->SetValue("ambient", 0.1f);
    lionStatueMaterial1->SetValue("diffuse", 0.8f);
    lionStatueMaterial1->SetValue("specular", 0.1f);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueRTMaterial->SetValue("ambient", 0.1f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);

    auto chairMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    chairMaterial1->SetTexture2D("mainTexture", chairTex1);
    chairMaterial1->SetValue("ambient", 0.1f);
    chairMaterial1->SetValue("diffuse", 0.8f);
    chairMaterial1->SetValue("specular", 0.1f);

    auto chairRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    chairRTMaterial->SetTexture2D("mainTexture", chairTex1);
    chairRTMaterial->SetValue("ambient", 0.1f);
    chairRTMaterial->SetValue("diffuse", 0.8f);
    chairRTMaterial->SetValue("specular", 0.3f);

    auto groundMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    groundMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundMaterial1->SetValue("ambient", 0.1f);
    groundMaterial1->SetValue("diffuse", 0.8f);
    groundMaterial1->SetValue("specular", 0.4f);

    auto groundRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    groundRTMaterial->SetTexture2D("mainTexture", groundBrickTex1);
    groundRTMaterial->SetValue("ambient", 0.1f);
    groundRTMaterial->SetValue("diffuse", 0.8f);
    groundRTMaterial->SetValue("specular", 0.4f);

    auto wallGBufferReflectionMaterial = materialFactory->CreateMaterial(gBufferStandardReflectionProgram);
    wallGBufferReflectionMaterial->SetTexture2D("mainTexture", wallColorTex);
    wallGBufferReflectionMaterial->SetTexture2D("reflectTexture", wallReflectionMaskTex);
    wallGBufferReflectionMaterial->SetValue("ambient", 0.1f);
    wallGBufferReflectionMaterial->SetValue("diffuse", 0.8f);
    wallGBufferReflectionMaterial->SetValue("specular", 0.8f);

    auto wallReflectionRTMaterial = materialFactory->CreateMaterial(reflectionStandardRTLightProgram);
    wallReflectionRTMaterial->SetTexture2D("mainTexture", wallColorTex);
    wallReflectionRTMaterial->SetTexture2D("reflectTexture", wallReflectionMaskTex);
    wallReflectionRTMaterial->SetValue("castShadow", 0);
    wallReflectionRTMaterial->SetValue("ambient", 0.1f);
    wallReflectionRTMaterial->SetValue("diffuse", 0.8f);
    wallReflectionRTMaterial->SetValue("specular", 0.8f);

    auto sphereReflectionRTMaterial1 = materialFactory->CreateMaterial(reflectionRTLightProgram);
    sphereReflectionRTMaterial1->SetTexture2D("mainTexture", swampTex1);
    sphereReflectionRTMaterial1->SetValue("castShadow", 0);
    sphereReflectionRTMaterial1->SetValue("reflectivity", 0.5f);
    sphereReflectionRTMaterial1->SetValue("ambient", 0.1f);
    sphereReflectionRTMaterial1->SetValue("diffuse", 0.8f);
    sphereReflectionRTMaterial1->SetValue("specular", 0.8f);

    auto sphereGBufferReflectionMaterial = materialFactory->CreateMaterial(gBufferReflectionProgram);
    sphereGBufferReflectionMaterial->SetTexture2D("mainTexture", swampTex1);
    sphereGBufferReflectionMaterial->SetValue("reflectivity", 0.5f);
    sphereGBufferReflectionMaterial->SetValue("ambient", 0.1f);
    sphereGBufferReflectionMaterial->SetValue("diffuse", 0.9f);
    sphereGBufferReflectionMaterial->SetValue("specular", 0.6f);

    ////// Creating the entities

    auto retroCarTransform = std::make_shared<Transform>(Vector3(-4.0f, 0.5f, 0.0f), Vector3(0.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto retroCarMeshRenderer = std::make_shared<Render::MeshRenderer>(retroCarMesh, retroCarMaterial);
    auto retroCarRTComponent = std::make_shared<Render::RayTracingComponent>(retroCarMesh, retroCarRTMaterial);
    auto retroCarEntity = std::make_shared<QuantumEngine::GameEntity>(retroCarTransform, retroCarMeshRenderer, retroCarRTComponent);

    ref<EntityRotator> carRotator = std::make_shared<EntityRotator>(retroCarTransform, 30);

    auto rabbitStatueTransform1 = std::make_shared<Transform>(Vector3(1.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 1.0f, 0.0f), -90);
    auto meshRenderer2 = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, rabbitStatueMaterial1);
    auto rtComponent2 = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform1, meshRenderer2, rtComponent2);

    auto lionStatueTransform = std::make_shared<Transform>(Vector3(3.0f, 1.5f, 0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto lionStatueMeshRenderer = std::make_shared<Render::MeshRenderer>(lionStatueMesh, lionStatueMaterial1);
    auto lionStatueRTComponent = std::make_shared<Render::RayTracingComponent>(lionStatueMesh, lionStatueRTMaterial);
    auto lionStatueEntity = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform, lionStatueMeshRenderer, lionStatueRTComponent);

    auto groundTransform1 = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer4 = std::make_shared<Render::MeshRenderer>(planeMesh, groundMaterial1);
    auto rtComponent4 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundRTMaterial);
    auto grountEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform1, meshRenderer4, rtComponent4);

    auto chairTransform1 = std::make_shared<Transform>(Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto meshRenderer5 = std::make_shared<Render::MeshRenderer>(chairMesh1, chairMaterial1);
    auto rtComponent5 = std::make_shared<Render::RayTracingComponent>(chairMesh1, chairRTMaterial);
    auto chairEntity1 = std::make_shared<QuantumEngine::GameEntity>(chairTransform1, meshRenderer5, rtComponent5);

    auto sphereTransform = std::make_shared<Transform>(Vector3(5.2f, 2.8f, -3.0f), Vector3(2.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto sphereGBufferRenderer = std::make_shared<Render::GBufferRTReflectionRenderer>(sphereMesh, sphereGBufferReflectionMaterial);
    auto sphereRTComponent = std::make_shared<Render::RayTracingComponent>(sphereMesh, sphereReflectionRTMaterial1);
    auto sphereEntity = std::make_shared<QuantumEngine::GameEntity>(sphereTransform, sphereGBufferRenderer, sphereRTComponent);

    ref<MaterialValueModifier> sphereReflectionModifier = std::make_shared<MaterialValueModifier>(sphereReflectionRTMaterial1, "reflectivity", 1.0f, 0.0f, 1.0f);
    ref<MaterialValueModifier> sphereGBufferReflectionModifier = std::make_shared<MaterialValueModifier>(sphereGBufferReflectionMaterial, "reflectivity", 1.0f, 0.0f, 1.0f);

    auto wallTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 3.0f), Vector3(15.0f), Vector3(1.0f, 0.0f, 0.0f), 90);
    auto wallGBufferRenderer = std::make_shared<Render::GBufferRTReflectionRenderer>(planeMesh, wallGBufferReflectionMaterial);
    auto wallRTComponent = std::make_shared<Render::RayTracingComponent>(planeMesh, wallReflectionRTMaterial);
    auto wallEntity = std::make_shared<QuantumEngine::GameEntity>(wallTransform, wallGBufferRenderer, wallRTComponent);

    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
        .intensity = 0.5f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(1.0f, 4.4f, 1.0f),
        .intensity = 3.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 1.0f,
            .c2 = 0.0f,
        },
        .radius = 9.0f,
        });

    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = {retroCarEntity, rabbitStatueEntity1, lionStatueEntity, chairEntity1, grountEntity1, sphereEntity, wallEntity };
    scene->behaviours = { cameraController, carRotator, sphereReflectionModifier, sphereGBufferReflectionModifier };
    scene->rtGlobalMaterial = rtGlobalMaterial;
    return scene;
}

ref<Scene> SceneBuilder::BuildShadowScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    std::string error;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring simpleLightRasterPath = root + L"\\Assets\\Shaders\\simple_light_raster_program.hlsl";
    auto lightRasterProgram = shaderRegistery->CompileProgram(simpleLightRasterPath, error);

    if (lightRasterProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(simpleLightRasterPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    auto globalRTProgram = shaderRegistery->CompileProgram(rtGlobalShaderPath, error);

    if (globalRTProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + error;
        return nullptr;
    }

    std::wstring rtShadowShaderPath = root + L"\\Assets\\Shaders\\shadow_rt.lib.hlsl";
    auto rtShadowProgram = shaderRegistery->CompileProgram(rtShadowShaderPath, error);

    if (rtShadowProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtShadowShaderPath) + "Error: \n" + error;
        return nullptr;
    }
    
    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(4.85f, 5.4f, 7.7f), Vector3(1.0f), Vector3(0.0f, 0.21f, -0.05f), 151);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    auto retroCarTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", error);
    auto pickupTruckTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\truck_color-green.jpg", error);
    auto lionStatueTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", error);
    auto droneTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\304_Drone_M_304_Drone_BaseColor.png", error);
    auto groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", error);
    assetManager->UploadTextureToGPU(retroCarTex);
    assetManager->UploadTextureToGPU(pickupTruckTex);
    assetManager->UploadTextureToGPU(lionStatueTex);
    assetManager->UploadTextureToGPU(groundBrickTex1);
    assetManager->UploadTextureToGPU(droneTex);

    ////// Importing meshes

    IMPORT_RETRO_CAR_MESH(retroCarMesh, error)

    IMPORT_LION_STATUE_MESH(lionStatueMesh, error)

    IMPORT_PICKUP_TRUCK_MESH(pickupTruckMesh, error)

    IMPORT_DRONE_MESH(droneMesh, error)

    std::vector<Vertex> planeVertices = {
        Vertex(Vector3(-1.0f, 0, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, -1.0f), Vector2(5.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, 1.0f), Vector2(5.0f, 5.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(-1.0f, 0, 1.0f), Vector2(0.0f, 5.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> planeIndices = {
        0, 1, 2, 0, 2, 3,
    };

    std::shared_ptr<Mesh> planeMesh = std::make_shared<Mesh>(planeVertices, planeIndices);

    ////// Creating the materials

    auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
    rtGlobalMaterial->SetValue("missColor", Color(0.9f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto retroCarMaterial = materialFactory->CreateMaterial(lightRasterProgram);
    retroCarMaterial->SetTexture2D("mainTexture", retroCarTex);
    retroCarMaterial->SetValue("ambient", 0.01f);
    retroCarMaterial->SetValue("diffuse", 1.0f);
    retroCarMaterial->SetValue("specular", 0.1f);

    auto retroCarRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    retroCarRTMaterial->SetTexture2D("mainTexture", retroCarTex);
    retroCarRTMaterial->SetValue("ambient", 0.01f);
    retroCarRTMaterial->SetValue("diffuse", 1.0f);
    retroCarRTMaterial->SetValue("specular", 0.1f);
    retroCarRTMaterial->SetValue("castShadow", 1);

    auto pickupTruckRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    pickupTruckRTMaterial->SetTexture2D("mainTexture", pickupTruckTex);
    pickupTruckRTMaterial->SetValue("ambient", 0.01f);
    pickupTruckRTMaterial->SetValue("diffuse", 0.8f);
    pickupTruckRTMaterial->SetValue("specular", 0.1f);
    pickupTruckRTMaterial->SetValue("castShadow", 1);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex);
    lionStatueRTMaterial->SetValue("ambient", 0.01f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);
    lionStatueRTMaterial->SetValue("castShadow", 1);

    auto droneRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    droneRTMaterial->SetTexture2D("mainTexture", droneTex);
    droneRTMaterial->SetValue("ambient", 0.01f);
    droneRTMaterial->SetValue("diffuse", 0.8f);
    droneRTMaterial->SetValue("specular", 0.1f);
    droneRTMaterial->SetValue("castShadow", 1);

    auto groundShadowMaterial1 = materialFactory->CreateMaterial(rtShadowProgram);
    groundShadowMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundShadowMaterial1->SetValue("ambient", 0.01f);
    groundShadowMaterial1->SetValue("diffuse", 0.8f);
    groundShadowMaterial1->SetValue("specular", 0.1f);

    ////// Creating the entities

    auto retroCarTransform = std::make_shared<Transform>(Vector3(-2.0f, 0.7f, 3.0f), Vector3(0.65f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto retroCarMeshRenderer = std::make_shared<Render::MeshRenderer>(retroCarMesh, retroCarMaterial);
    auto retroCarRTComponent = std::make_shared<Render::RayTracingComponent>(retroCarMesh, retroCarRTMaterial);
    auto retroCarEntity = std::make_shared<QuantumEngine::GameEntity>(retroCarTransform, retroCarMeshRenderer, retroCarRTComponent);

    auto pickupTruckTransform = std::make_shared<Transform>(Vector3(1.0f, 0.0f, 3.5f), Vector3(0.5f), Vector3(0.0f, 1.0f, 0.0f), 180);
    auto pickupTruckMeshRenderer = std::make_shared<Render::MeshRenderer>(pickupTruckMesh, retroCarRTMaterial);
    auto pickupTruckRTComponent = std::make_shared<Render::RayTracingComponent>(pickupTruckMesh, pickupTruckRTMaterial);
    auto pickupTruckEntity = std::make_shared<QuantumEngine::GameEntity>(pickupTruckTransform, pickupTruckMeshRenderer, pickupTruckRTComponent);

    auto lionStatueTransform1 = std::make_shared<Transform>(Vector3(3.2f, 1.1f, 2.0f), Vector3(0.8f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto lionStatueMeshRenderer = std::make_shared<Render::MeshRenderer>(lionStatueMesh, retroCarRTMaterial);
    auto lionStatueRTComponent = std::make_shared<Render::RayTracingComponent>(lionStatueMesh, lionStatueRTMaterial);
    auto lionStatueEntity1 = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform1, lionStatueMeshRenderer, lionStatueRTComponent);

    auto droneTransform = std::make_shared<Transform>(Vector3(2.2f, 3.4f, 1.0f), Vector3(1.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto droneMeshRenderer = std::make_shared<Render::MeshRenderer>(droneMesh, retroCarRTMaterial);
    auto droneRTComponent = std::make_shared<Render::RayTracingComponent>(droneMesh, droneRTMaterial);
    auto droneEntity = std::make_shared<QuantumEngine::GameEntity>(droneTransform, droneMeshRenderer, droneRTComponent);

    ref<EntityPositionController> droneController = std::make_shared<EntityPositionController>(droneTransform, 1.0f);

    auto groundTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto gBufferRenderer = std::make_shared<Render::MeshRenderer>(planeMesh, nullptr);
    auto rtComponent5 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundShadowMaterial1);
    auto groundEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform, gBufferRenderer, rtComponent5);

    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
        .intensity = 0.5f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-2.2f, 7.4f, 3.0f),
        .intensity = 10.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 0.0f,
            .c2 = 0.5f,
        },
        .radius = 9.0f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(3.2f, 7.4f, 3.0f),
        .intensity = 10.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 0.0f,
            .c2 = 0.5f,
        },
        .radius = 9.0f,
        });

    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { retroCarEntity, pickupTruckEntity, lionStatueEntity1, droneEntity, groundEntity1 };
    scene->behaviours = { cameraController, droneController };
    scene->rtGlobalMaterial = rtGlobalMaterial;

    return scene;
}

ref<Scene> SceneBuilder::BuildRefractionScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    std::string error;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring simpleLightRasterPath = root + L"\\Assets\\Shaders\\simple_light_raster_program.hlsl";
    auto lightRasterProgram = shaderRegistery->CompileProgram(simpleLightRasterPath, error);

    if (lightRasterProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(simpleLightRasterPath) + "Error: \n" + error;
        return nullptr;
    }

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    auto globalRTProgram = shaderRegistery->CompileProgram(rtGlobalShaderPath, error);

    if (globalRTProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + error;
        return nullptr;
    }

    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    auto simpleRTLightProgram = shaderRegistery->CompileProgram(rtSimpleLightShaderPath, error);

    if (simpleRTLightProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleLightShaderPath) + "Error: \n" + error;
        return nullptr;
    }
    
    std::wstring rtRefractionShaderPath = root + L"\\Assets\\Shaders\\refractor_rt.lib.hlsl";
    auto rtRefractionProgram = shaderRegistery->CompileProgram(rtRefractionShaderPath,  error);

    if (rtRefractionProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtRefractionShaderPath) + "Error: \n" + error;
        return nullptr;
    }

    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(4.4f, 3.5f, -7.8f), Vector3(1.0f), Vector3(-0.08f, 0.19f, -0.01f), 27);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    ref<QuantumEngine::Texture2D> carTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", errorStr);
    ref<QuantumEngine::Texture2D> rabbitStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Rabbit_basecolor.jpg", errorStr);
    ref<QuantumEngine::Texture2D> lionStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", errorStr);
    ref<QuantumEngine::Texture2D> chairTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\leather_chair_BaseColor.png", errorStr);
    ref<QuantumEngine::Texture2D> groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", errorStr);
    assetManager->UploadTextureToGPU(carTex1);
    assetManager->UploadTextureToGPU(rabbitStatueTex1);
    assetManager->UploadTextureToGPU(lionStatueTex1);
    assetManager->UploadTextureToGPU(groundBrickTex1);
    assetManager->UploadTextureToGPU(chairTex1);

    ////// Importing meshes

    IMPORT_RETRO_CAR_MESH(retroCarMesh, error)

    IMPORT_LION_STATUE_MESH(lionStatueMesh, error)

    auto rabbitStatuePath = root + L"\\Assets\\Models\\RabbitStatue.fbx";
    auto rabbitStatueModel1 = AssimpModel3DImporter::Import(WCharToString(rabbitStatuePath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 0, .scale = Vector3(20.0f) }, errorStr);
    if (rabbitStatueModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(rabbitStatuePath) + "Error: \n" + errorStr;
        return nullptr;
    }
    auto rabbitStatueMesh1 = rabbitStatueModel1->GetMesh("Rabbit_low_Stereo_textured_mesh");

    auto chairPath = root + L"\\Assets\\Models\\leather_chair.fbx";
    auto chairModel1 = AssimpModel3DImporter::Import(WCharToString(chairPath.c_str()), ModelImportProperties{ .axis = Vector3(1.0f, 0.0f, 0.0f), .angleDeg = 90, .scale = Vector3(1.0f) }, errorStr);

    if (chairModel1 == nullptr) {
        error = "Error in Importing Model At: \n" + WStringToString(chairPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    auto chairMesh1 = chairModel1->GetMesh("leather_chair.001");

    std::vector<Vertex> planeVertices = {
        Vertex(Vector3(-1.0f, 0, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, -1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(-1.0f, 0, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> planeIndices = {
        0, 1, 2, 0, 2, 3,
    };

    ref<Mesh> planeMesh = std::make_shared<Mesh>(planeVertices, planeIndices);

    ref<Mesh> cubeMesh = ShapeBuilder::CreateCompleteCube(1.0f);
    ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

    ////// Creating the materials

    auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
    rtGlobalMaterial->SetValue("missColor", Color(0.9f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto carMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    carMaterial1->SetTexture2D("mainTexture", carTex1);
    carMaterial1->SetValue("ambient", 0.1f);
    carMaterial1->SetValue("diffuse", 1.0f);
    carMaterial1->SetValue("specular", 0.1f);

    auto carRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    carRTMaterial->SetTexture2D("mainTexture", carTex1);
    carRTMaterial->SetValue("ambient", 0.1f);
    carRTMaterial->SetValue("diffuse", 1.0f);
    carRTMaterial->SetValue("specular", 0.1f);

    auto rabbitStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    rabbitStatueRTMaterial->SetTexture2D("mainTexture", rabbitStatueTex1);
    rabbitStatueRTMaterial->SetValue("ambient", 0.1f);
    rabbitStatueRTMaterial->SetValue("diffuse", 0.8f);
    rabbitStatueRTMaterial->SetValue("specular", 0.1f);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueRTMaterial->SetValue("ambient", 0.1f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);

    auto chairRTMaterial = materialFactory->CreateMaterial(simpleRTLightProgram);
    chairRTMaterial->SetTexture2D("mainTexture", chairTex1);
    chairRTMaterial->SetValue("ambient", 0.1f);
    chairRTMaterial->SetValue("diffuse", 0.8f);
    chairRTMaterial->SetValue("specular", 0.1f);

    auto groundRTMaterial1 = materialFactory->CreateMaterial(simpleRTLightProgram);
    groundRTMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundRTMaterial1->SetValue("ambient", 0.1f);
    groundRTMaterial1->SetValue("diffuse", 0.8f);
    groundRTMaterial1->SetValue("specular", 0.1f);

    auto refractionRTMaterial1 = materialFactory->CreateMaterial(rtRefractionProgram);
    refractionRTMaterial1->SetValue("refractionFactor", 1.2f);
    refractionRTMaterial1->SetValue("maxRecursion", 5);

    ////// Creating the entities

    auto retroCarTransform = std::make_shared<Transform>(Vector3(-2.0f, 1.0f, 1.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto retroCarMeshRenderer = std::make_shared<Render::MeshRenderer>(retroCarMesh, carMaterial1);
    auto retroCarRTComponent = std::make_shared<Render::RayTracingComponent>(retroCarMesh, carRTMaterial);
    auto retroCarEntity = std::make_shared<QuantumEngine::GameEntity>(retroCarTransform, retroCarMeshRenderer, retroCarRTComponent);

    auto rabbitStatueTransform = std::make_shared<Transform>(Vector3(-0.5f, 0.0f, 1.0f), Vector3(2.0f), Vector3(0.0f, 1.0f, 0.0f), 180);
    auto rabbitStatueMeshRenderer = std::make_shared<Render::MeshRenderer>(rabbitStatueMesh1, carRTMaterial);
    auto rabbitStatueRTComponent = std::make_shared<Render::RayTracingComponent>(rabbitStatueMesh1, rabbitStatueRTMaterial);
    auto rabbitStatueEntity = std::make_shared<QuantumEngine::GameEntity>(rabbitStatueTransform, rabbitStatueMeshRenderer, rabbitStatueRTComponent);

    auto lionStatueTransform1 = std::make_shared<Transform>(Vector3(1.5f, 1.0f, 2.0f), Vector3(0.8f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto lionStatueMeshRenderer = std::make_shared<Render::MeshRenderer>(lionStatueMesh, carRTMaterial);
    auto lionStatueRTComponent = std::make_shared<Render::RayTracingComponent>(lionStatueMesh, lionStatueRTMaterial);
    auto lionStatueEntity = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform1, lionStatueMeshRenderer, lionStatueRTComponent);

    auto chairTransform = std::make_shared<Transform>(Vector3(3.7f, 0.0f, 1.0f), Vector3(1.5f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto chairMeshRenderer = std::make_shared<Render::MeshRenderer>(chairMesh1, carRTMaterial);
    auto rtComponent4 = std::make_shared<Render::RayTracingComponent>(chairMesh1, chairRTMaterial);
    auto chairEntity1 = std::make_shared<QuantumEngine::GameEntity>(chairTransform, chairMeshRenderer, rtComponent4);

    auto groundTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto groundGBufferRenderer = std::make_shared<Render::MeshRenderer>(planeMesh, carRTMaterial);
    auto groundRTComponent = std::make_shared<Render::RayTracingComponent>(planeMesh, groundRTMaterial1);
    auto groundEntity = std::make_shared<QuantumEngine::GameEntity>(groundTransform, groundGBufferRenderer, groundRTComponent);

    auto slabTransform = std::make_shared<Transform>(Vector3(-0.2f, 0.4f, -3.0f), Vector3(2.0f, 5.0f, 0.5f), Vector3(0.0f, 0.0f, 1.0f), 90);
    auto slabRenderer = std::make_shared<Render::GBufferRTReflectionRenderer>(cubeMesh, carMaterial1);
    auto slabRTComponent = std::make_shared<Render::RayTracingComponent>(cubeMesh, refractionRTMaterial1);
    auto slabEntity = std::make_shared<QuantumEngine::GameEntity>(slabTransform, slabRenderer, slabRTComponent);

    auto sphereTransform = std::make_shared<Transform>(Vector3(3.2f, 2.4f, -2.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto sphereRTComponent = std::make_shared<Render::RayTracingComponent>(sphereMesh, refractionRTMaterial1);
    auto sphereEntity = std::make_shared<QuantumEngine::GameEntity>(sphereTransform, slabRenderer, sphereRTComponent);

    ref<MaterialValueModifier> sphereReflectionModifier = std::make_shared<MaterialValueModifier>(refractionRTMaterial1, "refractionFactor", 0.2f, 1.0f, 1.5f);


    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(2.0f, -6.0f, 2.0f),
        .intensity = 0.5f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-0.2f, 4.4f, 3.0f),
        .intensity = 2.0f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 1.0f,
            .c2 = 0.0f,
        },
        .radius = 9.0f,
        });

    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { retroCarEntity, rabbitStatueEntity, lionStatueEntity, chairEntity1, groundEntity, slabEntity, sphereEntity };
    scene->behaviours = { cameraController, sphereReflectionModifier };
    scene->rtGlobalMaterial = rtGlobalMaterial;

    return scene;
}

ref<Scene> SceneBuilder::BuildComplexScene(const ref<Render::GPUAssetManager>& assetManager, const ref<Render::ShaderRegistery>& shaderRegistery, const ref<Render::MaterialFactory>& materialFactory, ref<Platform::GraphicWindow> win, std::string& errorStr)
{
    std::string error;

    std::wstring root = Platform::Application::GetExecutablePath();
    std::string rootA = WStringToString(root);

    ////// Compiling Shaders

    std::wstring simpleLightRasterPath = root + L"\\Assets\\Shaders\\simple_light_raster_program.hlsl";
    auto lightRasterProgram = shaderRegistery->CompileProgram(simpleLightRasterPath, error);

    if (lightRasterProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(simpleLightRasterPath) + "Error: \n" + error;
        return nullptr;
    }

    std::wstring rtGlobalShaderPath = root + L"\\Assets\\Shaders\\rt_global.lib.hlsl";
    auto globalRTProgram = shaderRegistery->CompileProgram(rtGlobalShaderPath, error);

    if (globalRTProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtGlobalShaderPath) + "Error: \n" + error;
        return nullptr;
    }

    std::wstring rtSimpleLightShaderPath = root + L"\\Assets\\Shaders\\simple_light_rt.lib.hlsl";
    auto simpleRTLightProgram = shaderRegistery->CompileProgram(rtSimpleLightShaderPath, error);

    if (simpleRTLightProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleLightShaderPath) + "Error: \n" + error;
        return nullptr;
    }

    std::wstring rtReflectionShaderPath = root + L"\\Assets\\Shaders\\reflection_rt.lib.hlsl";
    auto reflectionRTLightProgram = shaderRegistery->CompileProgram(rtReflectionShaderPath, error);

    if (reflectionRTLightProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtReflectionShaderPath) + "Error: \n" + error;
        return nullptr;
    }

    std::wstring rtStandardReflectionShaderPath = root + L"\\Assets\\Shaders\\reflection_standard_rt.hlsl";
    auto reflectionStandardRTLightProgram = shaderRegistery->CompileProgram(rtStandardReflectionShaderPath, errorStr);

    if (reflectionStandardRTLightProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtStandardReflectionShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    std::wstring rtShadowShaderPath = root + L"\\Assets\\Shaders\\shadow_rt.lib.hlsl";
    auto rtShadowProgram = shaderRegistery->CompileProgram(rtShadowShaderPath, error);

    if (rtShadowProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtShadowShaderPath) + "Error: \n" + error;
        return nullptr;
    }

    std::wstring rtRefractionShaderPath = root + L"\\Assets\\Shaders\\refractor_rt.lib.hlsl";
    auto rtRefractionProgram = shaderRegistery->CompileProgram(rtRefractionShaderPath, error);

    if (rtRefractionProgram == nullptr) {
        errorStr = "Error in Compiling Shader At: \n" + WStringToString(rtRefractionShaderPath) + "Error: \n" + error;
        return nullptr;
    }

    std::wstring rtSimpleShaderPath = root + L"\\Assets\\Shaders\\simple_rt.lib.hlsl";
    auto rtSimpleProgram = shaderRegistery->CompileProgram(rtSimpleShaderPath, error);

    if (rtSimpleProgram == nullptr) {
        error = "Error in Compiling Shader At: \n" + WStringToString(rtSimpleShaderPath) + "Error: \n" + errorStr;
        return nullptr;
    }

    ////// Creating the camera

    auto camtransform = std::make_shared<Transform>(Vector3(-15.5f, 6.0f, -9.1f), Vector3(1.0f), Vector3(-0.18f, -0.79f, 0.09f), 65);
    ref<Camera> mainCamera = std::make_shared<PerspectiveCamera>(camtransform, 0.1f, 1000.0f, (float)win->GetWidth() / win->GetHeight(), 45);
    ref<CameraController> cameraController = std::make_shared<CameraController>(mainCamera);

    ////// Importing Textures

    auto pickupTruckColorTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\truck_color.jpg", error);
    auto pickupTruckReflTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\truck_refl.jpg", error);
    auto pedestalTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\tech_pedestal_COL.png", error);
    auto retroCarTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\RetroCarAlbedo.png", error);
    auto lionStatueTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\Lion_statue_raw_texture.jpg", error);
    auto groundBrickTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\brickGround.png", errorStr);
    auto waterTex1 = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\water.jpeg", errorStr);
    auto skyBoxTex = QuantumEngine::WICTexture2DImporter::Import(root + L"\\Assets\\Textures\\skybox.jpg", errorStr);

    assetManager->UploadTextureToGPU(pickupTruckColorTex);
    assetManager->UploadTextureToGPU(pickupTruckReflTex);
    assetManager->UploadTextureToGPU(pedestalTex);
    assetManager->UploadTextureToGPU(retroCarTex);
    assetManager->UploadTextureToGPU(lionStatueTex1);
    assetManager->UploadTextureToGPU(groundBrickTex1);
    assetManager->UploadTextureToGPU(waterTex1);
    assetManager->UploadTextureToGPU(skyBoxTex);

    ////// Import Meshes

    IMPORT_PICKUP_TRUCK_MESH(pickupTruckMesh, error)

    IMPORT_RETRO_CAR_MESH(retroCarMesh, error)

    IMPORT_LION_STATUE_MESH(lionStatueMesh, error)

    IMPORT_PEDESTAL_MESH(pedestalMesh, error)

    std::vector<Vertex> planeVertices = {
        Vertex(Vector3(-1.0f, 0, -1.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, -1.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(1.0f, 0, 1.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
        Vertex(Vector3(-1.0f, 0, 1.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)),
    };

    std::vector<UInt32> planeIndices = {
        0, 1, 2, 0, 2, 3,
    };

    ref<Mesh> planeMesh = std::make_shared<Mesh>(planeVertices, planeIndices);

       std::vector<Vertex> skyBoxVertices = {
       Vertex(Vector3(-1.0f, -1.0f, -1.0f), Vector2(1.0f, 2.0f / 3), Vector3(0.0f)),
       Vertex(Vector3(1.0f, -1.0f, -1.0f), Vector2(0.75f, 2.0f / 3), Vector3(0.0f)),
       Vertex(Vector3(1.0f, 1.0f, -1.0f), Vector2(0.75f, 1.0f / 3), Vector3(0.0f)),
       Vertex(Vector3(-1.0f, 1.0f, -1.0f), Vector2(1.0f, 1.0f / 3), Vector3(0.0f)),
       Vertex(Vector3(-1.0f, -1.0f, 1.0f), Vector2(0.25f, 2.0f / 3), Vector3(0.0f)),
       Vertex(Vector3(1.0f, -1.0f, 1.0f), Vector2(0.5f, 2.0f / 3), Vector3(0.0f)),
       Vertex(Vector3(1.0f, 1.0f, 1.0f), Vector2(0.5f, 1.0f / 3), Vector3(0.0f)),
       Vertex(Vector3(-1.0f, 1.0f, 1.0f), Vector2(0.25f, 1.0f / 3), Vector3(0.0f)),
       Vertex(Vector3(-1.0f, 1.0f, -1.0f), Vector2(0.25f, 0.0f), Vector3(0.0f)),
       Vertex(Vector3(1.0f, 1.0f, -1.0f), Vector2(0.5f, 0.0f), Vector3(0.0f)),
       Vertex(Vector3(1.0f, -1.0f, -1.0f), Vector2(0.25f, 1.0f), Vector3(0.0f)),
       Vertex(Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.5f, 1.0f), Vector3(0.0f)),
    };

    std::vector<UInt32> skyBoxIndices = {
        0, 2, 1, 0, 3, 2,
        4, 5, 6, 4, 6, 7,
        2, 6, 5, 2, 5, 1,
        0, 4, 7, 0, 7, 3,
        6, 8, 7, 6, 9, 8,
        4, 10, 5, 4, 11, 10,
    };

    ref<Mesh> skyBoxMesh = std::make_shared<Mesh>(skyBoxVertices, skyBoxIndices);
    ref<Mesh> cubeMesh = ShapeBuilder::CreateCompleteCube(1.0f);
    ref<Mesh> sphereMesh = ShapeBuilder::CreateSphere(1.0f, 30, 30);

    ////// Creating the materials

    auto rtGlobalMaterial = materialFactory->CreateMaterial(globalRTProgram);
    rtGlobalMaterial->SetValue("missColor", Color(0.9f, 0.4f, 0.6f, 1.0f));
    rtGlobalMaterial->SetValue("hitColor", Color(0.8f, 0.1f, 0.3f, 1.0f));

    auto carMaterial1 = materialFactory->CreateMaterial(lightRasterProgram);
    carMaterial1->SetTexture2D("mainTexture", retroCarTex);
    carMaterial1->SetValue("ambient", 0.1f);
    carMaterial1->SetValue("diffuse", 1.0f);
    carMaterial1->SetValue("specular", 0.1f);

    auto pickupTruckRTMaterial = materialFactory->CreateMaterial(reflectionStandardRTLightProgram);
    pickupTruckRTMaterial->SetTexture2D("mainTexture", pickupTruckColorTex);
    pickupTruckRTMaterial->SetTexture2D("reflectTexture", pickupTruckReflTex);
    pickupTruckRTMaterial->SetValue("castShadow", 1);
    pickupTruckRTMaterial->SetValue("ambient", 0.1f);
    pickupTruckRTMaterial->SetValue("diffuse", 1.0f);
    pickupTruckRTMaterial->SetValue("specular", 0.1f);

    auto retroCarRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    retroCarRTMaterial->SetTexture2D("mainTexture", retroCarTex);
    retroCarRTMaterial->SetValue("ambient", 0.1f);
    retroCarRTMaterial->SetValue("diffuse", 0.8f);
    retroCarRTMaterial->SetValue("specular", 0.1f);
    retroCarRTMaterial->SetValue("castShadow", 1);

    auto lionStatueRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    lionStatueRTMaterial->SetTexture2D("mainTexture", lionStatueTex1);
    lionStatueRTMaterial->SetValue("ambient", 0.1f);
    lionStatueRTMaterial->SetValue("diffuse", 0.8f);
    lionStatueRTMaterial->SetValue("specular", 0.1f);
    lionStatueRTMaterial->SetValue("castShadow", 1);

    auto pedestalRTMaterial = materialFactory->CreateMaterial(rtShadowProgram);
    pedestalRTMaterial->SetTexture2D("mainTexture", pedestalTex);
    pedestalRTMaterial->SetValue("ambient", 0.1f);
    pedestalRTMaterial->SetValue("diffuse", 0.8f);
    pedestalRTMaterial->SetValue("specular", 0.1f);
    pedestalRTMaterial->SetValue("castShadow", 1);

    auto groundRTMaterial1 = materialFactory->CreateMaterial(rtShadowProgram);
    groundRTMaterial1->SetTexture2D("mainTexture", groundBrickTex1);
    groundRTMaterial1->SetValue("ambient", 0.1f);
    groundRTMaterial1->SetValue("diffuse", 0.8f);
    groundRTMaterial1->SetValue("specular", 0.1f);
    groundRTMaterial1->SetValue("castShadow", 1);

    auto skyboxRTMaterial = materialFactory->CreateMaterial(rtSimpleProgram);
    skyboxRTMaterial->SetValue("color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    skyboxRTMaterial->SetTexture2D("mainTexture", skyBoxTex);

    auto reflectionRTMaterial1 = materialFactory->CreateMaterial(reflectionRTLightProgram);
    reflectionRTMaterial1->SetTexture2D("mainTexture", waterTex1);
    reflectionRTMaterial1->SetValue("reflectivity", 0.6f);
    reflectionRTMaterial1->SetValue("castShadow", 1);
    reflectionRTMaterial1->SetValue("ambient", 0.4f);
    reflectionRTMaterial1->SetValue("diffuse", 0.8f);
    reflectionRTMaterial1->SetValue("specular", 0.1f);

    auto refractionRTMaterial1 = materialFactory->CreateMaterial(rtRefractionProgram);
    refractionRTMaterial1->SetValue("refractionFactor", 1.2f);
    refractionRTMaterial1->SetValue("maxRecursion", 5);

    ////// Entities
    auto pickupTruckTransform = std::make_shared<Transform>(Vector3(-1.0f, 0.0f, 1.0f), Vector3(0.6f), Vector3(0.0f, 1.0f, 0.0f), -45);
    auto pickupTruckMeshRenderer = std::make_shared<Render::MeshRenderer>(pickupTruckMesh, carMaterial1);
    auto pickupTruckRTComponent = std::make_shared<Render::RayTracingComponent>(pickupTruckMesh, pickupTruckRTMaterial);
    auto pickupTruckEntity = std::make_shared<QuantumEngine::GameEntity>(pickupTruckTransform, pickupTruckMeshRenderer, pickupTruckRTComponent);
    auto pickupTruckMover = std::make_shared<EntityMover>(pickupTruckTransform, Vector3(2, 0.0f, 2), Vector3(-3, 0.0f, -3), 0.3f, 3);

    auto lionStatueTransform = std::make_shared<Transform>(Vector3(3.0f, 1.2f, -3.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto lionStatueMeshRenderer = std::make_shared<Render::MeshRenderer>(lionStatueMesh, carMaterial1);
    auto lionStatueRTComponent = std::make_shared<Render::RayTracingComponent>(lionStatueMesh, lionStatueRTMaterial);
    auto lionStatueEntity = std::make_shared<QuantumEngine::GameEntity>(lionStatueTransform, lionStatueMeshRenderer, lionStatueRTComponent);
    auto lionRotator = std::make_shared<EntityRotator>(lionStatueTransform, -30);

    auto pedestalTransform = std::make_shared<Transform>(Vector3(-5.5f, 0.0f, 2.0f), Vector3(3.3f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto pedestalRTComponent1 = std::make_shared<Render::RayTracingComponent>(pedestalMesh, pedestalRTMaterial);
    auto pedestalEntity = std::make_shared<QuantumEngine::GameEntity>(pedestalTransform, pickupTruckMeshRenderer, pedestalRTComponent1);

    auto retroCarTransform = std::make_shared<Transform>(Vector3(-5.5f, 1.0f, 2.0f), Vector3(0.5f), Vector3(0.0f, 1.0f, 0.0f), -90);
    auto retroCarMeshRenderer = std::make_shared<Render::MeshRenderer>(retroCarMesh, carMaterial1);
    auto retroCarRTComponent = std::make_shared<Render::RayTracingComponent>(retroCarMesh, retroCarRTMaterial);
    auto retroCarEntity = std::make_shared<QuantumEngine::GameEntity>(retroCarTransform, retroCarMeshRenderer, retroCarRTComponent);
    auto retroCarRotator = std::make_shared<EntityRotator>(retroCarTransform, 10);

    auto groundTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(20.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto gBufferRenderer = std::make_shared<Render::MeshRenderer>(planeMesh, pickupTruckRTMaterial);
    auto rtComponent5 = std::make_shared<Render::RayTracingComponent>(planeMesh, groundRTMaterial1);
    auto groundEntity1 = std::make_shared<QuantumEngine::GameEntity>(groundTransform, gBufferRenderer, rtComponent5);

    auto refractorTransform1 = std::make_shared<Transform>(Vector3(-3.2f, 0.4f, -7.0f), Vector3(2.0f, 5.0f, 0.2f), Vector3(0.0f, 1.0f, 0.0f), 90);
    auto Renderer = std::make_shared<Render::GBufferRTReflectionRenderer>(cubeMesh, carMaterial1);
    auto rtComponent6 = std::make_shared<Render::RayTracingComponent>(cubeMesh, refractionRTMaterial1);
    auto glassEntity1 = std::make_shared<QuantumEngine::GameEntity>(refractorTransform1, Renderer, rtComponent6);

    auto refractorTransform2 = std::make_shared<Transform>(Vector3(-8.2f, 2.7f, 0.0f), Vector3(1.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto rtComponent7 = std::make_shared<Render::RayTracingComponent>(sphereMesh, refractionRTMaterial1);
    auto glassEntity2 = std::make_shared<QuantumEngine::GameEntity>(refractorTransform2, Renderer, rtComponent7);
    auto glassMover = std::make_shared<EntityMover>(refractorTransform2, Vector3(-8.2f, 2.7f, -3), Vector3(-8.2f, 2.7f, 3), 0.3f, 2);
    
    auto sphereReflectTransform = std::make_shared<Transform>(Vector3(0.2f, 3.8f, 5.0f), Vector3(2.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto sphereReflectRTComponent = std::make_shared<Render::RayTracingComponent>(sphereMesh, reflectionRTMaterial1);
    auto sphereReflectEntity = std::make_shared<QuantumEngine::GameEntity>(sphereReflectTransform, gBufferRenderer, sphereReflectRTComponent);
    auto sphereMover = std::make_shared<EntityPositionController>(sphereReflectTransform, 3.0f);

    auto wallTransform = std::make_shared<Transform>(Vector3(8.0f, 0.0f, 0.0f), Vector3(15.0f), Vector3(0.0f, 0.0f, 1.0f), -90);
    auto wallRTComponent = std::make_shared<Render::RayTracingComponent>(planeMesh, reflectionRTMaterial1);
    auto mirrorEntity2 = std::make_shared<QuantumEngine::GameEntity>(wallTransform, gBufferRenderer, wallRTComponent);

    auto skyBoxTransform = std::make_shared<Transform>(Vector3(0.0f, 0.0f, 0.0f), Vector3(40.0f), Vector3(0.0f, 0.0f, 1.0f), 0);
    auto rtComponent10 = std::make_shared<Render::RayTracingComponent>(skyBoxMesh, skyboxRTMaterial);
    auto skyBoxEntity = std::make_shared<QuantumEngine::GameEntity>(skyBoxTransform, gBufferRenderer, rtComponent10);

    ////// Creating the lights

    SceneLightData lightData;

    lightData.directionalLights.push_back(DirectionalLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .direction = Vector3(-2.0f, -6.0f, -2.0f),
        .intensity = 0.4f,
        });

    lightData.pointLights.push_back(PointLight{
        .color = Color(1.0f, 1.0f, 1.0f, 1.0f),
        .position = Vector3(-2.2f, 4.4f, 0.0f),
        .intensity = 2.5f,
        .attenuation = Attenuation{
            .c0 = 0.0f,
            .c1 = 1.0f,
            .c2 = 0.0f,
        },
        .radius = 9.0f,
        });

    auto frameLogger = std::make_shared<FrameRateLogger>();

    ref<Scene> scene = std::make_shared<Scene>();
    scene->mainCamera = mainCamera;
    scene->lightData = lightData;
    scene->entities = { 
        pickupTruckEntity, 
        retroCarEntity,
        lionStatueEntity, 
        groundEntity1, 
        sphereReflectEntity,
        mirrorEntity2,
        glassEntity1,
        glassEntity2,
        skyBoxEntity,
        pedestalEntity,
    };
    scene->behaviours = { 
        cameraController, 
        pickupTruckMover, 
        retroCarRotator,
        lionRotator, 
        sphereMover,
        glassMover,
    };
    scene->rtGlobalMaterial = rtGlobalMaterial;

    return scene;
}
