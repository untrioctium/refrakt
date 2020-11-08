#pragma once

#include <glad/glad.h>

template<typename DataType>
class storage_buffer
{
public:

	storage_buffer(std::size_t buf_size, const DataType* src = nullptr) : size_(buf_size) {
		glCreateBuffers(1, &name_);
		//glNamedBufferData(name_, item_size_ * buf_size, src, GL_DYNAMIC_COPY);
		glNamedBufferStorage(name_, item_size_ * buf_size, src, GL_DYNAMIC_STORAGE_BIT);

	}

	storage_buffer(const std::vector<DataType>& src) : storage_buffer(src.size(), src.data()) {}

	~storage_buffer() {
		glDeleteBuffers(1, &name_);
	}

	std::size_t size() { return size_; }
	GLuint name() { return name_; }

	void update_one(std::size_t idx, const DataType& v) {
		glNamedBufferSubData(name_, item_size_ * idx, item_size_, (void*)&v);
	}

	void update_many(std::size_t offset, std::size_t count, const DataType* src) {
		glNamedBufferSubData(name_, item_size_ * offset, count * item_size_, (void*)src);
	}

	void update_all(const DataType* src) {
		glNamedBufferSubData(name_, 0, item_size_ * size_, (void*)src);
	}

	DataType get_one(std::size_t idx) {
		DataType result;
		glGetNamedBufferSubData(name_, idx * item_size_, item_size_,(void*) &result);
		return result;
	}

	void zero_out() {
		glClearNamedBufferData(name_, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	}

private:
	static inline const std::size_t item_size_ = sizeof(DataType);

	GLuint name_;
	std::size_t size_;

};

template<typename DataType>
class uniform_buffer {
public:
	uniform_buffer(const DataType* src = nullptr) {
		glCreateBuffers(1, &name_);
		glNamedBufferData(name_, data_size_, src, GL_DYNAMIC_DRAW);
		data_ = (DataType*)glMapNamedBuffer(name_, GL_READ_WRITE);
	}

	~uniform_buffer() {
		glUnmapNamedBuffer(name_);
		glDeleteBuffers(1, &name_);
	}

	void update(const DataType& v) {
		std::memcpy(data_, &v, data_size_);
	}

	DataType* ptr() { return data_; }
	GLuint name() { return name_; }

private:
	static inline const std::size_t data_size_ = sizeof(DataType);

	GLuint name_;
	DataType* data_;

};

template<typename DataType>
class texture {
public:
	texture(std::size_t w, std::size_t h): width_(w), height_(h)  {
		GLint bound_id;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound_id);
		glGenTextures(1, &name_);
		glBindTexture(GL_TEXTURE_2D, name_);
		glTexImage2D(GL_TEXTURE_2D, 0, gl_format(), w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, bound_id);
	}

	~texture() {
		glDeleteTextures(1, &name_);
	}

	auto width() const -> std::size_t { return width_; }
	auto height() const -> std::size_t { return height_; }
	auto format() const { return gl_format(); }

	GLuint name() { return name_;  }

	using pix_format_t = std::array<std::uint8_t, 4>;
	std::vector<pix_format_t> get_pixels() {
		auto ret = std::vector<pix_format_t>{};
		ret.resize(width_ * height_);
		auto total_bytes = ret.size() * sizeof(pix_format_t);
		glGetTextureImage(name_, 0, GL_RGBA, GL_UNSIGNED_BYTE, total_bytes, (void*)ret.data());
		return ret;
	}

private:

	static constexpr auto gl_format() {
		if constexpr (std::is_same_v<DataType, unsigned int>) return GL_RGBA32UI;
		if constexpr (std::is_same_v<DataType, int>) return GL_RGBA32I;
		if constexpr (std::is_same_v<DataType, float>) return GL_RGBA32F;
	}
	GLuint name_;

	std::size_t width_;
	std::size_t height_;
};

class frame_buffer {
public:
	frame_buffer() {
		glGenFramebuffers(1, &name_);
	}

	void bind() { glBindFramebuffer(GL_FRAMEBUFFER, name_); }
	void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

	static void attach(GLuint name) { glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, name, 0); }

private:
	GLuint name_;
};
