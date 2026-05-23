#include "SyphonPublisher.h"

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <Syphon/SyphonOpenGLServer.h>

#include <vector>

namespace SoundShape {

namespace
{
    NSString* nsString(const juce::String& s)
    {
        return [NSString stringWithUTF8String:s.toRawUTF8()];
    }

    juce::Result glResult(CGLError error, const char* action)
    {
        if (error == kCGLNoError)
            return juce::Result::ok();

        return juce::Result::fail(juce::String("Syphon could not ")
                                  + action
                                  + ": "
                                  + CGLErrorString(error));
    }

    juce::String exceptionMessage(NSException* exception)
    {
        if (exception == nil)
            return "Unknown Syphon error.";

        auto* name = exception.name != nil ? [exception.name UTF8String] : "NSException";
        auto* reason = exception.reason != nil ? [exception.reason UTF8String] : "No reason provided.";

        return juce::String(name)
               + ": "
               + juce::String(reason);
    }
}

class SyphonPublisher::Impl
{
public:
    ~Impl()
    {
        stop();

        if (context_ != nullptr)
        {
            CGLReleaseContext(context_);
            context_ = nullptr;
        }

        if (pixelFormat_ != nullptr)
        {
            CGLReleasePixelFormat(pixelFormat_);
            pixelFormat_ = nullptr;
        }
    }

    juce::Result start(const juce::String& sourceName)
    {
        if (server_ != nil)
            return juce::Result::ok();

        const auto contextResult = ensureContext();
        if (contextResult.failed())
            return contextResult;

        @try
        {
            server_ = [[SyphonOpenGLServer alloc] initWithName:nsString(sourceName)
                                                       context:context_
                                                       options:nil];
        }
        @catch (NSException* exception)
        {
            server_ = nil;
            return juce::Result::fail("Syphon could not start an external source: "
                                      + exceptionMessage(exception));
        }

        if (server_ == nil)
            return juce::Result::fail("Syphon could not start an external source.");

        sourceName_ = sourceName;
        return juce::Result::ok();
    }

    void stop()
    {
        if (server_ != nil)
        {
            [server_ stop];
            server_ = nil;
        }

        deleteTexture();
        sourceName_.clear();
    }

    bool isPublishing() const noexcept
    {
        return server_ != nil;
    }

    juce::String getSourceName() const
    {
        return sourceName_;
    }

    juce::Result publishFrame(const juce::Image& image)
    {
        if (server_ == nil)
            return juce::Result::ok();

        if (! image.isValid() || image.getWidth() <= 0 || image.getHeight() <= 0)
            return juce::Result::fail("Syphon received an empty frame.");

        const auto contextResult = makeContextCurrent();
        if (contextResult.failed())
            return contextResult;

        const juce::ScopedValueSetter<bool> unlocker(contextLocked_, true, false);
        const auto textureResult = uploadImage(image);
        if (textureResult.failed())
        {
            CGLUnlockContext(context_);
            return textureResult;
        }

        @try
        {
            [server_ publishFrameTexture:texture_
                            textureTarget:GL_TEXTURE_2D
                              imageRegion:NSMakeRect(0.0, 0.0, textureWidth_, textureHeight_)
                        textureDimensions:NSMakeSize(textureWidth_, textureHeight_)
                                  flipped:YES];
            glFlush();
        }
        @catch (NSException* exception)
        {
            CGLUnlockContext(context_);
            return juce::Result::fail("Syphon could not publish a frame: "
                                      + exceptionMessage(exception));
        }

        CGLUnlockContext(context_);
        return juce::Result::ok();
    }

private:
    juce::Result ensureContext()
    {
        if (context_ != nullptr)
            return juce::Result::ok();

        CGLPixelFormatAttribute attributes[] = {
            static_cast<CGLPixelFormatAttribute>(kCGLPFAAccelerated),
            static_cast<CGLPixelFormatAttribute>(kCGLPFAAllowOfflineRenderers),
            static_cast<CGLPixelFormatAttribute>(kCGLPFAColorSize),
            static_cast<CGLPixelFormatAttribute>(24),
            static_cast<CGLPixelFormatAttribute>(kCGLPFAAlphaSize),
            static_cast<CGLPixelFormatAttribute>(8),
            static_cast<CGLPixelFormatAttribute>(0)
        };

        GLint pixelFormatCount = 0;
        auto error = CGLChoosePixelFormat(attributes, &pixelFormat_, &pixelFormatCount);
        if (error != kCGLNoError || pixelFormat_ == nullptr || pixelFormatCount <= 0)
            return glResult(error, "choose an OpenGL pixel format");

        error = CGLCreateContext(pixelFormat_, nullptr, &context_);
        if (error != kCGLNoError || context_ == nullptr)
            return glResult(error, "create an OpenGL context");

        return juce::Result::ok();
    }

    juce::Result makeContextCurrent()
    {
        if (context_ == nullptr)
            return juce::Result::fail("Syphon has no OpenGL context.");

        if (contextLocked_)
            return juce::Result::ok();

        auto error = CGLLockContext(context_);
        if (error != kCGLNoError)
            return glResult(error, "lock its OpenGL context");

        error = CGLSetCurrentContext(context_);
        if (error != kCGLNoError)
        {
            CGLUnlockContext(context_);
            return glResult(error, "activate its OpenGL context");
        }

        return juce::Result::ok();
    }

    void deleteTexture()
    {
        if (texture_ == 0 || context_ == nullptr)
            return;

        if (makeContextCurrent().wasOk())
        {
            glDeleteTextures(1, &texture_);
            CGLUnlockContext(context_);
        }

        texture_ = 0;
        textureWidth_ = 0;
        textureHeight_ = 0;
    }

    juce::Result uploadImage(const juce::Image& image)
    {
        auto source = image;
        if (source.getFormat() != juce::Image::ARGB)
            source = source.convertedToFormat(juce::Image::ARGB);

        juce::Image::BitmapData bitmap(source, juce::Image::BitmapData::readOnly);
        if (bitmap.pixelStride != 4 || bitmap.lineStride <= 0)
        {
            packedPixels_.resize(static_cast<std::size_t>(source.getWidth())
                                 * static_cast<std::size_t>(source.getHeight())
                                 * 4U);

            for (int y = 0; y < source.getHeight(); ++y)
            {
                auto* dest = packedPixels_.data()
                             + static_cast<std::size_t>(y)
                                   * static_cast<std::size_t>(source.getWidth()) * 4U;

                for (int x = 0; x < source.getWidth(); ++x)
                {
                    const auto colour = bitmap.getPixelColour(x, y);
                    dest[x * 4 + 0] = colour.getBlue();
                    dest[x * 4 + 1] = colour.getGreen();
                    dest[x * 4 + 2] = colour.getRed();
                    dest[x * 4 + 3] = colour.getAlpha();
                }
            }

            return uploadPixels(packedPixels_.data(), source.getWidth(), source.getHeight(), source.getWidth());
        }

        return uploadPixels(bitmap.getLinePointer(0),
                            source.getWidth(),
                            source.getHeight(),
                            bitmap.lineStride / bitmap.pixelStride);
    }

    juce::Result uploadPixels(const void* pixels, int width, int height, int rowLengthPixels)
    {
        if (pixels == nullptr)
            return juce::Result::fail("Syphon received no frame pixels.");

        if (texture_ == 0)
            glGenTextures(1, &texture_);

        if (texture_ == 0)
            return juce::Result::fail("Syphon could not create an OpenGL texture.");

        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLengthPixels);

        if (width != textureWidth_ || height != textureHeight_)
        {
            textureWidth_ = width;
            textureHeight_ = height;
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA8,
                         width,
                         height,
                         0,
                         GL_BGRA,
                         GL_UNSIGNED_BYTE,
                         pixels);
        }
        else
        {
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            width,
                            height,
                            GL_BGRA,
                            GL_UNSIGNED_BYTE,
                            pixels);
        }

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        const auto error = glGetError();
        if (error != GL_NO_ERROR)
            return juce::Result::fail("Syphon could not upload the frame to OpenGL.");

        return juce::Result::ok();
    }

    CGLPixelFormatObj pixelFormat_ = nullptr;
    CGLContextObj context_ = nullptr;
    SyphonOpenGLServer* server_ = nil;
    GLuint texture_ = 0;
    int textureWidth_ = 0;
    int textureHeight_ = 0;
    bool contextLocked_ = false;
    juce::String sourceName_;
    std::vector<juce::uint8> packedPixels_;
};

SyphonPublisher::SyphonPublisher()
    : impl_(std::make_unique<Impl>())
{
}

SyphonPublisher::~SyphonPublisher() = default;

juce::Result SyphonPublisher::start(const juce::String& sourceName)
{
    return impl_->start(sourceName);
}

void SyphonPublisher::stop()
{
    impl_->stop();
}

bool SyphonPublisher::isPublishing() const noexcept
{
    return impl_->isPublishing();
}

juce::String SyphonPublisher::getSourceName() const
{
    return impl_->getSourceName();
}

juce::Result SyphonPublisher::publishFrame(const juce::Image& image)
{
    return impl_->publishFrame(image);
}

} // namespace SoundShape
