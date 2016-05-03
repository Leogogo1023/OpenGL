
attribute vec4 vPosition;

attribute vec4 vNormal;

attribute vec2 uv;

uniform mat4 Transform;
uniform float uv_factor;
uniform vec4 light_position;
uniform vec4 l_dif, m_dif, l_ab, m_ab, l_sp, m_sp;
uniform float m_shininess;
uniform vec4 viewer;
uniform float mode, type;

varying vec4 _vPosition;
varying vec2 _uv;
varying float _uv_factor;
varying vec4 _light_position;
varying vec4 _vNormal;
varying vec4 _l_dif, _m_dif, _l_ab, _m_ab, _l_sp, _m_sp;
varying float  _m_shininess;
varying vec4 _viewer;
varying float _mode, _type;

void main()
{
    _light_position = light_position;
    _uv = uv;
    _uv_factor = uv_factor;
    _vPosition = vPosition;
    _vNormal = vNormal;
    _l_dif = l_dif;
    _m_dif = m_dif;
    _l_ab = l_ab;
    _m_ab = m_ab;
    _l_sp = l_sp;
    _m_sp = m_sp;
    _m_shininess= m_shininess;
    _viewer = viewer;
    _mode = mode;
    _type = type;

    gl_Position = Transform * vPosition;
} 
