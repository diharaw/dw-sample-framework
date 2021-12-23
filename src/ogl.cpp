#if !defined(DWSF_VULKAN)

#    include <gtc/type_ptr.hpp>
#    include <logger.h>
#    include <ogl.h>
#    include <utility.h>
#    define STB_IMAGE_IMPLEMENTATION
#    include <stb_image.h>
#    define STB_IMAGE_WRITE_IMPLEMENTATION
#    include <stb_image_write.h>

namespace dw
{
namespace gl
{
// -----------------------------------------------------------------------------------------------------------------------------------

int num_channels_from_internal_format(GLenum fmt)
{
    if (fmt == GL_R8 || fmt == GL_R16F || fmt == GL_R32F)
        return 1;
    else if (fmt == GL_RG8 || fmt == GL_RG16F || fmt == GL_RG32F)
        return 2;
    else if (fmt == GL_RGB8 || fmt == GL_RGB16F || fmt == GL_RGB32F)
        return 3;
    else if (fmt == GL_RGBA8 || fmt == GL_RGBA16F || fmt == GL_RGBA32F)
        return 4;
    else
        return 0;
}

// -----------------------------------------------------------------------------------------------------------------------------------

size_t pixel_size_from_type(GLenum type)
{
    if (type == GL_UNSIGNED_BYTE || type == GL_BYTE)
        return sizeof(uint8_t);
    else if (type == GL_HALF_FLOAT)
        return sizeof(uint16_t);
    else if (type == GL_FLOAT)
        return sizeof(float);
    else
        return 0;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Object::Object(const GLenum& identifier) :
    m_identifier(identifier)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Object::~Object()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Object::set_name(const GLuint& name, const std::string& label)
{
    m_name = label;
    glObjectLabel(m_identifier, name, label.size(), label.c_str());
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture::Texture() :
    Object(GL_TEXTURE)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture::~Texture()
{
    make_texture_handle_non_resident();
    make_image_handle_non_resident();
    glDeleteTextures(1, &m_gl_tex);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::bind(uint32_t unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(m_target, m_gl_tex);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::unbind(uint32_t unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(m_target, 0);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::generate_mipmaps()
{
    glGenerateTextureMipmap(m_gl_tex);
}

// -----------------------------------------------------------------------------------------------------------------------------------

GLuint Texture::id()
{
    return m_gl_tex;
}

// -----------------------------------------------------------------------------------------------------------------------------------

GLenum Texture::target()
{
    return m_target;
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture::array_size()
{
    return m_array_size;
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture::version()
{
    return m_version;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_wrapping(GLenum s, GLenum t, GLenum r)
{
    glTextureParameteri(m_gl_tex, GL_TEXTURE_WRAP_S, s);
    glTextureParameteri(m_gl_tex, GL_TEXTURE_WRAP_T, t);
    glTextureParameteri(m_gl_tex, GL_TEXTURE_WRAP_R, r);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_border_color(float r, float g, float b, float a)
{
    float border_color[] = { r, g, b, a };
    glTextureParameterfv(m_gl_tex, GL_TEXTURE_BORDER_COLOR, border_color);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_min_filter(GLenum filter)
{
    glTextureParameteri(m_gl_tex, GL_TEXTURE_MIN_FILTER, filter);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_mag_filter(GLenum filter)
{
    glTextureParameteri(m_gl_tex, GL_TEXTURE_MAG_FILTER, filter);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::bind_image(uint32_t unit, uint32_t mip_level, uint32_t layer, GLenum access, GLenum format)
{
    bind(unit);

    // GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format
    if (m_array_size > 1)
        glBindImageTexture(unit, m_gl_tex, mip_level, GL_TRUE, layer, access, format);
    else
        glBindImageTexture(unit, m_gl_tex, mip_level, GL_FALSE, 0, access, format);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_compare_mode(GLenum mode)
{
    glTextureParameteri(m_gl_tex, GL_TEXTURE_COMPARE_MODE, mode);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_compare_func(GLenum func)
{
    glTextureParameteri(m_gl_tex, GL_TEXTURE_COMPARE_FUNC, func);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Texture::is_compressed(int mip_level)
{
    GLint param = 0;
    glGetTextureLevelParameteriv(m_gl_tex, mip_level, GL_TEXTURE_COMPRESSED, &param);

    return param == 1;
}

// -----------------------------------------------------------------------------------------------------------------------------------

int Texture::compressed_size(int mip_level)
{
    GLint param = 0;
    glGetTextureLevelParameteriv(m_gl_tex, mip_level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &param);

    return param;
}

// -----------------------------------------------------------------------------------------------------------------------------------

GLuint64 Texture::make_texture_handle_resident()
{
    m_texture_handle = glGetTextureHandleARB(m_gl_tex);
    glMakeTextureHandleResidentARB(m_texture_handle);
    return m_texture_handle;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::make_texture_handle_non_resident()
{
    if (m_texture_handle != 0)
    {
        glMakeTextureHandleNonResidentARB(m_texture_handle);
        m_texture_handle = 0;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

GLuint64 Texture::make_image_handle_resident(GLenum access, GLint level, GLboolean layered, GLint layer)
{
    m_image_handle = glGetImageHandleARB(m_gl_tex, level, layered, layer, m_format);
    glMakeImageHandleResidentARB(m_gl_tex, access);
    return m_image_handle;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::make_image_handle_non_resident()
{
    if (m_image_handle != 0)
    {
        glMakeImageHandleNonResidentARB(m_image_handle);
        m_image_handle = 0;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture::mip_levels()
{
    return m_mip_levels;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_name(const std::string& name)
{
    Object::set_name(m_gl_tex, name);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture1D::Ptr Texture1D::create(uint32_t w, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type)
{
    return std::shared_ptr<Texture1D>(new Texture1D(w, array_size, mip_levels, internal_format, format, type));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture1D::Texture1D(uint32_t w, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type) :
    Texture()
{
    m_array_size      = array_size;
    m_internal_format = internal_format;
    m_format          = format;
    m_type            = type;
    m_width           = w;

    // If mip levels is -1, calculate mip levels
    if (mip_levels == -1)
    {
        m_mip_levels = 1;

        int width = m_width;

        while (width > 1)
        {
            width = std::max(1, (width / 2));
            m_mip_levels++;
        }
    }
    else
        m_mip_levels = mip_levels;

    allocate();

    // Default sampling options.
    set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
    set_mag_filter(GL_LINEAR);

    if (m_mip_levels > 1)
        set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
    else
        set_min_filter(GL_LINEAR);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture1D::Texture1D() :
    Texture()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture1D::~Texture1D() {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture1D::allocate()
{
    // Allocate memory for mip levels.
    if (m_array_size > 1)
    {
        m_target = GL_TEXTURE_1D_ARRAY;
        glCreateTextures(m_target, 1, &m_gl_tex);
        glTextureStorage2D(m_gl_tex, m_mip_levels, m_internal_format, m_width, m_array_size);
    }
    else
    {
        m_target = GL_TEXTURE_1D;
        glCreateTextures(m_target, 1, &m_gl_tex);
        glTextureStorage1D(m_gl_tex, m_mip_levels, m_internal_format, m_width);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture1D::write_data(int array_index, int mip_level, void* data)
{
    int width = m_width;

    for (int i = 0; i < mip_level; i++)
        width = std::max(1, width / 2);

    write_sub_data(array_index, mip_level, 0, width, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture1D::write_sub_data(int array_index, int mip_level, int x_offset, int width, void* data)
{
    if (m_array_size > 1)
        glTextureSubImage2D(m_gl_tex, mip_level, x_offset, array_index, width, 1, m_format, m_type, data);
    else
        glTextureSubImage1D(m_gl_tex, mip_level, x_offset, width, m_format, m_type, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture1D::write_compressed_data(int array_index, int mip_level, int size, void* data)
{
    int width = m_width;

    for (int i = 0; i < mip_level; i++)
        width = std::max(1, width / 2);

    write_compressed_sub_data(array_index, mip_level, 0, width, size, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture1D::write_compressed_sub_data(int array_index, int mip_level, int x_offset, int width, int size, void* data)
{
    if (m_array_size > 1)
        glCompressedTextureSubImage2D(m_gl_tex, mip_level, x_offset, array_index, width, 1, m_internal_format, size, data);
    else
        glCompressedTextureSubImage1D(m_gl_tex, mip_level, x_offset, width, m_internal_format, size, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture1D::resize(uint32_t w)
{
    if (m_gl_tex != UINT32_MAX)
        glDeleteTextures(1, &m_gl_tex);

    m_version++;
    m_width = w;

    // Check if the max number of mip-levels possible with the current size is less than
    // the earlier number, if so use the smaller number.
    int mip_levels = 1;

    int width = m_width;

    while (width > 1)
    {
        width = std::max(1, (width / 2));
        mip_levels++;
    }

    if (mip_levels < m_mip_levels)
        m_mip_levels = mip_levels;

    allocate();
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture1D::width()
{
    return m_width;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture2D::Ptr Texture2D::create(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, uint32_t num_samples, GLenum internal_format, GLenum format, GLenum type)
{
    return std::shared_ptr<Texture2D>(new Texture2D(w, h, array_size, mip_levels, num_samples, internal_format, format, type));
}

// -----------------------------------------------------------------------------------------------------------------------------------
Texture2D::Ptr Texture2D::create_from_files(std::string path, bool flip_vertical, bool srgb)
{
    int x, y, n;
    stbi_set_flip_vertically_on_load(flip_vertical);

    std::string ext = utility::file_extension(path);

    if (ext == "hdr")
    {
        float* data = stbi_loadf(path.c_str(), &x, &y, &n, 0);

        if (!data)
            return nullptr;

        Texture2D::Ptr texture = Texture2D::create(x, y, 1, -1, 1, GL_RGB32F, GL_RGB, GL_FLOAT);
        texture->write_data(0, 0, data);
        texture->generate_mipmaps();

        stbi_image_free(data);

        return texture;
    }
    else
    {
        stbi_uc* data = stbi_load(path.c_str(), &x, &y, &n, 0);

        if (!data)
            return nullptr;

        GLenum internal_format, format;

        if (n == 1)
        {
            internal_format = GL_R8;
            format          = GL_RED;
        }
        else
        {
            if (srgb)
            {
                if (n == 4)
                {
                    internal_format = GL_SRGB8_ALPHA8;
                    format          = GL_RGBA;
                }
                else
                {
                    internal_format = GL_SRGB8;
                    format          = GL_RGB;
                }
            }
            else
            {
                if (n == 4)
                {
                    internal_format = GL_RGBA8;
                    format          = GL_RGBA;
                }
                else
                {
                    internal_format = GL_RGB8;
                    format          = GL_RGB;
                }
            }
        }

        Texture2D::Ptr texture = Texture2D::create(x, y, 1, -1, 1, internal_format, format, GL_UNSIGNED_BYTE);
        texture->write_data(0, 0, data);
        texture->generate_mipmaps();

        stbi_image_free(data);

        return texture;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture2D::Texture2D(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, uint32_t num_samples, GLenum internal_format, GLenum format, GLenum type) :
    Texture()
{
    m_array_size      = array_size;
    m_internal_format = internal_format;
    m_format          = format;
    m_type            = type;
    m_num_samples     = num_samples;
    m_mip_levels      = mip_levels;
    m_width           = w;
    m_height          = h;

    // If mip levels is -1, calculate mip levels
    if (m_mip_levels == -1)
    {
        m_mip_levels = 1;

        int width  = m_width;
        int height = m_height;

        while (width > 1 || height > 1)
        {
            width  = std::max(1, (width / 2));
            height = std::max(1, (height / 2));
            m_mip_levels++;
        }
    }

    allocate();

    // Default sampling options.
    set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
    set_mag_filter(GL_LINEAR);

    if (m_mip_levels > 1)
        set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
    else
        set_min_filter(GL_LINEAR);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture2D::Texture2D() :
    Texture()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture2D::~Texture2D() {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::allocate()
{
    // Allocate memory for mip levels.
    if (m_array_size > 1)
    {
        if (m_num_samples > 1)
            m_target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        else
            m_target = GL_TEXTURE_2D_ARRAY;

        glCreateTextures(m_target, 1, &m_gl_tex);

        if (m_num_samples > 1)
        {
            if (m_mip_levels > 1)
                DW_LOG_WARNING("OPENGL: Multisampled textures cannot have mipmaps. Setting mip levels to 1...");

            m_mip_levels = 1;
            glTextureStorage3DMultisample(m_gl_tex, m_num_samples, m_internal_format, m_width, m_height, m_array_size, true);
        }
        else
            glTextureStorage3D(m_gl_tex, m_mip_levels, m_internal_format, m_width, m_height, m_array_size);
    }
    else
    {
        if (m_num_samples > 1)
            m_target = GL_TEXTURE_2D_MULTISAMPLE;
        else
            m_target = GL_TEXTURE_2D;

        glCreateTextures(m_target, 1, &m_gl_tex);

        if (m_num_samples > 1)
        {
            if (m_mip_levels > 1)
                DW_LOG_WARNING("OPENGL: Multisampled textures cannot have mipmaps. Setting mip levels to 1...");

            m_mip_levels = 1;
            glTextureStorage2DMultisample(m_gl_tex, m_num_samples, m_internal_format, m_width, m_height, true);
        }
        else
            glTextureStorage2D(m_gl_tex, m_mip_levels, m_internal_format, m_width, m_height);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::write_data(int array_index, int mip_level, void* data)
{
    if (m_num_samples > 1)
        DW_LOG_ERROR("OPENGL: Multisampled texture data can only be assigned through Shaders or FBOs");
    else
    {
        int width  = m_width;
        int height = m_height;

        for (int i = 0; i < mip_level; i++)
        {
            width  = std::max(1, width / 2);
            height = std::max(1, (height / 2));
        }

        write_sub_data(array_index, mip_level, 0, 0, width, height, data);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::write_sub_data(int array_index, int mip_level, int x_offset, int y_offset, int width, int height, void* data)
{
    if (m_array_size > 1)
        glTextureSubImage3D(m_gl_tex, mip_level, x_offset, y_offset, array_index, width, height, 1, m_format, m_type, data);
    else
        glTextureSubImage2D(m_gl_tex, mip_level, x_offset, y_offset, width, height, m_format, m_type, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::write_compressed_data(int array_index, int mip_level, size_t size, void* data)
{
    if (m_num_samples > 1)
        DW_LOG_ERROR("OPENGL: Multisampled texture data can only be assigned through Shaders or FBOs");
    else
    {
        int width  = m_width;
        int height = m_height;

        for (int i = 0; i < mip_level; i++)
        {
            width  = std::max(1, width / 2);
            height = std::max(1, (height / 2));
        }

        write_compressed_sub_data(array_index, mip_level, 0, 0, width, height, size, data);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::write_compressed_sub_data(int array_index, int mip_level, int x_offset, int y_offset, int width, int height, size_t size, void* data)
{
    if (m_array_size > 1)
        glCompressedTextureSubImage3D(m_gl_tex, mip_level, x_offset, y_offset, array_index, width, height, 1, m_internal_format, size, data);
    else
        glCompressedTextureSubImage2D(m_gl_tex, mip_level, x_offset, y_offset, width, height, m_internal_format, size, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::read_data(int mip_level, std::vector<uint8_t>& buffer)
{
    int w, h;
    extents(mip_level, w, h);

    size_t size = is_compressed(mip_level) ? compressed_size(mip_level) : w * h * m_array_size * pixel_size_from_type(m_type) * num_channels_from_internal_format(m_format);
    buffer.resize(size);

    if (is_compressed(mip_level))
        glGetCompressedTextureImage(m_gl_tex, mip_level, size, &buffer[0]);
    else
        glGetTextureImage(m_gl_tex, mip_level, m_format, m_type, size, &buffer[0]);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::extents(int mip_level, int& width, int& height)
{
    glGetTextureLevelParameteriv(m_gl_tex, mip_level, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(m_gl_tex, mip_level, GL_TEXTURE_HEIGHT, &height);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::resize(uint32_t w, uint32_t h)
{
    if (m_gl_tex != UINT32_MAX)
        glDeleteTextures(1, &m_gl_tex);

    m_version++;
    m_width  = w;
    m_height = h;

    // Check if the max number of mip-levels possible with the current size is less than
    // the earlier number, if so use the smaller number.
    int mip_levels = 1;

    int width  = m_width;
    int height = m_height;

    while (width > 1 || height > 1)
    {
        width  = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
        mip_levels++;
    }

    if (mip_levels < m_mip_levels)
        m_mip_levels = mip_levels;

    allocate();
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture2D::width()
{
    return m_width;
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture2D::height()
{
    return m_height;
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture2D::num_samples()
{
    return m_num_samples;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture3D::Ptr Texture3D::create(uint32_t w, uint32_t h, uint32_t d, int mip_levels, GLenum internal_format, GLenum format, GLenum type)
{
    return std::shared_ptr<Texture3D>(new Texture3D(w, h, d, mip_levels, internal_format, format, type));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture3D::Texture3D(uint32_t w, uint32_t h, uint32_t d, int mip_levels, GLenum internal_format, GLenum format, GLenum type) :
    Texture()
{
    m_internal_format = internal_format;
    m_array_size      = 1;
    m_format          = format;
    m_type            = type;
    m_width           = w;
    m_height          = h;
    m_depth           = d;

    // If mip levels is -1, calculate mip levels
    if (mip_levels == -1)
    {
        m_mip_levels = 1;

        int width  = m_width;
        int height = m_height;
        int depth  = m_depth;

        while (width > 1 && height > 1 && depth > 1)
        {
            width  = std::max(1, (width / 2));
            height = std::max(1, (height / 2));
            depth  = std::max(1, (depth / 2));
            m_mip_levels++;
        }
    }
    else
        m_mip_levels = mip_levels;

    // Allocate memory for mip levels.
    m_target = GL_TEXTURE_3D;

    allocate();

    // Default sampling options.
    set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
    set_mag_filter(GL_LINEAR);

    if (m_mip_levels > 1)
        set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
    else
        set_min_filter(GL_LINEAR);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture3D::Texture3D() :
    Texture()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture3D::~Texture3D() {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture3D::allocate()
{
    glCreateTextures(m_target, 1, &m_gl_tex);

    glTextureStorage3D(m_gl_tex, m_mip_levels, m_internal_format, m_width, m_height, m_depth);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture3D::write_data(int slice, int mip_level, void* data)
{
    int width  = m_width;
    int height = m_height;

    for (int i = 0; i < mip_level; i++)
    {
        width  = std::max(1, width / 2);
        height = std::max(1, (height / 2));
    }

    write_sub_data(slice, mip_level, 0, 0, width, height, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture3D::write_sub_data(int slice, int mip_level, int x_offset, int y_offset, int width, int height, void* data)
{
    glTextureSubImage3D(m_gl_tex, mip_level, x_offset, y_offset, slice, width, height, 1, m_format, m_type, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture3D::write_compressed_data(int slice, int mip_level, size_t size, void* data)
{
    int width  = m_width;
    int height = m_height;

    for (int i = 0; i < mip_level; i++)
    {
        width  = std::max(1, width / 2);
        height = std::max(1, (height / 2));
    }

    write_compressed_sub_data(slice, mip_level, 0, 0, width, height, size, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture3D::write_compressed_sub_data(int slice, int mip_level, int x_offset, int y_offset, int width, int height, size_t size, void* data)
{
    glCompressedTextureSubImage3D(m_gl_tex, mip_level, x_offset, y_offset, slice, width, height, 1, m_internal_format, size, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture3D::read_data(int mip_level, std::vector<uint8_t>& buffer)
{
    int w, h, d;
    extents(mip_level, w, h, d);

    size_t size = is_compressed(mip_level) ? compressed_size(mip_level) : w * h * d * pixel_size_from_type(m_type) * num_channels_from_internal_format(m_format);
    buffer.resize(size);

    if (is_compressed(mip_level))
        glGetCompressedTextureImage(m_gl_tex, mip_level, size, &buffer[0]);
    else
        glGetTextureImage(m_gl_tex, mip_level, m_format, m_type, size, &buffer[0]);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture3D::extents(int mip_level, int& width, int& height, int& depth)
{
    glGetTextureLevelParameteriv(m_gl_tex, mip_level, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(m_gl_tex, mip_level, GL_TEXTURE_HEIGHT, &height);
    glGetTextureLevelParameteriv(m_gl_tex, mip_level, GL_TEXTURE_DEPTH, &depth);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture3D::resize(uint32_t w, uint32_t h, uint32_t d)
{
    if (m_gl_tex != UINT32_MAX)
        glDeleteTextures(1, &m_gl_tex);

    m_version++;
    m_width  = w;
    m_height = h;
    m_depth  = d;

    // Check if the max number of mip-levels possible with the current size is less than
    // the earlier number, if so use the smaller number.
    int mip_levels = 1;

    int width  = m_width;
    int height = m_height;
    int depth  = m_depth;

    while (width > 1 || height > 1 || depth > 1)
    {
        width  = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
        depth  = std::max(1, (depth / 2));
        mip_levels++;
    }

    if (mip_levels < m_mip_levels)
        m_mip_levels = mip_levels;

    allocate();
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture3D::width()
{
    return m_width;
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture3D::height()
{
    return m_height;
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture3D::depth()
{
    return m_depth;
}

// -----------------------------------------------------------------------------------------------------------------------------------

TextureCube::Ptr TextureCube::create(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type)
{
    return std::shared_ptr<TextureCube>(new TextureCube(w, h, array_size, mip_levels, internal_format, format, type));
}

// -----------------------------------------------------------------------------------------------------------------------------------
TextureCube::Ptr TextureCube::create_from_files(std::string path[], bool srgb)
{
    if (utility::file_extension(path[0]) == "hdr")
    {
        // Load the first image to determine format and dimensions.
        std::string tex_path = path[0];

        int    x, y, n;
        float* data = stbi_loadf(tex_path.c_str(), &x, &y, &n, 3);

        if (!data)
            return nullptr;

        GLenum internal_format, format;

        internal_format = GL_RGB32F;
        format          = GL_RGB;

        TextureCube::Ptr cube = TextureCube::create(x, y, 1, -1, internal_format, format, GL_FLOAT);

        cube->write_data(0, 0, 0, data);
        stbi_image_free(data);

        for (int i = 1; i < 6; i++)
        {
            tex_path = path[i];
            data     = stbi_loadf(tex_path.c_str(), &x, &y, &n, 3);

            if (!data)
                return nullptr;

            cube->write_data(i, 0, 0, data);
            stbi_image_free(data);
        }

        return cube;
    }
    else
    {
        // Load the first image to determine format and dimensions.
        std::string tex_path = path[0];

        int      x, y, n;
        stbi_uc* data = stbi_load(tex_path.c_str(), &x, &y, &n, 3);

        if (!data)
            return nullptr;

        GLenum internal_format, format;

        if (srgb)
        {
            internal_format = GL_SRGB8;
            format          = GL_RGB;
        }
        else
        {
            internal_format = GL_RGBA8;
            format          = GL_RGB;
        }

        TextureCube::Ptr cube = TextureCube::create(x, y, 1, -1, internal_format, format, GL_UNSIGNED_BYTE);

        cube->write_data(0, 0, 0, data);
        stbi_image_free(data);

        for (int i = 1; i < 6; i++)
        {
            tex_path = path[i];
            data     = stbi_load(tex_path.c_str(), &x, &y, &n, 3);

            if (!data)
                return nullptr;

            cube->write_data(i, 0, 0, data);
            stbi_image_free(data);
        }

        return cube;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

TextureCube::TextureCube(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type)
{
    m_array_size      = array_size;
    m_internal_format = internal_format;
    m_format          = format;
    m_type            = type;
    m_width           = w;
    m_height          = h;

    // If mip levels is -1, calculate mip levels
    if (mip_levels == -1)
    {
        m_mip_levels = 1;

        int width  = m_width;
        int height = m_height;

        while (width > 1 && height > 1)
        {
            width  = std::max(1, (width / 2));
            height = std::max(1, (height / 2));
            m_mip_levels++;
        }
    }
    else
        m_mip_levels = mip_levels;

    // Allocate memory for mip levels.
    if (array_size > 1)
        m_target = GL_TEXTURE_CUBE_MAP_ARRAY;
    else
        m_target = GL_TEXTURE_CUBE_MAP;

    allocate();

    // Default sampling options.
    set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
    set_mag_filter(GL_LINEAR);

    if (m_mip_levels > 1)
        set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
    else
        set_min_filter(GL_LINEAR);
}

// -----------------------------------------------------------------------------------------------------------------------------------

TextureCube::TextureCube() :
    Texture()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

TextureCube::~TextureCube() {}

// -----------------------------------------------------------------------------------------------------------------------------------

void TextureCube::allocate()
{
    glCreateTextures(m_target, 1, &m_gl_tex);

    if (m_array_size > 1)
        glTextureStorage3D(m_gl_tex, m_mip_levels, m_internal_format, m_width, m_height, m_array_size);
    else
        glTextureStorage2D(m_gl_tex, m_mip_levels, m_internal_format, m_width, m_height);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void TextureCube::write_data(int face_index, int array_index, int mip_level, void* data)
{
    int width  = m_width;
    int height = m_height;

    for (int i = 0; i < mip_level; i++)
    {
        width  = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
    }

    if (m_array_size == 1)
        array_index = 0;

    write_sub_data(face_index, array_index, mip_level, 0, 0, width, height, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void TextureCube::write_sub_data(int face_index, int array_index, int mip_level, int x_offset, int y_offset, int width, int height, void* data)
{
    glTextureSubImage3D(m_gl_tex, mip_level, x_offset, y_offset, array_index * 6 + face_index, width, height, 1, m_format, m_type, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void TextureCube::write_compressed_data(int face_index, int array_index, int mip_level, size_t size, void* data)
{
    int width  = m_width;
    int height = m_height;

    for (int i = 0; i < mip_level; i++)
    {
        width  = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
    }

    if (m_array_size == 1)
        array_index = 0;

    write_compressed_sub_data(face_index, array_index, mip_level, 0, 0, width, height, size, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void TextureCube::write_compressed_sub_data(int face_index, int array_index, int mip_level, int x_offset, int y_offset, int width, int height, size_t size, void* data)
{
    glCompressedTextureSubImage3D(m_gl_tex, mip_level, x_offset, y_offset, array_index * 6 + face_index, width, height, m_internal_format, size, m_type, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void TextureCube::read_data(int mip_level, std::vector<uint8_t>& buffer)
{
    int w, h;
    extents(mip_level, w, h);

    size_t size = is_compressed(mip_level) ? compressed_size(mip_level) : w * h * 6 * m_array_size * pixel_size_from_type(m_type) * num_channels_from_internal_format(m_format);
    buffer.resize(size);

    if (is_compressed(mip_level))
        glGetCompressedTextureImage(m_gl_tex, mip_level, size, &buffer[0]);
    else
        glGetTextureImage(m_gl_tex, mip_level, m_format, m_type, size, &buffer[0]);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void TextureCube::extents(int mip_level, int& width, int& height)
{
    glGetTextureLevelParameteriv(m_gl_tex, mip_level, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(m_gl_tex, mip_level, GL_TEXTURE_HEIGHT, &height);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void TextureCube::resize(uint32_t w, uint32_t h)
{
    if (m_gl_tex != UINT32_MAX)
        glDeleteTextures(1, &m_gl_tex);

    glCreateTextures(m_target, 1, &m_gl_tex);

    m_version++;
    m_width  = w;
    m_height = h;

    // Check if the max number of mip-levels possible with the current size is less than
    // the earlier number, if so use the smaller number.
    int mip_levels = 1;

    int width  = m_width;
    int height = m_height;

    while (width > 1 || height > 1)
    {
        width  = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
        mip_levels++;
    }

    if (mip_levels < m_mip_levels)
        m_mip_levels = mip_levels;

    allocate();
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t TextureCube::width()
{
    return m_width;
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t TextureCube::height()
{
    return m_height;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture1DView::Ptr Texture1DView::create(Texture1D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers)
{
    return std::shared_ptr<Texture1DView>(new Texture1DView(origin_texture, new_target, min_level, num_levels, min_layer, num_layers));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture1DView::Texture1DView(Texture1D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers) :
    Texture1D()
{
    m_target          = new_target;
    m_array_size      = num_layers;
    m_internal_format = origin_texture->internal_format();
    m_format          = origin_texture->format();
    m_type            = origin_texture->type();
    m_width           = origin_texture->width();

    glGenTextures(1, &m_gl_tex);
    glTextureView(m_gl_tex, m_target, origin_texture->id(), m_internal_format, min_level, num_levels, min_layer, num_layers);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture1DView::~Texture1DView()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture2DView::Ptr Texture2DView::create(Texture2D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers)
{
    return std::shared_ptr<Texture2DView>(new Texture2DView(origin_texture, new_target, min_level, num_levels, min_layer, num_layers));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture2DView::Texture2DView(Texture2D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers) :
    Texture2D()
{
    m_target          = new_target;
    m_mip_levels      = num_levels;
    m_array_size      = num_layers;
    m_internal_format = origin_texture->internal_format();
    m_format          = origin_texture->format();
    m_type            = origin_texture->type();
    m_width           = origin_texture->width();
    m_height          = origin_texture->height();

    glGenTextures(1, &m_gl_tex);
    glTextureView(m_gl_tex, m_target, origin_texture->id(), m_internal_format, min_level, num_levels, min_layer, num_layers);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture2DView::~Texture2DView()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture3DView::Ptr Texture3DView::create(Texture3D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers)
{
    return std::shared_ptr<Texture3DView>(new Texture3DView(origin_texture, new_target, min_level, num_levels, min_layer, num_layers));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture3DView::Texture3DView(Texture3D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers) :
    Texture3D()
{
    m_target          = new_target;
    m_mip_levels      = num_levels;
    m_array_size      = num_layers;
    m_internal_format = origin_texture->internal_format();
    m_format          = origin_texture->format();
    m_type            = origin_texture->type();
    m_width           = origin_texture->width();
    m_height          = origin_texture->height();
    m_depth           = origin_texture->depth();

    glGenTextures(1, &m_gl_tex);
    glTextureView(m_gl_tex, m_target, origin_texture->id(), m_internal_format, min_level, num_levels, min_layer, num_layers);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture3DView::~Texture3DView()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

TextureCubeView::Ptr TextureCubeView::create(TextureCube::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers)
{
    return std::shared_ptr<TextureCubeView>(new TextureCubeView(origin_texture, new_target, min_level, num_levels, min_layer, num_layers));
}

// -----------------------------------------------------------------------------------------------------------------------------------

TextureCubeView::TextureCubeView(TextureCube::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers) :
    TextureCube()
{
    m_target          = new_target;
    m_mip_levels      = num_levels;
    m_array_size      = num_layers;
    m_internal_format = origin_texture->internal_format();
    m_format          = origin_texture->format();
    m_type            = origin_texture->type();
    m_width           = origin_texture->width();
    m_height          = origin_texture->height();

    glGenTextures(1, &m_gl_tex);
    glTextureView(m_gl_tex, m_target, origin_texture->id(), m_internal_format, min_level, num_levels, min_layer, num_layers);
}

// -----------------------------------------------------------------------------------------------------------------------------------

TextureCubeView::~TextureCubeView()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Framebuffer::Ptr Framebuffer::create(std::vector<Texture::Ptr> color_attachments, Texture::Ptr depth_stencil_attachment)
{
    return std::shared_ptr<Framebuffer>(new Framebuffer(color_attachments, depth_stencil_attachment));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Framebuffer::Framebuffer(std::vector<Texture::Ptr> color_attachments, Texture::Ptr depth_stencil_attachment) :
    Object(GL_FRAMEBUFFER)
{
    glCreateFramebuffers(1, &m_gl_fbo);

    GLuint attachments[16];

    for (int i = 0; i < color_attachments.size(); i++)
    {
        glNamedFramebufferTexture(m_gl_fbo, GL_COLOR_ATTACHMENT0 + i, color_attachments[i]->id(), 0);
        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    glNamedFramebufferDrawBuffers(m_gl_fbo, color_attachments.size(), attachments);

    if (depth_stencil_attachment)
        glNamedFramebufferTexture(m_gl_fbo, GL_DEPTH_ATTACHMENT, depth_stencil_attachment->id(), 0);

    check_status();
}

// -----------------------------------------------------------------------------------------------------------------------------------

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &m_gl_fbo);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_gl_fbo);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::check_status()
{
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::string error = "Framebuffer Incomplete: ";

        switch (status)
        {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            {
                error += "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
                break;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            {
                error += "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
                break;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            {
                error += "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
                break;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            {
                error += "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
                break;
            }
            case GL_FRAMEBUFFER_UNSUPPORTED:
            {
                error += "GL_FRAMEBUFFER_UNSUPPORTED";
                break;
            }
            default:
                break;
        }

        DW_LOG_ERROR(error);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

Shader::Ptr Shader::create_from_file(GLenum type, std::string path, std::vector<std::string> defines)
{
    std::string source;

    if (!utility::read_shader(path, source, defines))
    {
        DW_LOG_ERROR("Failed to read GLSL shader source: " + path);

        // Force assertion failure for debug builds.
        assert(false);

        return nullptr;
    }

    Shader::Ptr shader = Shader::create(type, source);

    if (shader->compiled())
        return shader;

    return nullptr;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::set_name(const std::string& name)
{
    Object::set_name(m_gl_fbo, name);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Shader::Ptr Shader::create(GLenum type, std::string source)
{
    return std::shared_ptr<Shader>(new Shader(type, source));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Shader::Shader(GLenum type, std::string source) :
    m_type(type), Object(GL_SHADER)
{
    m_gl_shader = glCreateShader(type);

    source = "#version 450 core\n" + std::string(source);

    GLint  success;
    GLchar log[512];

    const GLchar* src = source.c_str();

    glShaderSource(m_gl_shader, 1, &src, NULL);
    glCompileShader(m_gl_shader);
    glGetShaderiv(m_gl_shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(m_gl_shader, 512, NULL, log);

        std::string log_error = "OPENGL: Shader compilation failed: ";
        log_error += std::string(log);
        log_error += ", Source: ";
        log_error += source;

        DW_LOG_ERROR(log_error);
        m_compiled = false;
    }
    else
        m_compiled = true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Shader::~Shader()
{
    glDeleteShader(m_gl_shader);
}

// -----------------------------------------------------------------------------------------------------------------------------------

GLenum Shader::type()
{
    return m_type;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Shader::compiled()
{
    return m_compiled;
}

// -----------------------------------------------------------------------------------------------------------------------------------

GLuint Shader::id()
{
    return m_gl_shader;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Shader::set_name(const std::string& name)
{
    Object::set_name(m_gl_shader, name);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Program::Ptr Program::create(std::vector<Shader::Ptr> shaders)
{
    return std::shared_ptr<Program>(new Program(shaders));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Program::Program(std::vector<Shader::Ptr> shaders) :
    Object(GL_PROGRAM)
{
    if (shaders.size() == 1 && shaders[0]->type() != GL_COMPUTE_SHADER)
    {
        DW_LOG_ERROR("OPENGL: Compute shader programs can only have one shader.");
        assert(false);

        return;
    }

    m_gl_program = glCreateProgram();

    for (int i = 0; i < shaders.size(); i++)
        glAttachShader(m_gl_program, shaders[i]->m_gl_shader);

    glLinkProgram(m_gl_program);

    GLint success;
    char  log[512];

    glGetProgramiv(m_gl_program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(m_gl_program, 512, NULL, log);

        std::string log_error = "OPENGL: Shader program linking failed: ";
        log_error += std::string(log);

        DW_LOG_ERROR(log_error);

        return;
    }

    int uniform_count = 0;
    glGetProgramiv(m_gl_program, GL_ACTIVE_UNIFORMS, &uniform_count);

    GLint        size;
    GLenum       type;
    GLsizei      length;
    const GLuint buf_size = 64;
    GLchar       name[buf_size];

    for (int i = 0; i < uniform_count; i++)
    {
        glGetActiveUniform(m_gl_program, i, buf_size, &length, &size, &type, name);
        GLuint loc = glGetUniformLocation(m_gl_program, name);

        if (loc != GL_INVALID_INDEX)
            m_location_map[std::string(name)] = loc;
    }

    glGetProgramiv(m_gl_program, GL_ACTIVE_UNIFORM_BLOCKS, &m_num_active_uniform_blocks);

    for (int i = 0; i < shaders.size(); i++)
        glDetachShader(m_gl_program, shaders[i]->m_gl_shader);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Program::~Program()
{
    glDeleteProgram(m_gl_program);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Program::use()
{
    glUseProgram(m_gl_program);
}

// -----------------------------------------------------------------------------------------------------------------------------------

int32_t Program::num_active_uniform_blocks()
{
    return m_num_active_uniform_blocks;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Program::uniform_block_binding(std::string name, int binding)
{
    GLuint idx = glGetUniformBlockIndex(m_gl_program, name.c_str());

    if (idx != GL_INVALID_INDEX)
        glUniformBlockBinding(m_gl_program, idx, binding);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int32_t value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform1i(m_location_map[name], value);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, uint32_t value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform1ui(m_location_map[name], value);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, float value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform1f(m_location_map[name], value);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, glm::vec2 value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform2f(m_location_map[name], value.x, value.y);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, glm::vec3 value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform3f(m_location_map[name], value.x, value.y, value.z);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, glm::vec4 value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform4f(m_location_map[name], value.x, value.y, value.z, value.w);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, glm::mat2 value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniformMatrix2fv(m_location_map[name], 1, GL_FALSE, glm::value_ptr(value));

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, glm::mat3 value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniformMatrix3fv(m_location_map[name], 1, GL_FALSE, glm::value_ptr(value));

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, glm::mat4 value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniformMatrix4fv(m_location_map[name], 1, GL_FALSE, glm::value_ptr(value));

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int count, int* value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform1iv(m_location_map[name], count, value);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int count, float* value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform1fv(m_location_map[name], count, value);

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int count, glm::vec2* value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform2fv(m_location_map[name], count, glm::value_ptr(value[0]));

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int count, glm::vec3* value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform3fv(m_location_map[name], count, glm::value_ptr(value[0]));

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int count, glm::vec4* value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform4fv(m_location_map[name], count, glm::value_ptr(value[0]));

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int count, glm::mat2* value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniformMatrix2fv(m_location_map[name], count, GL_FALSE, glm::value_ptr(value[0]));

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int count, glm::mat3* value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniformMatrix3fv(m_location_map[name], count, GL_FALSE, glm::value_ptr(value[0]));

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int count, glm::mat4* value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniformMatrix4fv(m_location_map[name], count, GL_FALSE, glm::value_ptr(value[0]));

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Program::extract_reflection_data(ReflectionData& reflection_data)
{
    std::vector<GLchar> name_buffer;

    int32_t num_uniforms = 0;
    glGetProgramInterfaceiv(m_gl_program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

    GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX };

    for (int i = 0; i < num_uniforms; i++)
    {
        GLint results[4];
        glGetProgramResourceiv(m_gl_program, GL_UNIFORM, i, 4, properties, 4, NULL, results);

        if (results[3] == -1)
        {
            name_buffer.reserve(size_t(results[0] + 1));

            glGetProgramResourceName(m_gl_program, GL_UNIFORM, i, results[0] + 1, NULL, name_buffer.data());

            std::string name = (const char*)name_buffer.data();

            switch (results[1])
            {
                case GL_SAMPLER_1D:
                case GL_SAMPLER_2D:
                case GL_SAMPLER_3D:
                case GL_SAMPLER_CUBE:
                case GL_SAMPLER_1D_SHADOW:
                case GL_SAMPLER_2D_SHADOW:
                case GL_SAMPLER_1D_ARRAY:
                case GL_SAMPLER_2D_ARRAY:
                case GL_SAMPLER_1D_ARRAY_SHADOW:
                case GL_SAMPLER_2D_ARRAY_SHADOW:
                case GL_SAMPLER_2D_MULTISAMPLE:
                case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
                case GL_SAMPLER_CUBE_SHADOW:
                case GL_SAMPLER_BUFFER:
                case GL_SAMPLER_2D_RECT:
                case GL_SAMPLER_2D_RECT_SHADOW:
                case GL_INT_SAMPLER_1D:
                case GL_INT_SAMPLER_2D:
                case GL_INT_SAMPLER_3D:
                case GL_INT_SAMPLER_CUBE:
                case GL_INT_SAMPLER_1D_ARRAY:
                case GL_INT_SAMPLER_2D_ARRAY:
                case GL_INT_SAMPLER_2D_MULTISAMPLE:
                case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
                case GL_INT_SAMPLER_BUFFER:
                case GL_INT_SAMPLER_2D_RECT:
                case GL_UNSIGNED_INT_SAMPLER_1D:
                case GL_UNSIGNED_INT_SAMPLER_2D:
                case GL_UNSIGNED_INT_SAMPLER_3D:
                case GL_UNSIGNED_INT_SAMPLER_CUBE:
                case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
                case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
                case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
                case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
                case GL_UNSIGNED_INT_SAMPLER_BUFFER:
                case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
                {
                    SamplerReflection reflection;

                    reflection.location = results[2];
                    reflection.type     = results[1];
                    reflection.name     = name;

                    reflection_data.samplers[reflection.location] = reflection;
                    break;
                }
                case GL_IMAGE_1D:
                case GL_IMAGE_2D:
                case GL_IMAGE_3D:
                case GL_IMAGE_2D_RECT:
                case GL_IMAGE_CUBE:
                case GL_IMAGE_BUFFER:
                case GL_IMAGE_1D_ARRAY:
                case GL_IMAGE_2D_ARRAY:
                case GL_IMAGE_CUBE_MAP_ARRAY:
                case GL_IMAGE_2D_MULTISAMPLE:
                case GL_IMAGE_2D_MULTISAMPLE_ARRAY:
                case GL_INT_IMAGE_1D:
                case GL_INT_IMAGE_2D:
                case GL_INT_IMAGE_3D:
                case GL_INT_IMAGE_2D_RECT:
                case GL_INT_IMAGE_CUBE:
                case GL_INT_IMAGE_BUFFER:
                case GL_INT_IMAGE_1D_ARRAY:
                case GL_INT_IMAGE_2D_ARRAY:
                case GL_INT_IMAGE_CUBE_MAP_ARRAY:
                case GL_INT_IMAGE_2D_MULTISAMPLE:
                case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
                case GL_UNSIGNED_INT_IMAGE_1D:
                case GL_UNSIGNED_INT_IMAGE_2D:
                case GL_UNSIGNED_INT_IMAGE_3D:
                case GL_UNSIGNED_INT_IMAGE_2D_RECT:
                case GL_UNSIGNED_INT_IMAGE_CUBE:
                case GL_UNSIGNED_INT_IMAGE_BUFFER:
                case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
                case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
                case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
                case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
                case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
                {
                    ImageReflection reflection;

                    reflection.location = results[2];
                    reflection.type     = results[1];
                    reflection.name     = name;

                    reflection_data.images[reflection.location] = reflection;
                    break;
                }
                default:
                {
                    UniformReflection reflection;

                    reflection.location = results[2];
                    reflection.type     = results[1];
                    reflection.name     = name;

                    reflection_data.uniforms[reflection.location] = reflection;
                    break;
                }
            }
        }
    }

    int32_t num_ssbos = 0;
    glGetProgramInterfaceiv(m_gl_program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &num_ssbos);

    GLint ssbo_max_len;
    glGetProgramInterfaceiv(m_gl_program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, &ssbo_max_len);
    name_buffer.resize(ssbo_max_len);

    for (int i = 0; i < num_ssbos; i++)
    {
        GLsizei strLength;
        glGetProgramResourceName(m_gl_program, GL_SHADER_STORAGE_BLOCK, i, ssbo_max_len, &strLength, name_buffer.data());

        SSBOReflection reflection;

        reflection.name    = (const char*)name_buffer.data();
        reflection.binding = glGetProgramResourceIndex(m_gl_program, GL_SHADER_STORAGE_BLOCK, reflection.name.c_str());

        reflection_data.ssbos[reflection.binding] = reflection;
    }

    int32_t num_ubos = 0;
    glGetProgramInterfaceiv(m_gl_program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &num_ubos);

    GLint ubo_max_len;
    glGetProgramInterfaceiv(m_gl_program, GL_UNIFORM_BLOCK, GL_MAX_NAME_LENGTH, &ubo_max_len);
    name_buffer.resize(ubo_max_len);

    for (int i = 0; i < num_ubos; i++)
    {
        GLsizei strLength;
        glGetProgramResourceName(m_gl_program, GL_UNIFORM_BLOCK, i, ubo_max_len, &strLength, name_buffer.data());

        UBOReflection reflection;

        reflection.name    = (const char*)name_buffer.data();
        reflection.binding = glGetProgramResourceIndex(m_gl_program, GL_UNIFORM_BLOCK, reflection.name.c_str());

        reflection_data.ubos[reflection.binding] = reflection;
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

GLint Program::id()
{
    return m_gl_program;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Program::set_name(const std::string& name)
{
    Object::set_name(m_gl_program, name);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Buffer::Ptr Buffer::create(GLenum type, GLenum flags, size_t size, void* data)
{
    return std::shared_ptr<Buffer>(new Buffer(type, flags, size, data));
}

// -----------------------------------------------------------------------------------------------------------------------------------

Buffer::Buffer(GLenum type, GLenum flags, size_t size, void* data) :
    m_type(type), m_size(size), Object(GL_BUFFER)
{
    glCreateBuffers(1, &m_gl_buffer);

    glNamedBufferStorage(m_gl_buffer, size, data, flags);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Buffer::~Buffer()
{
    glDeleteBuffers(1, &m_gl_buffer);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::bind()
{
    glBindBuffer(m_type, m_gl_buffer);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::bind(GLenum type)
{
    glBindBuffer(type, m_gl_buffer);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::bind_base(int index)
{
    glBindBufferBase(m_type, index, m_gl_buffer);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::bind_range(int index, size_t offset, size_t size)
{
    glBindBufferRange(m_type, index, m_gl_buffer, offset, size);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::bind_base(GLenum type, int index)
{
    glBindBufferBase(type, index, m_gl_buffer);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::bind_range(GLenum type, int index, size_t offset, size_t size)
{
    glBindBufferRange(type, index, m_gl_buffer, offset, size);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::unbind()
{
    glBindBuffer(m_type, 0);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void* Buffer::map(GLenum access)
{
    return glMapNamedBuffer(m_gl_buffer, access);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void* Buffer::map_range(GLenum access, size_t offset, size_t size)
{
    return glMapNamedBufferRange(m_gl_buffer, offset, size, access);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::unmap()
{
    glUnmapNamedBuffer(m_gl_buffer);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::flush_mapped_range(size_t offset, size_t length)
{
    glFlushMappedNamedBufferRange(m_gl_buffer, offset, length);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::write_data(size_t offset, size_t size, void* data)
{
    glNamedBufferSubData(m_gl_buffer, offset, size, data);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::copy(Buffer::Ptr dst, GLintptr read_offset, GLintptr write_offset, GLsizeiptr size)
{
    glCopyNamedBufferSubData(m_gl_buffer, dst->m_gl_buffer, read_offset, write_offset, size);
}

// -----------------------------------------------------------------------------------------------------------------------------------

size_t Buffer::size()
{
    return m_size;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::set_name(const std::string& name)
{
    Object::set_name(m_gl_buffer, name);
}

// -----------------------------------------------------------------------------------------------------------------------------------

VertexArray::Ptr VertexArray::create(Buffer::Ptr vbo, Buffer::Ptr ibo, size_t vertex_size, int attrib_count, VertexAttrib attribs[])
{
    return std::shared_ptr<VertexArray>(new VertexArray(vbo, ibo, vertex_size, attrib_count, attribs));
}

// -----------------------------------------------------------------------------------------------------------------------------------

VertexArray::VertexArray(Buffer::Ptr vbo, Buffer::Ptr ibo, size_t vertex_size, int attrib_count, VertexAttrib attribs[]) :
    Object(GL_VERTEX_ARRAY)
{
    glGenVertexArrays(1, &m_gl_vao);
    glBindVertexArray(m_gl_vao);

    vbo->bind();

    if (ibo)
        ibo->bind();

    for (uint32_t i = 0; i < attrib_count; i++)
    {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i,
                              attribs[i].num_sub_elements,
                              attribs[i].type,
                              attribs[i].normalized,
                              vertex_size,
                              (GLvoid*)((uint64_t)attribs[i].offset));
    }

    glBindVertexArray(0);

    vbo->unbind();

    if (ibo)
        ibo->unbind();
}

// -----------------------------------------------------------------------------------------------------------------------------------

VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &m_gl_vao);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void VertexArray::bind()
{
    glBindVertexArray(m_gl_vao);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void VertexArray::unbind()
{
    glBindVertexArray(0);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void VertexArray::set_name(const std::string& name)
{
    Object::set_name(m_gl_vao, name);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Query::Query() :
    Object(GL_QUERY)
{
    glGenQueries(1, &m_query);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Query::~Query()
{
    glDeleteQueries(1, &m_query);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Query::query_counter(GLenum type)
{
    glQueryCounter(m_query, type);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Query::begin(GLenum type)
{
    glBeginQuery(type, m_query);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Query::end(GLenum type)
{
    glEndQuery(type);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Query::result_64(uint64_t* ptr)
{
    glGetQueryObjectui64v(m_query, GL_QUERY_RESULT, ptr);
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Query::result_available()
{
    int done = 0;
    glGetQueryObjectiv(m_query, GL_QUERY_RESULT_AVAILABLE, &done);
    return done == 1;
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Query::set_name(const std::string& name)
{
    Object::set_name(m_query, name);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Fence::Fence()
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

Fence::~Fence()
{
    if (m_fence)
        glDeleteSync(m_fence);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Fence::insert()
{
    if (m_fence)
        wait();

    m_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Fence::wait()
{
    if (m_fence)
    {
        glClientWaitSync(m_fence, 0, 10000000);
        m_fence = nullptr;
        glDeleteSync(m_fence);
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace gl
} // namespace dw

#endif