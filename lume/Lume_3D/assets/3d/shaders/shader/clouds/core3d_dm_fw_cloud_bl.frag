#version 460 core

#extension GL_EXT_nonuniform_qualifier : require

#define CORE3D_DM_BINDLESS_FRAG_LAYOUT 1
#include "core3d_dm_fw_cloud_frag.h"

void main(void)
{
    cloudMain();
}
