// Copyright (c) 2020 Samsung Electronics Inc.
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

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include <emscripten.h>
#include <samsung/html/html_media_element.h>
#include <samsung/html/html_media_element_listener.h>
#include <samsung/wasm/elementary_media_stream_source.h>
#include <samsung/wasm/elementary_media_stream_source_listener.h>
#include <samsung/wasm/elementary_media_track.h>
#include <samsung/wasm/elementary_media_track_listener.h>

#include "sample_data.h"

using ElementaryMediaStreamSource = samsung::wasm::ElementaryMediaStreamSource;
using ElementaryMediaStreamSourceListener =
    samsung::wasm::ElementaryMediaStreamSourceListener;
using ElementaryMediaTrack = samsung::wasm::ElementaryMediaTrack;
using ElementaryMediaTrackListener =
    samsung::wasm::ElementaryMediaTrackListener;
using HTMLMediaElement = samsung::html::HTMLMediaElement;
using HTMLMediaElementListener = samsung::html::HTMLMediaElementListener;
using Seconds = samsung::wasm::Seconds;
using SessionId = samsung::wasm::SessionId;

static constexpr char kVideoTagId[] = "video-element";

// This class is responsible for sending elementary media data to Elementary
// Media Stream Source via ElementaryMediaTrack object.
class TrackDataPump : public ElementaryMediaTrackListener {
 public:
  // Controls how many packets should be buffered ahead of a current playback
  // position.
  static constexpr Seconds kBufferAhead = Seconds{3.};

  // Worker thread will be notified about advancing playback position every
  // kWorkerUpdateThreshold.
  static constexpr Seconds kWorkerUpdateThreshold = Seconds{0.5};

  explicit TrackDataPump(ElementaryMediaTrack video_track)
      : video_track_(std::move(video_track)),
        pump_worker_([this]() { this->PumpPackets(); }),
        last_reported_running_time_(0),
        session_id_(video_track_.GetSessionId().value) {
    video_track_.SetListener(this);
  }

  ~TrackDataPump() {
    messages_.PushTerminate();
    pump_worker_.detach();
  }

  // Notify pump about stream running time, so that elementary media data can be
  // buffered up to (new_time + kBufferAhead).
  void UpdateTime(Seconds new_time) {
    if (last_reported_running_time_ + kWorkerUpdateThreshold > new_time) {
      // Extensive locking of main (JS) thread is not advised and should be
      // avoided (and in this case - it is also not needed), therefore update
      // frequency is throttled to kWorkerUpdateThreshold.
      return;
    }
    last_reported_running_time_ = new_time;
    messages_.PushBufferToPts(new_time + kBufferAhead, session_id_);
  }

  // samsung::wasm::ElementaryMediaStreamSourceListener interface //////////////

  // Indicates ElementaryMediaTrack is ready to accept data.
  void OnTrackOpen() override {
    // Trigger buffering immediately.
    messages_.PushBufferToPts(last_reported_running_time_ + kBufferAhead,
                              session_id_);
  }

  // Indicates ElementaryMediaTrack can't accept data.
  void OnTrackClosed(ElementaryMediaTrack::CloseReason) override {
    messages_.Flush();
  }

  // Track is being seeked.
  //
  // This happens only when track is closed. When it will open data provider
  // should send elementary media data starting from a keyframe closest to the
  // new_time.
  void OnSeek(Seconds new_time) override {
    last_reported_running_time_ = new_time;
    messages_.PushSeekTo(new_time);
  }

  // Session id changed: stamp packets with a new session id from now on.
  void OnSessionIdChanged(SessionId session_id) override {
    session_id_ = session_id;
  }

 private:
  // Naive implementation of main thread -> worker thread message queue:
  class WorkerMessageQueue {
   public:
    struct Message {
      enum class Type { kSetBufferToPts, kSeekTo, kTerminate };

      Message(Type type, Seconds time, SessionId session_id)
        : type(type),
          time(time),
          session_id(session_id) {}

      Type type;
      Seconds time;
      SessionId session_id;
    };  // struct Message

    void Flush() {
      std::unique_lock<std::mutex> lock{messages_mutex_};
      FlushWhileLocked();
    }

    Message Pop() {
      std::unique_lock<std::mutex> lock{messages_mutex_};
      while (message_queue_.empty()) {
        messages_changed_.wait(lock);
      }
      auto result = message_queue_.front();
      message_queue_.pop();
      return result;
    }

    void PushBufferToPts(Seconds time, SessionId session_id) {
      {
        std::lock_guard<std::mutex> lock{messages_mutex_};
        message_queue_.emplace(
            WorkerMessageQueue::Message::Type::kSetBufferToPts, time,
            session_id);
      }
      messages_changed_.notify_one();
    }

    void PushSeekTo(Seconds time) {
      {
        // Seek invalidates any actions queued previously.
        FlushWhileLocked();
        std::lock_guard<std::mutex> lock{messages_mutex_};
        message_queue_.emplace(WorkerMessageQueue::Message::Type::kSeekTo, time,
                               0 /* ignored for kSeekTo */);
      }
      messages_changed_.notify_one();
    }

    void PushTerminate() {
      {
        std::lock_guard<std::mutex> lock{messages_mutex_};
        FlushWhileLocked();
        message_queue_.emplace(WorkerMessageQueue::Message::Type::kTerminate,
                               Seconds{0} /* ignored for kTerminate */,
                               0 /* ignored for kTerminate */);
      }
      messages_changed_.notify_one();
    }

   private:
    std::queue<Message> message_queue_;
    std::condition_variable messages_changed_;
    std::mutex messages_mutex_;

    void FlushWhileLocked() {
      std::queue<Message> tmp;
      message_queue_.swap(tmp);
    }
  };  // class WorkerMessageQueue

  WorkerMessageQueue messages_;
  ElementaryMediaTrack video_track_;
  std::thread pump_worker_;

  Seconds last_reported_running_time_;
  SessionId session_id_;

  // A first frame after Seek must always be a keyframe. This method finds a
  // closest keyframe preceding the given time in sample_data.
  static size_t GetClosestKeyframeIndex(Seconds time) {
    auto keyframe = std::find_if(
        sample_data::kVideoPackets.crbegin(),
        sample_data::kVideoPackets.crend(), [time](const auto& packet) {
          return (packet.is_key_frame && packet.pts < time);
        });
    if (keyframe == sample_data::kVideoPackets.crend())
      return 0;
    return (&(*keyframe) - sample_data::kVideoPackets.data());
  }

  // Sends packets to Source. Executes on a worker thread.
  //
  // This sample uses a simple, hard-coded media content. However, for a typical
  // media application data processing will be more complicated and time
  // consuming (e.g. it includes downloading data, demuxing containers, etc).
  // Therefore processing of elementary media data on a side thread is advised,
  // as it frees main thread (JS thread) and does not hinder application
  // responsiveness.
  void PumpPackets() {
    using Message = WorkerMessageQueue::Message;
    auto ended = false;
    auto packet_idx = 0u;
    auto session_id = 0u;
    while (true) {
      auto message = messages_.Pop();
      switch (message.type) {
        case Message::Type::kSetBufferToPts:
          session_id = message.session_id;
          while (packet_idx < sample_data::kVideoPackets.size() &&
                 sample_data::kVideoPackets[packet_idx].pts < message.time) {
            auto packet = sample_data::kVideoPackets[packet_idx];
            packet.session_id = session_id;
            video_track_.AppendPacket(packet);
            ++packet_idx;
          }
          if (!ended && packet_idx == sample_data::kVideoPackets.size()) {
            // Make sure to mark track as ended once all packets were sent.
            // Since HTML video tag's 'loop' property is set, Elementary Media
            // Stream Source will automatically seek to 0s once playback reaches
            // end.
            ended = true;
            video_track_.AppendEndOfTrack(session_id);
          }
          break;
        case Message::Type::kSeekTo:
          ended = false;
          packet_idx = GetClosestKeyframeIndex(message.time);
          break;
        case Message::Type::kTerminate:
          return;
      }
    }
  }
};  // class TrackDataPump

// A simple player that plays sample, looped video with HTMLMediaElement and
// ElementaryMediaStreamSource.
class SamplePlayer : public ElementaryMediaStreamSourceListener,
                     public HTMLMediaElementListener {
 public:
  SamplePlayer() = default;

  void SetUp() {
    media_element_ = std::make_unique<HTMLMediaElement>(kVideoTagId);
    media_element_->SetListener(this);

    source_ = std::make_unique<ElementaryMediaStreamSource>(
        ElementaryMediaStreamSource::Mode::kNormal);
    source_->SetListener(this);

    // When source_ is successfully attached to media_element_, it will change
    // state from kDetached to kClosed. This results in OnSourceClosed() firing,
    // where setting player up can continue.
    media_element_->SetSrc(source_.get());
  }

  // samsung::wasm::ElementaryMediaStreamSourceListener interface //////////////

  // This event will be fired when ElementaryMediaStreamSource enters kClosed
  // state, which happens once it's attached to HTMLMediaElement. Player can be
  // configured in kClosed state.
  void OnSourceClosed() override {
    // First, Source needs to be configured:
    source_->SetDuration(sample_data::kStreamDuration);
    auto add_track_result = source_->AddTrack(sample_data::kVideoTrackConfig);
    if (!add_track_result) {
      std::cout << "Cannot add a video track!" << std::endl;
      return;
    }
    auto video_track = std::move(add_track_result.value);
    track_data_pump_ = std::make_unique<TrackDataPump>(std::move(video_track));

    // Then Source can be requested to enter kOpen state (where it can accept
    // elementary media data).
    source_->Open([](auto result) {
      if (result != samsung::wasm::OperationResult::kSuccess) {
        std::cout << "Cannot open ElementaryMediaStreamSource." << std::endl;
      }
      // Source entered kOpen state after Open() request.
      //
      // App can send elementary media data to ElementaryMediaTrack now, so
      // packet sending mechanism should be started. Since entering kOpen state
      // can be triggered by reasons other than calling Open(), code that
      // controls sending packets should be placed either in
      // ElementaryMediaStreamSourceListener::OnSourceOpen() or in
      // ElementaryMediaTrackListener::OnTrackOpen().
      //
      // The latter option is preferred and is used in TrackDataPump.
    });
  }

  void OnPlaybackPositionChanged(Seconds new_time) override {
    if (track_data_pump_) {
      // Broadcast new time to a component managing data buffering...
      track_data_pump_->UpdateTime(new_time);
    }
  }

  // samsung::html::HTMLMediaElementListener interface /////////////////////////

  // This event will be fired as soon as enough data is buffered to start
  // playback.
  void OnCanPlay() override {
    if (!media_element_->IsPaused()) return;

    media_element_->Play([](samsung::wasm::OperationResult result) {
      if (result != samsung::wasm::OperationResult::kSuccess) {
        std::cout << "Cannot play." << std::endl;
      }
    });
  }

 private:
  std::unique_ptr<HTMLMediaElement> media_element_;

  // Make sure source_ outlives media_element_ when they are associated with
  // HTMLMediaElement::SetSrc().
  std::unique_ptr<ElementaryMediaStreamSource> source_;

  std::unique_ptr<TrackDataPump> track_data_pump_;
};  // class SimplePlayer

static SamplePlayer kSamplePlayerInstance;

int main() {
  // WASM module execution will not terminate when main exits.
  EM_ASM(noExitRuntime = true);

  // Start SamplePlayer.
  kSamplePlayerInstance.SetUp();
}
