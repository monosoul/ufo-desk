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

  desk_client_.set_event_handler([this](UfoDeskEvent e) {
    // The reset enters status "reset" (0x04) once it has bottomed out and is
    // bouncing back up to the zero reference (~720mm); the box then finishes the
    // climb on its own once 0x88 stops. Release when the desk is essentially
    // home, or after a 2s ceiling, whichever comes first -- so we neither cut it
    // off at the bottom (strands the desk) nor keep 0x88 asserted once it's back
    // at the top (which could re-trigger a fresh descent).
    if (this->reset_active_ && e.desk.position_status() == PositionStatus::reset) {
      if (e.type == UfoDeskEventType::positionStatusChanged) {
        this->set_timeout("reset_finish", 2000, [this]() { this->finish_reset_(); });
      }
      if (e.desk.cur_height_mm() >= 719) {
        this->finish_reset_();
      }
    }
    this->event_callbacks_.call(e);
  });
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
  pressed_[static_cast<int>(b)] = true;
  this->arm_or_cancel_calibration_();
  this->recompute_();
}

void UfoDesk::release(Button b) {
  pressed_[static_cast<int>(b)] = false;
  this->arm_or_cancel_calibration_();
  this->recompute_();
}

void UfoDesk::arm_or_cancel_calibration_() {
  // Once a reset is latched it runs to completion on its own; button changes
  // no longer matter until the box reports done.
  if (reset_active_) {
    return;
  }
  bool both = pressed_[static_cast<int>(Button::up)] && pressed_[static_cast<int>(Button::down)];
  if (both) {
    // Arm only after UP+DOWN are held together for 10s (matches the stock remote).
    this->set_timeout("calibrate_arm", 10000, [this]() {
      if (pressed_[static_cast<int>(Button::up)] && pressed_[static_cast<int>(Button::down)]) {
        // Latch: stream 0x88 until the box finishes (status 0x04) regardless of
        // the buttons, with a safety timeout so it can never run forever.
        reset_active_ = true;
        this->set_timeout("reset_timeout", 30000, [this]() { this->finish_reset_(); });
        this->recompute_();
      }
    });
  } else {
    this->cancel_timeout("calibrate_arm");
  }
}

void UfoDesk::finish_reset_() {
  reset_active_ = false;
  this->cancel_timeout("reset_timeout");
  this->cancel_timeout("reset_finish");
  this->recompute_();
}

void UfoDesk::recompute_() {
  Button cmd;
  if (reset_active_) {
    // Latched reset: drive it to completion.
    cmd = Button::calibrate;
  } else {
    bool u = pressed_[static_cast<int>(Button::up)];
    bool d = pressed_[static_cast<int>(Button::down)];
    if (u && d) {
      cmd = Button::none;  // both held (arming window): stay still
    } else if (u) {
      cmd = Button::up;
    } else if (d) {
      cmd = Button::down;
    } else if (pressed_[static_cast<int>(Button::mem)]) {
      cmd = Button::mem;
    } else if (pressed_[static_cast<int>(Button::preset1)]) {
      cmd = Button::preset1;
    } else if (pressed_[static_cast<int>(Button::preset2)]) {
      cmd = Button::preset2;
    } else {
      cmd = Button::none;
    }
  }
  desk_client_.push_button(cmd);
}
}  // namespace esphome::ufo_desk
