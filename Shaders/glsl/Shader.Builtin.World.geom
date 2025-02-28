// 内置输出
// varying out vec4 gl_FrontColor;
// varying out vec4 gl_BackColor;
// varying out vec4 gl_FrontSecondaryColor;
// varying out vec4 gl_BackSecondaryColor;
// varying out vec4 gl_TexCoord[]; // at most gl_MaxTextureCoords
// varying out float gl_FogFragCoord;

// 内置输入
// varying in vec4 gl_FrontColorIn[gl_VerticesIn];
// varying in vec4 gl_BackColorIn[gl_VerticesIn];
// varying in vec4 gl_FrontSecondaryColorIn[gl_VerticesIn];
// varying in vec4 gl_BackSecondaryColorIn[gl_VerticesIn];
// varying in vec4 gl_TexCoordIn[gl_VerticesIn][]; // at most will be// gl_MaxTextureCoords
// varying in float gl_FogFragCoordIn[gl_VerticesIn];
// varying in vec4 gl_PositionIn[gl_VerticesIn];
// varying in float gl_PointSizeIn[gl_VerticesIn];
// varying in vec4 gl_ClipVertexIn[gl_VerticesIn];

#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (location = 0) in int in_mode[];
layout (location = 1) in struct in_dto{
	vec2 vTexcoord;
	vec3 vNormal;
	vec4 vAmbientColor;
	vec3 vViewPosition;
	vec3 vFragPosition;
	vec3 vVertPosition;
	vec4 vColor;
	vec4 vTangent;
}InDto[];

layout (location = 0) out int out_mode;
layout (location = 1) out struct out_dto{
	vec2 vTexcoord;
	vec3 vNormal;
	vec4 vAmbientColor;
	vec3 vViewPosition;
	vec3 vFragPosition;
	vec3 vVertPosition;
	vec4 vColor;
	vec4 vTangent;
}OutDto;

out_dto VertexInfo(int index);
out_dto VertexCenterInfo();

// 空操作
//layout (triangles) in;
//layout (triangle_strip, max_vertices = 3) out;
void EmptyGeometryPipe();

// 获取每一个三角面的中心点并与三个顶点连接
//layout (triangles) in;
//layout (triangle_strip, max_vertices = 9) out;
void ComplexGeometryPipe();

void main()
{
	out_mode = in_mode[0];
	
	//ComplexGeometryPipe();
	EmptyGeometryPipe();
}

// ------------------------------ Functions ------------------------------------
void EmptyGeometryPipe(){
	for (int i = 0; i < 3; i++) {
        // 直接传递顶点的位置
        gl_Position = gl_in[i].gl_Position;
		OutDto = VertexInfo(i);
        // 发射顶点
        EmitVertex();
    }

    // 结束当前图元
    EndPrimitive();
}

void ComplexGeometryPipe(){
	vec4 p1 = gl_in[0].gl_Position;
	vec4 p2 = gl_in[1].gl_Position;
	vec4 p3 = gl_in[2].gl_Position;
	
	vec4 center = (p1+p2+p3)*0.33333;
	
	//1
	gl_Position = p1;
	OutDto = VertexInfo(0);
	EmitVertex();	
	
	gl_Position = p2;
	OutDto = VertexInfo(1);
	EmitVertex();
	
	gl_Position = center;
	OutDto = VertexCenterInfo();
	EmitVertex();
	
	EndPrimitive();
		
    //2
	gl_Position = p2;
	OutDto = VertexInfo(1);
	EmitVertex();	
	
	gl_Position = p3;
	OutDto = VertexInfo(2);
	EmitVertex();
	
	gl_Position = center;
	OutDto = VertexCenterInfo();
	EmitVertex();
	
	EndPrimitive();
	
	//3
	gl_Position = center;
	OutDto = VertexCenterInfo();
	EmitVertex();
	
	gl_Position = p3;
	OutDto = VertexInfo(2);
	EmitVertex();
	
	gl_Position = p1;
	OutDto = VertexInfo(0);
	EmitVertex();
	
	EndPrimitive();
}




// -------------------------- Utils ------------------------------------
out_dto VertexInfo(int index){
	out_dto NewOut;
	NewOut.vTexcoord		 = InDto[index].vTexcoord;
	NewOut.vNormal			 = InDto[index].vNormal;
	NewOut.vAmbientColor	 = InDto[index].vAmbientColor;
	NewOut.vViewPosition	 = InDto[index].vViewPosition;
	NewOut.vFragPosition	 = InDto[index].vFragPosition;
	NewOut.vVertPosition	 = InDto[index].vVertPosition;
	NewOut.vColor			 = InDto[index].vColor;
	NewOut.vTangent			 = InDto[index].vTangent;
	return NewOut;
}

out_dto VertexCenterInfo(){
	out_dto NewOut;
	NewOut.vTexcoord		 = (InDto[0].vTexcoord 	  + InDto[1].vTexcoord	   + InDto[2].vTexcoord		   ) * 0.33333;
	NewOut.vNormal			 = (InDto[0].vNormal 	  + InDto[1].vNormal	   + InDto[2].vNormal		   ) * 0.33333;
	NewOut.vAmbientColor	 = (InDto[0].vAmbientColor + InDto[1].vAmbientColor + InDto[2].vAmbientColor   ) * 0.33333;
	NewOut.vViewPosition	 = (InDto[0].vViewPosition + InDto[1].vViewPosition + InDto[2].vViewPosition   ) * 0.33333;
	NewOut.vFragPosition	 = (InDto[0].vFragPosition + InDto[1].vFragPosition + InDto[2].vFragPosition   ) * 0.33333;
	NewOut.vVertPosition	 = (InDto[0].vVertPosition + InDto[1].vVertPosition + InDto[2].vVertPosition   ) * 0.33333;
	NewOut.vColor			 = (InDto[0].vColor 		  + InDto[1].vColor		   + InDto[2].vColor	   ) * 0.33333;
	NewOut.vTangent			 = (InDto[0].vTangent 	  + InDto[1].vTangent	   + InDto[2].vTangent		   ) * 0.33333;
	return NewOut;
}	