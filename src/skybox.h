#ifndef CUBEMAP_LOADER_H
#define CUBEMAP_LOADER_H

#include <glad/glad.h>
#include <iostream>
#include <string>
#include <vector>

class Skybox
{
   public:
    static unsigned int loadCubemap(const std::vector<std::string>& faces);
    static unsigned int createSkyboxVAO();

   private:
    static void setCubemapParameters();
};

#endif
