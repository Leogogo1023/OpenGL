// Very simple display triangle program, that allows you to rotate the
// triangle around the Y axis.
//
// This program does NOT use a vertex shader to define the vertex colors.
// Instead, it computes the colors in the display callback (using Blinn/Phong)
// and passes those color values, one per vertex, to the vertex shader, which
// passes them directly to the fragment shader. This achieves what is called
// "gouraud shading".

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>
#endif


#include "amath.h"
#include <vector>
#include <iostream> 
#include <fstream>
#include <sstream>
#include <math.h>
#include <ctype.h>

#define PI 3.14159265

bool objFile;
bool refresh = true;
GLuint trans_loc, viewer_loc, uv_loc, mode_loc, obj_type_loc;
float maxY, minY;
static int lastx = 0;
static int lasty = 0;

typedef amath::vec4  point4;
typedef amath::vec4  color4;

float theta = 0.0;  // rotation around the Y (up) axis
float alpha = 0.0;
float posx = 0.0;   // translation along X
float posy = 0.0;   // translation along Y

GLuint buffers[2];
GLint matrix_loc;

std::vector<point4> vertices;
std::vector<point4> normals;
std::vector<point4> u_normals;
std::vector<vec2> uv;

// target to look at
vec4 target = vec4(0.0, 0.0, 0.0, 1.0);

vec4 up = vec4(0.0, 1.0, 0.0, 0.0);

// viewer's position, for lighting calculations
vec4 viewer = vec4(0.0, 0.0, 5.0, 1.0);
vec4 cur_viewer;

// light & material definitions, again for lighting calculations:
point4 light_position = point4(100.0, 100.0, 100.0, 1.0);
color4 light_ambient = color4(0.2, 0.2, 0.2, 1.0);
color4 light_diffuse = color4(1.0, 1.0, 1.0, 1.0);
color4 light_specular = color4(1.0, 1.0, 1.0, 1.0);

color4 material_ambient = color4(1.0, 0.0, 1.0, 1.0);
color4 material_diffuse = color4(1.0, 0.8, 0.0, 1.0);
color4 material_specular = color4(1.0, 0.8, 0.0, 1.0);
float material_shininess = 100.0;


// a transformation matrix, for the rotation, which we will apply to every
// vertex:
mat4 ctm;

int factor=2;        // sampling resolution
int uv_factor = 2;  // checkerboard resolution
float mode = 1.0;  // 1.0 for bling-phong, -1.0 for checkerboard
float obj_type;  // 1.0 for txt file, -1.0 for obj file


GLuint program; //shaders



std::vector<point4> point_list;
std::vector<int> surface_num;
std::vector<int> degree;


std::string remove_spaces(const std::string &s)
{
    int last = (int)s.size() - 1;
    while (last >= 0 && s[last] == ' ')
        --last;
    return s.substr(0, last + 1);
}

int count_middle_space(const std::string &s){
    int count = 0;
    for(int i=0; i<s.size(); i++){
        if(s[i] == ' ')
            count++;
    }
    return count;
}

void read_surface_file(const char *file,  std::vector<point4>& point_list,  std::vector<int>& degree)
{
    
    std::cout << "read_surfacefile" << std::endl;
    
    std::ifstream in(file);
    char buffer[1025];
    std::string cmd;
    
    for (int line=1; in.good(); line++) {
        int count=0;
        in.getline(buffer,1024);
        buffer[in.gcount()]=0;
        
        cmd="";
        
        std::istringstream iss (buffer);
        cmd = iss.str();
        cmd = remove_spaces(cmd);
        count = count_middle_space(cmd);
        if ( isalpha(cmd[0]) or cmd.empty()) {
            continue;
        }
        else if (count ==0 ){
            int num;
            iss >>num;
            surface_num.push_back(num);
        }
        else if (count == 1) {
            int u,v;
            iss >> u >> v;
            degree.insert(degree.begin(),u);
            degree.insert(degree.begin()+1,v);
        }
        else if (count >= 2){
            for(int i=0; i<degree[0]+1; i++){
                double x, y, z;
                iss >>x >>y >> z;
                point4 p(x,y,z,1);
                if(i == 0)
                    point_list.insert(point_list.begin(), p);
                else
                    point_list.insert(point_list.begin()+i,p);
            }
        }
        else {
            std::cerr << "Parser error: invalid command at line " << line << std::endl;
        }
        
    }
    
    in.close();
    
}

// for each array
void eval_bez(const std::vector<point4> controlpoints, int degree, const double t, point4 &pnt, vec4 &tangent){
    std:: vector<point4> tempPoints;
    unsigned long size = controlpoints.size();
    for(int i=0; i<size;i++){
        tempPoints.push_back(controlpoints[i]);
    }
    for(int n=0; n<=degree-2; n++){
        if( n == degree -2){
            tangent = (tempPoints[1] - tempPoints[0])*(degree-1);
        }
        for(int i=0; i<=size-2-n; i++){
            point4 temp = tempPoints[i]*(1-t) + tempPoints[i+1]*t;
            tempPoints[i] = temp;
        }
    }
    pnt[0] = tempPoints[0][0];
    pnt[1] = tempPoints[0][1];
    pnt[2] = tempPoints[0][2];
    pnt[3] = 1.0;
}

// will call eval_bez row by row
void start_Row(const std::vector<point4> row_points, const int u_degree, const int v_degree, const double u, const double v, point4& col_pnt, vec4 &col_tangent){
    std::vector<point4> temp_points;
    std::vector<point4> array;

    for(int i = 0; i<row_points.size(); i+=u_degree){
        point4 temp;
        vec4 tangent;
        array.clear();
        for(int j =i; j<i+u_degree; j++){
            array.push_back(row_points[j]);
        }
        eval_bez(array, u_degree, u, temp, tangent);
        temp_points.push_back(temp);
    }
    eval_bez(temp_points, v_degree, v, col_pnt, col_tangent);
}

// will call eval_bez col by col
void start_Col(const std::vector<point4> col_points, const int u_degree, const int v_degree, const double u, const double v, point4& row_pnt, vec4 &row_tangent){
    std::vector<point4> temp_points;
    std::vector<point4> array;
    
    for(int i = 0; i<col_points.size(); i+=v_degree){
        point4 temp;
        vec4 tangent;
        array.clear();
        for(int j =i; j<i+v_degree; j++){
            array.push_back(col_points[j]);
        }
        eval_bez(array, v_degree, v, temp, tangent);
        temp_points.push_back(temp);
    }
    eval_bez(temp_points, u_degree, u, row_pnt, row_tangent);
    
}

std::vector<point4> samplePoint;
std::vector<vec4> sampleNormal;

// should return a sample point and its normal
void sample(const std::vector<point4> point_list, const std::vector<int> degree, const double u, const double v, std::vector<point4> &samplePoint, std::vector<vec4> &sampleNormal){
    int curMinIndex = 0, curMaxIndex=0, j=0;
    for(int i=0; i<degree.size(); i=i+2){
        point4 row_pnt, col_pnt;
        vec4 row_tangent, col_tangent;
        std::vector<point4> row_points,col_points;
        
        int u_degree = degree[i]+1;
        int v_degree = degree[i+1]+1;
        
        curMaxIndex += u_degree * v_degree;
    
        for(j = curMinIndex; j<curMaxIndex; j++){
            row_points.push_back(point_list[j]);
        }
        for(j = curMinIndex; j<curMinIndex+u_degree; j++){
            for(int m = j; m<curMaxIndex; m+=u_degree)
                col_points.push_back(point_list[m]);
        }
        curMinIndex = curMaxIndex;
        
        start_Row(row_points, u_degree, v_degree, u, v, col_pnt, col_tangent);
        start_Col(col_points, v_degree, v_degree, u, v, row_pnt, row_tangent);

        samplePoint.push_back(row_pnt);
        vec3 c = normalize(cross(col_tangent, row_tangent));
        vec4 n(c[0],c[1],c[2],0);
        sampleNormal.push_back(n);
    }
}




std::vector<float> v_list;
std::vector<int > tris_list;

void read_wavefront_file (
                          const char *file,
                          std::vector< int > &tris,
                          std::vector< float > &verts)
{
    
    std::cout << "read_wavefile" << std::endl;
    // clear out the tris and verts vectors:
    tris.clear ();
    verts.clear ();
    
    std::ifstream in(file);
    char buffer[1025];
    std::string cmd;
    
    
    for (int line=1; in.good(); line++) {
        in.getline(buffer,1024);
        buffer[in.gcount()]=0;
        
        cmd="";
        
        std::istringstream iss (buffer);
        
        iss >> cmd;
        
        if (cmd[0]=='#' or cmd.empty()) {
            // ignore comments or blank lines
            continue;
        }
        else if (cmd=="v") {
            // got a vertex:
            
            // read in the parameters:
            double pa, pb, pc;
            iss >> pa >> pb >> pc;
            
            verts.push_back (pa);
            verts.push_back (pb);
            verts.push_back (pc);
        }
        else if (cmd=="f") {
            // got a face (triangle)
            
            // read in the parameters:
            int i, j, k;
            iss >> i >> j >> k;
            
            // vertex numbers in OBJ files start with 1, but in C++ array
            // indices start with 0, so we're shifting everything down by
            // 1
            tris.push_back (i-1);
            tris.push_back (j-1);
            tris.push_back (k-1);
        }
        else {
            std::cerr << "Parser error: invalid command at line " << line << std::endl;
        }
        
    }
    
    in.close();
    
}

void init_vertices(){
    // fill with zeros
    vec4 zero(0,0,0,0);
    for(unsigned int i=0; i<v_list.size()/3; i++){
        u_normals.push_back(zero);
    }
    
    // init vertices
    for(unsigned int i=0; i<tris_list.size()/3;i++){
        float t_ax = v_list[3*tris_list[3*i]];
        float t_ay = v_list[3*tris_list[3*i]+1];
        float t_az = v_list[3*tris_list[3*i]+2];
        float t_bx = v_list[3*tris_list[3*i+1]];
        float t_by = v_list[3*tris_list[3*i+1]+1];
        float t_bz = v_list[3*tris_list[3*i+1]+2];
        float t_cx = v_list[3*tris_list[3*i+2]];
        float t_cy = v_list[3*tris_list[3*i+2]+1];
        float t_cz = v_list[3*tris_list[3*i+2]+2];
        point4 p1(t_ax, t_ay, t_az, 1.0);
        point4 p2(t_bx, t_by, t_bz, 1.0);
        point4 p3(t_cx, t_cy, t_cz, 1.0);
        vertices.push_back(p1);
        vertices.push_back(p2);
        vertices.push_back(p3);
        
        vec3 n1 = normalize(cross(p2 - p1, p3 - p2));
        vec4 n = vec4(n1[0], n1[1], n1[2], 0.0);
        u_normals[tris_list[3*i]] += n;
        u_normals[tris_list[3*i+1]] += n;
        u_normals[tris_list[3*i+2]] += n;
    }
    // normalize
    for(unsigned int i=0; i<u_normals.size();i++){
        u_normals[i] = normalize(u_normals[i]);
    }
    
    for(unsigned int i=0; i<tris_list.size()/3;i++){
        normals.push_back(u_normals[tris_list[3*i]]);
        normals.push_back(u_normals[tris_list[3*i+1]]);
        normals.push_back(u_normals[tris_list[3*i+2]]);
    }
    
}









// product of components, which we will use for shading calculations:
vec4 product(vec4 a, vec4 b)
{
    return vec4(a[0]*b[0], a[1]*b[1], a[2]*b[2], a[3]*b[3]);
}


// initialization: set up a Vertex Array Object (VAO) and then
void init()
{
    
    // create a vertex array object - this defines memory that is stored
    // directly on the GPU
    GLuint vao;
    
    // depending on which version of the mac OS you have, you may have to do this:
#ifdef __APPLE__
    glGenVertexArraysAPPLE( 1, &vao );  // give us 1 VAO:
    glBindVertexArrayAPPLE( vao );      // make it active
#else
    glGenVertexArrays( 1, &vao );   // give us 1 VAO:
    glBindVertexArray( vao );       // make it active
#endif
    
    // set up vertex buffer object - this will be memory on the GPU where
    // we are going to store our vertex data (that is currently in the "points"
    // array)
    glGenBuffers(1, buffers);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);  // make it active
    
    program = InitShader("vshader_passthrough.glsl", "fshader_passthrough.glsl");
    
    // ...and set them to be active
    glUseProgram(program);
    
    if(objFile){
        unsigned long size = vertices.size()*sizeof(point4);
        std::cout << "vertice size: " << vertices.size()<< std::endl;
        
        glBufferData(GL_ARRAY_BUFFER, 2*size, NULL, GL_STATIC_DRAW);
        
        GLuint loc, loc2, loc3;
        loc = glGetAttribLocation(program, "vPosition");
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glBufferSubData( GL_ARRAY_BUFFER, 0, size, &vertices[0] );
        
        
        loc2 = glGetAttribLocation(program, "vNormal");
        glEnableVertexAttribArray(loc2);
        glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(size));
        glBufferSubData( GL_ARRAY_BUFFER, size, size, &normals[0] );
        
        loc3 = glGetAttribLocation(program, "uv");
        glEnableVertexAttribArray(loc3);
        glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(2*size));
        glBufferSubData( GL_ARRAY_BUFFER, 2*size, size/2, &normals[0] );   // for obj file, uv will not be used. Here is just for compilable 

    }
    
    trans_loc = glGetUniformLocation(program, "Transform");
    viewer_loc = glGetUniformLocation(program, "viewer");
    uv_loc = glGetUniformLocation(program, "uv_factor");
    mode_loc = glGetUniformLocation(program, "mode");
    obj_type_loc = glGetUniformLocation(program, "type");
    

    
    GLuint light_loc = glGetUniformLocation(program, "light_position");
    glUniform4f(light_loc, light_position[0],light_position[1],light_position[2], light_position[3]);

    
    vec4 l_dif = light_diffuse;
    GLuint l_dif_loc = glGetUniformLocation(program, "l_dif");
    glUniform4f(l_dif_loc, l_dif[0], l_dif[1], l_dif[2], l_dif[3]);
    
    vec4 m_dif = material_diffuse;
    GLuint m_dif_loc = glGetUniformLocation(program, "m_dif");
    glUniform4f(m_dif_loc, m_dif[0], m_dif[1], m_dif[2], m_dif[3]);
    
    vec4 l_ab = light_ambient;
    GLuint l_ab_loc = glGetUniformLocation(program, "l_ab");
    glUniform4f(l_ab_loc, l_ab[0], l_ab[1], l_ab[2], l_ab[3]);
    
    vec4 m_ab = material_ambient;
    GLuint m_ab_loc = glGetUniformLocation(program, "m_ab");
    glUniform4f(m_ab_loc, m_ab[0], m_ab[1], m_ab[2], m_ab[3]);
    
    vec4 l_sp = light_specular;
    GLuint l_sp_loc = glGetUniformLocation(program, "l_sp");
    glUniform4f(l_sp_loc, l_sp[0], l_sp[1], l_sp[2], l_sp[3]);
    
    vec4 m_sp = material_specular;
    GLuint m_sp_loc = glGetUniformLocation(program, "m_sp");
    glUniform4f(m_sp_loc, m_sp[0], m_sp[1], m_sp[2], m_sp[3]);
    

    GLuint m_shininess_loc = glGetUniformLocation(program, "m_shininess");
    glUniform1f(m_shininess_loc, material_shininess);
    
    
    // set the background color (white)
    glClearColor(1.0, 1.0, 1.0, 1.0); 
}


void display( void )
{
    // only when we change sample rate should this rate be recalculated
    if(refresh && !objFile){
    //    std::cout << "factor: " << factor << std::endl;
        int index = 0;
        vertices.clear();
        normals.clear();
        uv.clear();
        for(int m =0 ; m <degree.size(); m+=2){
            std::vector<int> cur_degree;
            std::vector<point4> cur_points;
            int cur_size = (degree[m]+1)*(degree[m+1]+1);
            cur_points.clear();
            cur_degree.clear();
            samplePoint.clear();
            sampleNormal.clear();
            
            cur_degree.push_back(degree[m]);
            cur_degree.push_back(degree[m+1]);
            
            for(int n=index; n< index + cur_size; n++){
                cur_points.push_back(point_list[n]);
            }
            index += cur_size;
            double colSample = factor * (degree[m+1]+1);
            double rowSample = factor * (degree[m]+1);
            for(int i = 0 ; i < colSample; i++){
                for(int j = 0; j < rowSample; j++){
                    sample(cur_points, cur_degree, j/rowSample, i/colSample, samplePoint, sampleNormal);
                    sample(cur_points, cur_degree, (j+1)/rowSample, i/colSample, samplePoint, sampleNormal);
                    sample(cur_points, cur_degree, (j+1)/rowSample, (i+1)/colSample, samplePoint, sampleNormal);
                    sample(cur_points, cur_degree, j/rowSample, i/colSample, samplePoint, sampleNormal);
                    sample(cur_points, cur_degree, (j+1)/rowSample, (i+1)/colSample, samplePoint, sampleNormal);
                    sample(cur_points, cur_degree, j/rowSample, (i+1)/colSample, samplePoint, sampleNormal);
                    uv.push_back(vec2(j/rowSample, i/colSample));
                    uv.push_back(vec2((j+1)/rowSample, i/colSample));
                    uv.push_back(vec2((j+1)/rowSample, (i+1)/colSample));
                    uv.push_back(vec2(j/rowSample, i/colSample));
                    uv.push_back(vec2((j+1)/rowSample, (i+1)/colSample));
                    uv.push_back(vec2(j/rowSample, (i+1)/colSample));
                }
            }
            for(int i = 0; i<samplePoint.size(); i++){
                vertices.push_back(samplePoint[i]);
                normals.push_back(sampleNormal[i]);
            }
            
        }
        
        unsigned long size = vertices.size()*sizeof(point4);
  //      std::cout << "vertice size: " << vertices.size()<< std::endl;
    
        glBufferData(GL_ARRAY_BUFFER, 3*size, NULL, GL_STATIC_DRAW);
        
        GLuint loc, loc2, loc3;
        loc = glGetAttribLocation(program, "vPosition");
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glBufferSubData( GL_ARRAY_BUFFER, 0, size, &vertices[0] );
        
        
        loc2 = glGetAttribLocation(program, "vNormal");
        glEnableVertexAttribArray(loc2);
        glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(size));
        glBufferSubData( GL_ARRAY_BUFFER, size, size, &normals[0] );

        
        loc3 = glGetAttribLocation(program, "uv");
        glEnableVertexAttribArray(loc3);
        glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(2*size));
        glBufferSubData( GL_ARRAY_BUFFER, 2*size, size/2, &uv[0] );
        refresh = false;
        
    }
    
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    ctm =  RotateY(theta) * RotateX(-alpha);
    
    cur_viewer = ctm * viewer;
    mat4 lookAt = LookAt( cur_viewer, target, up);
    mat4 perspecitve = Perspective(40.0f, 1.0f, 0.1f, 100.0f);
    
    
    glUniformMatrix4fv(trans_loc, 1, GL_TRUE, perspecitve * lookAt);
    glUniform4f(viewer_loc, cur_viewer[0], cur_viewer[1],cur_viewer[2],cur_viewer[3]);
    glUniform1f(uv_loc, uv_factor);
    glUniform1f(mode_loc, mode);
    glUniform1f(obj_type_loc, obj_type);
    // draw the VAO:
    
    glDrawArrays(GL_TRIANGLES, 0, (int)vertices.size());
    
    // move the buffer we drew into to the screen, and give us access to the one
    // that was there before:
    glutSwapBuffers();
    
}



void mouse_move_rotate (int x, int y)
{
    
    int amntX = x - lastx;
    if (amntX != 0) {
            theta += amntX;
        if (theta > 360.0 ) theta -= 360.0;
        if (theta < 0.0 ) theta += 360.0;
        
         lastx = x;
    }
    
    int amntY = y - lasty;
    alpha += amntY;
    if(alpha > 85 or alpha < -85)
        alpha -= amntY;
        lasty = y;
    
    // force the display routine to be called as soon as possible:
    glutPostRedisplay();
    
}

void mouse_click(int button, int state, int x, int y ){
    if(button == GLUT_LEFT_BUTTON){
        lastx = x;
        lasty = y;
    }
}



// the keyboard callback, called whenever the user types something with the
// regular keys.
void mykey(unsigned char key, int mousex, int mousey)
{
    if(key=='q'||key=='Q') exit(0);
    
    // and r resets the view:
    if (key =='r') {
        posx = 0;
        posy = 0;
        theta = 0;
        alpha = 0;
        factor = 2;
        uv_factor = 2;
        mode = 1.0;
        glutPostRedisplay();
    }
    
    if(key == 'z'){
        vec4 move = normalize(viewer - target);
        move = vec4(move[0], move[1],move[2],0);
        if(length(viewer - target) >= 3)
            viewer -=move;
        glutPostRedisplay();

    }
    if(key == 'x'){
        vec4 move = normalize(viewer - target);
        move = vec4(move[0], move[1],move[2],0);
        if(length(viewer - target) <= 49)
            viewer += move;
        glutPostRedisplay();
        
    }
    
    if(key == '<'){
        if(factor>=2){
                factor--;
                refresh = true;
        }
        glutPostRedisplay();
    }
    
    if(key == '>'){
        if(factor <= 9){
            factor++;
            refresh = true;
        }
        glutPostRedisplay();
    }
    
    if(key == 't'){
        if(uv_factor <= 9){
            uv_factor++;
        }
        glutPostRedisplay();
    }
    
    if(key == 'g'){
        if(uv_factor >= 2){
            uv_factor--;
        }
        glutPostRedisplay();
    }
    
    if(key == 's'){
        mode = -mode;
        glutPostRedisplay();
    }
}



int main(int argc, char** argv)
{
    
    std::cout << "How to use: " << std::endl;
    std::cout << "Press z and x to zoom in and out" <<std::endl;
    std::cout << "Press s to change mode (bling-phong <-> checkerboard)" <<std::endl;
    std::cout << "Press t and f to adjust checkerboard resolution" << std::endl;
    std::cout << "Press > and < to adjust sampling rate" << std:: endl;
    std::cout << "press r to reset" << std::endl;
    std::cout << " " << std::endl;
    

    std::string file = argv[1];
    unsigned long size = file.size();
    std::string type = file.substr(size-3,size);
    if(type.compare("txt") == 0)
    {
       objFile = false;
       obj_type = 1.0;
       read_surface_file(argv[1], point_list, degree);
    }
    else{
        objFile = true;
        obj_type = -1.0;
        read_wavefront_file(argv[1], tris_list, v_list);
        init_vertices();
    }
    
    // initialize glut, and set the display modes
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    
    // give us a window in which to display, and set its title:
    glutInitWindowSize(512, 512);
    glutCreateWindow("OpenGl Hw, UNI: ll2992");
    
    // for displaying things, here is the callback specification:
    glutDisplayFunc(display);
    
    
    // when the mouse is moved, call this function!
    // you can change this to mouse_move_translate to see how it works
    glutMotionFunc(mouse_move_rotate);
    glutMouseFunc(mouse_click);
   // glutMotionFunc(mouse_move_translate);
    // for any keyboard activity, here is the callback:
    glutKeyboardFunc(mykey);
    
#ifndef __APPLE__
    // initialize the extension manager: sometimes needed, sometimes not!
    glewInit();
//    glewExperimental = GL_TRUE;
#endif

    // call the init() function, defined above:
    init();
    
    // enable the z-buffer for hidden surface removel:
    glEnable(GL_DEPTH_TEST);

    // once we call this, we no longer have control except through the callbacks:
    glutMainLoop();
    return 0;
}
