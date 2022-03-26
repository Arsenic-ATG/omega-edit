/**********************************************************************************************************************
 * Copyright (c) 2021 Concurrent Technologies Corporation.                                                            *
 *                                                                                                                    *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance     *
 * with the License.  You may obtain a copy of the License at                                                         *
 *                                                                                                                    *
 *     http://www.apache.org/licenses/LICENSE-2.0                                                                     *
 *                                                                                                                    *
 * Unless required by applicable law or agreed to in writing, software is distributed under the License is            *
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or                   *
 * implied.  See the License for the specific language governing permissions and limitations under the License.       *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include "internal_fun.hpp"
#include "../../include/omega_edit/change.h"
#include "change_def.hpp"
#include "macros.h"
#include "model_def.hpp"
#include "model_segment_def.hpp"
#include "session_def.hpp"
#include "viewport_def.hpp"
#include <cassert>

enum class session_flags { pause_viewport_callbacks = 0x01 };

/**********************************************************************************************************************
 * Data segment functions
 **********************************************************************************************************************/

static int64_t read_segment_from_file_(FILE *from_file_ptr, int64_t offset, omega_byte_t *buffer,
                                       int64_t capacity) noexcept {
    assert(from_file_ptr);
    assert(buffer);
    int64_t rc = -1;
    if (0 == FSEEK(from_file_ptr, 0, SEEK_END)) {
        const auto len = FTELL(from_file_ptr) - offset;
        // make sure the offset does not exceed the file size
        if (len > 0) {
            // the length is going to be equal to what's left of the file, or the buffer capacity, whichever is less
            const auto count = (len < capacity) ? len : capacity;
            if (0 == FSEEK(from_file_ptr, offset, SEEK_SET) &&
                count == static_cast<int64_t>(fread(buffer, sizeof(omega_byte_t), count, from_file_ptr))) {
                rc = count;
            }
        }
    }
    return rc;
}

int populate_data_segment_(const omega_session_t *session_ptr, omega_data_segment_t *data_segment_ptr) noexcept {
    assert(session_ptr);
    assert(session_ptr->models_.back());
    assert(data_segment_ptr);
    const auto &model_ptr = session_ptr->models_.back();
    data_segment_ptr->length_ = 0;
    if (model_ptr->model_segments_.empty()) { return 0; }
    assert(0 < data_segment_ptr->capacity_);
    const auto data_segment_capacity = data_segment_ptr->capacity_;
    const auto data_segment_offset = data_segment_ptr->offset_ + data_segment_ptr->offset_adjustment_;
    int64_t read_offset = 0;

    for (auto iter = model_ptr->model_segments_.cbegin(); iter != model_ptr->model_segments_.cend(); ++iter) {
        if (read_offset != (*iter)->computed_offset_) {
            ABORT(print_model_segments_(session_ptr->models_.back().get(), CLOG);
                  LOG_ERROR("break in model continuity, expected: " << read_offset
                                                                    << ", got: " << (*iter)->computed_offset_););
        }
        if (read_offset <= data_segment_offset && data_segment_offset <= read_offset + (*iter)->computed_length_) {
            // We're at the first model segment that intersects with the data segment, but the model segment and the
            // data segment offsets are likely not aligned, so we need to compute how much of the segment to move past
            // (the delta).
            auto delta = data_segment_offset - (*iter)->computed_offset_;
            auto data_segment_buffer = omega_data_segment_get_data(data_segment_ptr);
            do {
                // This is how much data remains to be filled
                const auto remaining_capacity = data_segment_capacity - data_segment_ptr->length_;
                auto amount = (*iter)->computed_length_ - delta;
                amount = (amount > remaining_capacity) ? remaining_capacity : amount;
                switch (omega_model_segment_get_kind(iter->get())) {
                    case model_segment_kind_t::SEGMENT_READ:
                        // For read segments, we're reading a segment, or portion thereof, from the input file and
                        // writing it into the data segment
                        if (read_segment_from_file_(
                                    session_ptr->models_.back()->file_ptr_, (*iter)->change_offset_ + delta,
                                    data_segment_buffer + data_segment_ptr->length_, amount) != amount) {
                            return -1;
                        }
                        break;
                    case model_segment_kind_t::SEGMENT_INSERT:
                        // For insert segments, we're writing the change byte buffer, or portion thereof, into the data
                        // segment
                        memcpy(data_segment_buffer + data_segment_ptr->length_,
                               omega_change_get_bytes((*iter)->change_ptr_.get()) + (*iter)->change_offset_ + delta,
                               amount);
                        break;
                    default:
                        ABORT(LOG_ERROR("Unhandled model segment kind"););
                }
                // Add the amount written to the data segment length
                data_segment_ptr->length_ += amount;
                // After the first segment is written, the dela should be zero from that point on
                delta = 0;
                // Keep writing segments until we run out of viewport capacity or run out of segments
            } while (data_segment_ptr->length_ < data_segment_capacity && ++iter != model_ptr->model_segments_.end());
            assert(data_segment_ptr->length_ <= data_segment_capacity);
            // data segment buffer allocation is its capacity plus one, so we can null-terminate it
            data_segment_buffer[data_segment_ptr->length_] = '\0';
            return 0;
        }
        read_offset += (*iter)->computed_length_;
    }
    return -1;
}

int64_t get_computed_file_size_(const omega_session_t *session_ptr) {
    const auto computed_file_size =
            (session_ptr->models_.back()->model_segments_.empty())
                    ? 0
                    : session_ptr->models_.back()->model_segments_.back()->computed_offset_ +
                              session_ptr->models_.back()->model_segments_.back()->computed_length_;
    assert(0 <= computed_file_size);
    return computed_file_size;
}

const char *get_file_path_(const omega_session_t *session_ptr) {
    return (session_ptr->models_.back()->file_path_.empty()) ? nullptr
                                                             : session_ptr->models_.back()->file_path_.c_str();
}

void pause_viewport_event_callbacks_(omega_session_t *session_ptr) {
    session_ptr->session_flags_ |= (int8_t) session_flags::pause_viewport_callbacks;
}

void resume_viewport_event_callbacks_(omega_session_t *session_ptr) {
    session_ptr->session_flags_ &= ~(int8_t) session_flags::pause_viewport_callbacks;
}

int on_change_callbacks_paused_(const omega_session_t *session_ptr) {
    return (session_ptr->session_flags_ & (int8_t) session_flags::pause_viewport_callbacks) ? 1 : 0;
}

void session_notify_(const omega_session_t *session_ptr, omega_session_event_t session_event,
                     const omega_change_t *change_ptr) {
    if (session_ptr->event_handler_ &&
        (0 == session_ptr->event_interest_ || session_event & session_ptr->event_interest_)) {
        (*session_ptr->event_handler_)(session_ptr, session_event, change_ptr);
    }
}

int64_t get_num_changes_(const omega_session_t *session_ptr) {
    return (int64_t) session_ptr->models_.back()->changes_.size() + session_ptr->num_changes_adjustment_;
}

int64_t get_num_checkpoints_(const omega_session_t *session_ptr) {
    return static_cast<int64_t>(session_ptr->models_.size()) - 1;
}

void destroy_viewport_(omega_viewport_t *viewport_ptr) {
    for (auto iter = viewport_ptr->session_ptr->viewports_.rbegin();
         iter != viewport_ptr->session_ptr->viewports_.rend(); ++iter) {
        if (viewport_ptr == iter->get()) {
            if (7 < viewport_get_capacity_(iter->get())) { delete[](*iter)->data_segment.data_.bytes_ptr_; }
            viewport_ptr->session_ptr->viewports_.erase(std::next(iter).base());
            break;
        }
    }
}

void viewport_notify_(const omega_viewport_t *viewport_ptr, omega_viewport_event_t viewport_event,
                      const omega_change_t *change_ptr) {
    if (viewport_ptr->event_handler &&
        (0 == viewport_ptr->event_interest_ || viewport_event & viewport_ptr->event_interest_) &&
        !on_change_callbacks_paused_(viewport_ptr->session_ptr)) {
        (*viewport_ptr->event_handler)(viewport_ptr, viewport_event, change_ptr);
    }
}

int64_t viewport_get_capacity_(const omega_viewport_t *viewport_ptr) {
    // Negative capacities are only used internally for tracking dirty reads.  The capacity is always positive to the
    // public.
    return std::abs(viewport_ptr->data_segment.capacity_);
}

int64_t viewport_get_length_(const omega_viewport_t *viewport_ptr) {
    if (!viewport_has_changes_(viewport_ptr)) { return viewport_ptr->data_segment.length_; }
    auto const capacity = viewport_get_capacity_(viewport_ptr);
    auto const remaining_file_size =
            std::max(get_computed_file_size_(viewport_ptr->session_ptr) - viewport_get_offset_(viewport_ptr),
                     static_cast<int64_t>(0));
    return (capacity < remaining_file_size) ? capacity : remaining_file_size;
}

int64_t viewport_get_offset_(const omega_viewport_t *viewport_ptr) {
    return viewport_ptr->data_segment.offset_ + viewport_ptr->data_segment.offset_adjustment_;
}

int viewport_is_floating_(const omega_viewport_t *viewport_ptr) { return (viewport_ptr->is_floating_) ? 1 : 0; }

int viewport_has_changes_(const omega_viewport_t *viewport_ptr) {
    return (viewport_ptr->data_segment.capacity_ < 0) ? 1 : 0;
}

/**********************************************************************************************************************
 * Model segment functions
 **********************************************************************************************************************/

static void print_change_(const omega_change_t *change_ptr, std::ostream &out_stream) noexcept {
    assert(change_ptr);
    out_stream << R"({"serial": )" << omega_change_get_serial(change_ptr) << R"(, "kind": ")"
               << omega_change_get_kind_as_char(change_ptr) << R"(", "offset": )" << omega_change_get_offset(change_ptr)
               << R"(, "length": )" << omega_change_get_length(change_ptr);
    if (const auto bytes = omega_change_get_bytes(change_ptr); bytes) {
        out_stream << R"(, "bytes": ")" << std::string((char const *) bytes, omega_change_get_length(change_ptr))
                   << R"(")";
    }
    out_stream << "}";
}

static void print_model_segment_(const omega_model_segment_ptr_t &segment_ptr, std::ostream &out_stream) noexcept {
    out_stream << R"({"kind": ")" << omega_model_segment_kind_as_char(omega_model_segment_get_kind(segment_ptr.get()))
               << R"(", "computed_offset": )" << segment_ptr->computed_offset_ << R"(, "computed_length": )"
               << segment_ptr->computed_length_ << R"(, "change_offset": )" << segment_ptr->change_offset_
               << R"(, "change": )";
    print_change_(segment_ptr->change_ptr_.get(), out_stream);
    out_stream << "}" << std::endl;
}

void print_model_segments_(const omega_model_t *model_ptr, std::ostream &out_stream) noexcept {
    assert(model_ptr);
    for (const auto &segment : model_ptr->model_segments_) { print_model_segment_(segment, out_stream); }
}
