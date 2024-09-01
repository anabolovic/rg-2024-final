#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/shader_m.h>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct DirectionalLight {
        glm::vec3 direction;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 catPosition = glm::vec3(0.0f, -3.0f, -9.0f);
    float catRotationAngle = 0.0f; // Add this line for rotation
    float catScale = 0.05f;
    glm::vec3 moonPosition = glm::vec3(2.0f, 3.0f, -10.0f);
    float moonRotationAngle = 0.0f; // Add this line for rotation
    float moonScale = 0.1f;
    glm::vec3 bottomPosition = glm::vec3(0.0,-16.0,0.0);
    float bottomScale = 10.0f;
    PointLight pointLight;
    DirectionalLight directionalLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};



void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader catShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader bottomShader("resources/shaders/ground.vs", "resources/shaders/ground.fs");
    Shader moonShader("resources/shaders/sun.vs", "resources/shaders/sun.fs");

    // load models
    // -----------
    Model catModel("resources/objects/cat/12221_Cat_v1_l3.obj");
    catModel.SetShaderTextureNamePrefix("material.");

    Model bottomModel("resources/objects/ground/terrain.obj");
    bottomModel.SetShaderTextureNamePrefix("material.");

    Model moonModel("resources/objects/sun/Sun.obj");
    moonModel.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(0.0f, 10.0f, 0.0f);  // Closer to the cat model
    pointLight.ambient = glm::vec3(0.5f, 0.5f, 0.5f);
    pointLight.diffuse = glm::vec3(1.0f, 0.9f, 0.7f);
    pointLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.0f;
    pointLight.quadratic = 0.0f;

    DirectionalLight& directionalLight = programState->directionalLight;
    directionalLight.direction = glm::vec3(-10.0f, -5.0f, -2.0f);
    directionalLight.ambient = glm::vec3(0.3f, 0.3f, 0.3f);  // Increased ambient light
    directionalLight.diffuse = glm::vec3(1.0f, 0.9f, 0.7f);  // Increased diffuse light
    directionalLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);  // Increased specular light

    //blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //face culling

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));

        // don't forget to enable shader before setting uniforms
        catShader.use();
        pointLight.position = glm::vec3(3.0f, -3.0f, 0.0f);
        catShader.setVec3("pointLight.position", pointLight.position);
        catShader.setVec3("pointLight.ambient", pointLight.ambient);
        catShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        catShader.setVec3("pointLight.specular", pointLight.specular);
        catShader.setFloat("pointLight.constant", pointLight.constant);
        catShader.setFloat("pointLight.linear", pointLight.linear);
        catShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        catShader.setVec3("viewPosition", programState->camera.Position);
        catShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        catShader.setMat4("projection", projection);
        catShader.setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        float r = 2.0f;
        auto rotateVec = glm::vec3(glm::cos(programState->catRotationAngle), 0.0f, glm::sin(programState->catRotationAngle));
        model = glm::translate(model,
                               programState->catPosition + r * rotateVec);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(programState->catScale));    // it's a bit too big for our scene, so scale it down
        catShader.setMat4("model", model);
        catModel.Draw(catShader);

        moonShader.use();
        // pointLight.position = glm::vec3(3.0f, -3.0f, 0.0f);
        moonShader.setVec3("pointLight.position", pointLight.position);
        moonShader.setVec3("pointLight.ambient", pointLight.ambient);
        moonShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        moonShader.setVec3("pointLight.specular", pointLight.specular);
        moonShader.setFloat("pointLight.constant", pointLight.constant);
        moonShader.setFloat("pointLight.linear", pointLight.linear);
        moonShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        // moonShader.setVec3("directionalLight.direction",glm::vec3(-1.0f,-0.5f,-1.0f));
        // moonShader.setVec3("directionalLight.ambient",glm::vec3(0.1f,0.1f,0.1f));
        // moonShader.setVec3("directionalLight.diffuse",glm::vec3(0.9f,0.7f,0.5f));
        // moonShader.setVec3("directionalLight.specular",glm::vec3(0.05f,0.05f,0.05f));
        moonShader.setVec3("viewPosition", programState->camera.Position);
        moonShader.setFloat("material.shininess", 32.0f);
        moonShader.setMat4("projection", projection);
        moonShader.setMat4("view", view);

        // render the loaded model
        glm::mat4 moonMatrix = glm::mat4(1.0f);
        moonMatrix = glm::translate(moonMatrix, programState->moonPosition); // translate it down so it's at the center of the scene
        moonMatrix = glm::rotate(moonMatrix, glm::radians(programState->moonRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        moonMatrix = glm::scale(moonMatrix, glm::vec3(programState->moonScale)); // it's a bit too big for our scene, so scale it down
        moonShader.setMat4("model", moonMatrix);
        moonModel.Draw(moonShader);
        programState->moonRotationAngle += 10.0f * deltaTime;

        bottomShader.use();
        bottomShader.setVec3("directionalLight.direction",glm::vec3(-1.0f,-0.5f,-1.0f));
        bottomShader.setVec3("directionalLight.ambient",glm::vec3(0.1f,0.1f,0.1f));
        bottomShader.setVec3("directionalLight.diffuse",glm::vec3(0.9f,0.7f,0.5f));
        bottomShader.setVec3("directionalLight.specular",glm::vec3(0.05f,0.05f,0.05f));

        bottomShader.setVec3("viewPosition", programState->camera.Position);
        bottomShader.setFloat("material.shininess", 32.0f);

        bottomShader.setMat4("projection", projection);
        bottomShader.setMat4("view", view);

        unsigned int specularTextureID = TextureFromFile("specular.png", "resources/objects/ground");

        glActiveTexture(GL_TEXTURE0);
        bottomShader.setInt("material.texture_diffuse1", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularTextureID);
        bottomShader.setInt("material.texture_specular1", 1);


        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(programState->bottomPosition));
        //model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        //model = glm::rotate(model, glm::radians(-50.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model,glm::vec3(programState->bottomScale));
        bottomShader.setMat4("model", model);
        bottomModel.Draw(bottomShader);

        if (programState->ImGuiEnabled)
//            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
//    ImGui_ImplOpenGL3_Shutdown();
//    ImGui_ImplGlfw_Shutdown();
//    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        programState->catPosition.z += 0.1f * deltaTime;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->catPosition);
        ImGui::DragFloat3("Point light position", (float*)&programState->pointLight.position);
        ImGui::DragFloat("Backpack scale", &programState->catScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (key == GLFW_KEY_R && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        programState->catRotationAngle -= 0.1f;  // Move forward along the z-axis
    }
}
