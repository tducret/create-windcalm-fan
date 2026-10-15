#include "windcalm.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::windcalm {

static const char *const TAG = "windcalm";

void WindcalmHub::setup() {
  for (size_t i = 0; i < this->fans_.size(); i++) {
    for (size_t j = i + 1; j < this->fans_.size(); j++) {
      if (this->fans_[i]->get_remote_id() == this->fans_[j]->get_remote_id()) {
        ESP_LOGE(TAG, "Duplicate remote ID %05X", (unsigned) this->fans_[i]->get_remote_id());
        this->mark_failed();
        return;
      }
    }
  }
}

void WindcalmHub::dump_config() { ESP_LOGCONFIG(TAG, "Windcalm RF hub (%u fans)", (unsigned) this->fans_.size()); }

bool WindcalmHub::enqueue(WindcalmFan *fan, uint8_t command) {
  if (this->is_failed() || this->queue_.size() >= 32) {
    ESP_LOGW(TAG, "Command rejected: hub failed or transmit queue full");
    return false;
  }
  this->queue_.push_back({fan, command});
  return true;
}

void WindcalmHub::loop() {
  // Six frames take ~248 ms. Leave margin for asynchronous RMT completion
  // and the CC1101 on_complete -> RX transition before starting another.
  if (this->queue_.empty() || (this->sent_ && millis() - this->last_send_ < 400))
    return;
  auto pending = this->queue_.front();
  this->queue_.pop_front();
  const uint32_t packet = pending.fan->next_packet(pending.command);
  auto call = this->transmitter_->transmit();
  auto *data = call.get_data();
  data->set_carrier_frequency(0);
  data->reserve(66);
  for (int bit = 31; bit >= 0; bit--) {
    if (packet & (1UL << bit))
      data->item(720, 290);
    else
      data->item(330, 680);
  }
  data->item(320, 7460);
  call.set_send_times(6);
  call.set_send_wait(0);
  this->last_send_ = millis();
  this->sent_ = true;
  call.perform();
}

bool WindcalmHub::on_receive(remote_base::RemoteReceiveData data) {
  uint32_t packet;
  const auto &raw = data.get_raw_data();
  if (!decode(raw.data(), raw.size(), packet))
    return false;
  const uint32_t remote_id = packet >> 12;
  for (auto *fan : this->fans_) {
    if (fan->get_remote_id() == remote_id) {
      fan->receive(packet);
      return true;
    }
  }
  ESP_LOGD(TAG, "Unconfigured remote ID=%05X CMD=%02X", (unsigned) remote_id, (unsigned) ((packet >> 7) & 31));
  return false;
}

void WindcalmFan::setup() {
  this->counter_pref_ = global_preferences->make_preference<uint8_t>(0x57430000UL ^ this->remote_id_);
  this->counter_pref_.load(&this->counter_);
  this->counter_ &= 7;
  auto restored = this->restore_state_();
  if (restored.has_value())
    restored->apply(*this);  // Restore UI estimates without sending RF at boot.
  if (this->speed < 1 || this->speed > 6)
    this->speed = 1;
  this->publish_state();
}

void WindcalmFan::dump_config() {
  LOG_FAN("", "Windcalm Fan", this);
  ESP_LOGCONFIG(TAG, "  Remote ID: %05X; state is optimistic", (unsigned) this->remote_id_);
}

fan::FanTraits WindcalmFan::get_traits() {
  return fan::FanTraits(false, true, true, 6);
}

void WindcalmFan::save_counter_() { this->counter_pref_.save(&this->counter_); }

uint32_t WindcalmFan::next_packet(uint8_t command) {
  const uint32_t packet = encode(this->remote_id_, command, this->counter_);
  this->counter_ = (this->counter_ + 1) & 7;
  this->save_counter_();
  this->last_tx_packet_ = packet;
  this->last_tx_time_ = millis();
  this->transmitted_ = true;
  ESP_LOGI(TAG, "TX ID=%05X CMD=%02X PACKET=%08X", (unsigned) this->remote_id_, command, (unsigned) packet);
  return packet;
}

bool WindcalmFan::send_command(int command) {
  if (command < 0 || command > 31) {
    ESP_LOGW(TAG, "Command must be in 0..31");
    return false;
  }
  if (!this->parent_->enqueue(this, command))
    return false;
  this->apply_command_(command);
  return true;
}

void WindcalmFan::receive(uint32_t packet) {
  const uint32_t now = millis();
  if (this->transmitted_ && packet == this->last_tx_packet_ && now - this->last_tx_time_ < 600)
    return;
  const bool duplicate = this->received_ && packet == this->last_rx_packet_ && now - this->last_rx_time_ < 300;
  this->last_rx_packet_ = packet;
  this->last_rx_time_ = now;
  this->received_ = true;
  if (duplicate)
    return;
  this->counter_ = (((packet >> 4) & 7) + 1) & 7;
  this->save_counter_();
  const uint8_t command = (packet >> 7) & 31;
  ESP_LOGI(TAG, "RX ID=%05X CMD=%02X PACKET=%08X", (unsigned) this->remote_id_, command, (unsigned) packet);
  this->apply_command_(command);
}

void WindcalmFan::apply_command_(uint8_t command) {
  for (int i = 0; i < 6; i++) {
    if (command == SPEED_COMMANDS[i]) {
      this->speed = i + 1;
      if (this->speed_turns_on_)
        this->state = true;
    }
  }
  if (command == 0x0A)
    this->state = !this->state;
  if (command == 0x11)
    this->direction = this->direction == fan::FanDirection::FORWARD ? fan::FanDirection::REVERSE : fan::FanDirection::FORWARD;
  this->publish_state();
}

void WindcalmFan::control(const fan::FanCall &call) {
  const bool target_state = call.get_state().value_or(this->state);
  const int target_speed = call.get_speed().value_or(this->speed);
  const bool was_on = this->state;
  if (target_state != this->state && !this->send_command(0x0A))
    return;
  if (target_state && (!was_on || target_speed != this->speed)) {
    if (!this->send_command(SPEED_COMMANDS[target_speed - 1]))
      return;
  } else if (!target_state) {
    this->speed = target_speed;  // Remember a speed while off, without starting the motor.
  }
  if (call.get_direction().has_value() && *call.get_direction() != this->direction)
    this->send_command(0x11);
  this->publish_state();
}

}  // namespace esphome::windcalm
