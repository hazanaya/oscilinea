#include "VideoRecorder.h"

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreVideo/CoreVideo.h>

#include <cmath>
#include <vector>

namespace SoundShape {

namespace
{
    NSString* nsString(const juce::String& s)
    {
        return [NSString stringWithUTF8String:s.toRawUTF8()];
    }

    juce::String juceString(NSString* s)
    {
        return s != nil ? juce::String([s UTF8String]) : juce::String();
    }

    juce::String exceptionMessage(NSException* exception)
    {
        if (exception == nil)
            return "Video recording failed while preparing the encoder.";

        auto message = juceString(exception.name);
        const auto reason = juceString(exception.reason);
        if (reason.isNotEmpty())
            message << ": " << reason;

        return message.isNotEmpty() ? message
                                    : "Video recording failed while preparing the encoder.";
    }

    juce::String osStatusMessage(OSStatus status, const char* fallback)
    {
        return juce::String(fallback) + " (" + juce::String(static_cast<int>(status)) + ")";
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
    AVAssetWriterInput* audioInput = nil;
    AVAssetWriterInputPixelBufferAdaptor* adaptor = nil;
    CMAudioFormatDescriptionRef audioFormat = nullptr;
    int width = 0;
    int height = 0;
    int fps = 30;
    int64_t frameIndex = 0;
    int64_t audioSampleIndex = 0;
    int audioChannels = 0;
    int audioTimeScale = 48000;
    bool recording = false;
    bool preserveAlpha = false;

    void releaseAudioFormat() noexcept
    {
        if (audioFormat != nullptr)
        {
            CFRelease(audioFormat);
            audioFormat = nullptr;
        }
    }

    void clearHandles() noexcept
    {
        writer = nil;
        input = nil;
        audioInput = nil;
        adaptor = nil;
        releaseAudioFormat();
    }
};

VideoRecorder::VideoRecorder()
    : impl_(std::make_unique<Impl>())
{
}

VideoRecorder::~VideoRecorder()
{
    if (isRecording())
        stop();

    impl_->releaseAudioFormat();
}

bool VideoRecorder::isAvailable() const noexcept
{
    return true;
}

bool VideoRecorder::isRecording() const noexcept
{
    return impl_->recording;
}

bool VideoRecorder::canAcceptAudio() const noexcept
{
    return impl_->recording
           && impl_->audioInput != nil
           && impl_->audioInput.readyForMoreMediaData;
}

juce::Result VideoRecorder::start(const juce::File& outputFile,
                                  int width,
                                  int height,
                                  int fps,
                                  bool preserveAlpha,
                                  double audioSampleRate,
                                  int audioChannels)
{
    if (impl_->recording)
        return juce::Result::fail("Recording is already running.");

    if (width <= 1 || height <= 1)
        return juce::Result::fail("Invalid recording size.");

    width = width + (width % 2);
    height = height + (height % 2);
    fps = juce::jlimit(1, 120, fps);
    audioChannels = juce::jlimit(0, 2, audioChannels);
    const bool recordAudio = audioChannels > 0 && audioSampleRate >= 8000.0;
    const int audioTimeScale = juce::jlimit(8000, 192000, static_cast<int>(std::lround(audioSampleRate)));

    outputFile.getParentDirectory().createDirectory();
    if (outputFile.exists())
        return juce::Result::fail("The recording destination already exists.");

    auto failStart = [&](const juce::String& message) -> juce::Result
    {
        if (impl_->writer != nil)
            [impl_->writer cancelWriting];

        impl_->clearHandles();
        outputFile.deleteFile();
        return juce::Result::fail(message);
    };

    @try
    {
        @autoreleasepool
        {
            NSURL* url = [NSURL fileURLWithPath:nsString(outputFile.getFullPathName())];
            NSError* error = nil;

            impl_->writer = [[AVAssetWriter alloc] initWithURL:url
                                                       fileType:AVFileTypeQuickTimeMovie
                                                          error:&error];
            if (impl_->writer == nil)
                return failStart(error != nil ? juceString(error.localizedDescription)
                                              : "Could not create AVAssetWriter.");

            NSDictionary* settings = nil;
            if (preserveAlpha)
            {
                settings = @{
                    AVVideoCodecKey: AVVideoCodecTypeAppleProRes4444,
                    AVVideoWidthKey: @(width),
                    AVVideoHeightKey: @(height)
                };
            }
            else
            {
                NSDictionary* compression = @{
                    AVVideoAverageBitRateKey: @(juce::jmax(2000000, width * height * 6))
                };

                settings = @{
                    AVVideoCodecKey: AVVideoCodecTypeH264,
                    AVVideoWidthKey: @(width),
                    AVVideoHeightKey: @(height),
                    AVVideoCompressionPropertiesKey: compression
                };
            }

            impl_->input = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo
                                                               outputSettings:settings];
            if (impl_->input == nil)
                return failStart("Could not create the video input.");

            impl_->input.expectsMediaDataInRealTime = YES;

            NSDictionary* attributes = @{
                (NSString*) kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
                (NSString*) kCVPixelBufferWidthKey: @(width),
                (NSString*) kCVPixelBufferHeightKey: @(height)
            };

            impl_->adaptor =
                [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:impl_->input
                                                                                 sourcePixelBufferAttributes:attributes];
            if (impl_->adaptor == nil)
                return failStart("Could not create the video frame adaptor.");

            if (! [impl_->writer canAddInput:impl_->input])
                return failStart("AVAssetWriter cannot add the video input.");

            [impl_->writer addInput:impl_->input];

            if (recordAudio)
            {
                NSDictionary* audioSettings = @{
                    AVFormatIDKey: @(kAudioFormatMPEG4AAC),
                    AVSampleRateKey: @(audioSampleRate),
                    AVNumberOfChannelsKey: @(audioChannels),
                    AVEncoderBitRateKey: @(audioChannels > 1 ? 192000 : 96000)
                };

                impl_->audioInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio
                                                                        outputSettings:audioSettings];
                if (impl_->audioInput == nil)
                    return failStart("Could not create the audio input.");

                impl_->audioInput.expectsMediaDataInRealTime = YES;

                if (! [impl_->writer canAddInput:impl_->audioInput])
                    return failStart("AVAssetWriter cannot add the audio input.");

                [impl_->writer addInput:impl_->audioInput];

                AudioStreamBasicDescription description {};
                description.mSampleRate = audioSampleRate;
                description.mFormatID = kAudioFormatLinearPCM;
                description.mFormatFlags = kAudioFormatFlagIsFloat
                                           | kAudioFormatFlagIsPacked
                                           | kAudioFormatFlagIsNonInterleaved
                                           | kAudioFormatFlagsNativeEndian;
                description.mBytesPerPacket = sizeof(float);
                description.mFramesPerPacket = 1;
                description.mBytesPerFrame = sizeof(float);
                description.mChannelsPerFrame = static_cast<UInt32>(audioChannels);
                description.mBitsPerChannel = sizeof(float) * 8;

                const auto status = CMAudioFormatDescriptionCreate(kCFAllocatorDefault,
                                                                   &description,
                                                                   0,
                                                                   nullptr,
                                                                   0,
                                                                   nullptr,
                                                                   nullptr,
                                                                   &impl_->audioFormat);
                if (status != noErr || impl_->audioFormat == nullptr)
                    return failStart(osStatusMessage(status, "Could not create the audio format."));
            }

            if (! [impl_->writer startWriting])
            {
                const auto message = impl_->writer.error != nil
                                         ? juceString(impl_->writer.error.localizedDescription)
                                         : juce::String("AVAssetWriter could not start writing.");
                return failStart(message);
            }

            [impl_->writer startSessionAtSourceTime:kCMTimeZero];
        }
    }
    @catch (NSException* exception)
    {
        return failStart(exceptionMessage(exception));
    }

    impl_->width = width;
    impl_->height = height;
    impl_->fps = fps;
    impl_->frameIndex = 0;
    impl_->audioSampleIndex = 0;
    impl_->audioChannels = recordAudio ? audioChannels : 0;
    impl_->audioTimeScale = audioTimeScale;
    impl_->preserveAlpha = preserveAlpha;
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

        if (src.pixelStride == 4 && impl_->preserveAlpha)
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
                dstRow[x * 4 + 3] = impl_->preserveAlpha
                                    ? static_cast<unsigned char>(c.getAlpha())
                                    : 255;
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

juce::Result VideoRecorder::addAudioSamples(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (! impl_->recording || impl_->audioInput == nil || impl_->audioFormat == nullptr)
        return juce::Result::ok();

    if (numSamples <= 0)
        return juce::Result::ok();

    if (! impl_->audioInput.readyForMoreMediaData)
        return juce::Result::ok();

    if (buffer.getNumChannels() < impl_->audioChannels)
        return juce::Result::fail("Audio capture channel count changed during recording.");

    CMSampleBufferRef sampleBuffer = nullptr;

    const CMSampleTimingInfo timing {
        CMTimeMake(1, impl_->audioTimeScale),
        CMTimeMake(impl_->audioSampleIndex, impl_->audioTimeScale),
        kCMTimeInvalid
    };

    auto status = CMSampleBufferCreate(kCFAllocatorDefault,
                                       nullptr,
                                       false,
                                       nullptr,
                                       nullptr,
                                       impl_->audioFormat,
                                       numSamples,
                                       1,
                                       &timing,
                                       0,
                                       nullptr,
                                       &sampleBuffer);
    if (status != noErr || sampleBuffer == nullptr)
        return juce::Result::fail(osStatusMessage(status, "Could not create an audio sample buffer."));

    const auto listSize = sizeof(AudioBufferList)
                          + (static_cast<std::size_t>(juce::jmax(impl_->audioChannels, 1)) - 1)
                                * sizeof(AudioBuffer);
    std::vector<unsigned char> storage(listSize, 0);
    auto* audioBufferList = reinterpret_cast<AudioBufferList*>(storage.data());
    audioBufferList->mNumberBuffers = static_cast<UInt32>(impl_->audioChannels);

    for (int ch = 0; ch < impl_->audioChannels; ++ch)
    {
        auto& entry = audioBufferList->mBuffers[ch];
        entry.mNumberChannels = 1;
        entry.mDataByteSize = static_cast<UInt32>(numSamples * static_cast<int>(sizeof(float)));
        entry.mData = const_cast<float*>(buffer.getReadPointer(ch));
    }

    status = CMSampleBufferSetDataBufferFromAudioBufferList(sampleBuffer,
                                                            kCFAllocatorDefault,
                                                            kCFAllocatorDefault,
                                                            0,
                                                            audioBufferList);
    if (status != noErr)
    {
        CFRelease(sampleBuffer);
        return juce::Result::fail(osStatusMessage(status, "Could not attach audio samples."));
    }

    const BOOL appended = [impl_->audioInput appendSampleBuffer:sampleBuffer];
    CFRelease(sampleBuffer);

    if (! appended)
        return juce::Result::fail("Could not append audio samples.");

    impl_->audioSampleIndex += numSamples;
    return juce::Result::ok();
}

juce::Result VideoRecorder::stop()
{
    if (! impl_->recording)
        return juce::Result::ok();

    impl_->recording = false;

    if (impl_->input != nil)
        [impl_->input markAsFinished];

    if (impl_->audioInput != nil)
        [impl_->audioInput markAsFinished];

    __block BOOL success = YES;
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);

    [impl_->writer finishWritingWithCompletionHandler:^{
        success = impl_->writer.status == AVAssetWriterStatusCompleted;
        dispatch_semaphore_signal(sema);
    }];

    dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);

    impl_->clearHandles();

    return success ? juce::Result::ok()
                   : juce::Result::fail("AVAssetWriter did not finish successfully.");
}

} // namespace SoundShape
