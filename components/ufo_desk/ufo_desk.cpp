#include "esphome/core/log.h"
#include "ufo_desk.h"

namespace esphome::ufo_desk {

static const char *TAG = "ufo_desk.component";

UfoDesk::UfoDesk() {}

void UfoDesk::setup() {
  this->set_interval("uart_ping_pong", 200, [this]() {
    // ESP_LOGD(TAG, "Sending request to control box");
    this->write_array(this->desk_client_.request_buf(), this->desk_client_.request_buf_size());
    this->response_pending_ = true;
    this->resp_cur_size_ = 0;
    this->request_sent_at_ = millis();
  });

  desk_client_.set_event_handler([this](UfoDeskEvent e) { this->event_callbacks_.call(e); });
}

void UfoDesk::loop() {
  while (this->available()) {
    uint8_t b = 0;
    if (!this->read_byte(&b) || !this->response_pending_) {
      return;
    }

    if (millis() - request_sent_at_ >= kRESPONSE_TIMEOUT_MS) {
      response_pending_ = false;
      return;
    }

    response_buffer_[resp_cur_size_++] = b;
    if (resp_cur_size_ == response_buffer_.size()) {
      //      ESP_LOGD(TAG, "Response from control box received");
      response_pending_ = false;
      if (desk_client_.parse_response(response_buffer_)) {
        uint16_t crc = *reinterpret_cast<uint16_t *>(&response_buffer_[12]);
        if (crc != dbg_last_crc_) {
          ESP_LOGD(TAG, "Resp: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", response_buffer_[2],
                   response_buffer_[3], response_buffer_[4], response_buffer_[5], response_buffer_[6],
                   response_buffer_[7], response_buffer_[8], response_buffer_[9], response_buffer_[10],
                   response_buffer_[11]);
        }
        dbg_last_crc_ = crc;
      } else {
        ESP_LOGW(TAG, "Failed to parse response buffer");
      }
    }
  }
}

void UfoDesk::update() {}

void UfoDesk::dump_config() { ESP_LOGCONFIG(TAG, "UFO Desk"); }

void UfoDesk::add_event_callback(std::function<void(UfoDeskEvent)> &&callback) {
  event_callbacks_.add(std::move(callback));
}


void UfoDesk::press(Button b) {
  held_[static_cast<int>(b)] = true;
  this->arm_or_cancel_calibration_();
  this->recompute_();
}

void UfoDesk::release(Button b) {
  held_[static_cast<int>(b)] = false;
  this->arm_or_cancel_calibration_();
  this->recompute_();
}

void UfoDesk::arm_or_cancel_calibration_() {
  bool both = held_[static_cast<int>(Button::up)] && held_[static_cast<int>(Button::down)];
  if (both) {
    // Arm only after UP+DOWN are held together for 10s (matches the stock remote).
    this->set_timeout("calibrate_arm", 10000, [this]() {
      if (held_[static_cast<int>(Button::up)] && held_[static_cast<int>(Button::down)]) {
        calibrating_ = true;
        this->recompute_();
      }
    });
  } else {
    this->cancel_timeout("calibrate_arm");
    calibrating_ = false;
  }
}

void UfoDesk::recompute_() {
  bool u = held_[static_cast<int>(Button::up)];
  bool d = held_[static_cast<int>(Button::down)];
  Button cmd;
  if (u && d) {
    cmd = calibrating_ ? Button::calibrate : Button::none;
  } else if (u) {
    cmd = Button::up;
  } else if (d) {
    cmd = Button::down;
  } else if (held_[static_cast<int>(Button::mem)]) {
    cmd = Button::mem;
  } else if (held_[static_cast<int>(Button::preset1)]) {
    cmd = Button::preset1;
  } else if (held_[static_cast<int>(Button::preset2)]) {
    cmd = Button::preset2;
  } else {
    cmd = Button::none;
  }
  desk_client_.push_button(cmd);
}
}  // namespace esphome::ufo_desk
