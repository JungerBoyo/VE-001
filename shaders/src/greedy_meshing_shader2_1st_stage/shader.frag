#version 450 core

precision mediump float;

layout(location = 0) out vec4 out_fragment;

uniform usampler3D volume;

ivec2 getSliceSpacePosition() {
	// return ivec2(uint(gl_FragCoord.x) % 64, uint(gl_FragCoord.y) % 64)
	return ivec2(uint(gl_FragCoord.x) & 0x3F, uint(gl_FragCoord.y) & 0x3F)
}
int getAxisSpacePosition() {
	// ivec2 pos = ivec2((uint(gl_FragCoord.x) / 64) % 8, (uint(gl_FragCoord.y) / 64) % 8);
	ivec2 pos = ivec2((uint(gl_FragCoord.x) >> 6) & 0x7, (uint(gl_FragCoord.y) >> 6) & 0x7);
	// return pos.x + (pos.y * 8);	
	return pos.x + (pos.y << 3);	
}
ivec2 getAxisIdPolarity() {
	//ivec2 pos = ivec2((uint(gl_FragCoord.x) / 512), (uint(gl_FragCoord.y) / 512));
	ivec2 pos = ivec2((uint(gl_FragCoord.x) >> 9), (uint(gl_FragCoord.y) >> 9));
	int flattened_pos = pos.x + pos.y * 3;
	return ivec2(flattened_pos >> 1, 1 + (flattened_pos & 0x1) * -2);
}
//         |---1536----| (24*64)
//       - *---*---*---*
//       | | X |-X | Y |
//    1024 *---*---*---*
//(16*64)| |-Y | Z |-Z |
//       - *---*---*---*
//
// X:   |--------------512--------------|
//    - *---*---*---*---*---*---*---*---*
//    | | 0 | 1 | 2 |...|   |   |   |   |
//    | *---*---*---*---*---*---*---*---*
//    | | 8 | 9 |...|   |   |   |   |   |
//    | *---*---*---*---*---*---*---*---*
//    | |...|   |   |   |   |   |   |   |
//    | *---*---*---*---*---*---*---*---*
//    | |   |   |   |   |   |   |   |   |
//  512 *---*---*---*---*---*---*---*---*
//    | |   |   |   |   |   |   |   |   |
//    | *---*---*---*---*---*---*---*---*
//    | |   |   |   |   |   |   |   |   |
//    | *---*---*---*---*---*---*---*---*
//    | |   |   |   |   |   |   |   |   |
//    | *---*---*---*---*---*---*---*---*
//    | |   |   |   |   |   |   |...| 63|
//    - *---*---*---*---*---*---*---*---*
const ivec3 VOLUME_MAPPING_PER_AXIS[3] = {
	ivec3(2, 1, 0),
	ivec3(0, 2, 1),
	ivec3(0, 1, 2)
}
const ivec3 OFFSET_PER_AXIS[3] = {
	ivec3(1, 0, 0),
	ivec3(0, 1, 0),
	ivec3(0, 0, 1)
}

struct FragmentMetadata {
	ivec3 position;
	int polarity;
	int axis_id;
};
FragmentMetadata getFragmentMetadata() {
	ivec2 slice_space_pos = getSliceSpacePosition();
	int axis_space_pos = getAxisSpacePosition();
	ivec2 axis_id_polarity = getAxisIdPolarity();	
	
	ivec3 mapping = VOLUME_MAPPING_PER_AXIS[axis_id_polarity.x];
	
	ivec3 p = ivec3(slice_space_pos, axis_space_pos);

	FragmentMetadata metadata;
	metadata.position = ivec3(p[mapping[0]], p[mapping[1]], p[mapping[2]]);
	metadata.polarity = axis_id_polarity.y;
	metadata.axis_id = axis_id_polarity.x;
	
	return metadata;
}

uint getNeighbour(FragmentMetadata frag_metadata, vec3 vec) {
	ivec3 mapping = VOLUME_MAPPING_PER_AXIS[frag_metadata.axis_id];
	vec3 real_vec(vec[mapping[0]], vec[mapping[1]], vec[mapping[2]]);
	vec3 neighbour_pos = frag_metadata.position + real_vec;
	return texelFetch(volume, neighbour_pos, 0).r
}

const uint LUT[128] = {
		
};

void main() {
	ivec4 frag_metadata = getFragmentMetadata();

	uint voxel = texelFetch(volume, frag_metadata.position, 0).r;
	if (voxel == 0 || getNeighbour(frag_metadata, vec3(0, 0, frag_metadata.polarity)) > 0) {
		fragment = vec4(0);
		return;
	}

	uint lut_index 
	lut_index |= ((uint)(getNeighbour(frag_metadata, vec3(-1, -1, 0)) == voxel) << 0;
	lut_index |= ((uint)(getNeighbour(frag_metadata, vec3( 0, -1, 0)) == voxel) << 1;
	lut_index |= ((uint)(getNeighbour(frag_metadata, vec3( 1, -1, 0)) == voxel) << 2;
	lut_index |= ((uint)(getNeighbour(frag_metadata, vec3(-1,  0, 0)) == voxel) << 3;
	lut_index |= ((uint)(getNeighbour(frag_metadata, vec3( 1,  0, 0)) == voxel) << 4;
	lut_index |= ((uint)(getNeighbour(frag_metadata, vec3(-1,  1, 0)) == voxel) << 5;
	lut_index |= ((uint)(getNeighbour(frag_metadata, vec3( 0,  1, 0)) == voxel) << 6;
	lut_index |= ((uint)(getNeighbour(frag_metadata, vec3( 1,  1, 0)) == voxel) << 7;
	
	
}
