
#pragma once
#include <utility>
#include <vector>
#include "RenderObject.h"
#include "Camera.h"

class Scene
{
public:
	Scene(Camera camera, std::vector<RenderObject> render_objects) : m_Camera(camera), m_RenderObjects(std::move(render_objects)) {  }
	Scene(Camera& camera, size_t dimensionCubeCount, double padding);

	const Camera& camera() const { return m_Camera; }
	const std::vector<RenderObject>& renderObjects() const { return m_RenderObjects; }
private:
	Camera m_Camera;
	std::vector<RenderObject> m_RenderObjects;
};

inline Scene::Scene(Camera& camera, size_t dimensionCubeCount, double paddingFactor) : m_Camera(camera)
{
	//make oringin (0,0,0) the center of the "cube of cubes" by subtracting an offset:
	auto offset = (dimensionCubeCount + (dimensionCubeCount - 1) * paddingFactor) / 2;
	for(auto x = 0; x < dimensionCubeCount; ++x){
		for (auto y = 0; y < dimensionCubeCount; ++y) {
			for (auto z = 0; z < dimensionCubeCount; ++z) {
				m_RenderObjects.emplace_back(
					x + x * paddingFactor - offset, 
					y + y * paddingFactor - offset, 
					z + z * paddingFactor - offset

				);
			}
		}
	}
}
