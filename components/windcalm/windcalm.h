#pragma once

#include <deque>
#include <vector>
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/remote_base/remote_base.h"
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#include "protocol.h"

namespace esphome::windcalm {

class WindcalmFan;

class WindcalmHub : public Component, public remote_base::RemoteReceiverListener {
 public:
  void set_transmitter(remote_base::RemoteTransmitterBase *transmitter) { this->transmitter_ = transmitter; }
  void register_fan(WindcalmFan *fan) { this->fans_.push_back(fan); }
  void setup() override;
  void loop() override;
  void dump_config() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;
  bool enqueue(WindcalmFan *fan, uint8_t command);

 protected:
  struct Pending { WindcalmFan *fan; uint8_t command; };
  remote_base::RemoteTransmitterBase *transmitter_{nullptr};
  std::vector<WindcalmFan *> fans_;
  std::deque<Pending> queue_;
  uint32_t last_send_{0};
  bool sent_{false};
};

class WindcalmFan : public Component, public fan::Fan {
 public:
  void set_parent(WindcalmHub *parent) { this->parent_ = parent; }
  void set_remote_id(uint32_t remote_id) { this->remote_id_ = remote_id; }
  uint32_t get_remote_id() const { return this->remote_id_; }
  void set_speed_turns_on(bool value) { this->speed_turns_on_ = value; }
  void setup() override;
  void dump_config() override;
  fan::FanTraits get_traits() override;
  bool send_command(int command);
  void sync_state(bool state) { this->state = state; this->publish_state(); }
  void receive(uint32_t packet);
  uint32_t next_packet(uint8_t command);

 protected:
  void control(const fan::FanCall &call) override;
  void apply_command_(uint8_t command);
  void save_counter_();
  WindcalmHub *parent_{nullptr};
  uint32_t remote_id_{0};
  bool speed_turns_on_{true};
  uint8_t counter_{0};
  ESPPreferenceObject counter_pref_;
  uint32_t last_rx_packet_{0}, last_rx_time_{0};
  uint32_t last_tx_packet_{0}, last_tx_time_{0};
  bool received_{false}, transmitted_{false};
};

#ifdef USE_BUTTON
class WindcalmButton : public button::Button {
 public:
  void set_parent(WindcalmFan *parent) { this->parent_ = parent; }
  void set_command(uint8_t command) { this->command_ = command; }
 protected:
  void press_action() override { this->parent_->send_command(this->command_); }
  WindcalmFan *parent_{nullptr};
  uint8_t command_{0};
};
#endif

template<typename... Ts> class SendAction : public Action<Ts...> {
 public:
  explicit SendAction(WindcalmFan *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, command)
  void play(const Ts &...x) override { this->parent_->send_command(this->command_.value(x...)); }
 protected:
  WindcalmFan *parent_;
};

template<typename... Ts> class SyncAction : public Action<Ts...> {
 public:
  explicit SyncAction(WindcalmFan *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(bool, state)
  void play(const Ts &...x) override { this->parent_->sync_state(this->state_.value(x...)); }
 protected:
  WindcalmFan *parent_;
};

}  // namespace esphome::windcalm
