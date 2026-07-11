
#ifndef VARIABLE_MACROS
#define VARIABLE_MACROS

#ifdef _DX12_RAY_TRACING_LOCAL
    #define DX12_REGISTER_SPACE(x) register( x , space1)
#else
    #define DX12_REGISTER_SPACE(x) register( x , space0)
#endif

#define CONSTANT_VARIABLES_BEGIN struct MaterialConstantProperties{

#ifdef _VULKAN
    #ifdef _VK_RAY_TRACING
        #define CONSTANT_VARIABLES_END(propName, x) }; \
        [[vk::shader_record_ext]]    \
        cbuffer MaterialConstantsBuffer{  \
            MaterialConstantProperties propName;   \
        };
    #else
        #define CONSTANT_VARIABLES_END(propName, x) }; \
        [[vk::push_constant]]    \
        MaterialConstantProperties propName;
    #endif
#else
    #define CONSTANT_VARIABLES_END(propName, x) }; \
    cbuffer MaterialConstantsBuffer : DX12_REGISTER_SPACE(x) {  \
        MaterialConstantProperties propName;   \
    };
#endif


#ifdef _VULKAN
    #ifdef _VK_RAY_TRACING_LOCAL
        #define TEXTURE(texName, type, x)   Texture2D<type> texName##Array[];  \
        static Texture2D<type> texName = texName##Array[InstanceID()];
    #else
        #define TEXTURE(texName, type, x)   Texture2D<type> texName;
    #endif
#else
    #define TEXTURE(texName, type, x)   Texture2D<type> texName : DX12_REGISTER_SPACE(x) ;
#endif


#ifdef _VULKAN
    #define SAMPLER(samplerName, x)   sampler samplerName;
#else
    #define SAMPLER(samplerName, x)   sampler samplerName : DX12_REGISTER_SPACE(x) ;
#endif


#endif