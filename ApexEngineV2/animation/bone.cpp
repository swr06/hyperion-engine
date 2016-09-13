#include "bone.h"
#include "../math/matrix_util.h"

namespace apex {
Bone::Bone(const std::string &name)
    : Entity(name)
{
}

void Bone::ClearPose()
{
    pose_pos = Vector3::Zero();
    pose_rot = Quaternion::Identity();

    SetTransformUpdateFlag();
}

void Bone::ApplyPose(const Keyframe &pose)
{
    current_pose = pose;

    pose_pos = pose.GetTranslation();
    pose_rot = pose.GetRotation();

    SetTransformUpdateFlag();
}

const Keyframe &Bone::GetCurrentPose() const
{
    return current_pose;
}

void Bone::StoreBindingPose()
{
    inv_bind_pos = global_bone_pos * -1;
    inv_bind_rot = global_bone_rot;
    inv_bind_rot.Invert();
}

void Bone::SetToBindingPose()
{
    local_rotation = Quaternion::Identity();
    local_translation = Vector3::Zero();

    pose_pos = bind_pos;
    pose_rot = bind_rot;

    SetTransformUpdateFlag();
}

const Vector3 &Bone::CalcBindingTranslation()
{
    global_bone_pos = bind_pos;

    Bone *parent_bone;
    if (parent && (parent_bone = dynamic_cast<Bone*>(parent))) {
        global_bone_pos = parent_bone->global_bone_rot * bind_pos;
        global_bone_pos += parent_bone->global_bone_pos;
    }

    for (auto &&child : children) {
        auto child_bone = std::dynamic_pointer_cast<Bone>(child);
        if (child_bone) {
            child_bone->CalcBindingTranslation();
        }
    }

    return global_bone_pos;
}

const Quaternion &Bone::CalcBindingRotation()
{
    global_bone_rot = bind_rot;

    Bone *parent_bone;
    if (parent && (parent_bone = dynamic_cast<Bone*>(parent))) {
        global_bone_rot = parent_bone->global_bone_rot * bind_rot;
    }

    for (auto &&child : children) {
        auto child_bone = std::dynamic_pointer_cast<Bone>(child);
        if (child_bone) {
            child_bone->CalcBindingRotation();
        }
    }

    return global_bone_rot;
}

const Matrix4 &Bone::GetBoneMatrix() const
{
    return bone_matrix;
}

void Bone::UpdateTransform()
{
    Vector3 tmp_pos = global_bone_pos * -1;

    Matrix4 rot_matrix;
    MatrixUtil::ToTranslation(rot_matrix, tmp_pos);

    Quaternion tmp_rot = global_bone_rot * pose_rot * user_rot * inv_bind_rot;

    Matrix4 tmp_matrix;
    MatrixUtil::ToRotation(tmp_matrix, tmp_rot);
    rot_matrix *= tmp_matrix;

    tmp_pos *= -1;
    MatrixUtil::ToTranslation(tmp_matrix, tmp_pos);
    rot_matrix *= tmp_matrix;

    MatrixUtil::ToTranslation(tmp_matrix, pose_pos);
    rot_matrix *= tmp_matrix;

    MatrixUtil::ToTranslation(tmp_matrix, user_pos);
    rot_matrix *= tmp_matrix;

    bone_matrix = rot_matrix;

    Bone *parent_bone;
    if (parent != nullptr) {
        parent_bone = dynamic_cast<Bone*>(parent);
        if (parent_bone != nullptr) {
            bone_matrix *= parent_bone->bone_matrix;
        }
    }

    local_rotation = bind_rot * pose_rot * user_rot;
    local_translation = bind_pos + pose_pos + user_pos;

    Entity::UpdateTransform();
}
}