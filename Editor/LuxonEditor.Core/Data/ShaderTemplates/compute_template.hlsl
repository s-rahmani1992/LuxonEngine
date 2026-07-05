#include "Common/VariableMacros.hlsli"

CONSTANT_VARIABLES_BEGIN
    float var1;
CONSTANT_VARIABLES_END(constVariables, b0)

[numthreads(32, 1, 1)]
void $(COMPUTE_MAIN)( uint3 DTid : SV_DispatchThreadID )
{
    
}