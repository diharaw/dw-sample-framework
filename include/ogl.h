#pragma once

#if !defined(DWSF_VULKAN)

#    if defined(__EMSCRIPTEN__)
#        define GLFW_INCLUDE_ES3
#        include <GLFW/glfw3.h>
#        include <GLES3/gl3.h>
#        include <GLES3/gl2ext.h>
#    else
#        include <glad/glad.h>
#    endif
#    include <vector>
#    include <string>
#    include <unordered_map>
#    include <glm.hpp>
#    include <memory>
//#define DW_ENABLE_GL_ERROR_CHECK
// OpenGL error checking macro.
#    ifdef DW_ENABLE_GL_ERROR_CHECK
#        define GL_CHECK_ERROR(x)                                                                              \
            x;                                                                                                 \
            {                                                                                                  \
                GLenum err(glGetError());                                                                      \
                                                                                                               \
                while (err != GL_NO_ERROR)                                                                     \
                {                                                                                              \
                    std::string error;                                                                         \
                                                                                                               \
                    switch (err)                                                                               \
                    {                                                                                          \
                        case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;                         \
                        case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;                                   \
                        case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;                                 \
                        case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;                                 \
                        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break; \
                    }                                                                                          \
                                                                                                               \
                    std::string formatted_error = "OPENGL: ";                                                  \
                    formatted_error             = formatted_error + error;                                     \
                    formatted_error             = formatted_error + ", LINE:";                                 \
                    formatted_error             = formatted_error + std::to_string(__LINE__);                  \
                    DW_LOG_ERROR(formatted_error);                                                             \
                    err = glGetError();                                                                        \
                }                                                                                              \
            }
#    else
#        define GL_CHECK_ERROR(x) x
#    endif

#    if defined(__EMSCRIPTEN__)
#        define GL_WRITE_ONLY 0
#    endif

namespace dw
{
namespace gl
{
class Object
{
public:
    Object(const GLenum& identifier);
    virtual ~Object();

protected:
    void set_name(const GLuint& name, const std::string& label);

private:
    GLenum      m_identifier;
    std::string m_name;
};

// Texture base class.
class Texture : public Object
{
public:
    using Ptr = std::shared_ptr<Texture>;

    virtual ~Texture();

    // Bind texture to specified texture unit i.e GL_TEXTURE<unit>.
    void bind(uint32_t unit);
    void unbind(uint32_t unit);

    // Binding to image units.
    void bind_image(uint32_t unit, uint32_t mip_level, uint32_t layer, GLenum access, GLenum format);

    // Mipmap generation.
    void generate_mipmaps();

    // Getters.
    GLuint        id();
    GLenum        target();
    uint32_t      array_size();
    uint32_t      version();
    uint32_t      mip_levels();
    inline GLenum internal_format() { return m_internal_format; }
    inline GLenum format() { return m_format; }
    inline GLenum type() { return m_type; }

    // Texture sampler functions.
    void     set_wrapping(GLenum s, GLenum t, GLenum r);
    void     set_border_color(float r, float g, float b, float a);
    void     set_min_filter(GLenum filter);
    void     set_mag_filter(GLenum filter);
    void     set_compare_mode(GLenum mode);
    void     set_compare_func(GLenum func);
    bool     is_compressed(int mip_level);
    int      compressed_size(int mip_level);
    GLuint64 make_texture_handle_resident();
    void     make_texture_handle_non_resident();
    GLuint64 make_image_handle_resident(GLenum access, GLint level, GLboolean layered, GLint layer);
    void     make_image_handle_non_resident();

    void set_name(const std::string& name);

protected:
    Texture();

protected:
    GLuint   m_gl_tex = UINT32_MAX;
    GLenum   m_target;
    GLenum   m_internal_format;
    GLenum   m_format;
    GLenum   m_type;
    uint32_t m_version = 0;
    uint32_t m_array_size;
    uint32_t m_mip_levels;
    GLuint64 m_texture_handle = 0;
    GLuint64 m_image_handle   = 0;
};

class Texture1D : public Texture
{
public:
    using Ptr = std::shared_ptr<Texture1D>;

    static Texture1D::Ptr create(uint32_t w, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type);

    ~Texture1D();
    void     write_data(int array_index, int mip_level, void* data);
    void     write_sub_data(int array_index, int mip_level, int x_offset, int width, void* data);
    void     write_compressed_data(int array_index, int mip_level, int size, void* data);
    void     write_compressed_sub_data(int array_index, int mip_level, int x_offset, int width, int size, void* data);
    void     resize(uint32_t w);
    uint32_t width();

private:
    Texture1D(uint32_t w, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type);

protected:
    Texture1D();
    void allocate();

protected:
    uint32_t m_width;
};

class Texture2D : public Texture
{
public:
    using Ptr = std::shared_ptr<Texture2D>;

    static Texture2D::Ptr create(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, uint32_t num_samples, GLenum internal_format, GLenum format, GLenum type);
    static Texture2D::Ptr create_from_file(std::string path, bool flip_vertical = true, bool srgb = false);

    ~Texture2D();
    void     write_data(int array_index, int mip_level, void* data);
    void     write_sub_data(int array_index, int mip_level, int x_offset, int y_offset, int width, int height, void* data);
    void     write_compressed_data(int array_index, int mip_level, size_t size, void* data);
    void     write_compressed_sub_data(int array_index, int mip_level, int x_offset, int y_offset, int width, int height, size_t size, void* data);
    void     read_data(int mip_level, std::vector<uint8_t>& buffer);
    void     extents(int mip_level, int& width, int& height);
    void     resize(uint32_t w, uint32_t h);
    uint32_t width();
    uint32_t height();
    uint32_t num_samples();

private:
    Texture2D(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, uint32_t num_samples, GLenum internal_format, GLenum format, GLenum type);
    void allocate();

protected:
    Texture2D();

protected:
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_num_samples;
};

class Texture3D : public Texture
{
public:
    using Ptr = std::shared_ptr<Texture3D>;

    static Texture3D::Ptr create(uint32_t w, uint32_t h, uint32_t d, int mip_levels, GLenum internal_format, GLenum format, GLenum type);

    ~Texture3D();
    void     write_data(int slice, int mip_level, void* data);
    void     write_sub_data(int slice, int mip_level, int x_offset, int y_offset, int width, int height, void* data);
    void     write_compressed_data(int slice, int mip_level, size_t size, void* data);
    void     write_compressed_sub_data(int slice, int mip_level, int x_offset, int y_offset, int width, int height, size_t size, void* data);
    void     read_data(int mip_level, std::vector<uint8_t>& buffer);
    void     extents(int mip_level, int& width, int& height, int& depth);
    void     resize(uint32_t w, uint32_t h, uint32_t d);
    uint32_t width();
    uint32_t height();
    uint32_t depth();

private:
    Texture3D(uint32_t w, uint32_t h, uint32_t d, int mip_levels, GLenum internal_format, GLenum format, GLenum type);
    void allocate();

protected:
    Texture3D();

protected:
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_depth;
};

class TextureCube : public Texture
{
public:
    using Ptr = std::shared_ptr<TextureCube>;

    static TextureCube::Ptr create(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type);
    static TextureCube::Ptr create_from_files(std::string path[], bool srgb);

    ~TextureCube();
    void     write_data(int face_index, int array_index, int mip_level, void* data);
    void     write_sub_data(int face_index, int array_index, int mip_level, int x_offset, int y_offset, int width, int height, void* data);
    void     write_compressed_data(int face_index, int array_index, int mip_level, size_t size, void* data);
    void     write_compressed_sub_data(int face_index, int array_index, int mip_level, int x_offset, int y_offset, int width, int height, size_t size, void* data);
    void     read_data(int mip_level, std::vector<uint8_t>& buffer);
    void     extents(int mip_level, int& width, int& height);
    void     resize(uint32_t w, uint32_t h);
    uint32_t width();
    uint32_t height();

private:
    TextureCube(uint32_t w, uint32_t h, uint32_t array_size, int32_t mip_levels, GLenum internal_format, GLenum format, GLenum type);

protected:
    TextureCube();
    void allocate();

protected:
    uint32_t m_width;
    uint32_t m_height;
};

class Texture1DView : public Texture1D
{
public:
    using Ptr = std::shared_ptr<Texture1DView>;

    static Texture1DView::Ptr create(Texture1D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers);

    ~Texture1DView();

    inline Texture1D::Ptr original_texture() { return m_origin_texture.lock(); }

private:
    Texture1DView(Texture1D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers);

private:
    std::weak_ptr<Texture1D> m_origin_texture;
};

class Texture2DView : public Texture2D
{
public:
    using Ptr = std::shared_ptr<Texture2DView>;

    static Texture2DView::Ptr create(Texture2D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers);

    ~Texture2DView();

    inline Texture2D::Ptr original_texture() { return m_origin_texture.lock(); }

private:
    Texture2DView(Texture2D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers);

private:
    std::weak_ptr<Texture2D> m_origin_texture;
};

class Texture3DView : public Texture3D
{
public:
    using Ptr = std::shared_ptr<Texture3DView>;

    static Texture3DView::Ptr create(Texture3D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers);

    ~Texture3DView();

    inline Texture3D::Ptr original_texture() { return m_origin_texture.lock(); }

private:
    Texture3DView(Texture3D::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers);

private:
    std::weak_ptr<Texture3D> m_origin_texture;
};

class TextureCubeView : public TextureCube
{
public:
    using Ptr = std::shared_ptr<TextureCubeView>;

    static TextureCubeView::Ptr create(TextureCube::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers);

    ~TextureCubeView();

    inline TextureCube::Ptr original_texture() { return m_origin_texture.lock(); }

private:
    TextureCubeView(TextureCube::Ptr origin_texture, GLenum new_target, int min_level, int num_levels, int min_layer, int num_layers);

private:
    std::weak_ptr<TextureCube> m_origin_texture;
};

class Framebuffer : public Object
{
public:
    using Ptr = std::shared_ptr<Framebuffer>;

    static Framebuffer::Ptr create(std::vector<Texture::Ptr> color_attachments, Texture::Ptr depth_stencil_attachment);

    ~Framebuffer();

    void bind();
    void unbind();

    void set_name(const std::string& name);

private:
    Framebuffer(std::vector<Texture::Ptr> color_attachments, Texture::Ptr depth_stencil_attachment);

private:
    void check_status();

private:
    GLuint m_gl_fbo;
};

class Shader : public Object
{
    friend class Program;

public:
    using Ptr = std::shared_ptr<Shader>;

    static Shader::Ptr create_from_file(GLenum type, std::string path, std::vector<std::string> defines = std::vector<std::string>());

    static Shader::Ptr create(GLenum type, std::string source);

    ~Shader();
    GLenum type();
    bool   compiled();
    GLuint id();

    void set_name(const std::string& name);

private:
    Shader(GLenum type, std::string source);

private:
    bool   m_compiled;
    GLuint m_gl_shader;
    GLenum m_type;
};

class Program : public Object
{
public:
    struct UniformReflection
    {
        uint32_t    location;
        std::string name;
        GLenum      type;
    };

    struct SamplerReflection
    {
        uint32_t    location;
        std::string name;
        GLenum      type;
    };

    struct ImageReflection
    {
        uint32_t    location;
        std::string name;
        GLenum      type;
    };

    struct UBOReflection
    {
        uint32_t    binding;
        std::string name;
    };

    struct SSBOReflection
    {
        uint32_t    binding;
        std::string name;
    };

    struct ReflectionData
    {
        std::unordered_map<uint32_t, UniformReflection> uniforms;
        std::unordered_map<uint32_t, SamplerReflection> samplers;
        std::unordered_map<uint32_t, ImageReflection>   images;
        std::unordered_map<uint32_t, UBOReflection>     ubos;
        std::unordered_map<uint32_t, SSBOReflection>    ssbos;
    };

    using Ptr = std::shared_ptr<Program>;

    static Program::Ptr create(std::vector<Shader::Ptr> shaders);

    ~Program();
    void    use();
    int32_t num_active_uniform_blocks();
    void    uniform_block_binding(std::string name, int binding);
    bool    set_uniform(std::string name, int32_t value);
    bool    set_uniform(std::string name, uint32_t value);
    bool    set_uniform(std::string name, float value);
    bool    set_uniform(std::string name, glm::vec2 value);
    bool    set_uniform(std::string name, glm::vec3 value);
    bool    set_uniform(std::string name, glm::vec4 value);
    bool    set_uniform(std::string name, glm::mat2 value);
    bool    set_uniform(std::string name, glm::mat3 value);
    bool    set_uniform(std::string name, glm::mat4 value);
    bool    set_uniform(std::string name, int count, int* value);
    bool    set_uniform(std::string name, int count, float* value);
    bool    set_uniform(std::string name, int count, glm::vec2* value);
    bool    set_uniform(std::string name, int count, glm::vec3* value);
    bool    set_uniform(std::string name, int count, glm::vec4* value);
    bool    set_uniform(std::string name, int count, glm::mat2* value);
    bool    set_uniform(std::string name, int count, glm::mat3* value);
    bool    set_uniform(std::string name, int count, glm::mat4* value);
    void    extract_reflection_data(ReflectionData& reflection_data);
    GLint   id();

    void set_name(const std::string& name);

private:
    Program(std::vector<Shader::Ptr> shaders);

private:
    GLuint                                  m_gl_program;
    int32_t                                 m_num_active_uniform_blocks;
    std::unordered_map<std::string, GLuint> m_location_map;
};

class Buffer : public Object
{
public:
    using Ptr = std::shared_ptr<Buffer>;

    static Buffer::Ptr create(GLenum type, GLenum flags, size_t size, void* data = nullptr);

    virtual ~Buffer();
    void   bind();
    void   bind(GLenum type);
    void   bind_base(int index);
    void   bind_range(int index, size_t offset, size_t size);
    void   bind_base(GLenum type, int index);
    void   bind_range(GLenum type, int index, size_t offset, size_t size);
    void   unbind();
    void*  map(GLenum access);
    void*  map_range(GLenum access, size_t offset, size_t size);
    void   unmap();
    void   flush_mapped_range(size_t offset, size_t length);
    void   write_data(size_t offset, size_t size, void* data);
    void   copy(Buffer::Ptr dst, GLintptr read_offset, GLintptr write_offset, GLsizeiptr size);
    size_t size();

    void set_name(const std::string& name);

private:
    Buffer(GLenum type, GLenum flags, size_t size, void* data);

protected:
    GLenum m_type;
    GLuint m_gl_buffer;
    size_t m_size;
};

struct VertexAttrib
{
    uint32_t num_sub_elements;
    uint32_t type;
    bool     normalized;
    uint32_t offset;
};

class VertexArray : public Object
{
public:
    using Ptr = std::shared_ptr<VertexArray>;

    static VertexArray::Ptr create(Buffer::Ptr vbo, Buffer::Ptr ibo, size_t vertex_size, int attrib_count, VertexAttrib attribs[]);

    ~VertexArray();
    void bind();
    void unbind();

    void set_name(const std::string& name);

private:
    VertexArray(Buffer::Ptr vbo, Buffer::Ptr ibo, size_t vertex_size, int attrib_count, VertexAttrib attribs[]);

private:
    GLuint m_gl_vao;
};

class Query : public Object
{
public:
    Query();
    ~Query();
    void query_counter(GLenum type);
    void begin(GLenum type);
    void end(GLenum type);
    void result_64(uint64_t* ptr);
    bool result_available();

    void set_name(const std::string& name);

private:
    GLuint m_query;
};

class Fence
{
public:
    Fence();
    ~Fence();
    void insert();
    void wait();

private:
    GLsync m_fence = nullptr;
};
} // namespace gl
} // namespace dw

#endif