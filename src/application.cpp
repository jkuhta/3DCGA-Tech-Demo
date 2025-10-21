// #include "Image.h"
#include "mesh.h"
#include "texture.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>

#include "camera.h"
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
          m_texture(RESOURCE_ROOT "resources/checkerboard.png"),
          m_planeMesh(createPlaneMesh(20.0f, 20.0f, 10)),
          m_worldCamera(&m_window, glm::vec3(1.2f, 1.1f, 0.9f), -glm::vec3(1.2f, 0.6f, 0.9f)),
          m_objectCamera(&m_window, glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
          m_activeCamera(&m_worldCamera)

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

        m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/ufo.obj", true);

        Mesh planeMesh = createPlaneMesh(10.0f, 10.0f, 10);
        m_planeMesh    = GPUMesh(planeMesh);

        m_objectCamera.setFollowTarget(&m_meshPosition, glm::vec3(0.0f, 1.0f, 3.0f));

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

            ShaderBuilder lambertBuilder;
            lambertBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            lambertBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lambert_frag.glsl");
            m_lambert = lambertBuilder.build();

            ShaderBuilder phongBuilder;
            phongBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            phongBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/phong_frag.glsl");
            m_phong = phongBuilder.build();

            ShaderBuilder blinnbuilder;
            blinnbuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            blinnbuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blinn_phong_frag.glsl");
            m_blinn = blinnbuilder.build();

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
    }

    void update()
    {
        int dummyInteger = 0;  // Initialized to 0
        while (!m_window.shouldClose())
        {
            // This is your game loop

            m_window.updateInput();

            m_worldCamera.updateInput();

            if (m_selectedViewpoint == 1)
                updateObjectMovement();

            // Use ImGui for easy input/output of ints, floats, strings, etc...
            imgui();

            // Clear the screen
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ...
            glEnable(GL_DEPTH_TEST);

            glm::mat4 viewMatrix = m_activeCamera->viewMatrix();

            for (GPUMesh& mesh : m_meshes)
            {
                glm::mat4       modelMatrix       = glm::translate(m_modelMatrix, m_meshPosition);
                const glm::mat4 mvpMatrix         = m_projectionMatrix * viewMatrix * modelMatrix;
                const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));

                Shader* typeShader = (m_shadingMode == 0)   ? &m_defaultShader
                                     : (m_shadingMode == 1) ? &m_lambert
                                     : (m_shadingMode == 2) ? &m_phong
                                                            : &m_blinn;

                typeShader->bind();

                glUniformMatrix4fv(typeShader->getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                glUniformMatrix4fv(typeShader->getUniformLocation("modelMatrix"), 1, GL_FALSE,
                                   glm::value_ptr(modelMatrix));
                glUniformMatrix3fv(typeShader->getUniformLocation("normalModelMatrix"), 1, GL_FALSE,
                                   glm::value_ptr(normalModelMatrix));

                switch (m_shadingMode)
                {
                    case 0:
                    {  // Unlit
                        if (mesh.hasTextureCoords())
                        {
                            m_texture.bind(GL_TEXTURE0);
                            glUniform1i(typeShader->getUniformLocation("colorMap"), 0);
                            glUniform1i(typeShader->getUniformLocation("hasTexCoords"), GL_TRUE);
                            glUniform1i(typeShader->getUniformLocation("useMaterial"), GL_FALSE);
                        }
                        else
                        {
                            glUniform1i(typeShader->getUniformLocation("hasTexCoords"), GL_FALSE);
                            glUniform1i(typeShader->getUniformLocation("useMaterial"), m_useMaterial);
                        }
                        break;
                    }
                    case 1:
                    {  // Lambert
                        const Light& L = m_lights[0];
                        glUniform3fv(typeShader->getUniformLocation("lightPos"), 1, glm::value_ptr(L.position));
                        glUniform3fv(typeShader->getUniformLocation("lightColor"), 1, glm::value_ptr(L.color));
                        glUniform3fv(typeShader->getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                        break;
                    }
                    case 2:
                    {  // Phong
                        const Light& L = m_lights[0];
                        glUniform3fv(typeShader->getUniformLocation("lightPos"), 1, glm::value_ptr(L.position));
                        glUniform3fv(typeShader->getUniformLocation("lightColor"), 1, glm::value_ptr(L.color));
                        glUniform3fv(typeShader->getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                        glUniform3fv(typeShader->getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                        glUniform1f(typeShader->getUniformLocation("shininess"), shadingData.shininess);
                        glUniform3fv(typeShader->getUniformLocation("viewPos"), 1,
                                     glm::value_ptr(m_activeCamera->cameraPos()));
                        break;
                    }
                    case 3:
                    {  // Blinn
                        const Light& L = m_lights[0];
                        glUniform3fv(typeShader->getUniformLocation("lightPos"), 1, glm::value_ptr(L.position));
                        glUniform3fv(typeShader->getUniformLocation("lightColor"), 1, glm::value_ptr(L.color));
                        glUniform3fv(typeShader->getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                        glUniform1f(typeShader->getUniformLocation("shininess"), shadingData.shininess);
                        glUniform3fv(typeShader->getUniformLocation("viewPos"), 1,
                                     glm::value_ptr(m_activeCamera->cameraPos()));
                        break;
                    }
                    default:
                        break;
                }

                if (mesh.hasTextureCoords())
                {
                    m_texture.bind(GL_TEXTURE0);
                    glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
                    glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                    glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
                }
                else
                {
                    glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
                }
                mesh.draw(m_defaultShader);
            }

            // Render the main plane
            {
                const glm::mat4 mvpMatrix         = m_projectionMatrix * viewMatrix * m_modelMatrix;
                const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

                m_defaultShader.bind();
                glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE,
                                   glm::value_ptr(mvpMatrix));
                glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE,
                                   glm::value_ptr(normalModelMatrix));
                glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);

                {
                    glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
                }

                m_planeMesh.draw(m_defaultShader);
            }

            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
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
        if (m_window.isKeyPressed(GLFW_KEY_W))
            m_meshPosition.z -= objectSpeed;
        if (m_window.isKeyPressed(GLFW_KEY_S))
            m_meshPosition.z += objectSpeed;
        if (m_window.isKeyPressed(GLFW_KEY_A))
            m_meshPosition.x -= objectSpeed;
        if (m_window.isKeyPressed(GLFW_KEY_D))
            m_meshPosition.x += objectSpeed;
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
        const char* modes[] = {"Default", "Lambert", "Phong", "Blinn-Phong"};

        auto& L = m_lights[m_selectedLight];  // one light for now
        ImGui::DragFloat3("Light pos", &L.position[0], 0.05f);
        ImGui::ColorEdit3("Light color", &L.color[0]);
        ImGui::Separator();
        ImGui::Combo("Shading", &m_shadingMode, modes, 4);
        ImGui::ColorEdit3("Kd", &shadingData.kd[0]);
        ImGui::ColorEdit3("Ks", &shadingData.ks[0]);
        ImGui::SliderFloat("Shininess", &shadingData.shininess, 1.0f, 128.0f);
        ImGui::SliderFloat("Roughness", &shadingData.roughness, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Checkbox("Use material if no texture", &m_useMaterial);
        ImGui::End();
    }

   private:
    Window m_window;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_lambert, m_phong, m_blinn;
    int    m_shadingMode = 0;

    std::vector<GPUMesh> m_meshes;
    Texture              m_texture;
    bool                 m_useMaterial{true};
    glm::vec3            m_meshPosition{0.0f, 0.5f, 0.0f};

    GPUMesh m_planeMesh;

    // Lights
    struct Light
    {
        glm::vec3 position;
        glm::vec3 color;
    };
    std::vector<Light> m_lights        = {{glm::vec3(0.4f, 1.2f, 0.2f), glm::vec3(0.9f, 0.9f, 0.9f)}};
    size_t             m_selectedLight = 0;

    // Viewpoints
    Camera  m_worldCamera;
    Camera  m_objectCamera;
    Camera* m_activeCamera;
    int     m_selectedViewpoint{0};  // 0 = World, 1 = Object

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix       = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix{1.0f};

    // Shading parameters
    struct
    {
        // Diffuse (Lambert)
        glm::vec3 kd{0.5f};
        // Specular (Phong/Blinn Phong)
        glm::vec3 ks{0.5f};
        float     shininess = 3.0f;
        float     roughness = 0.5f;
        // Toon
        int   toonDiscretize        = 4;
        float toonSpecularThreshold = 0.49f;

    } shadingData;
};

int main()
{
    Application app;
    app.update();

    return 0;
}
