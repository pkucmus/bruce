/* <bruce/input_dg/partition_key/v0/v0_input_dg_writer.cc>

   ----------------------------------------------------------------------------
   Copyright 2013-2014 Tagged

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   ----------------------------------------------------------------------------

   Implements <bruce/input_dg/partition_key/v0/v0_input_dg_writer.h>.
 */

#include <bruce/input_dg/partition_key/v0/v0_input_dg_writer.h>

#include <cassert>
#include <cstring>

#include <base/field_access.h>
#include <bruce/input_dg/partition_key/v0/v0_input_dg_constants.h>
#include <bruce/input_dg/input_dg_constants.h>

using namespace Bruce;
using namespace Bruce::InputDg;
using namespace Bruce::InputDg::PartitionKey;
using namespace Bruce::InputDg::PartitionKey::V0;

static inline size_t GetDgOverhead() noexcept {
  return INPUT_DG_SZ_FIELD_SIZE + INPUT_DG_API_KEY_FIELD_SIZE +
      INPUT_DG_API_VERSION_FIELD_SIZE + INPUT_DG_P_KEY_V0_FLAGS_FIELD_SIZE +
      INPUT_DG_P_KEY_V0_PARTITION_KEY_FIELD_SIZE +
      INPUT_DG_P_KEY_V0_TOPIC_SZ_FIELD_SIZE + INPUT_DG_P_KEY_V0_TS_FIELD_SIZE +
      INPUT_DG_P_KEY_V0_KEY_SZ_FIELD_SIZE +
      INPUT_DG_P_KEY_V0_VALUE_SZ_FIELD_SIZE;
}

TV0InputDgWriter::TDgSizeResult
TV0InputDgWriter::CheckDgSize(size_t topic_size, size_t key_size,
    size_t value_size) noexcept {
  if (topic_size > std::numeric_limits<int8_t>::max()) {
    return TDgSizeResult::TopicTooLarge;
  }

  size_t key_value_space = std::numeric_limits<int32_t>::max() -
      GetDgOverhead() - topic_size;

  if ((key_size > key_value_space) ||
      (value_size > (key_value_space - key_size))) {
    return TDgSizeResult::MsgTooLarge;
  }

  return TDgSizeResult::Ok;
}

static inline size_t DoComputeDgSize(size_t topic_size, size_t key_size,
    size_t value_size) noexcept {
  return GetDgOverhead() + topic_size + key_size + value_size;
}

TV0InputDgWriter::TDgSizeResult
TV0InputDgWriter::ComputeDgSize(size_t &result, size_t topic_size,
    size_t key_size, size_t value_size) noexcept {
  result = 0;
  TDgSizeResult ret = CheckDgSize(topic_size, key_size, value_size);

  if (ret != TDgSizeResult::Ok) {
    return ret;
  }

  result = DoComputeDgSize(topic_size, key_size, value_size);
  return TDgSizeResult::Ok;
}

void TV0InputDgWriter::DoWriteDg(bool check_size, void *result_buf,
    int64_t timestamp, int32_t partition_key, const void *topic_begin,
    const void *topic_end, const void *key_begin, const void *key_end,
    const void *value_begin, const void *value_end) noexcept {
  assert(this);
  assert(result_buf);
  assert(topic_begin);
  assert(topic_end >= topic_begin);
  assert(key_begin || (key_end == key_begin));
  assert(key_end >= key_begin);
  assert(value_begin || (value_end == value_begin));
  assert(value_end >= value_begin);
  uint8_t *pos = reinterpret_cast<uint8_t *>(result_buf);
  const uint8_t *topic_start = reinterpret_cast<const uint8_t *>(topic_begin);
  const uint8_t *topic_finish = reinterpret_cast<const uint8_t *>(topic_end);
  const uint8_t *key_start = reinterpret_cast<const uint8_t *>(key_begin);
  const uint8_t *key_finish = reinterpret_cast<const uint8_t *>(key_end);
  const uint8_t *value_start = reinterpret_cast<const uint8_t *>(value_begin);
  const uint8_t *value_finish = reinterpret_cast<const uint8_t *>(value_end);
  size_t topic_size = topic_finish - topic_start;
  size_t key_size = key_finish - key_start;
  size_t value_size = value_finish - value_start;
  size_t dg_size = 0;

  if (check_size) {
    if (ComputeDgSize(dg_size, topic_size, key_size, value_size) !=
        TDgSizeResult::Ok) {
      assert(false);
      return;
    }
  } else {
    dg_size = DoComputeDgSize(topic_size, key_size, value_size);
  }

  topic_finish = topic_start + topic_size;
  key_finish = key_start + key_size;
  value_finish = value_start + value_size;
  WriteInt32ToHeader(pos, dg_size);
  pos += INPUT_DG_SZ_FIELD_SIZE;

  WriteInt16ToHeader(pos, 257);

  pos += INPUT_DG_API_KEY_FIELD_SIZE;
  WriteInt16ToHeader(pos, 0);  // API version
  pos += INPUT_DG_API_VERSION_FIELD_SIZE;
  WriteInt16ToHeader(pos, 0);  // flags
  pos += INPUT_DG_P_KEY_V0_FLAGS_FIELD_SIZE;
  WriteInt32ToHeader(pos, partition_key);
  pos += INPUT_DG_P_KEY_V0_PARTITION_KEY_FIELD_SIZE;
  *pos = static_cast<uint8_t>(topic_size);
  pos += INPUT_DG_P_KEY_V0_TOPIC_SZ_FIELD_SIZE;
  std::memcpy(pos, topic_start, topic_size);
  pos += topic_size;
  WriteInt64ToHeader(pos, timestamp);
  pos += INPUT_DG_P_KEY_V0_TS_FIELD_SIZE;
  WriteInt32ToHeader(pos, key_size);
  pos += INPUT_DG_P_KEY_V0_KEY_SZ_FIELD_SIZE;

  if (key_start) {
    std::memcpy(pos, key_start, key_size);
  }

  pos += key_size;
  WriteInt32ToHeader(pos, value_size);
  pos += INPUT_DG_P_KEY_V0_VALUE_SZ_FIELD_SIZE;

  if (value_start) {
    std::memcpy(pos, value_start, value_size);
  }
}
