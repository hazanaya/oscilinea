#include "VideoRecorder.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>

namespace SoundShape {

namespace
{
    NSString* nsString(const juce::String& s)
    {
        return [NSString stringWithUTF8String:s.toRawUTF8()];
    }

    juce::Image fittedARGB(const juce::Image& source, int width, int height)
    {
        if (source.isValid()
            && source.isARGB()
            && source.getWidth() == width
            && source.getHeight() == height)
            return source;

        juce::Image image(juce::Image::ARGB, width, height, true);
        juce::Graphics g(image);
        g.fillAll(juce::Colours::black);
        if (source.isValid())
            g.drawImage(source, image.getBounds().toFloat());
        return image;
    }
}

struct VideoRecorder::Impl
{
    AVAssetWriter* writer = nil;
    AVAssetWriterInput* input = nil;
    AVAssetWriterInputPixelBufferAdaptor* adaptor = nil;
    int width = 0;
    int height = 0;
    int fps = 30;
    int64_t frameIndex = 0;
    bool recording = false;
};

VideoRecorder::VideoRecorder()
    : impl_(std::make_unique<Impl>())
{
}

VideoRecorder::~VideoRecorder()
{
    if (isRecording())
        stop();
}

bool VideoRecorder::isAvailable() const noexcept
{
    return true;
}

bool VideoRecorder::isRecording() const noexcept
{
    return impl_->recording;
}

juce::Result VideoRecorder::start(const juce::File& outputFile, int width, int height, int fps)
{
    if (impl_->recording)
        return juce::Result::fail("Recording is already running.");

    if (width <= 1 || height <= 1)
        return juce::Result::fail("Invalid recording size.");

    width = width + (width % 2);
    height = height + (height % 2);
    fps = juce::jlimit(1, 120, fps);

    outputFile.getParentDirectory().createDirectory();
    if (outputFile.existsAsFile())
        outputFile.deleteFile();

    @autoreleasepool
    {
        NSURL* url = [NSURL fileURLWithPath:nsString(outputFile.getFullPathName())];
        NSError* error = nil;

        impl_->writer = [[AVAssetWriter alloc] initWithURL:url
                                                   fileType:AVFileTypeQuickTimeMovie
                                                      error:&error];
        if (impl_->writer == nil)
            return juce::Result::fail(error != nil ? juce::String([error.localizedDescription UTF8String])
                                                   : "Could not create AVAssetWriter.");

        NSDictionary* compression = @{
            AVVideoAverageBitRateKey: @(juce::jmax(2000000, width * height * 6))
        };

        NSDictionary* settings = @{
            AVVideoCodecKey: AVVideoCodecTypeH264,
            AVVideoWidthKey: @(width),
            AVVideoHeightKey: @(height),
            AVVideoCompressionPropertiesKey: compression
        };

        impl_->input = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo
                                                           outputSettings:settings];
        impl_->input.expectsMediaDataInRealTime = YES;

        NSDictionary* attributes = @{
            (NSString*) kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
            (NSString*) kCVPixelBufferWidthKey: @(width),
            (NSString*) kCVPixelBufferHeightKey: @(height)
        };

        impl_->adaptor =
            [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:impl_->input
                                                                             sourcePixelBufferAttributes:attributes];

        if (! [impl_->writer canAddInput:impl_->input])
            return juce::Result::fail("AVAssetWriter cannot add the video input.");

        [impl_->writer addInput:impl_->input];

        if (! [impl_->writer startWriting])
            return juce::Result::fail("AVAssetWriter could not start writing.");

        [impl_->writer startSessionAtSourceTime:kCMTimeZero];
    }

    impl_->width = width;
    impl_->height = height;
    impl_->fps = fps;
    impl_->frameIndex = 0;
    impl_->recording = true;
    return juce::Result::ok();
}

juce::Result VideoRecorder::addFrame(const juce::Image& image)
{
    if (! impl_->recording)
        return juce::Result::ok();

    if (impl_->input == nil || impl_->adaptor == nil)
        return juce::Result::fail("Recorder is not initialized.");

    if (! impl_->input.readyForMoreMediaData)
        return juce::Result::ok();

    CVPixelBufferPoolRef pool = impl_->adaptor.pixelBufferPool;
    if (pool == nullptr)
        return juce::Result::fail("Recorder has no pixel buffer pool.");

    CVPixelBufferRef pixelBuffer = nullptr;
    const auto status = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault,
                                                           pool,
                                                           &pixelBuffer);
    if (status != kCVReturnSuccess || pixelBuffer == nullptr)
        return juce::Result::fail("Could not allocate a video frame.");

    const juce::Image frame = fittedARGB(image, impl_->width, impl_->height);
    const juce::Image::BitmapData src(frame, juce::Image::BitmapData::readOnly);

    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    auto* dstBase = static_cast<unsigned char*>(CVPixelBufferGetBaseAddress(pixelBuffer));
    const auto dstStride = static_cast<int>(CVPixelBufferGetBytesPerRow(pixelBuffer));

    for (int y = 0; y < impl_->height; ++y)
    {
        const auto* srcRow = src.getLinePointer(y);
        auto* dstRow = dstBase + y * dstStride;

        if (src.pixelStride == 4)
        {
            std::memcpy(dstRow, srcRow, static_cast<std::size_t>(impl_->width * 4));
        }
        else
        {
            for (int x = 0; x < impl_->width; ++x)
            {
                const auto c = src.getPixelColour(x, y);
                dstRow[x * 4 + 0] = c.getBlue();
                dstRow[x * 4 + 1] = c.getGreen();
                dstRow[x * 4 + 2] = c.getRed();
                dstRow[x * 4 + 3] = 255;
            }
        }
    }

    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

    const CMTime frameTime = CMTimeMake(impl_->frameIndex, impl_->fps);
    const BOOL appended = [impl_->adaptor appendPixelBuffer:pixelBuffer
                                       withPresentationTime:frameTime];
    CVPixelBufferRelease(pixelBuffer);

    if (! appended)
        return juce::Result::fail("Could not append video frame.");

    ++impl_->frameIndex;
    return juce::Result::ok();
}

juce::Result VideoRecorder::stop()
{
    if (! impl_->recording)
        return juce::Result::ok();

    impl_->recording = false;

    if (impl_->input != nil)
        [impl_->input markAsFinished];

    __block BOOL success = YES;
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);

    [impl_->writer finishWritingWithCompletionHandler:^{
        success = impl_->writer.status == AVAssetWriterStatusCompleted;
        dispatch_semaphore_signal(sema);
    }];

    dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);

    impl_->writer = nil;
    impl_->input = nil;
    impl_->adaptor = nil;

    return success ? juce::Result::ok()
                   : juce::Result::fail("AVAssetWriter did not finish successfully.");
}

} // namespace SoundShape
