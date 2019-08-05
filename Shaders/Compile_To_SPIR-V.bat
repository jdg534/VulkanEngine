REM This code assumes that you've added the path to glslc.exe to your path environment variable

glslc Default.vert -o DefaultVert.spv
glslc Default.frag -o DefaultFrag.spv

echo Finished Shader Compilation
PAUSE