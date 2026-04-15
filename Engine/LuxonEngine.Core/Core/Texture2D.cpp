#include "Texture2D.h"

QuantumEngine::Texture2D::Texture2D(const TextureProperties& properties)
	:m_width(properties.width), m_height(properties.height), m_bpp(properties.bpp),
	m_size(properties.size), m_format(properties.format), 
	m_data(properties.copyPixelData ? new Byte[m_size] : properties.data)
{
	if(properties.copyPixelData)
		std::memcpy(m_data, properties.data, m_size);
}

void QuantumEngine::Texture2D::Release()
{
	delete[] m_data;
}
