#pragma 
#include "../BasicTypes.h"
#include <vector>

namespace QuantumEngine {
	class Mesh;
	class Texture2D;
}

namespace QuantumEngine::Rendering {
	class GPUAssetManager {
	public:
		virtual void UploadMeshToGPU(const ref<Mesh>& mesh) = 0;
		virtual void UploadMeshesToGPU(const std::vector<ref<Mesh>>& meshes) = 0;
		virtual void UploadTextureToGPU(const ref<Texture2D>& texture) = 0;
		virtual void UnloadAssets() = 0;
	};
}