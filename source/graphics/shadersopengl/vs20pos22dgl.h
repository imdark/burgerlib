/***************************************

	This file was generated by the
	stripcomments tool

***************************************/

const char g_vs20pos22dgl[] = 
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"uniform vec4 XYWidthHeight;\n"
"const vec4 Adjustments = vec4(2.0,-2.0,-1.0,1.0);\n"
"VERTEX_INPUT vec2 Position;\n"
"void main(void) {\n"
"gl_Position = vec4(XYWidthHeight.xy*Adjustments.xy + Adjustments.zw + Position*XYWidthHeight.zw,0,1.0);\n"
"}\n";
