// #include "Image.h"
#include "mesh.h"
#include "texture.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>

#include "camera.h"
#include "skybox.h"
#include "stb/stb_image.h"
#include "terrain.h"
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>

class Application
{
   public:
    Application()
        : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41),
          m_terrainTexture(RESOURCE_ROOT "resources/terrain/Ground050/Ground050_2K-JPG_Color.jpg"),
          m_worldCamera(&m_window, glm::vec3(-6.0f, 2.5f, 2.5f), -glm::vec3(-3.5f, 0.5f, 2.0f)),
          m_objectCamera(&m_window, glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
          m_activeCamera(&m_worldCamera),
          m_terrain(m_terrainParameters)

    {
        m_window.registerKeyCallback(
            [this](int key, int scancode, int action, int mods)
            {
                if (action == GLFW_PRESS)
                    onKeyPressed(key, mods);
                else if (action == GLFW_RELEASE)
                    onKeyReleased(key, mods);
            });

        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback(
            [this](int button, int action, int mods)
            {
                if (action == GLFW_PRESS)
                    onMouseClicked(button, mods);
                else if (action == GLFW_RELEASE)
                    onMouseReleased(button, mods);
            });

        m_ufoMeshes  = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/ufo/flying_Disk_flying.obj", true);
        m_baseMeshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/environment/MarsBase.obj");

        std::vector<std::string> cubemapFaces = {"resources/skybox/right.png", "resources/skybox/left.png",
                                                 "resources/skybox/top.png",   "resources/skybox/bottom.png",
                                                 "resources/skybox/front.png", "resources/skybox/back.png"};
        m_cubemapTex                          = Skybox::loadCubemap(cubemapFaces);
        m_skyboxVAO                           = Skybox::createSkyboxVAO();

        m_objectCamera.setFollowTarget(&m_meshPosition, &m_meshRotation);

        try
        {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            ShaderBuilder litBuilder;
            litBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            litBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_lit_frag.glsl");
            m_litShader = litBuilder.build();

            ShaderBuilder skyboxBuilder;
            skyboxBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl");
            skyboxBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/skybox_frag.glsl");
            m_skyboxShader = skyboxBuilder.build();

            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        }
        catch (ShaderLoadingException e)
        {
            std::cerr << e.what() << std::endl;
        }

        glGenTextures(1, &m_texShadow);
        glBindTexture(GL_TEXTURE_2D, m_texShadow);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_shadowWidth, m_shadowWidth, 0, GL_DEPTH_COMPONENT,
                     GL_FLOAT, NULL);

        // Set behaviour for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &m_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texShadow, 0);
        glDrawBuffer(GL_NONE);  // Important for depth-only FBO
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void update()
    {
        int dummyInteger = 0;  // Initialized to 0
        while (!m_window.shouldClose())
        {
            // This is your game loop

            m_window.updateInput();

            if (m_selectedViewpoint == 1)
                updateObjectMovement();

            m_activeCamera->updateInput();

            m_terrain.update(m_activeCamera->cameraPos());

            // Increment skybox rotation and update skybox rotation matrix
            m_skyboxRotation += 0.005f;
            glm::mat4 skyboxRotation =
                glm::rotate(glm::mat4(1.0f), glm::radians(m_skyboxRotation), glm::vec3(0.0f, 1.0f, 0.0f));

            // Use ImGui for easy input/output of ints, floats, strings, etc...
            imgui();

            // Clear the screen
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            if (m_wire_frame_enabled)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // * this renders the triangles as wireframe
            }

            // --- Setup light matrices ---
            glm::mat4 lightProjection  = glm::perspective(glm::radians(60.0f), 1.0f, 0.5f, 50.0f);
            glm::mat4 lightView        = glm::lookAt(m_lights[0].position, glm::vec3(0.0f), glm::vec3(0, 1, 0));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;

            // --- Perform shadow pass ---
            {
                glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
                glViewport(0, 0, m_shadowWidth, m_shadowWidth);
                glClearDepth(1.0);
                glClear(GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);

                m_shadowShader.bind();

                // Uniform for light space transform
                GLint mvpLoc = m_shadowShader.getUniformLocation("mvpMatrix");

                // Render static environment meshes
                for (auto& mesh : m_baseMeshes)
                {
                    glm::mat4 model    = m_modelMatrix;  // or however you compute it
                    glm::mat4 lightMVP = lightSpaceMatrix * model;
                    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(lightMVP));

                    glBindVertexArray(mesh.getVAO());
                    mesh.draw(m_shadowShader, false);
                }

                // Render moving UFO meshes
                for (auto& mesh : m_ufoMeshes)
                {
                    glm::mat4 model = glm::rotate(glm::translate(m_modelMatrix, m_meshPosition), m_meshRotation.y,
                                                  glm::vec3(0, 1, 0));

                    glm::mat4 lightMVP = lightSpaceMatrix * model;
                    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(lightMVP));

                    glBindVertexArray(mesh.getVAO());
                    mesh.draw(m_shadowShader, false);
                }

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, m_window.getWindowSize().x, m_window.getWindowSize().y);
                glClearColor(0.4f, 0.4f, 0.5f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            // Render all UFO meshes
            for (GPUMesh& mesh : m_ufoMeshes)
            {
                glm::mat4 model =
                    glm::rotate(glm::translate(m_modelMatrix, m_meshPosition), m_meshRotation.y, glm::vec3(0, 1, 0));
                glm::mat4 mvpMatrix         = m_projectionMatrix * m_activeCamera->viewMatrix() * model;
                glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(model));

                bindAndSetup(m_litShader, mvpMatrix, model, normalModelMatrix);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTex);
                glUniform1i(m_litShader.getUniformLocation("skybox"), 1);
                glUniformMatrix4fv(m_litShader.getUniformLocation("skyboxRotation"), 1, GL_FALSE,
                                   glm::value_ptr(skyboxRotation));

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_texShadow);
                glUniform1i(m_litShader.getUniformLocation("texShadow"), 0);
                glUniformMatrix4fv(m_litShader.getUniformLocation("lightMVP"), 1, GL_FALSE,
                                   glm::value_ptr(lightSpaceMatrix));

                if (mesh.hasTextureCoords())
                {
                    // m_terrainTexture.bind(
                    //     GL_TEXTURE0);  // TODO this has to be fixed -> meshes can get textures from .mtl
                    glUniform1i(m_litShader.getUniformLocation("colorMap"), 0);
                    glUniform1i(m_litShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                    glUniform1i(m_litShader.getUniformLocation("useMaterial"), GL_FALSE);
                }
                else
                {
                    glUniform1i(m_litShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(m_litShader.getUniformLocation("useMaterial"), m_useMaterial);
                }
                glUniform1i(m_litShader.getUniformLocation("useEnvironmentalMapping"), m_useEnvironmentalMapping);
                glUniform1i(m_litShader.getUniformLocation("useNormalMap"), GL_FALSE);
                glUniform1i(m_litShader.getUniformLocation("hasTangents"), GL_FALSE);

                mesh.draw(m_litShader);

                glDisable(GL_BLEND);
            }

            // Render base meshes
            for (GPUMesh& mesh : m_baseMeshes)
            {
                glm::mat4 mvpMatrix         = m_projectionMatrix * m_activeCamera->viewMatrix() * m_modelMatrix;
                glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

                bindAndSetup(m_litShader, mvpMatrix, m_modelMatrix, normalModelMatrix);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTex);
                glUniform1i(m_litShader.getUniformLocation("skybox"), 1);
                glUniformMatrix4fv(m_litShader.getUniformLocation("skyboxRotation"), 1, GL_FALSE,
                                   glm::value_ptr(skyboxRotation));

                if (mesh.hasTextureCoords())
                {
                    // m_terrainTexture.bind(
                    //     GL_TEXTURE0);  // TODO this has to be fixed -> meshes can get textures from .mtl
                    glUniform1i(m_litShader.getUniformLocation("colorMap"), 0);
                    glUniform1i(m_litShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                    glUniform1i(m_litShader.getUniformLocation("useMaterial"), GL_FALSE);
                }
                else
                {
                    glUniform1i(m_litShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(m_litShader.getUniformLocation("useMaterial"), m_useMaterial);
                }
                glUniform1i(m_litShader.getUniformLocation("useEnvironmentalMapping"), m_useEnvironmentalMapping);
                glUniform1i(m_litShader.getUniformLocation("useNormalMap"), GL_FALSE);
                glUniform1i(m_litShader.getUniformLocation("hasTangents"), GL_FALSE);

                mesh.draw(m_litShader);

                glDisable(GL_BLEND);
            }

            // Render terrain
            {
                glm::mat4 modelMatrix       = m_modelMatrix;
                glm::mat4 mvpMatrix         = m_projectionMatrix * m_activeCamera->viewMatrix() * modelMatrix;
                glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));
                bindAndSetup(m_litShader, mvpMatrix, modelMatrix, normalModelMatrix);

                if (m_useTexture)
                {
                    m_terrainTexture.bind(GL_TEXTURE2);
                    glUniform1i(m_litShader.getUniformLocation("colorMap"), 2);
                    glUniform1i(m_litShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                    glUniform1i(m_litShader.getUniformLocation("useMaterial"), GL_FALSE);
                }
                else
                {
                    glUniform1i(m_litShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(m_litShader.getUniformLocation("useMaterial"), m_useMaterial);
                }
                glUniform1i(m_litShader.getUniformLocation("useEnvironmentalMapping"), GL_FALSE);
                glUniform1i(m_litShader.getUniformLocation("useNormalMap"), m_useNormalMap);
                glUniform1i(m_litShader.getUniformLocation("hasTangents"), GL_TRUE);
                if (m_useNormalMap)
                {
                    m_terrainNormal.bind(GL_TEXTURE2);
                    glUniform1i(m_litShader.getUniformLocation("normalMap"), 2);
                    glUniform1f(m_litShader.getUniformLocation("normalStrength"), m_normalStrength);
                    glUniform1i(m_litShader.getUniformLocation("normalFlipY"), m_normalFlipY);
                }

                m_terrain.render(m_litShader);
            }

            // Render skybox
            {
                glm::mat4 skyboxView = glm::mat4(glm::mat3(m_activeCamera->viewMatrix())) * skyboxRotation;

                glDepthMask(GL_FALSE);
                glDepthFunc(GL_LEQUAL);
                m_skyboxShader.bind();
                glUniform1i(m_litShader.getUniformLocation("useNormalMap"), GL_FALSE);
                glUniform1i(m_litShader.getUniformLocation("hasTangents"), GL_FALSE);
                glUniformMatrix4fv(m_skyboxShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
                glUniformMatrix4fv(m_skyboxShader.getUniformLocation("projection"), 1, GL_FALSE,
                                   glm::value_ptr(m_projectionMatrix));
                glUniform1i(m_skyboxShader.getUniformLocation("skybox"), 0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTex);

                glBindVertexArray(m_skyboxVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                glBindVertexArray(0);

                glDepthMask(GL_TRUE);
                glDepthFunc(GL_LESS);
            }

            // Disable wireframe rendering after loop
            if (m_wire_frame_enabled)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
        glDeleteFramebuffers(1, &m_framebuffer);
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods)
    {
        std::cout << "Key pressed: " << key << std::endl;

        // switch viewpoints
        if (key == GLFW_KEY_1)
        {
            m_selectedViewpoint = 0;
            m_worldCamera.setUserInteraction(true);
            m_objectCamera.setUserInteraction(false);
            m_activeCamera = &m_worldCamera;
        }
        else if (key == GLFW_KEY_2)
        {
            m_selectedViewpoint = 1;
            m_worldCamera.setUserInteraction(false);
            m_objectCamera.setUserInteraction(true);
            m_activeCamera = &m_objectCamera;
        }
        else if (key == GLFW_KEY_3)
            m_shadingMode = 0;
        else if (key == GLFW_KEY_4)
            m_shadingMode = 1;
        else if (key == GLFW_KEY_5)
            m_shadingMode = 2;
        else if (key == GLFW_KEY_6)
            m_shadingMode = 3;
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods) { std::cout << "Key released: " << key << std::endl; }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2& cursorPos)
    {
        std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods) { std::cout << "Pressed mouse button: " << button << std::endl; }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods) { std::cout << "Released mouse button: " << button << std::endl; }

    // Update object position
    void updateObjectMovement()
    {
        constexpr float objectSpeed = 0.05f;
        constexpr float rotateSpeed = 0.01f;

        glm::vec3 forward = glm::vec3(sin(m_meshRotation.y), 0.0f, cos(m_meshRotation.y));

        if (m_window.isKeyPressed(GLFW_KEY_W))
            m_meshPosition += forward * objectSpeed;
        if (m_window.isKeyPressed(GLFW_KEY_S))
            m_meshPosition -= forward * objectSpeed;
        if (m_window.isKeyPressed(GLFW_KEY_A))
            m_meshRotation.y += rotateSpeed;
        if (m_window.isKeyPressed(GLFW_KEY_D))
            m_meshRotation.y -= rotateSpeed;
        if (m_window.isKeyPressed(GLFW_KEY_SPACE))
            m_meshPosition.y += objectSpeed;
        else if (m_meshPosition.y > 1.5f)
            m_meshPosition.y -= objectSpeed;
    }

    void imgui()
    {
        ImGui::Begin("Window");

        const char* viewpoints[] = {"World View", "Object View"};
        ImGui::Combo("Viewpoint", &m_selectedViewpoint, viewpoints, 2);
        if (m_selectedViewpoint == 0)
        {
            m_worldCamera.setUserInteraction(true);
            m_objectCamera.setUserInteraction(false);
            m_activeCamera = &m_worldCamera;
        }
        else
        {
            m_worldCamera.setUserInteraction(false);
            m_objectCamera.setUserInteraction(true);
            m_activeCamera = &m_objectCamera;
        }
        ImGui::Separator();

        ImGui::Text("Lights");
        auto& L = m_lights[m_selectedLight];  // one light for now
        ImGui::DragFloat3("Light pos", &L.position[0], 0.05f);
        ImGui::ColorEdit3("Light color", &L.color[0]);
        ImGui::Separator();

        ImGui::Text("Shading");
        const char* modes[] = {"Default", "Albedo", "Lambert", "Phong", "Blinn-Phong", "PBR"};
        ImGui::Combo("Mode", &m_shadingMode, modes, 6);
        ImGui::Checkbox("Add Diffuse", &m_useDiffuseInSpecular);
        ImGui::Checkbox("Use Environmental Mapping", &m_useEnvironmentalMapping);
        ImGui::Checkbox("Use material if no texture", &m_useMaterial);
        ImGui::Checkbox("Use Shadows", &m_useShadows);

        ImGui::Separator();
        ImGui::Text("Normal Mapping Terrain");
        ImGui::Checkbox("Use Normal Map", &m_useNormalMap);
        ImGui::SliderFloat("Normal Strength", &m_normalStrength, 0.0f, 2.0f);
        ImGui::Checkbox("Flip Normal Y", reinterpret_cast<bool*>(&m_normalFlipY));

        ImGui::Separator();
        ImGui::Text("Terrain");
        ImGui::Checkbox("Wireframe", &m_wire_frame_enabled);
        ImGui::Checkbox("Use Texture", &m_useTexture);

        ImGui::SliderFloat("Tile Size", &m_terrainParameters.tileSize, 1.0f, 50.0f);
        ImGui::SliderInt("Subdivisions", &m_terrainParameters.subdivisions, 1, 10);
        ImGui::SliderInt("Render Distance", &m_terrainParameters.renderDistance, 1, 10);
        ImGui::SliderFloat("Texture Scale", &m_terrainParameters.textureScale, 1.0f, 100.0f);

        if (ImGui::Button("Regenerate Terrain"))
        {
            m_terrain.setParameters(m_terrainParameters);
        }
        ImGui::End();
    }

    void bindAndSetup(Shader& sh, const glm::mat4& mvp, const glm::mat4& model, const glm::mat3& normal)
    {
        sh.bind();
        glUniformMatrix4fv(sh.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(sh.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(sh.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normal));

        const Light& L = m_lights[0];
        glUniform3fv(sh.getUniformLocation("lightPos"), 1, glm::value_ptr(L.position));
        glUniform3fv(sh.getUniformLocation("lightColor"), 1, glm::value_ptr(L.color));

        glUniform1i(sh.getUniformLocation("shadingMode"), m_shadingMode);
        glUniform1i(sh.getUniformLocation("useDiffuse"), m_useDiffuseInSpecular);
        glUniform1i(sh.getUniformLocation("useShadows"), m_useShadows);
        glUniform3fv(sh.getUniformLocation("viewPos"), 1, glm::value_ptr(m_activeCamera->cameraPos()));
    }

   private:
    Window m_window;

    bool m_wire_frame_enabled      = false;
    bool m_useTexture              = true;
    bool m_useEnvironmentalMapping = true;
    bool m_useDiffuseInSpecular    = true;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_litShader;
    Shader m_skyboxShader;
    int    m_shadingMode = 0;
    Shader m_lightShader;

    std::vector<GPUMesh> m_ufoMeshes;
    std::vector<GPUMesh> m_baseMeshes;
    Texture              m_terrainTexture;
    bool                 m_useMaterial{true};
    glm::vec3            m_meshPosition{0.0f, 1.5f, 0.0f};
    glm::vec3            m_meshRotation{0.0f, 0.0f, 0.0f};

    unsigned int m_cubemapTex;
    unsigned int m_skyboxVAO;
    unsigned int m_cubemapVAO;
    float        m_skyboxRotation = 0.0f;

    // Lights
    struct Light
    {
        glm::vec3 position;
        glm::vec3 color;
        bool      is_spotlight;
        glm::vec3 direction;
    };
    std::vector<Light> m_lights        = {{glm::vec3(0.4f, 10.2f, 0.2f), glm::vec3(0.9f, 0.9f, 0.9f)}};
    size_t             m_selectedLight = 0;

    // Viewpoints
    Camera  m_worldCamera;
    Camera  m_objectCamera;
    Camera* m_activeCamera;
    int     m_selectedViewpoint{0};  // 0 = World, 1 = Object

    TerrainParameters m_terrainParameters{100, 50.0f, 5};
    Terrain           m_terrain;

    Texture m_terrainNormal{RESOURCE_ROOT "resources/terrain/Ground050/Ground050_2K-JPG_NormalGL.jpg"};
    bool    m_useNormalMap   = true;
    float   m_normalStrength = 1.0f;
    int     m_normalFlipY    = 0;

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 m_viewMatrix       = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix{1.0f};

    // Shadows
    GLuint m_texShadow;
    GLuint m_framebuffer;
    int    m_shadowWidth = 4096;
    bool   m_useShadows  = true;
};

int main()
{
    Application app;
    app.update();

    return 0;
}
