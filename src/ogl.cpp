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

Texture::Texture() { GL_CHECK_ERROR(glGenTextures(1, &m_gl_tex)); }

// -----------------------------------------------------------------------------------------------------------------------------------

Texture::~Texture() { GL_CHECK_ERROR(glDeleteTextures(1, &m_gl_tex)); }

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::bind(uint32_t unit)
{
    GL_CHECK_ERROR(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::unbind(uint32_t unit)
{
    GL_CHECK_ERROR(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::generate_mipmaps()
{
    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
    GL_CHECK_ERROR(glGenerateMipmap(m_target));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

GLuint Texture::id() { return m_gl_tex; }

// -----------------------------------------------------------------------------------------------------------------------------------

GLenum Texture::target() { return m_target; }

// -----------------------------------------------------------------------------------------------------------------------------------

GLenum Texture::internal_format() { return m_internal_format; }

// -----------------------------------------------------------------------------------------------------------------------------------

GLenum Texture::format() { return m_format; }

// -----------------------------------------------------------------------------------------------------------------------------------

GLenum Texture::type() { return m_type; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture::array_size() { return m_array_size; }

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_wrapping(GLenum s, GLenum t, GLenum r)
{
    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
    GL_CHECK_ERROR(glTexParameteri(m_target, GL_TEXTURE_WRAP_S, s));
    GL_CHECK_ERROR(glTexParameteri(m_target, GL_TEXTURE_WRAP_T, t));
    GL_CHECK_ERROR(glTexParameteri(m_target, GL_TEXTURE_WRAP_R, r));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_border_color(float r, float g, float b, float a)
{
#    if !defined(__EMSCRIPTEN__)
    float border_color[] = { r, g, b, a };
    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
    GL_CHECK_ERROR(
        glTexParameterfv(m_target, GL_TEXTURE_BORDER_COLOR, border_color));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));
#    endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_min_filter(GLenum filter)
{
    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
    GL_CHECK_ERROR(glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, filter));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_mag_filter(GLenum filter)
{
    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
    GL_CHECK_ERROR(glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, filter));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

#    if !defined(__EMSCRIPTEN__)
void Texture::bind_image(uint32_t unit, uint32_t mip_level, uint32_t layer, GLenum access, GLenum format)
{
    bind(unit);

    if (m_array_size > 1)
        glBindImageTexture(unit, m_gl_tex, mip_level, GL_TRUE, layer, access, format);
    else if (m_target == GL_TEXTURE_CUBE_MAP)
        glBindImageTexture(unit, m_gl_tex, mip_level, GL_TRUE, 0, access, format);
    else
        glBindImageTexture(unit, m_gl_tex, mip_level, GL_FALSE, 0, access, format);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_compare_mode(GLenum mode)
{
    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
    GL_CHECK_ERROR(glTexParameteri(m_target, GL_TEXTURE_COMPARE_MODE, mode));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture::set_compare_func(GLenum func)
{
    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
    GL_CHECK_ERROR(glTexParameteri(m_target, GL_TEXTURE_COMPARE_FUNC, func));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));
}
#    endif

// -----------------------------------------------------------------------------------------------------------------------------------

#    if !defined(__EMSCRIPTEN__)
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

    // Allocate memory for mip levels.
    if (array_size > 1)
    {
        m_target = GL_TEXTURE_1D_ARRAY;

        int width = m_width;

        GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));

        for (int i = 0; i < m_mip_levels; i++)
        {
            GL_CHECK_ERROR(glTexImage2D(m_target, i, m_internal_format, width, m_array_size, 0, m_format, m_type, NULL));
            width = std::max(1, (width / 2));
        }

        GL_CHECK_ERROR(glBindTexture(m_target, 0));
    }
    else
    {
        m_target = GL_TEXTURE_1D;

        int width = m_width;

        GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));

        for (int i = 0; i < m_mip_levels; i++)
        {
            GL_CHECK_ERROR(glTexImage1D(m_target, i, m_internal_format, width, 0, m_format, m_type, NULL));
            width = std::max(1, (width / 2));
        }

        GL_CHECK_ERROR(glBindTexture(m_target, 0));
    }

    // Default sampling options.
    set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
    set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
    set_mag_filter(GL_LINEAR);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture1D::~Texture1D() {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture1D::set_data(int array_index, int mip_level, void* data)
{
    int width = m_width;

    for (int i = 0; i < mip_level; i++)
        width = std::max(1, width / 2);

    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));

    if (m_array_size > 1)
    {
        GL_CHECK_ERROR(glTexImage2D(m_target, mip_level, m_internal_format, width, array_index, 0, m_format, m_type, data));
    }
    else
    {
        GL_CHECK_ERROR(glTexImage1D(m_target, mip_level, m_internal_format, width, 0, m_format, m_type, data));
    }

    GL_CHECK_ERROR(glBindTexture(m_target, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture1D::width() { return m_width; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture1D::mip_levels() { return m_mip_levels; }
#    endif

// -----------------------------------------------------------------------------------------------------------------------------------

Texture2D* Texture2D::create_from_files(std::string path, bool flip_vertical, bool srgb)
{
    int x, y, n;
    stbi_set_flip_vertically_on_load(flip_vertical);

    std::string ext = utility::file_extension(path);

    if (ext == "hdr")
    {
        float* data = stbi_loadf(path.c_str(), &x, &y, &n, 0);

        if (!data)
            return nullptr;

        Texture2D* texture = new Texture2D(x, y, 1, -1, 1, GL_RGB32F, GL_RGB, GL_FLOAT);
        texture->set_data(0, 0, data);
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

        Texture2D* texture = new Texture2D(x, y, 1, -1, 1, internal_format, format, GL_UNSIGNED_BYTE);
        texture->set_data(0, 0, data);
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
    m_width           = w;
    m_height          = h;
    m_num_samples     = num_samples;

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
    {
        if (m_num_samples > 1)
        {
#    if defined(__EMSCRIPTEN__)
            assert(false);
            DW_LOG_FATAL("WEBGL: GL_TEXTURE_2D_MULTISAMPLE_ARRAY Not Supported!");
#    else
            m_target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
#    endif
        }
        else
            m_target = GL_TEXTURE_2D_ARRAY;

        int width  = m_width;
        int height = m_height;

        GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));

        if (m_num_samples > 1)
        {
#    if defined(__EMSCRIPTEN__)
            DW_LOG_FATAL("WEBGL: glTexImage3DMultisample unsupported on WebGL!");
#    else
            if (m_mip_levels > 1)
                DW_LOG_WARNING("OPENGL: Multisampled textures cannot have mipmaps. "
                               "Setting mip levels to 1...");

            m_mip_levels = 1;
            GL_CHECK_ERROR(glTexImage3DMultisample(m_target, m_num_samples, m_internal_format, width, height, m_array_size, true));
#    endif
        }
        else
        {
            for (int i = 0; i < m_mip_levels; i++)
            {
                GL_CHECK_ERROR(glTexImage3D(m_target, i, m_internal_format, width, height, m_array_size, 0, m_format, m_type, NULL));

                width  = std::max(1, (width / 2));
                height = std::max(1, (height / 2));
            }
        }

        GL_CHECK_ERROR(glBindTexture(m_target, 0));
    }
    else
    {
        if (m_num_samples > 1)
            m_target = GL_TEXTURE_2D_MULTISAMPLE;
        else
            m_target = GL_TEXTURE_2D;

        int width  = m_width;
        int height = m_height;

        GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));

        if (m_num_samples > 1)
        {
#    if defined(__EMSCRIPTEN__)
            DW_LOG_FATAL("WEBGL: glTexImage2DMultisample unsupported on WebGL!");
#    else
            if (m_mip_levels > 1)
                DW_LOG_WARNING("OPENGL: Multisampled textures cannot have mipmaps. "
                               "Setting mip levels to 1...");

            m_mip_levels = 1;
            GL_CHECK_ERROR(glTexImage2DMultisample(
                m_target, m_num_samples, m_internal_format, width, height, true));
#    endif
        }
        else
        {
            for (int i = 0; i < m_mip_levels; i++)
            {
                GL_CHECK_ERROR(glTexImage2D(m_target, i, m_internal_format, width, height, 0, m_format, m_type, NULL));

                width  = std::max(1, (width / 2));
                height = std::max(1, (height / 2));
            }
        }

        GL_CHECK_ERROR(glBindTexture(m_target, 0));
    }

    // Default sampling options.
    set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);

    if (m_mip_levels > 1)
        set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
    else
        set_min_filter(GL_LINEAR);

    set_mag_filter(GL_LINEAR);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture2D::~Texture2D() {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::set_data(int32_t array_index, int32_t mip_level, void* data)
{
    if (m_num_samples > 1)
    {
        DW_LOG_ERROR("OPENGL: Multisampled texture data can only be assigned "
                     "through Shaders or FBOs");
    }
    else
    {
        int width  = m_width;
        int height = m_height;

        for (int i = 0; i < mip_level; i++)
        {
            width  = std::max(1, width / 2);
            height = std::max(1, (height / 2));
        }

        GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));

        if (m_array_size > 1)
        {
            GL_CHECK_ERROR(glTexImage3D(m_target, mip_level, m_internal_format, width, height, array_index, 0, m_format, m_type, data));
        }
        else
        {
            GL_CHECK_ERROR(glTexImage2D(m_target, mip_level, m_internal_format, width, height, 0, m_format, m_type, data));
        }

        GL_CHECK_ERROR(glBindTexture(m_target, 0));
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture2D::width() { return m_width; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture2D::height() { return m_height; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture2D::mip_levels() { return m_mip_levels; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture2D::num_samples() { return m_num_samples; }

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture2D::save_to_disk(std::string path, int32_t array_index, int32_t mip_level)
{
#    if defined(__APPLE__)
    if (array_index > 0)
        DW_LOG_WARNING("Saving specific slice of texture not supported on macOS, saving entire texture instead...");

    int width  = m_width;
    int height = m_height;

    for (int i = 0; i < mip_level; i++)
    {
        width  = std::max(1, width / 2);
        height = std::max(1, (height / 2));
    }

    int pixel_size = 1;

    if (m_type == GL_FLOAT)
        pixel_size = 4;
    else if (m_type == GL_HALF_FLOAT)
        pixel_size = 2;

    int num_comp = 1;

    if (m_format == GL_RG)
        num_comp = 2;
    else if (m_format == GL_RGB)
        num_comp = 3;
    else if (m_format == GL_RGBA)
        num_comp = 4;

    void* data = malloc(width * height * num_comp * m_array_size * pixel_size);

    GL_CHECK_ERROR(glActiveTexture(GL_TEXTURE0));
    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
    GL_CHECK_ERROR(glGetTexImage(m_target, mip_level, m_format, m_type, data));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));

    if (m_type == GL_FLOAT)
    {
        std::string full_path = path + ".hdr";
        stbi_write_hdr(full_path.c_str(), width, height, num_comp, (float*)data);
    }
    else if (m_type == GL_HALF_FLOAT)
    {
        int pixel_count = width * height * num_comp * m_array_size;

        int16_t* pixels     = (int16_t*)data;
        float*   new_pixels = (float*)malloc(pixel_count * sizeof(float));

        for (int i = 0; i < pixel_count; i++)
            new_pixels[i] = pixels[i];

        std::string full_path = path + ".hdr";
        stbi_write_hdr(full_path.c_str(), width, height, num_comp, new_pixels);

        free(new_pixels);
    }
    else
    {
        std::string full_path = path + ".tga";
        stbi_write_tga(full_path.c_str(), width, height, num_comp, data);
    }

    free(data);
#    else

#    endif
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

    int width  = m_width;
    int height = m_height;
    int depth  = m_depth;

    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));

    for (int i = 0; i < m_mip_levels; i++)
    {
        GL_CHECK_ERROR(glTexImage3D(m_target, i, m_internal_format, width, height, depth, 0, m_format, m_type, NULL));
        width  = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
        depth  = std::max(1, (depth / 2));
    }

    GL_CHECK_ERROR(glBindTexture(m_target, 0));

    // Default sampling options.
    set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
    set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
    set_mag_filter(GL_LINEAR);
}

// -----------------------------------------------------------------------------------------------------------------------------------

Texture3D::~Texture3D() {}

// -----------------------------------------------------------------------------------------------------------------------------------

void Texture3D::set_data(int mip_level, void* data)
{
    int width  = m_width;
    int height = m_height;
    int depth  = m_depth;

    for (int i = 0; i < mip_level; i++)
    {
        width  = std::max(1, width / 2);
        height = std::max(1, (height / 2));
        depth  = std::max(1, (depth / 2));
    }

    GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
    GL_CHECK_ERROR(glTexImage3D(m_target, mip_level, m_internal_format, width, height, depth, 0, m_format, m_type, data));
    GL_CHECK_ERROR(glBindTexture(m_target, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture3D::width() { return m_width; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture3D::height() { return m_height; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture3D::depth() { return m_depth; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t Texture3D::mip_levels() { return m_mip_levels; }

// -----------------------------------------------------------------------------------------------------------------------------------

TextureCube* TextureCube::create_from_files(std::string path[], bool srgb)
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

        TextureCube* cube = new TextureCube(x, y, 1, -1, internal_format, format, GL_FLOAT);

        cube->set_data(0, 0, 0, data);
        stbi_image_free(data);

        for (int i = 1; i < 6; i++)
        {
            tex_path = path[i];
            data     = stbi_loadf(tex_path.c_str(), &x, &y, &n, 3);

            if (!data)
                return nullptr;

            cube->set_data(i, 0, 0, data);
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

        TextureCube* cube = new TextureCube(x, y, 1, -1, internal_format, format, GL_UNSIGNED_BYTE);

        cube->set_data(0, 0, 0, data);
        stbi_image_free(data);

        for (int i = 1; i < 6; i++)
        {
            tex_path = path[i];
            data     = stbi_load(tex_path.c_str(), &x, &y, &n, 3);

            if (!data)
                return nullptr;

            cube->set_data(i, 0, 0, data);
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

#    if !defined(__EMSCRIPTEN__)
    // Allocate memory for mip levels.
    if (array_size > 1)
    {
        m_target = GL_TEXTURE_CUBE_MAP_ARRAY;

        int width  = m_width;
        int height = m_height;

        GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));

        for (int i = 0; i < m_mip_levels; i++)
        {
            GL_CHECK_ERROR(glTexImage3D(m_target, i, m_internal_format, width, height, m_array_size * 6, 0, m_format, m_type, NULL));
            width  = std::max(1, (width / 2));
            height = std::max(1, (height / 2));
        }

        GL_CHECK_ERROR(glBindTexture(m_target, 0));
    }
    else
#    endif
    {
        m_target = GL_TEXTURE_CUBE_MAP;

        int width  = m_width;
        int height = m_height;

        GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));

        for (int face = 0; face < 6; face++)
        {
            GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, m_internal_format, width, height, 0, m_format, m_type, NULL));
        }

        if (m_mip_levels > 1)
        {
            GL_CHECK_ERROR(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
        }

        GL_CHECK_ERROR(glBindTexture(m_target, 0));
    }

    // Default sampling options.
    set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
    set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
    set_mag_filter(GL_LINEAR);
}

// -----------------------------------------------------------------------------------------------------------------------------------

TextureCube::~TextureCube() {}

// -----------------------------------------------------------------------------------------------------------------------------------

void TextureCube::set_data(int face_index, int layer_index, int mip_level, void* data)
{
    int width  = m_width;
    int height = m_height;

    for (int i = 0; i < mip_level; i++)
    {
        width  = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
    }

#    if !defined(__EMSCRIPTEN__)
    if (m_array_size > 1)
    {
        GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
        GL_CHECK_ERROR(glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, mip_level, 0, 0, layer_index * 6 + face_index, width, height, 1, m_format, m_type, data));
        GL_CHECK_ERROR(glBindTexture(m_target, 0));
    }
    else
#    endif
    {
        GL_CHECK_ERROR(glBindTexture(m_target, m_gl_tex));
        GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index,
                                    mip_level,
                                    m_internal_format,
                                    width,
                                    height,
                                    0,
                                    m_format,
                                    m_type,
                                    data));
        GL_CHECK_ERROR(glBindTexture(m_target, 0));
    }
}

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t TextureCube::width() { return m_width; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t TextureCube::height() { return m_height; }

// -----------------------------------------------------------------------------------------------------------------------------------

uint32_t TextureCube::mip_levels() { return m_mip_levels; }

// -----------------------------------------------------------------------------------------------------------------------------------

Framebuffer::Framebuffer() { GL_CHECK_ERROR(glGenFramebuffers(1, &m_gl_fbo)); }

// -----------------------------------------------------------------------------------------------------------------------------------

Framebuffer::~Framebuffer()
{
    GL_CHECK_ERROR(glDeleteFramebuffers(1, &m_gl_fbo));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::bind()
{
    GL_CHECK_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_gl_fbo));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::unbind()
{
    GL_CHECK_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::attach_render_target(uint32_t attachment, Texture* texture, uint32_t layer, uint32_t mip_level, bool draw, bool read)
{
    glBindTexture(texture->target(), texture->id());
    bind();

    if (texture->array_size() > 1)
    {
        GL_CHECK_ERROR(glFramebufferTextureLayer(GL_FRAMEBUFFER,
                                                 GL_COLOR_ATTACHMENT0 + attachment,
                                                 texture->id(),
                                                 mip_level,
                                                 layer));
    }
    else
    {
        GL_CHECK_ERROR(glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, texture->target(), texture->id(), mip_level));
    }

#    if defined(__EMSCRIPTEN__)
    if (draw)
        m_attachments[m_render_target_count++] = GL_COLOR_ATTACHMENT0 + attachment;

    glDrawBuffers(m_render_target_count, m_attachments);
#    else
    if (draw)
    {
        GL_CHECK_ERROR(glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachment));
    }
    else
    {
        GL_CHECK_ERROR(glDrawBuffer(GL_NONE));
    }
#    endif

    if (read)
    {
        GL_CHECK_ERROR(glReadBuffer(GL_COLOR_ATTACHMENT0 + attachment));
    }
    else
    {
        GL_CHECK_ERROR(glReadBuffer(GL_NONE));
    }

    check_status();

    unbind();
    glBindTexture(texture->target(), 0);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::attach_multiple_render_targets(uint32_t  attachment_count,
                                                 Texture** texture)
{
    bind();

    m_render_target_count = attachment_count;

    for (int i = 0; i < m_render_target_count; i++)
    {
        glBindTexture(texture[i]->target(), texture[i]->id());
        GL_CHECK_ERROR(
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, texture[i]->target(), texture[i]->id(), 0));
        m_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    glDrawBuffers(m_render_target_count, m_attachments);

    check_status();

    unbind();
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::attach_render_target(uint32_t     attachment,
                                       TextureCube* texture,
                                       uint32_t     face,
                                       uint32_t     layer,
                                       uint32_t     mip_level,
                                       bool         draw,
                                       bool         read)
{
    glBindTexture(texture->target(), texture->id());
    bind();

    if (texture->array_size() > 1)
    {
#    if defined(__EMSCRIPTEN__)
        DW_LOG_ERROR("WEBGL: glFramebufferTexture3D unsupported!");
#    else
        GL_CHECK_ERROR(glFramebufferTexture3D(GL_FRAMEBUFFER,
                                              GL_COLOR_ATTACHMENT0 + attachment,
                                              GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                              texture->id(),
                                              mip_level,
                                              layer));
#    endif
    }
    else
    {
        GL_CHECK_ERROR(glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture->id(), mip_level));
    }

#    if defined(__EMSCRIPTEN__)
    if (draw)
        m_attachments[m_render_target_count++] = GL_COLOR_ATTACHMENT0 + attachment;

    glDrawBuffers(m_render_target_count, m_attachments);
#    else
    if (draw)
    {
        GL_CHECK_ERROR(glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachment));
    }
    else
    {
        GL_CHECK_ERROR(glDrawBuffer(GL_NONE));
    }
#    endif

    if (read)
    {
        GL_CHECK_ERROR(glReadBuffer(GL_COLOR_ATTACHMENT0 + attachment));
    }
    else
    {
        GL_CHECK_ERROR(glReadBuffer(GL_NONE));
    }

    check_status();

    unbind();
    glBindTexture(texture->target(), 0);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::attach_depth_stencil_target(Texture* texture, uint32_t layer, uint32_t mip_level)
{
    glBindTexture(texture->target(), texture->id());
    bind();

    if (texture->array_size() > 1)
    {
        GL_CHECK_ERROR(glFramebufferTextureLayer(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->id(), mip_level, layer));
    }
    else
    {
#    if defined(__EMSCRIPTEN__)
        DW_LOG_ERROR("WEBGL: glFramebufferTexture unsupported!");
#    else
        GL_CHECK_ERROR(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->id(), mip_level));
#    endif
    }

    check_status();

    unbind();
    glBindTexture(texture->target(), 0);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Framebuffer::attach_depth_stencil_target(TextureCube* texture,
                                              uint32_t     face,
                                              uint32_t     layer,
                                              uint32_t     mip_level)
{
    glBindTexture(texture->target(), texture->id());
    bind();

    if (texture->array_size() > 1)
    {
#    if defined(__EMSCRIPTEN__)
        DW_LOG_ERROR("WEBGL: glFramebufferTexture3D unsupported!");
#    else
        GL_CHECK_ERROR(glFramebufferTexture3D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture->id(), mip_level, layer));
#    endif
    }
    else
    {
        GL_CHECK_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture->id(), mip_level));
    }

#    if !defined(__EMSCRIPTEN__)
    GL_CHECK_ERROR(glDrawBuffer(GL_NONE));
#    endif
    GL_CHECK_ERROR(glReadBuffer(GL_NONE));

    check_status();

    unbind();
    glBindTexture(texture->target(), 0);
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
#    ifndef __EMSCRIPTEN__
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
#    endif
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

uint32_t Framebuffer::render_targets() { return m_render_target_count; }

// -----------------------------------------------------------------------------------------------------------------------------------

Shader* Shader::create_from_file(GLenum type, std::string path, std::vector<std::string> defines)
{
    std::string source;

    if (!utility::read_shader(path, source, defines))
    {
        DW_LOG_ERROR("Failed to read GLSL shader source: " + path);

        // Force assertion failure for debug builds.
        assert(false);

        return nullptr;
    }

    Shader* shader = new Shader(type, source);

    if (shader->compiled())
        return shader;

    return nullptr;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Shader::Shader(GLenum type, std::string source) :
    m_type(type)
{
    GL_CHECK_ERROR(m_gl_shader = glCreateShader(type));

#    if defined(__APPLE__)
    source = "#version 410 core\n" + std::string(source);
#    elif defined(__EMSCRIPTEN__)
    source = "#version 300 es\n precision highp float;\n" + std::string(source);
#    else
    source = "#version 430 core\n" + std::string(source);
#    endif

    GLint  success;
    GLchar log[512];

    const GLchar* src = source.c_str();

    GL_CHECK_ERROR(glShaderSource(m_gl_shader, 1, &src, NULL));
    GL_CHECK_ERROR(glCompileShader(m_gl_shader));
    GL_CHECK_ERROR(glGetShaderiv(m_gl_shader, GL_COMPILE_STATUS, &success));

    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(m_gl_shader, 512, NULL, log);

        std::string log_error = "OPENGL: Shader compilation failed: ";
        log_error += std::string(log);

        DW_LOG_ERROR(log_error);
        m_compiled = false;
    }
    else
        m_compiled = true;
}

// -----------------------------------------------------------------------------------------------------------------------------------

Shader::~Shader() { GL_CHECK_ERROR(glDeleteShader(m_gl_shader)); }

// -----------------------------------------------------------------------------------------------------------------------------------

GLenum Shader::type() { return m_type; }

// -----------------------------------------------------------------------------------------------------------------------------------

bool Shader::compiled() { return m_compiled; }

// -----------------------------------------------------------------------------------------------------------------------------------

Program::Program(uint32_t count, Shader** shaders)
{
#    if !defined(__EMSCRIPTEN__)
    if (count == 1 && shaders[0]->type() != GL_COMPUTE_SHADER)
    {
        DW_LOG_ERROR("OPENGL: Compute shader programs can only have one shader.");
        assert(false);

        return;
    }
#    endif

    GL_CHECK_ERROR(m_gl_program = glCreateProgram());

    for (int i = 0; i < count; i++)
    {
        GL_CHECK_ERROR(glAttachShader(m_gl_program, shaders[i]->m_gl_shader));
    }

    GL_CHECK_ERROR(glLinkProgram(m_gl_program));

    GLint success;
    char  log[512];

    GL_CHECK_ERROR(glGetProgramiv(m_gl_program, GL_LINK_STATUS, &success));

    if (!success)
    {
        glGetProgramInfoLog(m_gl_program, 512, NULL, log);

        std::string log_error = "OPENGL: Shader program linking failed: ";
        log_error += std::string(log);

        DW_LOG_ERROR(log_error);

        return;
    }

    int uniform_count = 0;
    GL_CHECK_ERROR(
        glGetProgramiv(m_gl_program, GL_ACTIVE_UNIFORMS, &uniform_count));

    GLint        size;
    GLenum       type;
    GLsizei      length;
    const GLuint buf_size = 64;
    GLchar       name[buf_size];

    for (int i = 0; i < uniform_count; i++)
    {
        GL_CHECK_ERROR(glGetActiveUniform(m_gl_program, i, buf_size, &length, &size, &type, name));
        GL_CHECK_ERROR(GLuint loc = glGetUniformLocation(m_gl_program, name));

        if (loc != GL_INVALID_INDEX)
            m_location_map[std::string(name)] = loc;
    }

#    if defined(__EMSCRIPTEN__)
    // Bind attributes in OpenGL ES/WebGL versions.

    // int attrib_count = 0;
    // GL_CHECK_ERROR(glGetProgramiv(m_gl_program, GL_ACTIVE_ATTRIBUTES,
    // &attrib_count));

    // for (int i = 0; i < attrib_count; i++)
    //{
    //	GL_CHECK_ERROR(glGetActiveAttrib(m_gl_program, (GLuint)i, buf_size,
    //&length, &size, &type, name));
    //	GL_CHECK_ERROR(glBindAttribLocation(m_gl_program, i, name));
    //}
#    endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

Program::~Program() { glDeleteProgram(m_gl_program); }

// -----------------------------------------------------------------------------------------------------------------------------------

void Program::use() { glUseProgram(m_gl_program); }

// -----------------------------------------------------------------------------------------------------------------------------------

void Program::uniform_block_binding(std::string name, int binding)
{
    GL_CHECK_ERROR(GLuint idx = glGetUniformBlockIndex(m_gl_program, name.c_str()));

    if (idx == GL_INVALID_INDEX)
    {
        std::string uniform_error = "OPENGL: Failed to get Uniform Block Index for Uniform Buffer : ";
        uniform_error += name;

        DW_LOG_ERROR(uniform_error);
    }
    else
        GL_CHECK_ERROR(glUniformBlockBinding(m_gl_program, idx, binding));
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Program::set_uniform(std::string name, int value)
{
    if (m_location_map.find(name) == m_location_map.end())
        return false;

    glUniform1i(m_location_map[name], value);

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

Buffer::Buffer(GLenum type, GLenum usage, size_t size, void* data) :
    m_type(type), m_size(size)
{
    GL_CHECK_ERROR(glGenBuffers(1, &m_gl_buffer));

    GL_CHECK_ERROR(glBindBuffer(m_type, m_gl_buffer));
    GL_CHECK_ERROR(glBufferData(m_type, size, data, usage));
    GL_CHECK_ERROR(glBindBuffer(m_type, 0));

#    if defined(__EMSCRIPTEN__)
    m_staging = malloc(m_size);
#    endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

Buffer::~Buffer()
{
#    if defined(__EMSCRIPTEN__)
    free(m_staging);
#    endif
    glDeleteBuffers(1, &m_gl_buffer);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::bind() { GL_CHECK_ERROR(glBindBuffer(m_type, m_gl_buffer)); }

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::bind_base(int index)
{
    GL_CHECK_ERROR(glBindBufferBase(m_type, index, m_gl_buffer));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::bind_range(int index, size_t offset, size_t size)
{
    GL_CHECK_ERROR(glBindBufferRange(m_type, index, m_gl_buffer, offset, size));
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::unbind() { GL_CHECK_ERROR(glBindBuffer(m_type, 0)); }

// -----------------------------------------------------------------------------------------------------------------------------------

void* Buffer::map(GLenum access)
{
#    if defined(__EMSCRIPTEN__)
    m_mapped_size   = m_size;
    m_mapped_offset = 0;
    return m_staging;
#    else
    GL_CHECK_ERROR(glBindBuffer(m_type, m_gl_buffer));
    GL_CHECK_ERROR(void* ptr = glMapBuffer(m_type, access));
    GL_CHECK_ERROR(glBindBuffer(m_type, 0));
    return ptr;
#    endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void* Buffer::map_range(GLenum access, size_t offset, size_t size)
{
#    if defined(__EMSCRIPTEN__)
    m_mapped_size   = size;
    m_mapped_offset = offset;
    return static_cast<char*>(m_staging) + offset;
#    else
    GL_CHECK_ERROR(glBindBuffer(m_type, m_gl_buffer));
    GL_CHECK_ERROR(void* ptr = glMapBufferRange(m_type, offset, size, access));
    GL_CHECK_ERROR(glBindBuffer(m_type, 0));
    return ptr;
#    endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::unmap()
{
#    if defined(__EMSCRIPTEN__)
    GL_CHECK_ERROR(glBindBuffer(m_type, m_gl_buffer));
    glBufferSubData(m_type, m_mapped_offset, m_mapped_size, static_cast<char*>(m_staging) + m_mapped_offset);
    GL_CHECK_ERROR(glBindBuffer(m_type, 0));
#    else
    GL_CHECK_ERROR(glBindBuffer(m_type, m_gl_buffer));
    GL_CHECK_ERROR(glUnmapBuffer(m_type));
    GL_CHECK_ERROR(glBindBuffer(m_type, 0));
#    endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Buffer::set_data(size_t offset, size_t size, void* data)
{
    GL_CHECK_ERROR(glBindBuffer(m_type, m_gl_buffer));
    glBufferSubData(m_type, offset, size, data);
    GL_CHECK_ERROR(glBindBuffer(m_type, 0));
}

// -----------------------------------------------------------------------------------------------------------------------------------

GLuint Buffer::handle()
{
    return m_gl_buffer;
}

// -----------------------------------------------------------------------------------------------------------------------------------

VertexBuffer::VertexBuffer(GLenum usage, size_t size, void* data) :
    Buffer(GL_ARRAY_BUFFER, usage, size, data) {}

// -----------------------------------------------------------------------------------------------------------------------------------

VertexBuffer::~VertexBuffer() {}

// -----------------------------------------------------------------------------------------------------------------------------------

IndexBuffer::IndexBuffer(GLenum usage, size_t size, void* data) :
    Buffer(GL_ELEMENT_ARRAY_BUFFER, usage, size, data) {}

// -----------------------------------------------------------------------------------------------------------------------------------

IndexBuffer::~IndexBuffer() {}

// -----------------------------------------------------------------------------------------------------------------------------------

UniformBuffer::UniformBuffer(GLenum usage, size_t size, void* data) :
    Buffer(GL_UNIFORM_BUFFER, usage, size, data) {}

// -----------------------------------------------------------------------------------------------------------------------------------

UniformBuffer::~UniformBuffer() {}

// -----------------------------------------------------------------------------------------------------------------------------------

#    if !defined(__EMSCRIPTEN__)
ShaderStorageBuffer::ShaderStorageBuffer(GLenum usage, size_t size, void* data) :
    Buffer(GL_SHADER_STORAGE_BUFFER, usage, size, data)
{
}

// -----------------------------------------------------------------------------------------------------------------------------------

ShaderStorageBuffer::~ShaderStorageBuffer() {}
#    endif

// -----------------------------------------------------------------------------------------------------------------------------------

VertexArray::VertexArray(VertexBuffer* vbo, IndexBuffer* ibo, size_t vertex_size, int attrib_count, VertexAttrib attribs[])
{
    GL_CHECK_ERROR(glGenVertexArrays(1, &m_gl_vao));
    GL_CHECK_ERROR(glBindVertexArray(m_gl_vao));
    vbo->bind();

    if (ibo)
        ibo->bind();

    for (uint32_t i = 0; i < attrib_count; i++)
    {
        GL_CHECK_ERROR(glEnableVertexAttribArray(i));

        if (attribs[i].type == GL_INT)
        {
            GL_CHECK_ERROR(glVertexAttribIPointer(
                i, attribs[i].num_sub_elements, attribs[i].type, vertex_size, (GLvoid*)((uint64_t)attribs[i].offset)));
        }
        else
        {
            GL_CHECK_ERROR(
                glVertexAttribPointer(i, attribs[i].num_sub_elements, attribs[i].type, attribs[i].normalized, vertex_size, (GLvoid*)((uint64_t)attribs[i].offset)));
        }
    }

    GL_CHECK_ERROR(glBindVertexArray(0));

    vbo->unbind();

    if (ibo)
        ibo->unbind();
}

// -----------------------------------------------------------------------------------------------------------------------------------

VertexArray::~VertexArray() { glDeleteVertexArrays(1, &m_gl_vao); }

// -----------------------------------------------------------------------------------------------------------------------------------

void VertexArray::bind() { GL_CHECK_ERROR(glBindVertexArray(m_gl_vao)); }

// -----------------------------------------------------------------------------------------------------------------------------------

void VertexArray::unbind() { GL_CHECK_ERROR(glBindVertexArray(0)); }

// -----------------------------------------------------------------------------------------------------------------------------------

Query::Query() { GL_CHECK_ERROR(glGenQueries(1, &m_query)); }

// -----------------------------------------------------------------------------------------------------------------------------------

Query::~Query() { GL_CHECK_ERROR(glDeleteQueries(1, &m_query)); }

// -----------------------------------------------------------------------------------------------------------------------------------

void Query::query_counter(GLenum type)
{
#    if defined(__EMSCRIPTEN__)
    DW_LOG_FATAL("OPENGL ES: glQueryCounter unsupported on OpenGL ES!");
#    else
    GL_CHECK_ERROR(glQueryCounter(m_query, type));
#    endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

void Query::begin(GLenum type) { GL_CHECK_ERROR(glBeginQuery(type, m_query)); }

// -----------------------------------------------------------------------------------------------------------------------------------

void Query::end(GLenum type) { GL_CHECK_ERROR(glEndQuery(type)); }

// -----------------------------------------------------------------------------------------------------------------------------------

void Query::result_64(uint64_t* ptr)
{
#    if defined(__EMSCRIPTEN__)
    DW_LOG_FATAL("OPENGL ES: glGetQueryObjectui64v unsupported on OpenGL ES!");
#    else
    GL_CHECK_ERROR(glGetQueryObjectui64v(m_query, GL_QUERY_RESULT, ptr));
#    endif
}

// -----------------------------------------------------------------------------------------------------------------------------------

bool Query::result_available()
{
#    if defined(__EMSCRIPTEN__)
    GLuint done = 0;
    GL_CHECK_ERROR(
        glGetQueryObjectuiv(m_query, GL_QUERY_RESULT_AVAILABLE, &done));
#    else
    int done = 0;
    GL_CHECK_ERROR(glGetQueryObjectiv(m_query, GL_QUERY_RESULT_AVAILABLE, &done));
#    endif
    return done == 1;
}

// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace gl
} // namespace dw

#endif