#include "pch.h"
#include "msCommon.h"

static const int msProtocolVersion = 103;


namespace ms {
namespace {

template<class T>
struct ssize_impl
{
    uint32_t operator()(const T&) { return sizeof(T); }
};
template<class T>
struct write_impl
{
    void operator()(std::ostream& os, const T& v)
    {
        os.write((const char*)&v, sizeof(T));
    }
};
template<class T>
struct read_impl
{
    void operator()(std::istream& is, T& v)
    {
        is.read((char*)&v, sizeof(T));
    }
};

#define DefSpecialize(T)\
    template<> struct ssize_impl<T> { uint32_t operator()(const T& v) { return v.getSerializeSize(); } };\
    template<> struct write_impl<T> { void operator()(std::ostream& os, const T& v) { return v.serialize(os); } };\
    template<> struct read_impl<T>  { void operator()(std::istream& is, T& v) { return v.deserialize(is); } };\

DefSpecialize(Material)
DefSpecialize(DeleteMessage::Identifier)

#undef DefSpecialize


template<class T>
struct ssize_impl<RawVector<T>>
{
    uint32_t operator()(const RawVector<T>& v) { return uint32_t(4 + sizeof(T) * v.size()); }
};
template<>
struct ssize_impl<std::string>
{
    uint32_t operator()(const std::string& v) { return uint32_t(4 + v.size()); }
};
template<class T>
struct ssize_impl<std::vector<T>>
{
    uint32_t operator()(const std::vector<T>& v) {
        uint32_t ret = 4;
        for (const auto& e  :v) {
            ret += ssize_impl<T>()(e);
        }
        return ret;
    }
};
template<class T>
struct ssize_impl<std::shared_ptr<T>>
{
    uint32_t operator()(const std::shared_ptr<T>& v) {
        return v->getSerializeSize();
    }
};
template<class T>
struct ssize_impl<std::vector<std::shared_ptr<T>>>
{
    uint32_t operator()(const std::vector<std::shared_ptr<T>>& v) {
        uint32_t ret = 4;
        for (const auto& e : v) {
            ret += e->getSerializeSize();
        }
        return ret;
    }
};


template<class T>
struct write_impl<RawVector<T>>
{
    void operator()(std::ostream& os, const RawVector<T>& v)
    {
        auto size = (uint32_t)v.size();
        os.write((const char*)&size, 4);
        os.write((const char*)v.data(), sizeof(T) * size);
    }
};
template<>
struct write_impl<std::string>
{
    void operator()(std::ostream& os, const std::string& v)
    {
        auto size = (uint32_t)v.size();
        os.write((const char*)&size, 4);
        os.write((const char*)v.data(), size);
    }
};
template<class T>
struct write_impl<std::vector<T>>
{
    void operator()(std::ostream& os, const std::vector<T>& v)
    {
        auto size = (uint32_t)v.size();
        os.write((const char*)&size, 4);
        for (const auto& e : v) {
            write_impl<T>()(os, e);
        }
    }
};
template<class T>
struct write_impl<std::shared_ptr<T>>
{
    void operator()(std::ostream& os, const std::shared_ptr<T>& v)
    {
        v->serialize(os);
    }
};
template<class T>
struct write_impl<std::vector<std::shared_ptr<T>>>
{
    void operator()(std::ostream& os, const std::vector<std::shared_ptr<T>>& v)
    {
        auto size = (uint32_t)v.size();
        os.write((const char*)&size, 4);
        for (const auto& e : v) {
            e->serialize(os);
        }
    }
};



template<class T>
struct read_impl<RawVector<T>>
{
    void operator()(std::istream& is, RawVector<T>& v)
    {
        uint32_t size = 0;
        is.read((char*)&size, 4);
        v.resize(size);
        is.read((char*)v.data(), sizeof(T) * size);
    }
};
template<>
struct read_impl<std::string>
{
    void operator()(std::istream& is, std::string& v)
    {
        uint32_t size = 0;
        is.read((char*)&size, 4);
        v.resize(size);
        is.read((char*)v.data(), size);
    }
};
template<class T>
struct read_impl<std::vector<T>>
{
    void operator()(std::istream& is, std::vector<T>& v)
    {
        uint32_t size = 0;
        is.read((char*)&size, 4);
        v.resize(size);
        for (auto& e : v) {
            read_impl<T>()(is, e);
        }
    }
};
template<class T>
struct read_impl<std::shared_ptr<T>>
{
    void operator()(std::istream& is, std::shared_ptr<T>& v)
    {
        v.reset(new T());
        v->deserialize(is);
    }
};
template<class T>
struct read_impl<std::vector<std::shared_ptr<T>>>
{
    void operator()(std::istream& is, std::vector<std::shared_ptr<T>>& v)
    {
        uint32_t size = 0;
        is.read((char*)&size, 4);
        v.resize(size);
        for (auto& e : v) {
            e.reset(new T);
            e->deserialize(is);
        }
    }
};


template<class T>
struct clear_impl
{
    void operator()(T& v) { v = {}; }
};
template<class T>
struct clear_impl<RawVector<T>>
{
    void operator()(RawVector<T>& v) { v.clear(); }
};
template<class T>
struct clear_impl<std::vector<T>>
{
    void operator()(std::vector<T>& v) { v.clear(); }
};


template<class T> inline uint32_t ssize(const T& v) { return ssize_impl<T>()(v); }
template<class T> inline void write(std::ostream& os, const T& v) { return write_impl<T>()(os, v); }
template<class T> inline void read(std::istream& is, T& v) { return read_impl<T>()(is, v); }
template<class T> inline void vclear(T& v) { return clear_impl<T>()(v); }
} // namespace


static void LogImpl2(const char *fmt, va_list args)
{
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    ::OutputDebugStringA(buf);
}
void LogImpl(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogImpl2(fmt, args);
    va_end(args);
}

std::string ToUTF8(const char *src)
{
    // to UTF-16
    const int wsize = ::MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1, nullptr, 0);
    std::wstring ws;
    ws.resize(wsize);
    ::MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1, (LPWSTR)ws.data(), wsize);

    // to UTF-8
    const int u8size = ::WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws.data(), -1, nullptr, 0, nullptr, nullptr);
    std::string u8s;
    u8s.resize(u8size);
    ::WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws.data(), -1, (LPSTR)u8s.data(), u8size, nullptr, nullptr);
    return u8s;
}
std::string ToUTF8(const std::string& src)
{
    return ToUTF8(src.c_str());
}

std::string ToANSI(const char *src)
{
    // to UTF-16
    const int wsize = ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)src, -1, nullptr, 0);
    std::wstring ws;
    ws.resize(wsize);
    ::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)src, -1, (LPWSTR)ws.data(), wsize);

    // to ANSI
    const int u8size = ::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)ws.data(), -1, nullptr, 0, nullptr, nullptr);
    std::string u8s;
    u8s.resize(u8size);
    ::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)ws.data(), -1, (LPSTR)u8s.data(), u8size, nullptr, nullptr);
    return u8s;
}
std::string ToANSI(const std::string& src)
{
    return ToANSI(src.c_str());
}



Message::~Message()
{
}
uint32_t Message::getSerializeSize() const
{
    return ssize(msProtocolVersion);
}
void Message::serialize(std::ostream& os) const
{
    write(os, msProtocolVersion);
}
bool Message::deserialize(std::istream& is)
{
    int pv = 0;
    read(is, pv);
    return pv == msProtocolVersion;
}

GetMessage::GetMessage()
{
}
uint32_t GetMessage::getSerializeSize() const
{
    uint32_t ret = super::getSerializeSize();
    ret += ssize(flags);
    ret += ssize(scene_settings);
    ret += ssize(refine_settings);
    return ret;
}
void GetMessage::serialize(std::ostream& os) const
{
    super::serialize(os);
    write(os, flags);
    write(os, scene_settings);
    write(os, refine_settings);
}
bool GetMessage::deserialize(std::istream& is)
{
    if (!super::deserialize(is)) { return false; }
    read(is, flags);
    read(is, scene_settings);
    read(is, refine_settings);
    return true;
}


SetMessage::SetMessage()
{
}
uint32_t SetMessage::getSerializeSize() const
{
    uint32_t ret = super::getSerializeSize();
    ret += scene.getSerializeSize();
    return ret;
}
void SetMessage::serialize(std::ostream& os) const
{
    super::serialize(os);
    scene.serialize(os);
}
bool SetMessage::deserialize(std::istream& is)
{
    if (!super::deserialize(is)) { return false; }
    scene.deserialize(is);
    return true;
}


uint32_t DeleteMessage::Identifier::getSerializeSize() const
{
    return ssize(path) + ssize(id);
}
void DeleteMessage::Identifier::serialize(std::ostream& os) const
{
    write(os, path); write(os, id);
}
void DeleteMessage::Identifier::deserialize(std::istream& is)
{
    read(is, path); read(is, id);
}

DeleteMessage::DeleteMessage()
{
}
uint32_t DeleteMessage::getSerializeSize() const
{
    uint32_t ret = super::getSerializeSize();
    ret += ssize(targets);
    return ret;
}
void DeleteMessage::serialize(std::ostream& os) const
{
    super::serialize(os);
    write(os, targets);
}
bool DeleteMessage::deserialize(std::istream& is)
{
    if (!super::deserialize(is)) { return false; }
    read(is, targets);
    return true;
}


FenceMessage::~FenceMessage() {}
uint32_t FenceMessage::getSerializeSize() const
{
    return super::getSerializeSize()
        + ssize(type);
}
void FenceMessage::serialize(std::ostream& os) const
{
    super::serialize(os);
    write(os, type);
}
bool FenceMessage::deserialize(std::istream& is)
{
    if (!super::deserialize(is)) { return false; }
    read(is, type);
    return true;
}

TextMessage::~TextMessage() {}
uint32_t TextMessage::getSerializeSize() const
{
    return super::getSerializeSize()
        + ssize(text)
        + ssize(type);
}
void TextMessage::serialize(std::ostream& os) const
{
    super::serialize(os);
    write(os, text);
    write(os, type);
}
bool TextMessage::deserialize(std::istream& is)
{
    if (!super::deserialize(is)) { return false; }
    read(is, text);
    read(is, type);
    return true;
}


ScreenshotMessage::ScreenshotMessage() {}
uint32_t ScreenshotMessage::getSerializeSize() const { return super::getSerializeSize(); }
void ScreenshotMessage::serialize(std::ostream& os) const { super::serialize(os); }
bool ScreenshotMessage::deserialize(std::istream& is) { return super::deserialize(is); }


Entity::~Entity()
{
}

Entity::TypeID Entity::getTypeID() const
{
    return TypeID::Unknown;
}

uint32_t Entity::getSerializeSize() const
{
    uint32_t ret = 0;
    ret += ssize(id);
    ret += ssize(index);
    ret += ssize(path);
    return ret;
}
void Entity::serialize(std::ostream& os) const
{
    write(os, id);
    write(os, index);
    write(os, path);
}
void Entity::deserialize(std::istream& is)
{
    read(is, id);
    read(is, index);
    read(is, path);
}

const char* Entity::getName() const
{
    size_t name_pos = path.find_last_of('/');
    if (name_pos != std::string::npos) { ++name_pos; }
    else { name_pos = 0; }
    return path.c_str() + name_pos;
}

Animation::~Animation()
{
}

uint32_t Animation::getSerializeSize() const
{
    return 0;
}

void Animation::serialize(std::ostream &) const
{
}

void Animation::deserialize(std::istream &)
{
}

bool Animation::empty() const
{
    return true;
}


uint32_t Material::getSerializeSize() const
{
    uint32_t ret = 0;
    ret += ssize(id);
    ret += ssize(name);
    ret += ssize(color);
    return ret;
}
void Material::serialize(std::ostream& os) const
{
    write(os, id);
    write(os, name);
    write(os, color);
}
void Material::deserialize(std::istream& is)
{
    read(is, id);
    read(is, name);
    read(is, color);
}



float4x4 TRS::toMatrix() const
{
    return ms::transform(position, rotation, scale);
}

struct TransformDataFlags
{
    uint32_t has_animation : 1;
};

Entity::TypeID Transform::getTypeID() const
{
    return TypeID::Transform;
}

uint32_t Transform::getSerializeSize() const
{
    uint32_t ret = super::getSerializeSize();
    ret += sizeof(TransformDataFlags);
    ret += ssize(transform);
    ret += ssize(visible);
    if (animation) { ret += ssize(animation); }
    return ret;
}
void Transform::serialize(std::ostream& os) const
{
    super::serialize(os);

    TransformDataFlags flags = {};
    flags.has_animation = animation ? 1 : 0;
    write(os, flags);
    write(os, transform);
    write(os, visible);
    if (flags.has_animation) { write(os, animation); }
}
void Transform::deserialize(std::istream& is)
{
    super::deserialize(is);

    TransformDataFlags flags;
    read(is, flags);
    read(is, transform);
    read(is, visible);
    if(flags.has_animation) {
        createAnimation();
        animation->deserialize(is);
    }
}

void Transform::createAnimation()
{
    if (!animation) {
        animation.reset(new TransformAnimation());
    }
}

void Transform::swapHandedness()
{
    transform.position.x *= -1.0f;
    transform.rotation = swap_handedness(transform.rotation);

    if (animation) {
        auto& anim = static_cast<TransformAnimation&>(*animation);
        for (auto& tvp : anim.translation) {
            tvp.value.x *= -1.0f;
        }
        for (auto& tvp : anim.rotation) {
            tvp.value = swap_handedness(tvp.value);
        }
    }
}

void Transform::applyScaleFactor(float scale)
{
    transform.position *= scale;
}

uint32_t TransformAnimation::getSerializeSize() const
{
    uint32_t ret = 0;
    ret += ssize(translation);
    ret += ssize(rotation);
    ret += ssize(scale);
    ret += ssize(visible);
    return ret;
}

void TransformAnimation::serialize(std::ostream & os) const
{
    write(os, translation);
    write(os, rotation);
    write(os, scale);
    write(os, visible);
}

void TransformAnimation::deserialize(std::istream & is)
{
    read(is, translation);
    read(is, rotation);
    read(is, scale);
    read(is, visible);
}

bool TransformAnimation::empty() const
{
    return translation.empty() && rotation.empty() && scale.empty() && visible.empty();
}



Entity::TypeID Camera::getTypeID() const
{
    return TypeID::Camera;
}

uint32_t Camera::getSerializeSize() const
{
    uint32_t ret = super::getSerializeSize();
    ret += ssize(is_ortho);
    ret += ssize(fov);
    ret += ssize(near_plane);
    ret += ssize(far_plane);
    ret += ssize(vertical_aperture);
    ret += ssize(horizontal_aperture);
    ret += ssize(focal_length);
    ret += ssize(focus_distance);
    return ret;
}
void Camera::serialize(std::ostream& os) const
{
    super::serialize(os);
    write(os, is_ortho);
    write(os, fov);
    write(os, near_plane);
    write(os, far_plane);
    write(os, vertical_aperture);
    write(os, horizontal_aperture);
    write(os, focal_length);
    write(os, focus_distance);
}
void Camera::deserialize(std::istream& is)
{
    super::deserialize(is);
    read(is, is_ortho);
    read(is, fov);
    read(is, near_plane);
    read(is, far_plane);
    read(is, vertical_aperture);
    read(is, horizontal_aperture);
    read(is, focal_length);
    read(is, focus_distance);
}

void Camera::createAnimation()
{
    if (!animation) {
        animation.reset(new CameraAnimation());
    }
}

void Camera::applyScaleFactor(float scale)
{
    super::applyScaleFactor(scale);
    near_plane *= scale;
    far_plane *= scale;
}

uint32_t CameraAnimation::getSerializeSize() const
{
    uint32_t ret = super::getSerializeSize();
    ret += ssize(fov);
    ret += ssize(near_plane);
    ret += ssize(far_plane);
    return ret;
}

void CameraAnimation::serialize(std::ostream & os) const
{
    super::serialize(os);
    write(os, fov);
    write(os, near_plane);
    write(os, far_plane);
}

void CameraAnimation::deserialize(std::istream & is)
{
    super::deserialize(is);
    read(is, fov);
    read(is, near_plane);
    read(is, far_plane);
}

bool CameraAnimation::empty() const
{
    return super::empty() &&
        fov.empty() && near_plane.empty() && far_plane.empty();
}


Entity::TypeID Light::getTypeID() const
{
    return TypeID::Light;
}

uint32_t Light::getSerializeSize() const
{
    uint32_t ret = super::getSerializeSize();
    ret += ssize(type);
    ret += ssize(color);
    ret += ssize(intensity);
    ret += ssize(range);
    ret += ssize(spot_angle);
    return ret;
}

void Light::serialize(std::ostream & os) const
{
    super::serialize(os);
    write(os, type);
    write(os, color);
    write(os, intensity);
    write(os, range);
    write(os, spot_angle);
}

void Light::deserialize(std::istream & is)
{
    super::deserialize(is);
    read(is, type);
    read(is, color);
    read(is, intensity);
    read(is, range);
    read(is, spot_angle);
}

void Light::createAnimation()
{
    if (!animation) {
        animation.reset(new LightAnimation());
    }
}

void Light::applyScaleFactor(float scale)
{
    super::applyScaleFactor(scale);
    range *= scale;
}

uint32_t LightAnimation::getSerializeSize() const
{
    uint32_t ret = super::getSerializeSize();
    ret += ssize(color);
    ret += ssize(intensity);
    ret += ssize(range);
    ret += ssize(spot_angle);
    return ret;
}

void LightAnimation::serialize(std::ostream & os) const
{
    write(os, color);
    write(os, intensity);
    write(os, range);
    write(os, spot_angle);
}

void LightAnimation::deserialize(std::istream & is)
{
    read(is, color);
    read(is, intensity);
    read(is, range);
    read(is, spot_angle);
}

bool LightAnimation::empty() const
{
    return super::empty() &&
        color.empty() && intensity.empty() && range.empty() && spot_angle.empty();
}


uint32_t BlendshapeData::getSerializeSize() const
{
    uint32_t ret = 0;
    ret += ssize(name);
    ret += ssize(weight);
    ret += ssize(points);
    ret += ssize(normals);
    ret += ssize(tangents);
    return ret;
}
void BlendshapeData::serialize(std::ostream& os) const
{
    write(os, name);
    write(os, weight);
    write(os, points);
    write(os, normals);
    write(os, tangents);
}
void BlendshapeData::deserialize(std::istream& is)
{
    read(is, name);
    read(is, weight);
    read(is, points);
    read(is, normals);
    read(is, tangents);
}



#define EachVertexProperty(Body) Body(points) Body(normals) Body(tangents) Body(uv) Body(colors) Body(counts) Body(indices) Body(materialIDs)
#define EachBoneProperty(Body) Body(bones_per_vertex) Body(bone_weights) Body(bone_indices) Body(bones) Body(bindposes)

Mesh::Mesh()
{
}

void Mesh::clear()
{
    id = 0;
    path.clear();
    flags = {0};

    transform = TRS();
    refine_settings = MeshRefineSettings();

#define Body(A) vclear(A);
    EachVertexProperty(Body);
    EachBoneProperty(Body);
#undef Body

    submeshes.clear();
    splits.clear();
    weights4.clear();
}

Entity::TypeID Mesh::getTypeID() const
{
    return TypeID::Mesh;
}

uint32_t Mesh::getSerializeSize() const
{
    uint32_t ret = super::getSerializeSize();
    ret += ssize(flags);

    if (flags.has_refine_settings) ret += ssize(refine_settings);

#define Body(A) if(flags.has_##A) ret += ssize(A);
    EachVertexProperty(Body);
#undef Body

    if (flags.has_bones) {
#define Body(A) ret += ssize(A);
        EachBoneProperty(Body);
#undef Body
    }
    if (flags.has_blendshapes) {
        ret += ssize(blendshape);
    }
    return ret;
}

void Mesh::serialize(std::ostream& os) const
{
    super::serialize(os);
    write(os, flags);

    if (flags.has_refine_settings) write(os, refine_settings);

#define Body(A) if(flags.has_##A) write(os, A);
    EachVertexProperty(Body);
#undef Body

    if (flags.has_bones) {
#define Body(A) write(os, A);
        EachBoneProperty(Body);
#undef Body
    }
    if (flags.has_blendshapes) {
        write(os, blendshape);
    }
}

void Mesh::deserialize(std::istream& is)
{
    clear();
    super::deserialize(is);
    read(is, flags);

    if (flags.has_refine_settings) read(is, refine_settings);

#define Body(A) if(flags.has_##A) read(is, A);
    EachVertexProperty(Body);
#undef Body

    if (flags.has_bones) {
#define Body(A) read(is, A);
        EachBoneProperty(Body);
#undef Body
    }
    if (flags.has_blendshapes) {
        read(is, blendshape);
    }
}

#undef EachVertexProperty
#undef EachBoneProperty

void Mesh::swapHandedness()
{
    super::swapHandedness();
    mu::InvertX(points.data(), points.size());
    mu::InvertX(normals.data(), normals.size());
    for (auto& bp : bindposes) {
        bp = swap_handedness(bp);
    }
}

void Mesh::applyScaleFactor(float scale)
{
    super::applyScaleFactor(scale);
    mu::Scale(points.data(), scale, points.size());
    for (auto& bp : bindposes) {
        (float3&)bp[3] *= scale;
    }
}

void Mesh::refine(const MeshRefineSettings& mrs)
{
    if (mrs.flags.invert_v) {
        mu::InvertV(uv.data(), uv.size());
    }

    if (mrs.flags.apply_local2world) {
        applyTransform(mrs.local2world);
    }
    if (mrs.flags.apply_world2local) {
        applyTransform(mrs.world2local);
    }
    if (mrs.flags.mirror_x) {
        float3 plane_n = { 1.0f, 0.0f, 0.0f };
        float plane_d = 0.0f;
        applyMirror(plane_n, plane_d, true);
    }
    if (mrs.flags.mirror_y) {
        float3 plane_n = { 0.0f, 1.0f, 0.0f };
        float plane_d = 0.0f;
        applyMirror(plane_n, plane_d, true);
    }
    if (mrs.flags.mirror_z) {
        float3 plane_n = { 0.0f, 0.0f, 1.0f };
        float plane_d = 0.0f;
        applyMirror(plane_n, plane_d, true);
    }
    if (mrs.scale_factor != 1.0f) {
        applyScaleFactor(mrs.scale_factor);
    }
    if (mrs.flags.swap_handedness) {
        swapHandedness();
    }
    if (mrs.flags.gen_weights4 && !bone_weights.empty()) {
        generateWeights4();
    }

    mu::MeshRefiner refiner;
    refiner.triangulate = mrs.flags.triangulate;
    refiner.swap_faces = mrs.flags.swap_faces;
    refiner.split_unit = mrs.split_unit;
    refiner.prepare(counts, indices, points);
    refiner.uv = uv;
    refiner.colors = colors;
    refiner.weights4 = weights4;

    // normals
    if (mrs.flags.gen_normals_with_smooth_angle) {
        refiner.genNormals(mrs.smooth_angle);
    }
    else if (mrs.flags.gen_normals) {
        refiner.genNormals();
    }
    else {
        refiner.normals = normals;
    }

    // tangents
    bool gen_tangents = mrs.flags.gen_tangents && !refiner.normals.empty() && !refiner.uv.empty();
    if (gen_tangents) {
        refiner.genTangents();
    }

    // refine topology
    bool refine_topology = !mrs.flags.no_reindexing  && (
         mrs.flags.triangulate ||
        (mrs.flags.split && points.size() > mrs.split_unit) ||
        (points.size() != indices.size() && (normals.size() == indices.size() || uv.size() == indices.size()))
    );
    if(refine_topology) {
        refiner.refine(mrs.flags.optimize_topology);
        refiner.genSubmesh(materialIDs);
        refiner.swapNewData(points, normals, tangents, uv, colors, weights4, indices);

        splits.clear();
        int *sub_indices = indices.data();
        int offset_vertices = 0;
        for (auto& split : refiner.splits) {
            auto sp = SplitData();

            sp.indices.reset(sub_indices, split.num_indices_triangulated);
            sub_indices += split.num_indices_triangulated;

            if (!points.empty()) {
                sp.points.reset(&points[offset_vertices], split.num_vertices);
            }
            if (!normals.empty()) {
                sp.normals.reset(&normals[offset_vertices], split.num_vertices);
            }
            if (!tangents.empty()) {
                sp.tangents.reset(&tangents[offset_vertices], split.num_vertices);
            }
            if (!uv.empty()) {
                sp.uv.reset(&uv[offset_vertices], split.num_vertices);
            }
            if (!colors.empty()) {
                sp.colors.reset(&colors[offset_vertices], split.num_vertices);
            }
            if (!weights4.empty()) {
                sp.weights4.reset(&weights4[offset_vertices], split.num_vertices);
            }
            offset_vertices += split.num_vertices;
            splits.push_back(sp);
        }

        // setup submeshes
        {
            int nsm = 0;
            int *tri = indices.data();
            for (auto& split : refiner.splits) {
                for (int i = 0; i < split.num_submeshes; ++i) {
                    auto& sm = refiner.submeshes[nsm + i];
                    SubmeshData tmp;
                    tmp.materialID = sm.materialID;
                    tmp.indices.reset(tri, sm.num_indices_tri);
                    tri += sm.num_indices_tri;
                    submeshes.push_back(tmp);
                }
                nsm += split.num_submeshes;
            }
            nsm = 0;
            for (int i = 0; i < splits.size(); ++i) {
                int n = refiner.splits[i].num_submeshes;
                splits[i].submeshes.reset(&submeshes[nsm], n);
                nsm += n;
            }
        }
    }
    else {
        refiner.swapNewData(points, normals, tangents, uv, colors, weights4, indices);
    }

    flags.has_points = !points.empty();
    flags.has_normals = !normals.empty();
    flags.has_tangents = !tangents.empty();
    flags.has_uv = !uv.empty();
    flags.has_indices = !indices.empty();
}

void Mesh::applyMirror(const float3 & plane_n, float plane_d, bool /*welding*/)
{
    size_t num_points = points.size();
    size_t num_faces = counts.size();
    size_t num_indices = indices.size();

    RawVector<int> indirect(num_points);
    RawVector<int> copylist;
    copylist.reserve(num_points);
    {
        // welding
        int idx = 0;
        for (size_t pi = 0; pi < num_points; ++pi) {
            auto& p = points[pi];
            float d = dot(plane_n, p) - plane_d;
            if (near_equal(d, 0.0f)) {
                indirect[pi] = (int)pi;
            }
            else {
                copylist.push_back((int)pi);
                indirect[pi] = (int)num_points + idx++;
            }

        }
    }

    points.resize(num_points + copylist.size());
    mu::MirrorPoints(points.data() + num_points, IArray<float3>{points.data(), num_points}, copylist, plane_n, plane_d);

    counts.resize(num_faces * 2);
    indices.resize(num_indices * 2);
    mu::MirrorTopology(counts.data() + num_faces, indices.data() + num_indices,
        IArray<int>{counts.data(), num_faces}, IArray<int>{indices.data(), num_indices}, IArray<int>{indirect.data(), indirect.size()});

    if (uv.data()) {
        if (uv.size() == num_points) {
            uv.resize(points.size());
            mu::CopyWithIndices(&uv[num_points], &uv[0], copylist);
        }
        else if (uv.size() == num_indices) {
            uv.resize(indices.size());
            auto dst = &uv[num_indices];
            mu::EnumerateReverseFaceIndices(IArray<int>{counts.data(), num_faces}, [dst, this](int, int idx, int ridx) {
                dst[idx] = uv[ridx];
            });
        }
    }
    if (colors.data()) {
        if (colors.size() == num_points) {
            colors.resize(points.size());
            mu::CopyWithIndices(&colors[num_points], &colors[0], copylist);
        }
        else if (colors.size() == num_indices) {
            colors.resize(indices.size());
            auto dst = &colors[num_indices];
            mu::EnumerateReverseFaceIndices(IArray<int>{counts.data(), num_faces}, [dst, this](int, int idx, int ridx) {
                dst[idx] = colors[ridx];
            });
        }
    }
    if (materialIDs.data()) {
        size_t n = materialIDs.size();
        materialIDs.resize(n * 2);
        memcpy(materialIDs.data() + n, materialIDs.data(), sizeof(int) * n);
    }
}


void Mesh::applyTransform(const float4x4& m)
{
    for (auto& v : points) { v = applyTRS(m, v); }
    for (auto& v : normals) { v = m * v; }
    mu::Normalize(normals.data(), normals.size());
}

void Mesh::generateWeights4()
{
    if (!GenerateWeightsN(weights4, bone_indices, bone_weights, bones_per_vertex)) {
        msLogError("Mesh::normalizeWeights(): should not be here\n");
    }
}

uint32_t SceneSettings::getSerializeSize() const
{
    uint32_t ret = 0;
    ret += ssize(name);
    ret += ssize(handedness);
    ret += ssize(scale_factor);
    return ret;
}
void SceneSettings::serialize(std::ostream& os) const
{
    write(os, name);
    write(os, handedness);
    write(os, scale_factor);
}
void SceneSettings::deserialize(std::istream& is)
{
    read(is, name);
    read(is, handedness);
    read(is, scale_factor);
}

uint32_t Scene::getSerializeSize() const
{
    uint32_t ret = 0;
    ret += settings.getSerializeSize();
    ret += ssize(meshes);
    ret += ssize(transforms);
    ret += ssize(cameras);
    ret += ssize(lights);
    ret += ssize(materials);
    return ret;
}
void Scene::serialize(std::ostream& os) const
{
    settings.serialize(os);
    write(os, meshes);
    write(os, transforms);
    write(os, cameras);
    write(os, lights);
    write(os, materials);
}
void Scene::deserialize(std::istream& is)
{
    settings.deserialize(is);
    read(is, meshes);
    read(is, transforms);
    read(is, cameras);
    read(is, lights);
    read(is, materials);
}

} // namespace ms
