// Copyright (c) 2021 Samsung Electronics Inc.
// Licensed under the MIT license.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//          *** Sample Tizen WASM Video Decoder Application ***
//
// This file contains sample implementation of a simple WASM module that decodes
// video content using Tizen WASM Player with ElementaryMediaStreamSource as
// a data source and renders it on `canvas` HTML element using GL.
// The sample uses hardcoded data (see sample_data.h).

#ifndef VIDEO_DECODER_SAMPLE_VIDEO_DECODER_SDF_SAMPLE_H
#define VIDEO_DECODER_SAMPLE_VIDEO_DECODER_SDF_SAMPLE_H

#include "emss_sdf_sample.h"

#include <GLES2/gl2.h>
#include <SDL2/SDL.h>

// This class is responsible for sending elementary media data to Elementary
// Media Stream Source via ElementaryMediaTrack object.
class VideoDecoderTrackDataPump : public TrackDataPump {
 public:
  using ElementaryMediaTrack = samsung::wasm::ElementaryMediaTrack;
  using Seconds = samsung::wasm::Seconds;
  using SessionId = samsung::wasm::SessionId;

  explicit VideoDecoderTrackDataPump(ElementaryMediaTrack video_track);

  ~VideoDecoderTrackDataPump() override = default;

  void OnDrawCompleted();
  void RequestNewVideoTexture();

 private:
  void CreateGLObjects();
  void CreateProgram();
  void Draw();
  void InitializeSDL();
  void InitializeGL();

  GLuint texture_{0};
  SDL_Window* window_{nullptr};
  SDL_GLContext gl_context_{nullptr};
  GLuint program_{0};
  GLuint texcoord_scale_location_{0};
};  // class VideoDecoderTrackDataPump

class VideoDecoderSamplePlayer : public SamplePlayer {
 public:
  using ElementaryMediaStreamSource =
      samsung::wasm::ElementaryMediaStreamSource;
  using HTMLMediaElement = samsung::html::HTMLMediaElement;
  using Seconds = samsung::wasm::Seconds;

  VideoDecoderSamplePlayer() = default;

  // samsung::wasm::ElementaryMediaStreamSourceListener interface //
  void OnCanPlay() override;

 private:
  std::unique_ptr<TrackDataPump> CreateTrackDataPump(
      ElementaryMediaTrack&& video_track) override;
};  // class VideoDecoderSamplePlayer

#endif  // VIDEO_DECODER_SAMPLE_VIDEO_DECODER_SDF_SAMPLE_H
