// the simplest fragment shader: get the color (from the vertex shader -
// actually interpolated from values specified in the vertex shader - and
// just pass them through to the render:
// 
// on the mac, you may need to say "varying vec4 color;" instead of this:

varying vec4 _vPosition;
varying vec2 _uv;

varying vec4 _light_position;
varying vec4 _vNormal;
varying vec4 _l_dif, _m_dif, _l_ab, _m_ab, _l_sp, _m_sp;
varying float _m_shininess;
varying vec4 _viewer;
varying float _uv_factor;
varying float _mode, _type;

void main() 
{
    vec4 ambient_color, diffuse_color, specular_color;
    vec4 _l_dot_m_dif, _l_dot_m_ab, _l_dot_m_sp;
    
    
    // in glsl, when comparing float, _mode == 1.0 is wrong! tricky!
    if(_mode > 0.0){
        _l_dot_m_dif = _l_dif * _m_dif;
        _l_dot_m_ab = _l_ab * _m_ab;
        _l_dot_m_sp = _l_sp * _m_sp;
    }
    else{
        float x_cor, y_cor,z_cor, c_cor;
        
        // .txt file, U-V mapping
        if(_type > 0.0){
            x_cor = mod(float(floor(_uv.x * _uv_factor)),2.0);
            y_cor = mod(float(floor(_uv.y * _uv_factor)),2.0);
            c_cor = mod((x_cor + y_cor),2.0);
            
        }
        // .obj file, world-space mapping
        else{
            x_cor = mod(float(abs(floor(_vPosition.x * _uv_factor/ 0.5))), 2.0);
            y_cor = mod(float(abs(floor(_vPosition.y * _uv_factor/ 0.5))), 2.0);
            z_cor = mod(float(abs(floor(_vPosition.z * _uv_factor/ 0.5))), 2.0);
            c_cor = mod((x_cor + y_cor + z_cor),2.0);
        }
        if(c_cor > 0.0){
            _l_dot_m_dif = _l_dif * vec4(1, 1, 1, 1.0);
            _l_dot_m_ab = _l_ab * vec4(1.0, 1.0, 1.0, 1.0);
            _l_dot_m_sp = _l_sp * vec4(1, 1, 1, 1.0);
        }
        else {
            _l_dot_m_dif = _l_dif * vec4(0, 0, 0, 1.0);
            _l_dot_m_ab = _l_ab * vec4(0.0, 0.0, 0.0, 1.0);
            _l_dot_m_sp = _l_sp * vec4(0, 0, 0, 1.0);
        }
        
    }
    
    
    float dd = max(0.0, dot( normalize(_light_position - _vPosition), _vNormal));
    if(dd>0.0) diffuse_color = dd * _l_dot_m_dif;
    else diffuse_color =  vec4(0.0, 0.0, 0.0, 1.0);
    
    ambient_color = _l_dot_m_ab;
    
    vec4 vhalf = normalize(normalize(_light_position - _vPosition) + normalize(_viewer - _vPosition));
    dd = max(0.0, dot(vhalf, _vNormal));
    if(dd > 0.0) specular_color = pow(dd, _m_shininess) * _l_dot_m_sp;
    else specular_color = vec4(0.0, 0.0, 0.0, 1.0);
 
    
    gl_FragColor = specular_color + ambient_color + diffuse_color;
} 

