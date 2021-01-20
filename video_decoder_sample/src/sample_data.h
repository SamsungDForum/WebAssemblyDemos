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

#ifndef SAMPLE_DATA_H_
#define SAMPLE_DATA_H_

#include <array>

#include "samsung/wasm/elementary_media_packet.h"
#include "samsung/wasm/elementary_video_track_config.h"

namespace sample_data {

// Duration of a stream.
//
// Usually it wouldn't be hardcoded but read from a stream source. Please note
// duration is applicable only in some ElementaryMediaStreamSource modes (see
// samsung::wasm::ElementaryMediaStreamSource::Mode).
extern const samsung::wasm::Seconds kStreamDuration;

// Contains video track configuration.
//
// Usually it wouldn't be hardcoded but read from a stream source.
extern const samsung::wasm::ElementaryVideoTrackConfig kVideoTrackConfig;

// Contains  Elementary Media Packets of the sample video track.
//
// Usually those wouldn't be hardcoded but either demuxed from a container or
// received from network (depending on the use case and protocol used by an
// application).
extern const std::array<samsung::wasm::ElementaryMediaPacket, 208>
    kVideoPackets;

}  // namespace sample_data

#endif  // SAMPLE_DATA_H_
