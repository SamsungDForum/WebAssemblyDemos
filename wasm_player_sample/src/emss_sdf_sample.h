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

//          *** Sample Tizen WASM Player Application ***
//
// This file contains sample implementation of a simple WASM module that plays
// media content with Tizen WASM Player using HTMLMediaElement with
// ElementaryMediaStreamSource as a data source. The sample uses hardcoded data
// (see sample_data.h).

#ifndef WASM_PLAYER_SAMPLE_EMSS_SDF_SAMPLE_H
#define WASM_PLAYER_SAMPLE_EMSS_SDF_SAMPLE_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include <samsung/html/html_media_element.h>
#include <samsung/html/html_media_element_listener.h>
#include <samsung/wasm/elementary_media_stream_source.h>
#include <samsung/wasm/elementary_media_stream_source_listener.h>
#include <samsung/wasm/elementary_media_track.h>
#include <samsung/wasm/elementary_media_track_listener.h>

// This class is responsible for sending elementary media data to Elementary
// Media Stream Source via ElementaryMediaTrack object.
class TrackDataPump : public samsung::wasm::ElementaryMediaTrackListener {
 public:
  using ElementaryMediaTrack = samsung::wasm::ElementaryMediaTrack;
  using Seconds = samsung::wasm::Seconds;
  using SessionId = samsung::wasm::SessionId;

  // Controls how many packets should be buffered ahead of a current playback
  // position.
  static constexpr Seconds kBufferAhead = Seconds{3.};

  // Worker thread will be notified about advancing playback position every
  // kWorkerUpdateThreshold.
  static constexpr Seconds kWorkerUpdateThreshold = Seconds{0.5};

  explicit TrackDataPump(ElementaryMediaTrack video_track);

  ~TrackDataPump() override;

  // Notify pump about stream running time, so that elementary media data can be
  // buffered up to (new_time + kBufferAhead).
  void UpdateTime(Seconds new_time);

  // samsung::wasm::ElementaryMediaStreamSourceListener interface //////////////

  // Indicates ElementaryMediaTrack is ready to accept data.
  void OnTrackOpen() override;

  // Indicates ElementaryMediaTrack can't accept data.
  void OnTrackClosed(ElementaryMediaTrack::CloseReason) override;

  // Track is being seeked.
  //
  // This happens only when track is closed. When it will open data provider
  // should send elementary media data starting from a keyframe closest to the
  // new_time.
  void OnSeek(Seconds new_time) override;

  // Session id changed: stamp packets with a new session id from now on.
  void OnSessionIdChanged(SessionId session_id) override;

 protected:
  ElementaryMediaTrack video_track_;

 private:
  // Naive implementation of main thread -> worker thread message queue:
  class WorkerMessageQueue {
   public:
    struct Message {
      enum class Type { kSetBufferToPts, kSeekTo, kTerminate };

      Message(Type type, Seconds time, SessionId session_id);

      Type type;
      Seconds time;
      SessionId session_id;
    };  // struct Message

    void Flush();
    Message Pop();
    void PushBufferToPts(Seconds time, SessionId session_id);
    void PushSeekTo(Seconds time);
    void PushTerminate();

   private:
    void FlushWhileLocked();

    std::queue<Message> message_queue_;
    std::condition_variable messages_changed_;
    std::mutex messages_mutex_;
  };  // class WorkerMessageQueue

  WorkerMessageQueue messages_;

  std::thread pump_worker_;

  Seconds last_reported_running_time_;
  SessionId session_id_;

  // A first frame after Seek must always be a keyframe. This method finds a
  // closest keyframe preceeding the given time in sample_data.
  static size_t GetClosestKeyframeIndex(Seconds time);

  // Sends packets to Source. Executes on a worker thread.
  //
  // This sample uses a simple, hard-coded media content. However, for a typical
  // media application data processing will be more complicated and time
  // consuming (e.g. it includes downloading data, demuxing containers, etc).
  // Therefore processing of elementary media data on a side thread is advised,
  // as it frees main thread (JS thread) and does not hinder application
  // responsiveness.
  void PumpPackets();
};  // class TrackDataPump

class SamplePlayer : public samsung::wasm::ElementaryMediaStreamSourceListener,
                     public samsung::html::HTMLMediaElementListener {
 public:
  using ElementaryMediaStreamSource =
      samsung::wasm::ElementaryMediaStreamSource;
  using ElementaryMediaTrack = samsung::wasm::ElementaryMediaTrack;
  using HTMLMediaElement = samsung::html::HTMLMediaElement;
  using Seconds = samsung::wasm::Seconds;

  SamplePlayer() = default;

  void SetUp(ElementaryMediaStreamSource::RenderingMode);

  // samsung::wasm::ElementaryMediaStreamSourceListener interface //

  // This event will be fired when ElementaryMediaStreamSource enters kClosed
  // state, which happens once it's attached to HTMLMediaElement. Player can be
  // configured in kClosed state.
  void OnSourceClosed() override;

  void OnPlaybackPositionChanged(Seconds new_time) override;

  // samsung::html::HTMLMediaElementListener interface //

  // This event will be fired as soon as enough data is buffered to start
  // playback.
  void OnCanPlay() override;

 protected:
  virtual std::unique_ptr<TrackDataPump> CreateTrackDataPump(
      ElementaryMediaTrack&& video_track);

  std::unique_ptr<HTMLMediaElement> media_element_;
  std::unique_ptr<TrackDataPump> track_data_pump_;

  // Make sure source_ outlives media_element_ when they are associated with
  // HTMLMediaElement::SetSrc().
  std::unique_ptr<ElementaryMediaStreamSource> source_;
};  // class SimplePlayer

#endif  // WASM_PLAYER_SAMPLE_EMSS_SDF_SAMPLE_H
