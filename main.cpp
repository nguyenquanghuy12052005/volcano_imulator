#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Buffers
GLuint VAO, VBO[2];
std::vector<float> vertices;
std::vector<float> normals;
GLuint shaderProgram;

GLuint uTimeLocation;

// Phép chiếu
bool isPerspective = true;

// Góc xoay vật thể (Mouse và Phím mũi tên)
float rotationX = 0.0f; // Góc xoay quanh trục X 
float rotationY = 0.0f; // Góc xoay quanh trục Y

// Vị trí quan sát (Camera) (Phím W,A,S,D,R,F)
float eyeX = 0.0f, eyeY = 1.0f, eyeZ = 4.0f; // Vị trí mắt nhìn 
float centerX = 0.0f, centerY = -0.5f, centerZ = 0.0f; // Điểm nhìn tới (nhìn vào chân núi)
float upX = 0.0f, upY = 1.0f, upZ = 0.0f; // Hướng "lên" của camera

// Phóng to / Thu nhỏ (Scroll và phím +,-)
float zoom = 1.0f; // Hệ số zoom (sẽ dùng làm glScale)

// Trạng thái (Phím 'M')
bool isWireframe = false; // true = khung dây, false = mặt đa giác

// Camera
bool mousePressed = false;
double lastX, lastY;

// Vector 3D cơ bản
struct Vec3 {
    float x, y, z;
};

Vec3 normalize(Vec3 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len > 0.0f) {
        return {v.x / len, v.y / len, v.z / len};
    }
    return v;
}

Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float dot(Vec3 a, Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

//  Ma trận 4x4 cơ bản
struct Matrix4x4 {
    float m[16];
    Matrix4x4() {
        for(int i=0;i<16;i++) m[i]=0;
        m[0]=m[5]=m[10]=m[15]=1.0f;
    }
};
Matrix4x4 multiply(const Matrix4x4 &a,const Matrix4x4 &b){
    Matrix4x4 r;
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            r.m[i*4+j] = a.m[i*4+0]*b.m[0*4+j] + a.m[i*4+1]*b.m[1*4+j]
                        + a.m[i*4+2]*b.m[2*4+j] + a.m[i*4+3]*b.m[3*4+j];
        }
    }
    return r;
}
Matrix4x4 rotateXY(float ax,float ay){
    Matrix4x4 rx, ry;
    float cx=cosf(ax), sx=sinf(ax);
    rx.m[5]=cx; rx.m[6]=-sx; rx.m[9]=sx; rx.m[10]=cx;
    float cy=cosf(ay), sy=sinf(ay);
    ry.m[0]=cy; ry.m[2]=sy; ry.m[8]=-sy; ry.m[10]=cy;
    return multiply(ry, rx);
}

Matrix4x4 scale(float sx,float sy,float sz){
    Matrix4x4 s;
    s.m[0]=sx; s.m[5]=sy; s.m[10]=sz;
    return s;
}

Matrix4x4 translate(float tx,float ty,float tz){
    Matrix4x4 t;
    t.m[12]=tx; t.m[13]=ty; t.m[14]=tz;
    return t;
}

Matrix4x4 ortho(float left, float right, float bottom, float top, float nearVal, float farVal) {
    Matrix4x4 r;
    r.m[0] = 2.0f / (right - left);
    r.m[5] = 2.0f / (top - bottom);
    r.m[10] = -2.0f / (farVal - nearVal);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(farVal + nearVal) / (farVal - nearVal);
    r.m[15] = 1.0f;
    return r;
}

Matrix4x4 perspective(float fovy, float aspect, float nearVal, float farVal) {
    Matrix4x4 r;
    float f = 1.0f / tanf(fovy / 2.0f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (farVal + nearVal) / (nearVal - farVal);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * farVal * nearVal) / (nearVal - farVal);
    r.m[15] = 0.0f; // Quan trọng: m[15] phải là 0
    return r;
}

Matrix4x4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = normalize({center.x - eye.x, center.y - eye.y, center.z - eye.z});
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);

    Matrix4x4 r;
    r.m[0] = s.x;
    r.m[1] = u.x;
    r.m[2] = -f.x;
    r.m[3] = 0;

    r.m[4] = s.y;
    r.m[5] = u.y;
    r.m[6] = -f.y;
    r.m[7] = 0;

    r.m[8] = s.z;
    r.m[9] = u.z;
    r.m[10] = -f.z;
    r.m[11] = 0;

    r.m[12] = -dot(s, eye);
    r.m[13] = -dot(u, eye);
    r.m[14] = dot(f, eye);
    r.m[15] = 1;
    return r;
}

// Noise đơn giản
float simpleNoise(float x,float z,float freq){
    return (sinf(x*freq)*cosf(z*freq))*0.2f;
}

// Tính normal cho 3 đỉnh
void computeNormal(const float* v0,const float* v1,const float* v2,float* outNormal){
    float U[3]={v1[0]-v0[0],v1[1]-v0[1],v1[2]-v0[2]};
    float V[3]={v2[0]-v0[0],v2[1]-v0[1],v2[2]-v0[2]};
    outNormal[0]=U[1]*V[2]-U[2]*V[1];
    outNormal[1]=U[2]*V[0]-U[0]*V[2];
    outNormal[2]=U[0]*V[1]-U[1]*V[0];
    float len = sqrtf(outNormal[0]*outNormal[0]+outNormal[1]*outNormal[1]+outNormal[2]*outNormal[2]);
    if(len>0){ outNormal[0]/=len; outNormal[1]/=len; outNormal[2]/=len;}
}

// Thêm tam giác và normal
void addTriangle(float* v0,float* v1,float* v2){
    vertices.insert(vertices.end(),{v0[0],v0[1],v0[2], v1[0],v1[1],v1[2], v2[0],v2[1],v2[2]});
    float n[3]; computeNormal(v0,v1,v2,n);
    for(int i=0;i<3;i++) normals.insert(normals.end(),{n[0],n[1],n[2]});
}

// Tạo núi lửa chi tiết
void createDetailedVolcano(){
    srand(time(0));
    const int BASE_SEGMENTS=64;
    const int HEIGHT_SEGMENTS=8;
    const float BASE_RADIUS=2.0f;
    const float CRATER_RADIUS=0.3f;
    const float VOLCANO_HEIGHT=2.5f;
    const float CRATER_DEPTH=0.4f;

    // Thân núi
    for(int layer=0;layer<HEIGHT_SEGMENTS;layer++){
        float h0=(VOLCANO_HEIGHT/HEIGHT_SEGMENTS)*layer;
        float h1=(VOLCANO_HEIGHT/HEIGHT_SEGMENTS)*(layer+1);
        float r0=BASE_RADIUS-(BASE_RADIUS-CRATER_RADIUS)*(h0/VOLCANO_HEIGHT);
        float r1=BASE_RADIUS-(BASE_RADIUS-CRATER_RADIUS)*(h1/VOLCANO_HEIGHT);
        for(int i=0;i<BASE_SEGMENTS;i++){
            float a0=2.0f*M_PI*i/BASE_SEGMENTS;
            float a1=2.0f*M_PI*(i+1)/BASE_SEGMENTS;
            float n0=1.0f+simpleNoise(cosf(a0),sinf(a0),3.0f+layer);
            float n1=1.0f+simpleNoise(cosf(a1),sinf(a1),3.0f+layer);
            float n2=1.0f+simpleNoise(cosf(a0),sinf(a0),3.0f+layer+1);
            float n3=1.0f+simpleNoise(cosf(a1),sinf(a1),3.0f+layer+1);

            float v0[3]={r0*n0*cosf(a0),h0,r0*n0*sinf(a0)};
            float v1[3]={r1*n3*cosf(a1),h1,r1*n3*sinf(a1)};
            float v2[3]={r0*n1*cosf(a1),h0,r0*n1*sinf(a1)};
            addTriangle(v0,v1,v2);

            float v3[3]={r0*n0*cosf(a0),h0,r0*n0*sinf(a0)};
            float v4[3]={r1*n2*cosf(a0),h1,r1*n2*sinf(a0)};
            float v5[3]={r1*n3*cosf(a1),h1,r1*n3*sinf(a1)};
            addTriangle(v3,v4,v5);
        }
    }

    // Đáy núi
    for(int i=0;i<BASE_SEGMENTS;i++){
        float a0=2.0f*M_PI*i/BASE_SEGMENTS;
        float a1=2.0f*M_PI*(i+1)/BASE_SEGMENTS;
        float v0[3]={0,0,0};
        float v1[3]={BASE_RADIUS*cosf(a0),0,BASE_RADIUS*sinf(a0)};
        float v2[3]={BASE_RADIUS*cosf(a1),0,BASE_RADIUS*sinf(a1)};
        addTriangle(v0,v1,v2);
    }

    // Miệng núi
    const int CRATER_SEGMENTS=32;
    float cTop=VOLCANO_HEIGHT;
    float cBot=VOLCANO_HEIGHT-CRATER_DEPTH;
    for(int i=0;i<CRATER_SEGMENTS;i++){
        float a0=2.0f*M_PI*i/CRATER_SEGMENTS;
        float a1=2.0f*M_PI*(i+1)/CRATER_SEGMENTS;
        float v0[3]={CRATER_RADIUS*cosf(a0),cTop,CRATER_RADIUS*sinf(a0)};
        float v1[3]={CRATER_RADIUS*cosf(a1),cTop,CRATER_RADIUS*sinf(a1)};
        float v2[3]={CRATER_RADIUS*0.8f*cosf(a0),cBot,CRATER_RADIUS*0.8f*sinf(a0)};
        addTriangle(v0,v1,v2);

        float v3[3]={CRATER_RADIUS*cosf(a1),cTop,CRATER_RADIUS*sinf(a1)};
        float v4[3]={CRATER_RADIUS*0.8f*cosf(a1),cBot,CRATER_RADIUS*0.8f*sinf(a1)};
        float v5[3]={CRATER_RADIUS*0.8f*cosf(a0),cBot,CRATER_RADIUS*0.8f*sinf(a0)};
        addTriangle(v3,v4,v5);
    }

    // Đáy miệng (dung nham)
    for(int i=0;i<CRATER_SEGMENTS;i++){
        float a0=2.0f*M_PI*i/CRATER_SEGMENTS;
        float a1=2.0f*M_PI*(i+1)/CRATER_SEGMENTS;
        float v0[3]={0,cBot,0};
        float v1[3]={CRATER_RADIUS*0.8f*cosf(a0),cBot,CRATER_RADIUS*0.8f*sinf(a0)};
        float v2[3]={CRATER_RADIUS*0.8f*cosf(a1),cBot,CRATER_RADIUS*0.8f*sinf(a1)};
        addTriangle(v0,v1,v2);
    }

    std::cout<<"Tạo núi lửa xong: "<<vertices.size()/3<<" đỉnh\n";
}

// Vertex shader
const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uTransform;
out vec3 vNormal;
out vec3 vPos;
void main(){
    vPos = aPos;
    vNormal = aNormal;
    gl_Position = uTransform*vec4(aPos,1.0);
}
)";

// Fragment shader
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vNormal;
in vec3 vPos;          
out vec4 FragColor;
uniform float u_time;

vec3 getVolcanoColor(float height){
    vec3 deepBrown = vec3(0.3,0.15,0.05);
    vec3 earthBrown = vec3(0.5,0.25,0.1);
    vec3 rockGray = vec3(0.4,0.35,0.3);
    vec3 volcanicOrange = vec3(0.8,0.3,0.1);
    vec3 lavaRed = vec3(1.0,0.2,0.05);
    if(height<0.5) return mix(deepBrown,earthBrown,height*2.0);
    else if(height<1.2) return mix(earthBrown,rockGray,(height-0.5)/0.7);
    else if(height<2.0) return mix(rockGray,volcanicOrange,(height-1.2)/0.8);
    else return mix(volcanicOrange,lavaRed,(height-2.0)/0.5);
}

void main(){
    if (vPos.y < -0.005) { 
        FragColor = vec4(1.0, 0.4, 0.0, 1.0);
        return; // Thoát ra ngay
    }

    vec3 lightDir = normalize(vec3(1.0,2.0,1.0));
    float diff = max(dot(normalize(vNormal),lightDir),0.36);
    vec3 baseColor = getVolcanoColor(vPos.y);
    float staticNoise = (sin(vPos.x*10.0+vPos.z*10.0)*0.3+0.7);
    float pulse = (sin(u_time * 4.0) * 0.5 + 0.5);
    float lavaGlow = (vPos.y > 2.1) ? (staticNoise * pulse) : 0.0;
    baseColor += vec3(0.6, 0.2, 0.0) * lavaGlow;
    vec4 finalColor = vec4(baseColor * diff, 1.0);
    
    FragColor = finalColor;
}
)";

// Compile shader
GLuint compileShader(){
    GLuint vs=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vertexShaderSource,NULL);
    glCompileShader(vs);
    GLint success; char info[512];
    glGetShaderiv(vs,GL_COMPILE_STATUS,&success);
    if(!success){glGetShaderInfoLog(vs,512,NULL,info);std::cout<<"Vertex Shader Error:\n"<<info<<std::endl;}
    GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fragmentShaderSource,NULL);
    glCompileShader(fs);
    glGetShaderiv(fs,GL_COMPILE_STATUS,&success);
    if(!success){glGetShaderInfoLog(fs,512,NULL,info);std::cout<<"Fragment Shader Error:\n"<<info<<std::endl;}
    GLuint prog=glCreateProgram();
    glAttachShader(prog,vs);glAttachShader(prog,fs);glLinkProgram(prog);
    glGetProgramiv(prog,GL_LINK_STATUS,&success);
    if(!success){glGetProgramInfoLog(prog,512,NULL,info);std::cout<<"Shader Program Link Error:\n"<<info<<std::endl;}
    glDeleteShader(vs);glDeleteShader(fs);
    return prog;
}

// Setup buffers
void setupBuffers(){
    glGenVertexArrays(1,&VAO);
    glGenBuffers(2,VBO);
    glBindVertexArray(VAO);

    // Vertex
    glBindBuffer(GL_ARRAY_BUFFER,VBO[0]);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnableVertexAttribArray(0);

    // Normal
    glBindBuffer(GL_ARRAY_BUFFER,VBO[1]);
    glBufferData(GL_ARRAY_BUFFER,normals.size()*sizeof(float),normals.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,(void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// Input callbacks
void mouseButtonCallback(GLFWwindow* w,int b,int a,int m){
    if(b==GLFW_MOUSE_BUTTON_LEFT){
        mousePressed=(a==GLFW_PRESS);
        if(mousePressed) glfwGetCursorPos(w,&lastX,&lastY);
    }
}
void cursorPosCallback(GLFWwindow* w,double x,double y){
    if(mousePressed){
        rotationY += (x-lastX)*0.01f;
        rotationX += (y-lastY)*0.01f;
        if(rotationX>1.5f) rotationX=1.5f;
        if(rotationX<-1.5f) rotationX=-1.5f;
        lastX=x; lastY=y;
    }
}
void scrollCallback(GLFWwindow* w,double xoffset,double yoffset){
    zoom += yoffset*0.1f;
    if(zoom<0.2f) zoom=0.2f;
    if(zoom>3.0f) zoom=3.0f;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        //  'P' để chuyển đổi phép chiếu
        if (key == GLFW_KEY_P)
        {
            isPerspective = !isPerspective;
            std::cout << "Phep chieu: " << (isPerspective ? "Phoi Canh" : "Song Song") << std::endl;
        }

        //  'M' để chuyển đổi chế độ vẽ
        if (key == GLFW_KEY_M)
        {
            isWireframe = !isWireframe;
            std::cout << "Che do: " << (isWireframe ? "Khung Day (Wireframe)" : "Mat Da Giac (Solid)") << std::endl;
        }

        // Thoát
        if (key == GLFW_KEY_ESCAPE)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }
}

//  Xử lý các phím giữ (Di chuyển) 
void processInput(GLFWwindow *window)
{
    float moveSpeed = 0.03f;  
    float rotSpeed = 0.03f;   
    float zoomSpeed = 0.05f;  

    //  Di chuyển camera (W,A,S,D,R,F)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) eyeZ -= moveSpeed; // Đi tới
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) eyeZ += moveSpeed; // Lùi lại
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) eyeX -= moveSpeed; // Sang trái
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) eyeX += moveSpeed; // Sang phải
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) eyeY += moveSpeed; // Lên trên
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) eyeY -= moveSpeed; // Xuống dưới

    // Xoay vật thể (Các phím mũi tên)
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) rotationX += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) rotationX -= rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) rotationY += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) rotationY -= rotSpeed;

    //  Zoom (Phím +/-)
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) zoom += zoomSpeed; // Phím + (dấu =)
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) zoom -= zoomSpeed; // Phím -
    
    // Giữ giới hạn zoom
    if(zoom<0.1f) zoom=0.1f;
    if(zoom>5.0f) zoom=5.0f;
    
    // Giữ giới hạn xoay
    if(rotationX>1.5f) rotationX=1.5f;
    if(rotationX<-1.5f) rotationX=-1.5f;
}


void createLavaPlane() {
    std::cout << "Tao mat phang dung nham..." << std::endl;
    
    // Kích thước của mặt phẳng (rất lớn)
    const float PLANE_SIZE = 40.0f;
    // Đặt Y thấp hơn chân núi lửa một chút (chân núi ở Y=0)
    const float PLANE_Y = -0.01f; 

    // Bốn đỉnh của hình vuông
    float v0[3] = { -PLANE_SIZE, PLANE_Y, -PLANE_SIZE };
    float v1[3] = {  PLANE_SIZE, PLANE_Y, -PLANE_SIZE };
    float v2[3] = {  PLANE_SIZE, PLANE_Y,  PLANE_SIZE };
    float v3[3] = { -PLANE_SIZE, PLANE_Y,  PLANE_SIZE };
    
    // Normal (pháp tuyến) cho cả mặt phẳng (luôn trỏ lên trên)
    float n[3] = { 0.0f, 1.0f, 0.0f };

    // Thêm 2 tam giác
    
    // Tam giác 1: v0, v1, v2
    vertices.insert(vertices.end(), {v0[0], v0[1], v0[2]});
    vertices.insert(vertices.end(), {v1[0], v1[1], v1[2]});
    vertices.insert(vertices.end(), {v2[0], v2[1], v2[2]});
    // Thêm 3 normal
    normals.insert(normals.end(), {n[0], n[1], n[2]});
    normals.insert(normals.end(), {n[0], n[1], n[2]});
    normals.insert(normals.end(), {n[0], n[1], n[2]});

    // Tam giác 2: v0, v2, v3
    vertices.insert(vertices.end(), {v0[0], v0[1], v0[2]});
    vertices.insert(vertices.end(), {v2[0], v2[1], v2[2]});
    vertices.insert(vertices.end(), {v3[0], v3[1], v3[2]});
    // Thêm 3 normal
    normals.insert(normals.end(), {n[0], n[1], n[2]});
    normals.insert(normals.end(), {n[0], n[1], n[2]});
    normals.insert(normals.end(), {n[0], n[1], n[2]});
}

int main(){
    std::cout<<"Khởi tạo núi lửa 3D..."<<std::endl;
    if(!glfwInit()){std::cout<<"GLFW init error\n";return -1;}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Nui Lua 3D - Full Controls",NULL,NULL);
    if(!window){std::cout<<"Window creation error\n";glfwTerminate();return -1;}
    glfwMakeContextCurrent(window);
    if(glewInit()!=GLEW_OK){std::cout<<"GLEW init error\n";return -1;}
    std::cout<<"OpenGL version: "<<glGetString(GL_VERSION)<<std::endl;

    createDetailedVolcano();
    createLavaPlane();
    setupBuffers();
    shaderProgram = compileShader();

    uTimeLocation = glGetUniformLocation(shaderProgram, "u_time");

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback); 

    glEnable(GL_DEPTH_TEST);

    while(!glfwWindowShouldClose(window)){
        processInput(window);

        if (isWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        
        glClearColor(0.7f,0.9f,1.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform1f(uTimeLocation, (float)glfwGetTime());
        glBindVertexArray(VAO);


        // MODEL Matrix (Xoay và Phóng to vật thể)
        Matrix4x4 rotMat = rotateXY(rotationX, rotationY);
        Matrix4x4 scaleMat = scale(1.0f, 1.0f, 1.0f);
        // Dịch chuyển núi lửa xuống 1 chút để tâm xoay ở gốc 0,0,0
        Matrix4x4 transMat = translate(0.0f, -0.5f, 0.0f); 
        Matrix4x4 modelMat = multiply(transMat, multiply(rotMat, scaleMat));

        // VIEW Matrix (Camera)
        Matrix4x4 viewMat = lookAt(
            {eyeX, eyeY, eyeZ}, 
            {centerX, centerY, centerZ}, 
            {upX, upY, upZ}
        );

        // PROJECTION Matrix (Phép chiếu)
        Matrix4x4 projMat;
        float ratio = (float)SCR_WIDTH / (float)SCR_HEIGHT;

        float nearVal = 0.01f; // Giảm near plane để đỡ bị cắt khi lại gần
        float farVal = 100.0f;

        if (isPerspective) {
            // Phép chiếu phối cảnh
            // Áp dụng zoom bằng cách thay đổi FOV (góc nhìn)
            // 45 độ là FOV cơ bản. Chia cho zoom để thu hẹp góc nhìn
            float fovy_radians = (45.0f / zoom) * M_PI / 180.0f; 
            
            // Giới hạn fov để tránh bị lật hình (quá zoom)
            if (fovy_radians < 0.01f) fovy_radians = 0.01f; 
            if (fovy_radians > 3.1f) fovy_radians = 3.1f; // ~178 độ

            projMat = perspective(fovy_radians, ratio, nearVal, farVal);
        } else {
            // Phép chiếu song song
            // Áp dụng zoom bằng cách thu hẹp/mở rộng khung nhìn
            float left = -2.0f * ratio / zoom;
            float right = 2.0f * ratio / zoom;
            float bottom = -2.0f / zoom;
            float top = 2.0f / zoom;
            
            projMat = ortho(left, right, bottom, top, nearVal, farVal);
        }

        // FINAL Matrix (M * V * P)
        Matrix4x4 finalMat = multiply(modelMat, multiply(viewMat, projMat));

        // Gửi ma trận lên Shader
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"uTransform"),1,GL_FALSE, finalMat.m);
        
        //  Vẽ vật thể
        glDrawArrays(GL_TRIANGLES,0,vertices.size()/3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(2,VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    std::cout<<"Thoát chương trình.\n";
    return 0;
}
